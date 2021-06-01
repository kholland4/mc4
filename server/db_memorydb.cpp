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
#include "log.h"

MapblockUpdateInfo MemoryDB::get_mapblockupdateinfo(MapPos<int> pos) {
  std::shared_lock<std::shared_mutex> d_lock(datastore_lock);
  
  auto search = datastore.find(pos);
  if(search != datastore.end()) {
    return MapblockUpdateInfo(search->second);
  }
  return MapblockUpdateInfo(pos);
}

void MemoryDB::set_mapblockupdateinfo(MapPos<int> pos, MapblockUpdateInfo info) {
  std::unique_lock<std::shared_mutex> d_lock(datastore_lock);
  
  auto search = datastore.find(pos);
  if(search != datastore.end()) {
    info.write_to_mapblock(search->second);
  }
}

Mapblock* MemoryDB::get_mapblock(MapPos<int> pos) {
  std::shared_lock<std::shared_mutex> d_lock(datastore_lock);
  
  auto search = datastore.find(pos);
  if(search != datastore.end()) {
    return new Mapblock(*(search->second));
  }
  return new Mapblock(pos);
}

MapblockCompressed* MemoryDB::get_mapblock_compressed(MapPos<int> pos) {
  std::shared_lock<std::shared_mutex> d_lock(datastore_lock);
  
  auto search = datastore.find(pos);
  if(search != datastore.end()) {
    return new MapblockCompressed(search->second);
  }
  
  //not found; return a nil mapblock
  return new MapblockCompressed(pos);
}

void MemoryDB::set_mapblock(MapPos<int> pos, Mapblock *mb) {
  std::unique_lock<std::shared_mutex> d_lock(datastore_lock);
  
  auto search = datastore.find(pos);
  if(search != datastore.end()) {
    delete search->second;
  }
  Mapblock *mb_store = new Mapblock(*mb);
  datastore[pos] = mb_store;
}

void MemoryDB::set_mapblock_if_not_exists(MapPos<int> pos, Mapblock *mb) {
  std::unique_lock<std::shared_mutex> d_lock(datastore_lock);
  
  auto search = datastore.find(pos);
  if(search != datastore.end()) {
    return;
  }
  Mapblock *mb_store = new Mapblock(*mb);
  datastore[pos] = mb_store;
}

void MemoryDB::clean_cache() {
  //TODO purge non-dirty mapblocks using similar algorithm to SQLiteDB
}



NodeMeta* MemoryDB::get_node_meta(MapPos<int> pos) {
  std::shared_lock<std::shared_mutex> d_lock(node_meta_datastore_lock);
  
  auto search = node_meta_datastore.find(pos);
  if(search != node_meta_datastore.end()) {
    return new NodeMeta(*(search->second));
  }
  return new NodeMeta(pos);
}
void MemoryDB::set_node_meta(MapPos<int> pos, NodeMeta *meta) {
  std::unique_lock<std::shared_mutex> d_lock(node_meta_datastore_lock);
  
  auto search = node_meta_datastore.find(pos);
  if(search != node_meta_datastore.end()) {
    delete search->second;
  }
  NodeMeta *meta_store = new NodeMeta(*meta);
  node_meta_datastore[pos] = meta_store;
}
