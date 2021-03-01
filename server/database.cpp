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



SQLiteDB::SQLiteDB(const char* filename) {
  int rc = sqlite3_open(filename, &db);
  if(rc != SQLITE_OK) {
    std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
    exit(1);
  }
  
  const char *sql =
"CREATE TABLE IF NOT EXISTS map ( \
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
  
  //Mapblock doesn't exist; the default update info will be accurate.
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
  
  //Try database.
  const char *sql = "SELECT data, id_to_is, is_to_id, light_needs_update, sunlit, dirty FROM map WHERE x=? AND y=? AND z=? LIMIT 1;";
  sqlite3_stmt *statement;
  if(sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::get_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return new Mapblock(pos);
  }
  if(sqlite3_bind_int(statement, 1, pos.x) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::get_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return new Mapblock(pos);
  }
  if(sqlite3_bind_int(statement, 2, pos.y) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::get_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return new Mapblock(pos);
  }
  if(sqlite3_bind_int(statement, 3, pos.z) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::get_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return new Mapblock(pos);
  }
  
  //Will be returned as-is if not found in database.
  Mapblock *mb = new Mapblock(pos);
  
  if(sqlite3_step(statement) == SQLITE_ROW) {
    //Have row.
    mb->is_nil = false;
    
    const uint64_t *data_raw = (const uint64_t*)sqlite3_column_blob(statement, 0);
    int data_raw_len = sqlite3_column_bytes(statement, 0);
    if(data_raw_len != MAPBLOCK_SIZE_X * MAPBLOCK_SIZE_Y * MAPBLOCK_SIZE_Z * sizeof(uint64_t)) {
      std::cerr << "Error in SQLiteDB::get_mapblock: unexpected mapblock data size" << std::endl;
      sqlite3_finalize(statement);
      return mb;
    }
    const unsigned char *IDtoIS_raw = sqlite3_column_text(statement, 1);
    const unsigned char *IStoID_raw = sqlite3_column_text(statement, 2);
    mb->light_needs_update = sqlite3_column_int(statement, 3);
    mb->sunlit = (bool)sqlite3_column_int(statement, 4);
    mb->dirty = (bool)sqlite3_column_int(statement, 5);
    
    std::stringstream ss;
    ss << IDtoIS_raw;
    
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss, pt);
    
    mb->IDtoIS.clear();
    for(auto& it : pt) {
      std::string id_key = std::string(it.first);
      std::string itemstring = pt.get<std::string>(boost::property_tree::path(id_key, '\n'));
      mb->IDtoIS.push_back(itemstring);
    }
    
    std::stringstream ss2;
    ss2 << IStoID_raw;
    
    boost::property_tree::ptree pt2;
    boost::property_tree::read_json(ss2, pt2);
    
    mb->IStoID.clear();
    for(auto& it : pt2) {
      std::string itemstring = it.first;
      unsigned int id = pt2.get<unsigned int>(boost::property_tree::path(itemstring, '\n'));
      mb->IStoID[itemstring] = id;
    }
    
    //std::copy(data_raw, data_raw + MAPBLOCK_SIZE_X * MAPBLOCK_SIZE_Y * MAPBLOCK_SIZE_Z, mb->data);
    for(int x = 0; x < MAPBLOCK_SIZE_X; x++) {
      for(int y = 0; y < MAPBLOCK_SIZE_Y; y++) {
        for(int z = 0; z < MAPBLOCK_SIZE_Z; z++) {
          mb->data[x][y][z] = data_raw[x * MAPBLOCK_SIZE_Y * MAPBLOCK_SIZE_Z + y * MAPBLOCK_SIZE_Z + z];
        }
      }
    }
    
    //Since it's not cached, the lighting is probably outdated.
    if(mb->light_needs_update == 0) {
      mb->light_needs_update = 1;
    }
    
    //Save to read cache.
    Mapblock *mb_store = new Mapblock(*mb);
    read_cache[pos] = mb_store;
  } else if(sqlite3_step(statement) != SQLITE_DONE) {
    std::cerr << "Error in SQLiteDB::get_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return mb;
  }
  sqlite3_finalize(statement);
  
  return mb;
}

void SQLiteDB::set_mapblock(Vector3<int> pos, Mapblock *mb) {
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
  
  //Update database.
  const char *sql = "REPLACE INTO map (x, y, z, data, id_to_is, is_to_id, light_needs_update, sunlit, dirty) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
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
  //FIXME probably not portable across big/little endian machines
  uint64_t data_raw[MAPBLOCK_SIZE_X * MAPBLOCK_SIZE_Y * MAPBLOCK_SIZE_Z];
  for(int x = 0; x < MAPBLOCK_SIZE_X; x++) {
    for(int y = 0; y < MAPBLOCK_SIZE_Y; y++) {
      for(int z = 0; z < MAPBLOCK_SIZE_Z; z++) {
        data_raw[x * MAPBLOCK_SIZE_Y * MAPBLOCK_SIZE_Z + y * MAPBLOCK_SIZE_Z + z] = mb->data[x][y][z];
      }
    }
  }
  if(sqlite3_bind_blob(statement, 4, data_raw, MAPBLOCK_SIZE_X * MAPBLOCK_SIZE_Y * MAPBLOCK_SIZE_Z * sizeof(uint64_t), SQLITE_TRANSIENT) != SQLITE_OK) {
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
  
  std::ostringstream IStoID_json;
  IStoID_json << "{";
  first = true;
  for(auto p : mb->IStoID) {
    if(!first) { IStoID_json << ","; }
    first = false;
    
    IStoID_json << "\"" << json_escape(p.first) << "\":" << p.second;
  }
  IStoID_json << "}";
  std::string IStoID_json_str = IStoID_json.str();
  const char* IStoID_json_cstr = IStoID_json_str.c_str();
  if(sqlite3_bind_text(statement, 6, IStoID_json_cstr, -1, SQLITE_TRANSIENT) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::set_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return;
  }
  
  if(sqlite3_bind_int(statement, 7, mb->light_needs_update) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::set_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 8, mb->sunlit) != SQLITE_OK) {
    std::cerr << "Error in SQLiteDB::set_mapblock: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(statement);
    return;
  }
  if(sqlite3_bind_int(statement, 9, mb->dirty) != SQLITE_OK) {
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
