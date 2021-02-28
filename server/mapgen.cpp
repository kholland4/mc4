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
#include <iostream>

Mapblock* MapgenDefault::generate_at(Vector3<int> pos) {
  Mapblock *mb = new Mapblock(pos);
  
  if(pos.y > 0) {
    //do nothing, leave it full of air per default
  } else if(pos.y == 0) {
    unsigned int grass_id = mb->itemstring_to_id("default:grass");
    unsigned int dirt_id = mb->itemstring_to_id("default:dirt");
    unsigned int stone_id = mb->itemstring_to_id("default:stone");
    
    for(int y = 0; y < MAPBLOCK_SIZE_Y; y++) {
      for(int x = 0; x < MAPBLOCK_SIZE_X; x++) {
        for(int z = 0; z < MAPBLOCK_SIZE_Z; z++) {
          if(y == 15) {
            mb->data[x][y][z] = grass_id;
          } else if(y >= 13) {
            mb->data[x][y][z] = dirt_id;
          } else {
            mb->data[x][y][z] = stone_id;
          }
        }
      }
    }
    
    mb->sunlit = false;
  } else {
    unsigned int stone_id = mb->itemstring_to_id("default:stone");
    
    for(int y = 0; y < MAPBLOCK_SIZE_Y; y++) {
      for(int x = 0; x < MAPBLOCK_SIZE_X; x++) {
        for(int z = 0; z < MAPBLOCK_SIZE_Z; z++) {
          mb->data[x][y][z] = stone_id;
        }
      }
    }
    
    mb->sunlit = false;
  }
  
  mb->is_nil = false;
  return mb;
}
