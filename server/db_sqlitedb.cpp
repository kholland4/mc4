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
#include "log.h"
#include "config.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

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
/* Version 3:

//NOTE: No changes were made to row storage format; row format remains at version 2

//Changes:
//  - recreate table with w, world, universe columns and new primary key

//Upgrade path:
//  - add new columns with default values 0

CREATE TABLE IF NOT EXISTS map ( \
  x INT,\
  y INT,\
  z INT,\
  w INT,\ (DEFAULT 0 for upgraded databases)
  world INT,\ (DEFAULT 0 for upgraded databases)
  universe INT,\ (DEFAULT 0 for upgraded databases)
  data MEDIUMBLOB,\
  id_to_is TEXT,\
  sunlit BOOLEAN,\
  dirty BOOLEAN,\
  version SMALLINT UNSIGNED,\
  primary key (x, y, z, w, world, universe)\
);

*/
/* Version 4:

//Changes:
//  - new player auth table
//  - new player data table

//Upgrade path:
//  - create the new tables

CREATE TABLE IF NOT EXISTS map ( \
  x INT,\
  y INT,\
  z INT,\
  w INT,\
  world INT,\
  universe INT,\
  data MEDIUMBLOB,\
  id_to_is TEXT,\
  sunlit BOOLEAN,\
  dirty BOOLEAN,\
  version SMALLINT UNSIGNED,\
  primary key (x, y, z, w, world, universe)\
);
CREATE TABLE IF NOT EXISTS player_auth ( \
  entry_id INTEGER PRIMARY KEY,\
  type VARCHAR(32),\
  login_name VARCHAR(255),\
  auth_id VARCHAR(80),\
  data TEXT\
);
CREATE TABLE IF NOT EXISTS player_data ( \
  auth_id VARCHAR(80) PRIMARY KEY,\
  name VARCHAR(255),\
  data MEDIUMTEXT\
);
*/
/* Version 5:

//Changes:
//  - new node meta table

//Upgrade path:
//  - create the new table

CREATE TABLE IF NOT EXISTS map ( \
  x INT,\
  y INT,\
  z INT,\
  w INT,\
  world INT,\
  universe INT,\
  data MEDIUMBLOB,\
  id_to_is TEXT,\
  sunlit BOOLEAN,\
  dirty BOOLEAN,\
  version SMALLINT UNSIGNED,\
  primary key (x, y, z, w, world, universe)\
);
CREATE TABLE IF NOT EXISTS node_meta ( \
  x INT,\
  y INT,\
  z INT,\
  w INT,\
  world INT,\
  universe INT,\
  data MEDIUMTEXT,\
  primary key (x, y, z, w, world, universe)\
);
CREATE TABLE IF NOT EXISTS player_auth ( \
  entry_id INTEGER PRIMARY KEY,\
  type VARCHAR(32),\
  login_name VARCHAR(255),\
  auth_id VARCHAR(80),\
  data TEXT\
);
CREATE TABLE IF NOT EXISTS player_data ( \
  auth_id VARCHAR(80) PRIMARY KEY,\
  name VARCHAR(255),\
  data MEDIUMTEXT\
);
*/

