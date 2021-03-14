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

#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <map>
#include <chrono>

#include <sqlite3.h>

#include "vector.h"
#include "mapblock.h"
#include "player_data.h"

class Database {
  public:
    virtual MapblockUpdateInfo get_mapblockupdateinfo(MapPos<int> pos) = 0;
    virtual void set_mapblockupdateinfo(MapPos<int> pos, MapblockUpdateInfo info) = 0;
    virtual Mapblock* get_mapblock(MapPos<int> pos) = 0;
    virtual void set_mapblock(MapPos<int> pos, Mapblock *mb) = 0;
    virtual void clean_cache() = 0;
    
    virtual void store_pw_info(PlayerPasswordAuthInfo info) = 0;
    virtual PlayerPasswordAuthInfo fetch_pw_info(std::string login_name) = 0;
    virtual void update_pw_info(std::string old_login_name, PlayerPasswordAuthInfo info) = 0;
    virtual void delete_pw_info(std::string login_name) = 0;
    
    virtual void store_player_data(PlayerData data) = 0;
    virtual PlayerData fetch_player_data(std::string auth_id) = 0;
    virtual void update_player_data(PlayerData data) = 0;
    virtual void delete_player_data(std::string auth_id) = 0;
};

class MemoryDB : public Database {
  public:
    virtual MapblockUpdateInfo get_mapblockupdateinfo(MapPos<int> pos);
    virtual void set_mapblockupdateinfo(MapPos<int> pos, MapblockUpdateInfo info);
    virtual Mapblock* get_mapblock(MapPos<int> pos);
    virtual void set_mapblock(MapPos<int> pos, Mapblock *mb);
    virtual void clean_cache();
    
    virtual void store_pw_info(PlayerPasswordAuthInfo info);
    virtual PlayerPasswordAuthInfo fetch_pw_info(std::string login_name);
    virtual void update_pw_info(std::string old_login_name, PlayerPasswordAuthInfo info); //might change login name
    virtual void delete_pw_info(std::string login_name);
    
    virtual void store_player_data(PlayerData data);
    virtual PlayerData fetch_player_data(std::string auth_id);
    virtual void update_player_data(PlayerData data);
    virtual void delete_player_data(std::string auth_id);
  
  private:
    std::map<MapPos<int>, Mapblock*> datastore;
    std::map<std::string, PlayerPasswordAuthInfo*> pw_auth_store;
    std::map<std::string, PlayerData*> player_data_store;
};

class SQLiteDB: public Database {
  public:
    SQLiteDB(const char* filename);
    virtual ~SQLiteDB();
    virtual MapblockUpdateInfo get_mapblockupdateinfo(MapPos<int> pos);
    virtual void set_mapblockupdateinfo(MapPos<int> pos, MapblockUpdateInfo info);
    virtual Mapblock* get_mapblock(MapPos<int> pos);
    virtual void set_mapblock(MapPos<int> pos, Mapblock *mb);
    virtual void clean_cache();
  
  private:
    sqlite3 *db;
    int db_version;
    std::map<MapPos<int>, Mapblock*> read_cache;
    std::map<MapPos<int>, std::chrono::time_point<std::chrono::steady_clock>> read_cache_hits;
};

#endif
