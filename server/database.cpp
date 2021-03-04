/*
    mc4 server
    Copyright (C) 2021 kholland4

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "database.h"
#include "json.h"

#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

MapblockUpdateInfo MemoryDB::get_mapblockupdateinfo(Vector3<int> pos) {
  auto search = datastore.find(pos);
  if(search != datastore.end()) {
    return MapblockUpdateInfo(search->second);
  }
  return MapblockUpdateInfo(pos);
}

void MemoryDB::set_mapblockupdateinfo(Vector3<int> pos, MapblockUpdateInfo info) {
  auto search = datastore.find(pos);
  if(search != datastore.end()) {
    info.write_to_mapblock(search->second);
  }
}

Mapblock* MemoryDB::get_mapblock(Vector3<int> pos) {
  auto search = datastore.find(pos);
  if(search != datastore.end()) {
    return new Mapblock(*(search->second));
  }
  return new Mapblock(pos);
}

void MemoryDB::set_mapblock(Vector3<int> pos, Mapblock *mb) {
  auto search = datastore.find(pos);
  if(search != datastore.end()) {
    delete search->second;
  }
  Mapblock *mb_store = new Mapblock(*mb);
  datastore[pos] = mb_store;
}



//SQLiteDB row versions:
/* Version 1:

CREATE TABLE IF NOT EXISTS map ( \
  x INT,\
  y INT,\
  z INT,\
  data MEDIUMBLOB,\
  id_to_is TEXT,\
  is_to_id TEXT,\
  light_needs_update TINYINT,\
  sunlit BOOLEAN,\
  dirty BOOLEAN,\
  primary key (x, y, z)\
);

*/
/* Version 2:

//NOTE: It is recommended to run 'sqlite3 mapfile_name.sqlite vacuum' to clean out old data from the map file after __successfully__ upgrading to version 2.

//Changes:
//  - add version column.
//  - removed is_to_id column because it is redundant.
//  - use uint32_t instead of uint64_t for the data blob, don't store lighting data, and compress with run-length encoding.
//  - remove light_needs_update, as light information isn't stored to the database anyways

//Upgrade path:
//  - recreate the table with the is_to_id and light_needs_update columns removed and the version column added.
//  - data blob is incompatible between versions 1 and 2; rows will need to be upgraded by being read and rewritten.

CREATE TABLE IF NOT EXISTS map ( \
  x INT,\
  y INT,\
  z INT,\
  data MEDIUMBLOB,\
  id_to_is TEXT,\
  sunlit BOOLEAN,\
  dirty BOOLEAN,\
  version SMALLINT UNSIGNED,\   (DEFAULT 1 for upgraded databases, no default specified for new ones)
  primary key (x, y, z)\
);

*/