SQLiteDB::SQLiteDB(const char* filename) {
  std::unique_lock<std::shared_mutex> db_l(db_lock);
  
  log(LogSource::SQLITEDB, LogLevel::INFO, "SQLite3 database file is '" + std::string(filename) + "'");
  
  int rc = sqlite3_open(filename, &db);
  if(rc != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Can't open database: ") + std::string(sqlite3_errmsg(db)));
    exit(1);
  }
  
  //Read the database version.
  sqlite3_stmt *pragma_reader;
  if(sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &pragma_reader, NULL) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to read pragma 'user_version': ") + std::string(sqlite3_errmsg(db)));
    exit(1);
  }
  if(sqlite3_step(pragma_reader) == SQLITE_ROW) {
    db_version = sqlite3_column_int(pragma_reader, 0);
  } else {
    log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to read pragma 'user_version': ") + std::string(sqlite3_errmsg(db)));
    exit(1);
  }
  if(sqlite3_step(pragma_reader) != SQLITE_DONE) {
    log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to read pragma 'user_version': ") + std::string(sqlite3_errmsg(db)));
    exit(1);
  }
  sqlite3_finalize(pragma_reader);
  
  int orig_db_version = db_version;
  
  //If the version is 0, the database is either (a) empty and uninitialized or (b) a version 1 database, from the dark ages before versions were used
  //Find out by checking for the existence of a 'map' table.
  if(db_version == 0) {
    sqlite3_stmt *table_check;
    if(sqlite3_prepare_v2(db, "SELECT name FROM sqlite_master WHERE type='table' AND name='map';", -1, &table_check, NULL) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable check for existence of 'map' table: ") + std::string(sqlite3_errmsg(db)));
      exit(1);
    }
    
    int res = sqlite3_step(table_check);
    if(res == SQLITE_ROW) {
      //Table found, must be version 1.
      db_version = 1;
    } else if(res == SQLITE_DONE) {
      //No result, must be version 0.
    } else {
      log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable check for existence of 'map' table: ") + std::string(sqlite3_errmsg(db)));
      exit(1);
    }
    
    sqlite3_finalize(table_check);
  }
  
  //If version is still 0, the database is empty and uninitialized.
  //Create the appropriate table(s) and set the 'user_version' pragma.
  if(db_version == 0) {
    db_version = 5;
    
    std::map<std::string, const char*> sql = {
{"map",
"CREATE TABLE IF NOT EXISTS map ( \
  x INT,\
  y INT,\
  z INT,\
  w INT,\
  world INT,\
  universe INT,\
  data MEDIUMBLOB,\
  id_to_is TEXT,\
  sunlit BOOLEAN,\
  dirty BOOLEAN,\
  version SMALLINT UNSIGNED,\
  primary key (x, y, z, w, world, universe)\
);"},
{"node_meta",
"CREATE TABLE IF NOT EXISTS node_meta ( \
  x INT,\
  y INT,\
  z INT,\
  w INT,\
  world INT,\
  universe INT,\
  data MEDIUMTEXT,\
  primary key (x, y, z, w, world, universe)\
);"},
{"player_auth",
"CREATE TABLE IF NOT EXISTS player_auth ( \
  entry_id INTEGER PRIMARY KEY,\
  type VARCHAR(32),\
  login_name VARCHAR(255),\
  auth_id VARCHAR(80),\
  data TEXT\
);"},
{"player_data",
"CREATE TABLE IF NOT EXISTS player_data ( \
  auth_id VARCHAR(80) PRIMARY KEY,\
  name VARCHAR(255),\
  data MEDIUMTEXT\
);"}
    };
    
    for(auto it : sql) {
      sqlite3_stmt *statement;
      if(sqlite3_prepare_v2(db, it.second, -1, &statement, NULL) != SQLITE_OK) {
        log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to create table '" + it.first + "': ") + std::string(sqlite3_errmsg(db)));
        exit(1);
      }
      if(sqlite3_step(statement) != SQLITE_DONE) {
        log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to create table '" + it.first + "': ") + std::string(sqlite3_errmsg(db)));
        exit(1);
      }
      sqlite3_finalize(statement);
    }
    
    log(LogSource::SQLITEDB, LogLevel::NOTICE, std::string("Initialized new SQLite database with version ") + std::to_string(db_version));
  }
  
  //Upgrades from version 1 to version 2
  if(db_version == 1) {
    log(LogSource::SQLITEDB, LogLevel::NOTICE, "Upgrading database from version 1 to version 2");
    
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
        log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to migrate table 'map': ") + std::string(sqlite3_errmsg(db)));
        exit(1);
      }
      if(sqlite3_step(statement) != SQLITE_DONE) {
        log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to migrate table 'map': ") + std::string(sqlite3_errmsg(db)));
        exit(1);
      }
      sqlite3_finalize(statement);
    }
    
    db_version = 2;
  }
  
  //Upgrades from version 2 to version 3
  if(db_version == 2) {
    log(LogSource::SQLITEDB, LogLevel::NOTICE, "Upgrading database from version 2 to version 3");
    
    std::array<std::string, 7> sql_str = {
      /*"BEGIN EXCLUSIVE TRANSACTION;",
      "ALTER TABLE map ADD COLUMN w INT DEFAULT 0;",
      "ALTER TABLE map ADD COLUMN world INT DEFAULT 0;",
      "ALTER TABLE map ADD COLUMN universe INT DEFAULT 0;",
      "PRAGMA user_version = 3;",
      "COMMIT TRANSACTION;"*/
      "BEGIN EXCLUSIVE TRANSACTION;",
      "CREATE TABLE map_new ( \
        x INT,\
        y INT,\
        z INT,\
        w INT,\
        world INT,\
        universe INT,\
        data MEDIUMBLOB,\
        id_to_is TEXT,\
        sunlit BOOLEAN,\
        dirty BOOLEAN,\
        version SMALLINT UNSIGNED,\
        primary key (x, y, z, w, world, universe)\
      );",
      "INSERT INTO map_new (x, y, z, w, world, universe, data, id_to_is, sunlit, dirty, version) SELECT x, y, z, 0, 0, 0, data, id_to_is, sunlit, dirty, version FROM map;",
      "DROP TABLE map;",
      "ALTER TABLE map_new RENAME TO map;",
      "PRAGMA user_version = 3;",
      "COMMIT TRANSACTION;"
    };
    
    for(size_t i = 0; i < sql_str.size(); i++) {
      sqlite3_stmt *statement;
      if(sqlite3_prepare_v2(db, sql_str[i].c_str(), -1, &statement, NULL) != SQLITE_OK) {
        log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to migrate table 'map': ") + std::string(sqlite3_errmsg(db)));
        exit(1);
      }
      if(sqlite3_step(statement) != SQLITE_DONE) {
        log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to migrate table 'map': ") + std::string(sqlite3_errmsg(db)));
        exit(1);
      }
      sqlite3_finalize(statement);
    }
    
    db_version = 3;
  }
  
  //Upgrades from version 3 to version 4
  if(db_version == 3) {
    log(LogSource::SQLITEDB, LogLevel::NOTICE, "Upgrading database from version 3 to version 4");
    
    std::array<std::string, 5> sql_str = {
      "BEGIN EXCLUSIVE TRANSACTION;",
      "CREATE TABLE player_auth ( \
        entry_id INTEGER PRIMARY KEY,\
        type VARCHAR(32),\
        login_name VARCHAR(255),\
        auth_id VARCHAR(80),\
        data TEXT\
      );",
      "CREATE TABLE player_data ( \
        auth_id VARCHAR(80) PRIMARY KEY,\
        name VARCHAR(255),\
        data MEDIUMTEXT\
      );",
      "PRAGMA user_version = 4;",
      "COMMIT TRANSACTION;"
    };
    
    for(size_t i = 0; i < sql_str.size(); i++) {
      sqlite3_stmt *statement;
      if(sqlite3_prepare_v2(db, sql_str[i].c_str(), -1, &statement, NULL) != SQLITE_OK) {
        log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to migrate database: ") + std::string(sqlite3_errmsg(db)));
        exit(1);
      }
      if(sqlite3_step(statement) != SQLITE_DONE) {
        log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to migrate database: ") + std::string(sqlite3_errmsg(db)));
        exit(1);
      }
      sqlite3_finalize(statement);
    }
    
    db_version = 4;
  }
  
  //Upgrades from version 4 to version 5
  if(db_version == 4) {
    log(LogSource::SQLITEDB, LogLevel::NOTICE, "Upgrading database from version 4 to version 5");
    
    std::array<std::string, 4> sql_str = {
      "BEGIN EXCLUSIVE TRANSACTION;",
      "CREATE TABLE node_meta ( \
        x INT,\
        y INT,\
        z INT,\
        w INT,\
        world INT,\
        universe INT,\
        data MEDIUMTEXT,\
        primary key (x, y, z, w, world, universe)\
      );",
      "PRAGMA user_version = 5;",
      "COMMIT TRANSACTION;"
    };
    
    for(size_t i = 0; i < sql_str.size(); i++) {
      sqlite3_stmt *statement;
      if(sqlite3_prepare_v2(db, sql_str[i].c_str(), -1, &statement, NULL) != SQLITE_OK) {
        log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to migrate database: ") + std::string(sqlite3_errmsg(db)));
        exit(1);
      }
      if(sqlite3_step(statement) != SQLITE_DONE) {
        log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to migrate database: ") + std::string(sqlite3_errmsg(db)));
        exit(1);
      }
      sqlite3_finalize(statement);
    }
    
    db_version = 5;
  }
  
  
  //Store the DB user_version, if changed.
  if(db_version != orig_db_version) {
    std::string sql_str = "PRAGMA user_version = " + std::to_string(db_version) + ";";
    const char *sql = sql_str.c_str();
    
    sqlite3_stmt *pragma_writer;
    if(sqlite3_prepare_v2(db, sql, -1, &pragma_writer, NULL) != SQLITE_OK) {
      log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable write pragma 'user_version': ") + std::string(sqlite3_errmsg(db)));
      exit(1);
    }
    if(sqlite3_step(pragma_writer) != SQLITE_DONE) {
      log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable write pragma 'user_version': ") + std::string(sqlite3_errmsg(db)));
      exit(1);
    }
    sqlite3_finalize(pragma_writer);
  }
  
  
  //Get number of stored mapblock rows (to show to the user).
  std::map<int, std::string> sql_str;
  std::map<int, int> row_counts;
  sql_str[-2] = "SELECT COUNT(*) FROM player_data;";
  sql_str[-1] = "SELECT COUNT(*) FROM map;";
  //Iterate over possible versions.
  for(int i = 1; i <= 2; i++) {
    sql_str[i] = "SELECT COUNT(*) FROM map WHERE version=" + std::to_string(i) + ";";
  }
  
  
  for(auto it : sql_str) {
    sqlite3_stmt *count_checker;
    if(sqlite3_prepare_v2(db, it.second.c_str(), -1, &count_checker, NULL) != SQLITE_OK) {
      log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to count rows in table: ") + std::string(sqlite3_errmsg(db)));
      exit(1);
    }
    if(sqlite3_step(count_checker) == SQLITE_ROW) {
      row_counts[it.first] = sqlite3_column_int(count_checker, 0);
    } else {
      log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to count rows in table: ") + std::string(sqlite3_errmsg(db)));
      exit(1);
    }
    if(sqlite3_step(count_checker) != SQLITE_DONE) {
      log(LogSource::SQLITEDB, LogLevel::EMERG, std::string("Unable to count rows in table: ") + std::string(sqlite3_errmsg(db)));
      exit(1);
    }
    sqlite3_finalize(count_checker);
  }
  
  log(LogSource::SQLITEDB, LogLevel::INFO,
      std::string("Loaded SQLite database with ") + std::to_string(row_counts[-1]) +
      std::string(" mapblocks and ") + std::to_string(row_counts[-2]) +
      std::string(" players (version ") + std::to_string(db_version) + std::string(")"));
  for(auto it : row_counts) {
    int version = it.first;
    if(version < 1) { continue; }
    int count = it.second;
    if(count == 0) { continue; }
    log(LogSource::SQLITEDB, LogLevel::INFO, std::to_string(count) + std::string(" mapblocks are version ") + std::to_string(version));
  }
}

