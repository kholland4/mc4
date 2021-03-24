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

#ifndef __MAPBLOCK_H__
#define __MAPBLOCK_H__

#include <string>
#include <vector>
#include <map>
#include "vector.h"
#include "node.h"

class Mapblock {
  public:
    Mapblock(MapPos<int> _pos);
    
    Node get_node_rel(MapPos<int> rel_pos);
    void set_node_rel(MapPos<int> rel_pos, Node node);
    
    //Utility functions for direct modification of the data array (used primarily in mapgen).
    unsigned int itemstring_to_id(std::string itemstring);
    std::string id_to_itemstring(unsigned int id);
    
    std::string as_json();
    
    const MapPos<int> pos;
    
    //These properties are set to sensible defaults upon mapblock initialization, but they are not updated by the Mapblock class.
    //They are managed by other classes that operate on mapblocks.
    //For example, Map::set_node will set light_needs_update to 2 and increment update_num.
    unsigned int update_num;
    unsigned int light_update_num;
    int light_needs_update;
    
    std::vector<std::string> IDtoIS;
    std::map<std::string, unsigned int> IStoID;
    
    bool sunlit; //Set to 'false' after direct modification that changes any node from air to non-air
    
    bool is_nil; //Set to 'false' after directly modifying the data array.
    bool dirty;
    bool dont_write_to_db; //Will be set to 'true' if there is a database error when loading a mapblock, this ensures that the botched mapblock won't overwrite the existing one.
    uint32_t data[MAPBLOCK_SIZE_X][MAPBLOCK_SIZE_Y][MAPBLOCK_SIZE_Z];
};

class MapblockUpdateInfo {
  public:
    MapblockUpdateInfo() : pos(MapPos<int>(0, 0, 0, 0, 0, 0)), update_num(0), light_update_num(0), light_needs_update(1) {}; //FIXME this constructor shouldn't be needed
    MapblockUpdateInfo(MapPos<int> _pos) : pos(_pos), update_num(0), light_update_num(0), light_needs_update(1) {};
    MapblockUpdateInfo(Mapblock *mb) : pos(mb->pos), update_num(mb->update_num), light_update_num(mb->light_update_num), light_needs_update(mb->light_needs_update) {};
    /*bool operator!=(Mapblock *mb) const {
      return update_num != mb->update_num || light_update_num != mb->light_update_num || light_needs_update != mb->light_needs_update;
    };*/
    bool operator!=(MapblockUpdateInfo other) const {
      return update_num != other.update_num || light_update_num != other.light_update_num || light_needs_update != other.light_needs_update;
    };
    void write_to_mapblock(Mapblock *mb) {
      mb->update_num = update_num;
      mb->light_update_num = light_update_num;
      mb->light_needs_update = light_needs_update;
    };
    
    MapPos<int> pos;
    
    unsigned int update_num;
    unsigned int light_update_num;
    int light_needs_update;
};

#endif