SQLiteDB::SQLiteDB(const char* filename) {
  int rc = sqlite3_open(filename, &db);
  if(rc != SQLITE_OK) {
    std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
    exit(1);
  }
  
  //Read the database version.
  sqlite3_stmt *pragma_reader;
  if(sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &pragma_reader, NULL) != SQLITE_OK) {
    std::cerr << "Unable to read pragma 'user_version': " << sqlite3_errmsg(db) << std::endl;
    exit(1);
  }
  if(sqlite3_step(pragma_reader) == SQLITE_ROW) {
    db_version = sqlite3_column_int(pragma_reader, 0);
  } else {
    std::cerr << "Unable to read pragma 'user_version': " << sqlite3_errmsg(db) << std::endl;
    exit(1);
  }
  if(sqlite3_step(pragma_reader) != SQLITE_DONE) {
    std::cerr << "Unable to read pragma 'user_version': " << sqlite3_errmsg(db) << std::endl;
    exit(1);
  }
  sqlite3_finalize(pragma_reader);
  
  int orig_db_version = db_version;
  
  //If the version is 0, the database is either (a) empty and uninitialized or (b) a version 1 database, from the dark ages before versions were used
  //Find out by checking for the existence of a 'map' table.
  if(db_version == 0) {
    sqlite3_stmt *table_check;
    if(sqlite3_prepare_v2(db, "SELECT name FROM sqlite_master WHERE type='table' AND name='map';", -1, &table_check, NULL) != SQLITE_OK) {
      std::cerr << "Unable check for existence of 'map' table: " << sqlite3_errmsg(db) << std::endl;
      exit(1);
    }
    
    int res = sqlite3_step(pragma_reader);
    if(res == SQLITE_ROW) {
      //Table found, must be version 1.
      db_version = 1;
    } else if(res == SQLITE_DONE) {
      //No result, must be version 0.
    } else {
      std::cerr << "Unable check for existence of 'map' table: " << sqlite3_errmsg(db) << std::endl;
      exit(1);
    }
    
    sqlite3_finalize(table_check);
  }
  
  //If version is still 0, the database is empty and uninitialized.
  //Create the appropriate table(s) and set the 'user_version' pragma.
  if(db_version == 0) {
    db_version = 2;
    
    const char *sql =
"CREATE TABLE map ( \
  x INT,\
  y INT,\
  z INT,\
  data MEDIUMBLOB,\
  id_to_is TEXT,\
  sunlit BOOLEAN,\
  dirty BOOLEAN,\
  version SMALLINT UNSIGNED,\
  primary key (x, y, z)\
);";
    
    sqlite3_stmt *statement;
    if(sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
      std::cerr << "Unable to create table 'map': " << sqlite3_errmsg(db) << std::endl;
      exit(1);
    }
    if(sqlite3_step(statement) != SQLITE_DONE) {
      std::cerr << "Unable to create table 'map': " << sqlite3_errmsg(db) << std::endl;
      exit(1);
    }
    sqlite3_finalize(statement);
    
    //TODO proper logging
    std::cout << "Initialized new SQLite database with version " << db_version << std::endl;
  }
  
  //Upgrades from version 1 to version 2
  if(db_version == 1) {
    //TODO proper logging
    std::cout << "Upgrading database from version 1 to version 2" << std::endl;
    
    //FIXME use transactions
    
    std::array<std::string, 7> sql_str = {
      "BEGIN EXCLUSIVE TRANSACTION;",
      "CREATE TABLE map_new ( \
        x INT,\
        y INT,\
        z INT,\
        data MEDIUMBLOB,\
        id_to_is TEXT,\
        sunlit BOOLEAN,\
        dirty BOOLEAN,\
        version SMALLINT UNSIGNED,\
        primary key (x, y, z)\
      );",
      "INSERT INTO map_new (x, y, z, data, id_to_is, sunlit, dirty, version) SELECT x, y, z, data, id_to_is, sunlit, dirty, 1 FROM map;",
      "DROP TABLE map;",
      "ALTER TABLE map_new RENAME TO map;",
      "PRAGMA user_version = 2;",
      "COMMIT TRANSACTION;"
    };
    
    for(size_t i = 0; i < sql_str.size(); i++) {
      sqlite3_stmt *statement;
      if(sqlite3_prepare_v2(db, sql_str[i].c_str(), -1, &statement, NULL) != SQLITE_OK) {
        std::cerr << "Unable to recreate table 'map': " << sqlite3_errmsg(db) << std::endl;
        exit(1);
      }
      if(sqlite3_step(statement) != SQLITE_DONE) {
        std::cerr << "Unable to recreate table 'map': " << sqlite3_errmsg(db) << std::endl;
        exit(1);
      }
      sqlite3_finalize(statement);
    }
    
    db_version = 2;
  }
  
  
  //Store the DB user_version, if changed.
  if(db_version != orig_db_version) {
    std::string sql_str = "PRAGMA user_version = " + std::to_string(db_version) + ";";
    const char *sql = sql_str.c_str();
    
    sqlite3_stmt *pragma_writer;
    if(sqlite3_prepare_v2(db, sql, -1, &pragma_writer, NULL) != SQLITE_OK) {
      std::cerr << "Unable write pragma 'user_version': " << sqlite3_errmsg(db) << std::endl;
      exit(1);
    }
    if(sqlite3_step(pragma_writer) != SQLITE_DONE) {
      std::cerr << "Unable write pragma 'user_version': " << sqlite3_errmsg(db) << std::endl;
      exit(1);
    }
    sqlite3_finalize(pragma_writer);
  }
  
  
  //Get number of stored mapblock rows (to show to the user).
  std::map<int, std::string> sql_str;
  std::map<int, int> row_counts;
  sql_str[-1] = "SELECT COUNT(*) FROM map;";
  //Iterate over possible versions.
  for(int i = 1; i <= 2; i++) {
    sql_str[i] = "SELECT COUNT(*) FROM map WHERE version=" + std::to_string(i) + ";";
  }
  
  
  for(auto it : sql_str) {
    sqlite3_stmt *count_checker;
    if(sqlite3_prepare_v2(db, it.second.c_str(), -1, &count_checker, NULL) != SQLITE_OK) {
      std::cerr << "Unable to count rows in 'map' table: " << sqlite3_errmsg(db) << std::endl;
      exit(1);
    }
    if(sqlite3_step(count_checker) == SQLITE_ROW) {
      row_counts[it.first] = sqlite3_column_int(count_checker, 0);
    } else {
      std::cerr << "Unable to count rows in 'map' table: " << sqlite3_errmsg(db) << std::endl;
      exit(1);
    }
    if(sqlite3_step(count_checker) != SQLITE_DONE) {
      std::cerr << "Unable to count rows in 'map' table: " << sqlite3_errmsg(db) << std::endl;
      exit(1);
    }
    sqlite3_finalize(count_checker);
  }
  
  //TODO proper logging
  std::cout << "Loaded SQLite database with " << row_counts[-1] << " mapblocks (version " << db_version << ")" << std::endl;
  for(auto it : row_counts) {
    int version = it.first;
    if(version == -1) { continue; }
    int count = it.second;
    if(count == 0) { continue; }
    std::cout << count << " mapblocks are version " << version << "." << std::endl;
  }
}