SQLiteDB::~SQLiteDB() {
  std::unique_lock<std::shared_mutex> db_l(db_lock);
  
  sqlite3_close_v2(db);
}

MapblockUpdateInfo SQLiteDB::get_mapblockupdateinfo(MapPos<int> pos) {
  std::unique_lock<std::shared_mutex> cache_l(cache_lock);
  
  //Try read cache.
  auto search = read_cache.find(pos);
  if(search != read_cache.end()) {
    //Move to front of read cache
    read_cache_hits.erase(search->second.second);
    typename std::list<MapPos<int>>::iterator k = read_cache_hits.insert(read_cache_hits.end(), pos);
    search->second.second = k;
    
    return MapblockUpdateInfo(search->second.first);
  }
  
  //Try L2 cache.
  auto search_L2 = L2_cache.find(pos);
  if(search_L2 != L2_cache.end()) {
    //Move to front of L2 cache
    L2_cache_hits.erase(search_L2->second.second);
    typename std::list<MapPos<int>>::iterator k = L2_cache_hits.insert(L2_cache_hits.end(), pos);
    search_L2->second.second = k;
    
    return MapblockUpdateInfo(search_L2->second.first);
  }
  
  //Mapblock doesn't exist or isn't loaded; the default update info will be accurate.
  return MapblockUpdateInfo(pos);
}

