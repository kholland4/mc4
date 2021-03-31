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
    Map(Database& _db, std::map<int, World*> worlds);
    Mapblock* get_mapblock(MapPos<int> mb_pos);
    MapblockCompressed* get_mapblock_compressed(MapPos<int> mb_pos);
    void set_mapblock(MapPos<int> mb_pos, Mapblock *mb);
    Node get_node(MapPos<int> pos);
    void set_node(MapPos<int> pos, Node node);
    void update_mapblock_light(MapblockUpdateInfo info);
    void update_mapblock_light(std::set<MapPos<int>> prelocked, MapPos<int> min_pos, MapPos<int> max_pos);
    void update_mapblock_light(std::set<MapPos<int>> prelocked, std::set<MapPos<int>> mapblocks_to_update);
    MapPos<int> containing_mapblock(MapPos<int> pos);
    MapblockUpdateInfo get_mapblockupdateinfo(MapPos<int> mb_pos);
    
    void tick_fluids(std::set<MapPos<int>> mapblocks);
    
    std::map<int, World*> worlds;
  private:
    Mapblock* get_mapblock_known_nil(MapPos<int> mb_pos);
    void update_mapblock_light_optimized_singlenode_transparent(std::set<MapPos<int>> prelocked, MapPos<int> mb_pos, MapPos<int> rel_pos);
    void save_changed_lit_mapblocks(std::map<MapPos<int>, Mapblock*>& mapblocks, std::set<MapPos<int>>& mapblocks_to_update, bool do_clear_light_needs_update);
    
    Database& db;
};

#endif