SQLiteDB::~SQLiteDB() {
  sqlite3_close_v2(db);
}

MapblockUpdateInfo SQLiteDB::get_mapblockupdateinfo(Vector3<int> pos) {
  //Try read cache.
  auto search = read_cache.find(pos);
  if(search != read_cache.end()) {
    return MapblockUpdateInfo(search->second);
  }
  
  //Try database.
  //FIXME: only need to retrieve light_needs_update
  
  //Mapblock doesn't exist or isn't loaded; the default update info will be accurate.
  return MapblockUpdateInfo(pos);
}

void SQLiteDB::set_mapblockupdateinfo(Vector3<int> pos, MapblockUpdateInfo info) {
  //Update info (with the exception of light_needs_update) only needs to be stored in read cache because it's irrelevant after all the clients disconnect.
  auto search = read_cache.find(pos);
  if(search == read_cache.end()) {
    return; //not present
  }
  
  info.write_to_mapblock(search->second);
}

Mapblock* SQLiteDB::get_mapblock(Vector3<int> pos) {
  //Try read cache.
  auto search = read_cache.find(pos);
  if(search != read_cache.end()) {
    return new Mapblock(*(search->second));
  }
  
  Mapblock *mb = new Mapblock(pos);
  
  //Try database.
  const char *sql = "SELECT data, id_to_is, sunlit, dirty, version FROM map WHERE x=? AND y=? AND z=? LIMIT 1;";
  sqlite3_stmt *statement;
  if(sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::get_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    mb->dont_write_to_db = true;
    return mb;
  }
  if(sqlite3_bind_int(statement, 1, pos.x) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::get_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    mb->dont_write_to_db = true;
    return mb;
  }
  if(sqlite3_bind_int(statement, 2, pos.y) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::get_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    mb->dont_write_to_db = true;
    return mb;
  }
  if(sqlite3_bind_int(statement, 3, pos.z) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::get_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    mb->dont_write_to_db = true;
    return mb;
  }
  
  if(sqlite3_step(statement) == SQLITE_ROW) {
    //Have row.
    int row_version = sqlite3_column_int(statement, 4);
    
    const uint64_t *data_raw = (const uint64_t*)sqlite3_column_blob(statement, 0);
    int data_raw_len = sqlite3_column_bytes(statement, 0);
    const unsigned char *IDtoIS_raw = sqlite3_column_text(statement, 1);
    mb->sunlit = (bool)sqlite3_column_int(statement, 2);
    mb->dirty = (bool)sqlite3_column_int(statement, 3);
    
    std::stringstream ss;
    ss << IDtoIS_raw;
    
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss, pt);
    
    mb->IDtoIS.clear();
    mb->IStoID.clear();
    for(auto& it : pt) {
      std::string id_key = std::string(it.first);
      std::string itemstring = pt.get<std::string>(boost::property_tree::path(id_key, '\n'));
      int id = mb->IDtoIS.size();
      mb->IDtoIS.push_back(itemstring);
      mb->IStoID[itemstring] = id;
    }
    
    if(row_version == 1) {
      if(data_raw_len != MAPBLOCK_SIZE_X * MAPBLOCK_SIZE_Y * MAPBLOCK_SIZE_Z * sizeof(uint64_t)) {
        std::cerr << "Error in SQLiteDB::get_mapblock: unexpected mapblock data size" << std::endl;
        sqlite3_finalize(statement);
        mb->dont_write_to_db = true;
        mb->is_nil = true;
        return mb;
      }
      
      for(int x = 0; x < MAPBLOCK_SIZE_X; x++) {
        for(int y = 0; y < MAPBLOCK_SIZE_Y; y++) {
          for(int z = 0; z < MAPBLOCK_SIZE_Z; z++) {
            mb->data[x][y][z] = data_raw[x * MAPBLOCK_SIZE_Y * MAPBLOCK_SIZE_Z + y * MAPBLOCK_SIZE_Z + z];
          }
        }
      }
      
      mb->is_nil = false;
    } else if(row_version == 2) {
      const uint32_t* data_compressed = (const uint32_t*)data_raw;
      
      int y = 0;
      int x = 0;
      int z = 0;
      bool full = false;
      for(size_t i = 0; i < (data_raw_len / sizeof(uint32_t)); i++) {
        uint32_t run_val = data_compressed[i] & 0b00000000011111111111111111111111;
        size_t run_length = ((data_compressed[i] >> 23) & 511) + 1; //Run lengths are offset by 1 to allow storing [1, 512] instead of [0, 511]
        
        for(size_t n = 0; n < run_length; n++) {
          if(full) {
            std::cerr << "Error in SQLiteDB::get_mapblock: decompressed mapblock is too long" << std::endl;
            sqlite3_finalize(statement);
            mb->dont_write_to_db = true;
            return mb;
          }
          
          mb->data[x][y][z] = run_val;
          z++;
          if(z >= MAPBLOCK_SIZE_Z) {
            z = 0;
            x++;
            if(x >= MAPBLOCK_SIZE_X) {
              x = 0;
              y++;
              if(y >= MAPBLOCK_SIZE_Y) {
                y = 0;
                full = true;
              }
            }
          }
        }
      }
      
      if(!full) {
        std::cerr << "Error in SQLiteDB::get_mapblock: decompressed mapblock is too short" << std::endl;
        sqlite3_finalize(statement);
        mb->dont_write_to_db = true;
        return mb;
      }
      
      mb->is_nil = false;
    } else {
      std::cerr << "Error in SQLiteDB::get_mapblock: unknown row version '" << row_version << "'" << std::endl;
      sqlite3_finalize(statement);
      mb->dont_write_to_db = true;
      mb->is_nil = true;
      return mb;
    }
    
    //Since it's not cached, the lighting is probably outdated.
    mb->light_needs_update = 1;
    
    //Save to read cache.
    Mapblock *mb_store = new Mapblock(*mb);
    read_cache[pos] = mb_store;
  } else if(sqlite3_step(statement) != SQLITE_DONE) {
    std::cerr << "Error in SQLiteDB::get_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    mb->dont_write_to_db = true;
    return mb;
  }
  sqlite3_finalize(statement);
  
  //If the mapblock wasn't found, an empty one is returned.
  return mb;
}