void SQLiteDB::set_mapblockupdateinfo(MapPos<int> pos, MapblockUpdateInfo info) {
  std::unique_lock<std::shared_mutex> cache_l(cache_lock);
  
  //Update info (with the exception of light_needs_update) only needs to be stored in read cache because it's irrelevant after all the clients disconnect.
  auto search = read_cache.find(pos);
  if(search != read_cache.end()) {
    //Move to front of read cache
    read_cache_hits.erase(search->second.second);
    typename std::list<MapPos<int>>::iterator k = read_cache_hits.insert(read_cache_hits.end(), pos);
    search->second.second = k;
    
    info.write_to_mapblock(search->second.first);
    return;
  }
  
  //If not in read cache, check L2 cache
  auto search_L2 = L2_cache.find(pos);
  if(search_L2 != L2_cache.end()) {
    //Move to front of L2 cache
    L2_cache_hits.erase(search_L2->second.second);
    typename std::list<MapPos<int>>::iterator k = L2_cache_hits.insert(L2_cache_hits.end(), pos);
    search_L2->second.second = k;
    
    info.write_to_mapblock(search_L2->second.first);
    return;
  }
  
  //not found
  return;
}

MapblockCompressed* SQLiteDB::get_mapblock_compressed(MapPos<int> pos) {
  std::unique_lock<std::shared_mutex> cache_l(cache_lock);
  
  //Try L2 cache
  auto search_L2 = L2_cache.find(pos);
  if(search_L2 != L2_cache.end()) {
    //Move to front of L2 cache
    L2_cache_hits.erase(search_L2->second.second);
    typename std::list<MapPos<int>>::iterator k = L2_cache_hits.insert(L2_cache_hits.end(), pos);
    search_L2->second.second = k;
    
    return new MapblockCompressed(search_L2->second.first);
  }
  
  cache_l.unlock();
  
  Mapblock *mb = get_mapblock(pos);
  MapblockCompressed *mbc = new MapblockCompressed(mb);
  delete mb;
  return mbc;
}

