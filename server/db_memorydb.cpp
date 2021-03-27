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
  auto search = datastore.find(pos);
  if(search != datastore.end()) {
    return MapblockUpdateInfo(search->second);
  }
  return MapblockUpdateInfo(pos);
}

void MemoryDB::set_mapblockupdateinfo(MapPos<int> pos, MapblockUpdateInfo info) {
  auto search = datastore.find(pos);
  if(search != datastore.end()) {
    info.write_to_mapblock(search->second);
  }
}

Mapblock* MemoryDB::get_mapblock(MapPos<int> pos) {
  auto search = datastore.find(pos);
  if(search != datastore.end()) {
    return new Mapblock(*(search->second));
  }
  return new Mapblock(pos);
}

MapblockCompressed* MemoryDB::get_mapblock_compressed(MapPos<int> pos) {
  auto search = datastore.find(pos);
  if(search != datastore.end()) {
    return new MapblockCompressed(search->second);
  }
  
  //not found; return a nil mapblock
  return new MapblockCompressed(pos);
}

void MemoryDB::set_mapblock(MapPos<int> pos, Mapblock *mb) {
  auto search = datastore.find(pos);
  if(search != datastore.end()) {
    delete search->second;
  }
  Mapblock *mb_store = new Mapblock(*mb);
  datastore[pos] = mb_store;
}

void MemoryDB::clean_cache() {
  //TODO purge non-dirty mapblocks using similar algorithm to SQLiteDB
}