void SQLiteDB::set_mapblock(Vector3<int> pos, Mapblock *mb) {
  int row_version = 2;
  
  //Update read cache.
  unsigned int old_update_num = 0;
  bool found_in_cache = false;
  
  auto search = read_cache.find(pos);
  if(search != read_cache.end()) {
    found_in_cache = true;
    old_update_num = search->second->update_num;
    delete search->second;
  }
  Mapblock *mb_store = new Mapblock(*mb);
  read_cache[pos] = mb_store;
  
  if(!mb->dirty) { return; }
  //To avoid hitting the disk for lighting updates.
  if(found_in_cache && mb->update_num == old_update_num) { return; }
  
  if(mb->dont_write_to_db) {
    std::cerr << "SQLiteDB::set_mapblock: did not write mapblock at " << pos << " due to prior errors." << std::endl;
    return;
  }
  
  //Update database.
  const char *sql = "REPLACE INTO map (x, y, z, data, id_to_is, sunlit, dirty, version) VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
  sqlite3_stmt *statement;
  if(sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::set_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 1, pos.x) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::set_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 2, pos.y) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::set_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 3, pos.z) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::set_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return;
  }
  
  
  //Write a version 2 data blob.
  //FIXME probably not portable across big/little endian machines
  uint32_t data_compressed[MAPBLOCK_SIZE_X * MAPBLOCK_SIZE_Y * MAPBLOCK_SIZE_Z];
  
  uint32_t run_val = 0;
  size_t run_length = 0;
  size_t out_cursor = 0;
  
  for(int y = 0; y < MAPBLOCK_SIZE_Y; y++) {
    for(int x = 0; x < MAPBLOCK_SIZE_X; x++) {
      for(int z = 0; z < MAPBLOCK_SIZE_Z; z++) {
        uint32_t val = mb->data[x][y][z] & 0b00000000011111111111111111111111; //exclude lighting information
        if(run_length == 0) {
          run_val = val;
          run_length++;
        } else if(val == run_val && run_length < 512) { //only 9 bits are used to store the run length, so 512 is the max possible.
          run_length++;
        } else {
          data_compressed[out_cursor] = run_val | ((run_length - 1) << 23);
          out_cursor++;
          run_length = 0;
          
          run_val = val;
          run_length++;
        }
      }
    }
  }
  if(run_length > 0) {
    data_compressed[out_cursor] = run_val | ((run_length - 1) << 23);
    out_cursor++;
    run_length = 0;
  }
  if(sqlite3_bind_blob(statement, 4, data_compressed, out_cursor * sizeof(uint32_t), SQLITE_TRANSIENT) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::set_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return;
  }
  
  std::ostringstream IDtoIS_json;
  IDtoIS_json << "{";
  bool first = true;
  for(size_t i = 0; i < mb->IDtoIS.size(); i++) {
    if(!first) { IDtoIS_json << ","; }
    first = false;
    
    IDtoIS_json << "\"" << i << "\":\"" << json_escape(mb->IDtoIS[i]) << "\"";
  }
  IDtoIS_json << "}";
  std::string IDtoIS_json_str = IDtoIS_json.str();
  const char* IDtoIS_json_cstr = IDtoIS_json_str.c_str();
  if(sqlite3_bind_text(statement, 5, IDtoIS_json_cstr, -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::set_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return;
  }
  
  if(sqlite3_bind_int(statement, 6, mb->sunlit) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::set_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 7, mb->dirty) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::set_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 8, row_version) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::set_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return;
  }
  
  if(sqlite3_step(statement) != SQLITE_DONE) {
    std::cerr << "Error in SQLiteDB::set_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return;
  }
  sqlite3_finalize(statement);
}