Mapblock* SQLiteDB::get_mapblock(MapPos<int> pos) {
  std::unique_lock<std::shared_mutex> cache_l(cache_lock);
  std::unique_lock<std::shared_mutex> db_l(db_lock);
  
  return get_mapblock_common_prelock(pos);
}

bool SQLiteDB::mapblock_exists_prelock(MapPos<int> pos) {
  //std::unique_lock<std::shared_mutex> cache_l(cache_lock);
  
  auto search = read_cache.find(pos);
  if(search != read_cache.end()) {
    return true;
  }
  auto search_L2 = L2_cache.find(pos);
  if(search_L2 != L2_cache.end()) {
    return true;
  }
  
  std::unique_lock<std::shared_mutex> db_l(db_lock);
  
  Mapblock *check = get_mapblock_common_prelock(pos);
  bool res = !check->is_nil;
  delete check;
  return res;
}

Mapblock* SQLiteDB::get_mapblock_common_prelock(MapPos<int> pos) {
  //Try read cache.
  auto search = read_cache.find(pos);
  if(search != read_cache.end()) {
    //Move to front of read_cache
    read_cache_hits.erase(search->second.second);
    typename std::list<MapPos<int>>::iterator k = read_cache_hits.insert(read_cache_hits.end(), pos);
    search->second.second = k;
    
    return new Mapblock(*(search->second.first));
  }
  
  //Try L2 cache
  auto search_L2 = L2_cache.find(pos);
  if(search_L2 != L2_cache.end()) {
    //Decompress & promote
    Mapblock *mb = search_L2->second.first.decompress();
    
    //Remove from L2 cache
    L2_cache_hits.erase(search_L2->second.second);
    L2_cache.erase(search_L2);
    
    //Add to read cache
    typename std::list<MapPos<int>>::iterator k = read_cache_hits.insert(read_cache_hits.end(), pos);
    Mapblock *mb_store = new Mapblock(*mb);
    read_cache.insert(std::make_pair(pos, std::make_pair(mb_store, k)));
    
    clean_cache_prelock();
    
    return mb;
  }
  
  Mapblock *mb = new Mapblock(pos);
  
  //Try database.
  const char *sql = "SELECT data, id_to_is, sunlit, dirty, version FROM map WHERE x=? AND y=? AND z=? AND w=? AND world=? AND universe=? LIMIT 1;";
  sqlite3_stmt *statement;
  if(sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    mb->dont_write_to_db = true;
    return mb;
  }
  if(sqlite3_bind_int(statement, 1, pos.x) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    mb->dont_write_to_db = true;
    return mb;
  }
  if(sqlite3_bind_int(statement, 2, pos.y) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    mb->dont_write_to_db = true;
    return mb;
  }
  if(sqlite3_bind_int(statement, 3, pos.z) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    mb->dont_write_to_db = true;
    return mb;
  }
  if(sqlite3_bind_int(statement, 4, pos.w) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    mb->dont_write_to_db = true;
    return mb;
  }
  if(sqlite3_bind_int(statement, 5, pos.world) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    mb->dont_write_to_db = true;
    return mb;
  }
  if(sqlite3_bind_int(statement, 6, pos.universe) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    mb->dont_write_to_db = true;
    return mb;
  }
  
  int step_result = sqlite3_step(statement);
  if(step_result == SQLITE_ROW) {
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
    
    //FIXME: try-catch?
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
        log(LogSource::SQLITEDB, LogLevel::ALERT, "Error in SQLiteDB::get_mapblock: unexpected mapblock data size");
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
            log(LogSource::SQLITEDB, LogLevel::ALERT, "Error in SQLiteDB::get_mapblock: decompressed mapblock is too long");
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
        log(LogSource::SQLITEDB, LogLevel::ALERT, "Error in SQLiteDB::get_mapblock: decompressed mapblock is too short");
        sqlite3_finalize(statement);
        mb->dont_write_to_db = true;
        return mb;
      }
      
      mb->is_nil = false;
    } else {
      log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_mapblock: unknown row version '") + std::to_string(row_version) + std::string("'"));
      sqlite3_finalize(statement);
      mb->dont_write_to_db = true;
      mb->is_nil = true;
      return mb;
    }
    
    //Since it's not cached, the lighting is probably outdated.
    mb->light_needs_update = 1;
    
    //Save to read cache.
    if(search != read_cache.end()) {
      delete search->second.first;
      
      //Update read cache & move to front
      read_cache_hits.erase(search->second.second);
      typename std::list<MapPos<int>>::iterator k = read_cache_hits.insert(read_cache_hits.end(), pos);
      Mapblock *mb_store = new Mapblock(*mb);
      search->second = std::make_pair(mb_store, k);
    } else {
      //Add to read cache
      typename std::list<MapPos<int>>::iterator k = read_cache_hits.insert(read_cache_hits.end(), pos);
      Mapblock *mb_store = new Mapblock(*mb);
      read_cache.insert(std::make_pair(pos, std::make_pair(mb_store, k)));
      
      clean_cache_prelock();
    }
  } else if(step_result != SQLITE_DONE) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    mb->dont_write_to_db = true;
    return mb;
  }
  sqlite3_finalize(statement);
  
  //If the mapblock wasn't found, an empty one is returned.
  return mb;
}

