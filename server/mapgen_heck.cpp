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

#include "mapgen.h"
#include "log.h"

std::map<MapPos<int>, Mapblock*> MapgenHeck::generate_near(MapPos<int> pos) {
  Mapblock *mb = new Mapblock(pos);
  MapPos<int> global_offset = MapPos<int>(pos.x * MAPBLOCK_SIZE_X, pos.y * MAPBLOCK_SIZE_Y, pos.z * MAPBLOCK_SIZE_Z, pos.w, pos.world, pos.universe);
  
  unsigned int brick_id = mb->itemstring_to_id("default:brick");
  unsigned int iron_block_id = mb->itemstring_to_id("default:iron_block");
  
  for(int x = 0; x < MAPBLOCK_SIZE_X; x++) {
    for(int z = 0; z < MAPBLOCK_SIZE_Z; z++) {
      double val = perlin.accumulatedOctaveNoise3D((x + global_offset.x) / 50.0, (z + global_offset.z) / 50.0, global_offset.w / 30.0, 3);
      
      int height = std::floor(val * 50);
      height -= global_offset.y;
      
      if(height >= 0) {
        mb->sunlit = false;
        
        for(int y = 0; y < MAPBLOCK_SIZE_Y; y++) {
          if(y > height) {
            //air, which is already there
          } else if(y == height) {
            mb->data[x][y][z] = brick_id;
          } else if(y >= height - 2) {
            mb->data[x][y][z] = iron_block_id;
          } else {
            mb->data[x][y][z] = brick_id;
          }
        }
      }
    }
  }
  
  mb->is_nil = false;
  
  return std::map<MapPos<int>, Mapblock*>({{pos, mb}});
}
