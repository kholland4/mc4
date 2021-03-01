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

#ifndef __MAP_H__
#define __MAP_H__

#include "database.h"
#include "vector.h"
#include "node.h"
#include "mapgen.h"
#include "mapblock.h"

#include <map>
#include <set>

class Map {
  public:
    Map(Database& _db, Mapgen& mapgen);
    Mapblock* get_mapblock(Vector3<int> mb_pos);
    void set_mapblock(Vector3<int> mb_pos, Mapblock *mb);
    Node get_node(Vector3<int> pos);
    void set_node(Vector3<int> pos, Node node);
    void update_mapblock_light(MapblockUpdateInfo info);
    void update_mapblock_light(Vector3<int> min_pos, Vector3<int> max_pos);
    void update_mapblock_light(std::set<Vector3<int>>);
    void update_mapblock_light_optimized_singlenode_transparent(Vector3<int> mb_pos, Vector3<int> rel_pos);
    Vector3<int> containing_mapblock(Vector3<int> pos);
    MapblockUpdateInfo get_mapblockupdateinfo(Vector3<int> mb_pos);
  
  private:
    Database& db;
    Mapgen& mapgen;
};

#endif
