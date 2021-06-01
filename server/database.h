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
#include <vector>
#include <list>
#include <shared_mutex>

#include <sqlite3.h>

#include "vector.h"
#include "mapblock.h"
#include "player_data.h"
#include "node_meta.h"

class Database {
  public:
    virtual MapblockUpdateInfo get_mapblockupdateinfo(MapPos<int> pos) = 0;
    virtual void set_mapblockupdateinfo(MapPos<int> pos, MapblockUpdateInfo info) = 0;
    virtual Mapblock* get_mapblock(MapPos<int> pos) = 0;
    virtual MapblockCompressed* get_mapblock_compressed(MapPos<int> pos) = 0;
    virtual void set_mapblock(MapPos<int> pos, Mapblock *mb) = 0;
    virtual void set_mapblock_if_not_exists(MapPos<int> pos, Mapblock *mb) = 0;
    virtual void clean_cache() = 0;
    
    virtual NodeMeta* get_node_meta(MapPos<int> pos) = 0;
    virtual void set_node_meta(MapPos<int> pos, NodeMeta *meta) = 0;
    
    virtual void store_pw_info(PlayerAuthInfo& info) = 0;
    virtual std::vector<PlayerAuthInfo> fetch_pw_info(std::string login_name, std::string type) = 0;
    virtual std::vector<PlayerAuthInfo> fetch_pw_info(std::string auth_id) = 0;
    virtual void update_pw_info(PlayerAuthInfo info) = 0;
    virtual void delete_pw_info(PlayerAuthInfo& info) = 0;
    
    virtual void store_player_data(PlayerData data) = 0;
    virtual PlayerData fetch_player_data(std::string auth_id) = 0;
    virtual void update_player_data(PlayerData data) = 0;
    virtual void delete_player_data(std::string auth_id) = 0;
    virtual bool player_data_name_used(std::string name) = 0;
    
    void lock_mapblock_shared(MapPos<int> pos);
    void unlock_mapblock_shared(MapPos<int> pos);
    void lock_mapblock_unique(MapPos<int> pos);
    void unlock_mapblock_unique(MapPos<int> pos);
    
    void lock_node_meta(MapPos<int> pos);
    void unlock_node_meta(MapPos<int> pos);
    
  private:
    //first mutex locks the map entry, second locks the mapblock
    std::map<MapPos<int>, std::pair<std::shared_mutex*, std::shared_mutex*>> mapblock_locks;
    std::shared_mutex mapblock_locks_lock;
    
    std::map<MapPos<int>, std::pair<std::shared_mutex*, std::shared_mutex*>> node_meta_locks;
    std::shared_mutex node_meta_locks_lock;
};

class MemoryDB : public Database {
  public:
    virtual MapblockUpdateInfo get_mapblockupdateinfo(MapPos<int> pos);
    virtual void set_mapblockupdateinfo(MapPos<int> pos, MapblockUpdateInfo info);
    virtual Mapblock* get_mapblock(MapPos<int> pos);
    virtual MapblockCompressed* get_mapblock_compressed(MapPos<int> pos);
    virtual void set_mapblock(MapPos<int> pos, Mapblock *mb);
    virtual void set_mapblock_if_not_exists(MapPos<int> pos, Mapblock *mb);
    virtual void clean_cache();
    
    virtual NodeMeta* get_node_meta(MapPos<int> pos);
    virtual void set_node_meta(MapPos<int> pos, NodeMeta *meta);
    
    virtual void store_pw_info(PlayerAuthInfo& info);
    virtual std::vector<PlayerAuthInfo> fetch_pw_info(std::string login_name, std::string type);
    virtual std::vector<PlayerAuthInfo> fetch_pw_info(std::string auth_id);
    virtual void update_pw_info(PlayerAuthInfo info);
    virtual void delete_pw_info(PlayerAuthInfo& info);
    
    virtual void store_player_data(PlayerData data);
    virtual PlayerData fetch_player_data(std::string auth_id);
    virtual void update_player_data(PlayerData data);
    virtual void delete_player_data(std::string auth_id);
    virtual bool player_data_name_used(std::string name);
  
  private:
    std::map<MapPos<int>, Mapblock*> datastore;
    std::shared_mutex datastore_lock;
    
    std::map<MapPos<int>, NodeMeta*> node_meta_datastore;
    std::shared_mutex node_meta_datastore_lock;
    
    std::map<unsigned int, PlayerAuthInfo*> pw_auth_store;
    unsigned int pw_auth_id_counter = 1; //locked by pw_auth_store_lock
    std::shared_mutex pw_auth_store_lock;
    
    std::map<std::string, PlayerData*> player_data_store;
    std::shared_mutex player_data_store_lock;
};

class SQLiteDB: public Database {
  public:
    SQLiteDB(const char* filename);
    virtual ~SQLiteDB();
    virtual MapblockUpdateInfo get_mapblockupdateinfo(MapPos<int> pos);
    virtual void set_mapblockupdateinfo(MapPos<int> pos, MapblockUpdateInfo info);
    virtual Mapblock* get_mapblock(MapPos<int> pos);
    virtual MapblockCompressed* get_mapblock_compressed(MapPos<int> pos);
    virtual void set_mapblock(MapPos<int> pos, Mapblock *mb);
    virtual void set_mapblock_if_not_exists(MapPos<int> pos, Mapblock *mb);
    virtual void clean_cache();
    
    virtual NodeMeta* get_node_meta(MapPos<int> pos);
    virtual void set_node_meta(MapPos<int> pos, NodeMeta *meta);
    
    virtual void store_pw_info(PlayerAuthInfo& info);
    virtual std::vector<PlayerAuthInfo> fetch_pw_info(std::string login_name, std::string type);
    virtual std::vector<PlayerAuthInfo> fetch_pw_info(std::string auth_id);
    virtual void update_pw_info(PlayerAuthInfo info);
    virtual void delete_pw_info(PlayerAuthInfo& info);
    
    virtual void store_player_data(PlayerData data);
    virtual PlayerData fetch_player_data(std::string auth_id);
    virtual void update_player_data(PlayerData data);
    virtual void delete_player_data(std::string auth_id);
    virtual bool player_data_name_used(std::string name);
  
  private:
    sqlite3 *db;
    int db_version;
    std::shared_mutex db_lock;
    
    std::map<MapPos<int>, std::pair<Mapblock*, typename std::list<MapPos<int>>::iterator>> read_cache;
    std::list<MapPos<int>> read_cache_hits;
    std::map<MapPos<int>, std::pair<MapblockCompressed, typename std::list<MapPos<int>>::iterator>> L2_cache;
    std::list<MapPos<int>> L2_cache_hits;
    std::shared_mutex cache_lock;
    
    Mapblock* get_mapblock_common_prelock(MapPos<int> pos);
    bool mapblock_exists_prelock(MapPos<int> pos);
    void set_mapblock_common_prelock(MapPos<int> pos, Mapblock *mb);
    void clean_cache_prelock();
};

#endif