void SQLiteDB::set_mapblock(MapPos<int> pos, Mapblock *mb) {
  std::unique_lock<std::shared_mutex> cache_l(cache_lock);
  std::unique_lock<std::shared_mutex> db_l(db_lock);
  
  set_mapblock_common_prelock(pos, mb);
}
void SQLiteDB::set_mapblock_if_not_exists(MapPos<int> pos, Mapblock *mb) {
  std::unique_lock<std::shared_mutex> cache_l(cache_lock);
  
  if(mapblock_exists_prelock(pos)) {
    return;
  }
  
  std::unique_lock<std::shared_mutex> db_l(db_lock);
  
  set_mapblock_common_prelock(pos, mb);
}

void SQLiteDB::set_mapblock_common_prelock(MapPos<int> pos, Mapblock *mb) {
  int row_version = 2;
  
  //If in L2 cache, remove.
  auto search_L2 = L2_cache.find(pos);
  if(search_L2 != L2_cache.end()) {
    L2_cache_hits.erase(search_L2->second.second);
    L2_cache.erase(search_L2);
  }
  
  //Update read cache.
  unsigned int old_update_num = 0;
  bool found_in_cache = false;
  
  auto search = read_cache.find(pos);
  if(search != read_cache.end()) {
    found_in_cache = true;
    old_update_num = search->second.first->update_num;
    delete search->second.first;
    
    //Update read cache & move to front
    read_cache_hits.erase(search->second.second);
    typename std::list<MapPos<int>>::iterator k = read_cache_hits.insert(read_cache_hits.end(), pos);
    Mapblock *mb_store = new Mapblock(*mb);
    search->second = std::make_pair(mb_store, k);
  } else {
    //Add to read cache
    typename std::list<MapPos<int>>::iterator k = read_cache_hits.insert(read_cache_hits.end(), pos);
    Mapblock *mb_store = new Mapblock(*mb);
    read_cache.insert(std::make_pair(pos, std::make_pair(mb_store, k)));
    
    clean_cache_prelock();
  }
  
  if(!mb->dirty) { return; }
  //To avoid hitting the disk for lighting updates.
  if(found_in_cache && mb->update_num == old_update_num) { return; }
  
  if(mb->dont_write_to_db) {
    log(LogSource::SQLITEDB, LogLevel::WARNING, std::string("SQLiteDB::set_mapblock: did not write mapblock at ") + pos.to_string() + std::string(" due to prior errors."));
    return;
  }
  
  //Update database
  const char *sql = "REPLACE INTO map (x, y, z, w, world, universe, data, id_to_is, sunlit, dirty, version) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
  sqlite3_stmt *statement;
  if(sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 1, pos.x) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 2, pos.y) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 3, pos.z) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 4, pos.w) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 5, pos.world) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 6, pos.universe) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_mapblock: ") + std::string(sqlite3_errmsg(db)));
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
  if(sqlite3_bind_blob(statement, 7, data_compressed, out_cursor * sizeof(uint32_t), SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_mapblock: ") + std::string(sqlite3_errmsg(db)));
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
  if(sqlite3_bind_text(statement, 8, IDtoIS_json_cstr, -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  
  if(sqlite3_bind_int(statement, 9, mb->sunlit) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 10, mb->dirty) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 11, row_version) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  
  if(sqlite3_step(statement) != SQLITE_DONE) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_mapblock: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  sqlite3_finalize(statement);
}



void SQLiteDB::clean_cache() {
  std::unique_lock<std::shared_mutex> cache_l(cache_lock);
  clean_cache_prelock();
}
void SQLiteDB::clean_cache_prelock() {
  size_t target_cache_count = get_config<int>("database.L1_cache_target");
  size_t L2_target_cache_count = get_config<int>("database.L2_cache_target");
  //spam warnings bad; TODO -- warn more nicely, perhaps some sort of "server health info"
  /*if(target_cache_count < 1000) {
    log(LogSource::SQLITEDB, LogLevel::WARNING, "Low L1 cache target value! Please increase database.L1_cache_target.");
  }*/
  size_t evicted_count = 0;
  size_t L2_evicted_count = 0;
  /*if(read_cache_hits.size() <= target_cache_count) {
    log(LogSource::SQLITEDB, LogLevel::DEBUG, std::to_string(read_cache_hits.size()) + " mapblocks in cache, " +
                                              std::to_string(L2_cache_hits.size()) + " in L2 cache.");
    return;
  }*/
  
  while(read_cache_hits.size() > target_cache_count) {
    const auto oldest = read_cache.find(read_cache_hits.front());
    Mapblock *mb_to_demote = oldest->second.first;
    read_cache.erase(oldest);
    read_cache_hits.pop_front();
    evicted_count++;
    
    if(L2_cache.size() >= L2_target_cache_count) {
      const auto it = L2_cache.find(L2_cache_hits.front());
      L2_cache.erase(it);
      L2_cache_hits.pop_front();
      
      L2_evicted_count++;
    }
    
    typename std::list<MapPos<int>>::iterator k = L2_cache_hits.insert(L2_cache_hits.end(), mb_to_demote->pos);
    MapblockCompressed c(mb_to_demote);
    L2_cache.insert(std::make_pair(mb_to_demote->pos, std::make_pair(c, k)));
    
    delete mb_to_demote;
  }
  
  
  if(evicted_count >= 100 || L2_evicted_count >= 100) {
    log(LogSource::SQLITEDB, LogLevel::EXTRA, std::to_string(read_cache_hits.size()) + " mapblocks in cache (" + std::to_string(evicted_count) + " demoted), " +
                                              std::to_string(L2_cache.size()) + " in L2 cache (" + std::to_string(L2_evicted_count) + " evicted).");
  }
}



NodeMeta* SQLiteDB::get_node_meta(MapPos<int> pos) {
  std::unique_lock<std::shared_mutex> db_l(db_lock);
  
  NodeMeta *meta = new NodeMeta(pos);
  
  const char *sql = "SELECT data FROM node_meta WHERE x=? AND y=? AND z=? AND w=? AND world=? AND universe=? LIMIT 1;";
  sqlite3_stmt *statement;
  if(sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_node_meta: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return meta;
  }
  if(sqlite3_bind_int(statement, 1, pos.x) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_node_meta: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    meta->db_error = true;
    return meta;
  }
  if(sqlite3_bind_int(statement, 2, pos.y) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_node_meta: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    meta->db_error = true;
    return meta;
  }
  if(sqlite3_bind_int(statement, 3, pos.z) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_node_meta: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    meta->db_error = true;
    return meta;
  }
  if(sqlite3_bind_int(statement, 4, pos.w) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_node_meta: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    meta->db_error = true;
    return meta;
  }
  if(sqlite3_bind_int(statement, 5, pos.world) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_node_meta: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    meta->db_error = true;
    return meta;
  }
  if(sqlite3_bind_int(statement, 6, pos.universe) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::get_node_meta: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    meta->db_error = true;
    return meta;
  }
  
  int step_result = sqlite3_step(statement);
  if(step_result == SQLITE_ROW) {
    //Have row.
    const unsigned char *data_json_raw = sqlite3_column_text(statement, 0);
    std::string data_json(reinterpret_cast<const char*>(data_json_raw));
    
    delete meta;
    NodeMeta *result = new NodeMeta(pos, data_json);
    
    sqlite3_finalize(statement);
    return result;
  } else if(step_result == SQLITE_DONE) {
    //no more rows
    sqlite3_finalize(statement);
    return meta;
  } else {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::fetch_player_data: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    meta->db_error = true;
    return meta;
  }
}

void SQLiteDB::set_node_meta(MapPos<int> pos, NodeMeta *meta) {
  //if meta->is_nil, this will delete instead
  std::string json = meta->to_json();
  
  std::unique_lock<std::shared_mutex> db_l(db_lock);
  
  const char *sql_replace = "REPLACE INTO node_meta (x, y, z, w, world, universe, data) VALUES (?, ?, ?, ?, ?, ?, ?);";
  const char *sql_delete = "DELETE FROM node_meta WHERE x=? AND y=? AND z=? AND w=? AND world=? AND universe=?;";
  const char *sql = sql_replace;
  if(meta->is_nil)
    sql = sql_delete;
  
  sqlite3_stmt *statement;
  if(sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_node_meta: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 1, pos.x) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_node_meta: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 2, pos.y) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_node_meta: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 3, pos.z) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_node_meta: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 4, pos.w) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_node_meta: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 5, pos.world) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_node_meta: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 6, pos.universe) != SQLITE_OK) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_node_meta: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  if(!meta->is_nil) {
    if(sqlite3_bind_text(statement, 7, json.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
      log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_node_meta: ") + std::string(sqlite3_errmsg(db)));
      sqlite3_finalize(statement);
      return;
    }
  }
  
  if(sqlite3_step(statement) != SQLITE_DONE) {
    log(LogSource::SQLITEDB, LogLevel::ERR, std::string("Error in SQLiteDB::set_node_meta: ") + std::string(sqlite3_errmsg(db)));
    sqlite3_finalize(statement);
    return;
  }
  sqlite3_finalize(statement);
}
