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

class Database {
  public:
    virtual MapblockUpdateInfo get_mapblockupdateinfo(Vector3<int> pos) = 0;
    virtual void set_mapblockupdateinfo(Vector3<int> pos, MapblockUpdateInfo info) = 0;
    virtual Mapblock* get_mapblock(Vector3<int> pos) = 0;
    virtual void set_mapblock(Vector3<int> pos, Mapblock *mb) = 0;
    virtual void clean_cache() = 0;
};

class MemoryDB : public Database {
  public:
    virtual MapblockUpdateInfo get_mapblockupdateinfo(Vector3<int> pos);
    virtual void set_mapblockupdateinfo(Vector3<int> pos, MapblockUpdateInfo info);
    virtual Mapblock* get_mapblock(Vector3<int> pos);
    virtual void set_mapblock(Vector3<int> pos, Mapblock *mb);
    virtual void clean_cache();
  
  private:
    std::map<Vector3<int>, Mapblock*> datastore;
};

class SQLiteDB: public Database {
  public:
    SQLiteDB(const char* filename);
    virtual ~SQLiteDB();
    virtual MapblockUpdateInfo get_mapblockupdateinfo(Vector3<int> pos);
    virtual void set_mapblockupdateinfo(Vector3<int> pos, MapblockUpdateInfo info);
    virtual Mapblock* get_mapblock(Vector3<int> pos);
    virtual void set_mapblock(Vector3<int> pos, Mapblock *mb);
    virtual void clean_cache();
  
  private:
    sqlite3 *db;
    int db_version;
    std::map<Vector3<int>, Mapblock*> read_cache;
    std::map<Vector3<int>, std::chrono::time_point<std::chrono::steady_clock>> read_cache_hits;
};

#endif
