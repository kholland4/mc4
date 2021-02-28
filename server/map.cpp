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

#include "map.h"

#include <algorithm>

#define SUNLIGHT_CHECK_DISTANCE 4

Map::Map(Database& _db, Mapgen& _mapgen)
    : db(_db), mapgen(_mapgen)
{
  
}

Node Map::get_node(Vector3<int> pos) {
  //C++ integer division always rounds "towards zero", i. e. the fractional part is discarded.
  //Unfortunately, we want to floor the result -- round it down.
  //For positive numbers, C++ gives us the correct answer.
  //For negative numbers, it may not. -9 / 4 gives -2, but we want -3.
  //However, when a negative dividend is divided evenly by its divisor, we get the desired result (i. e. -8 / 2 = -4).
  Vector3<int> mb_pos(
      (pos.x < 0 && pos.x % MAPBLOCK_SIZE_X != 0) ? (pos.x / MAPBLOCK_SIZE_X - 1) : (pos.x / MAPBLOCK_SIZE_X),
      (pos.y < 0 && pos.y % MAPBLOCK_SIZE_Y != 0) ? (pos.y / MAPBLOCK_SIZE_Y - 1) : (pos.y / MAPBLOCK_SIZE_Y),
      (pos.z < 0 && pos.z % MAPBLOCK_SIZE_Z != 0) ? (pos.z / MAPBLOCK_SIZE_Z - 1) : (pos.z / MAPBLOCK_SIZE_Z));
  
  //Since the modulo operation gives remainders, doing something like -9 % 5 would give -4. We wrap this around into the positive interval [0, MAPBLOCK_SIZE_*).
  Vector3<int> rel_pos(
      ((pos.x % MAPBLOCK_SIZE_X) + MAPBLOCK_SIZE_X) % MAPBLOCK_SIZE_X,
      ((pos.y % MAPBLOCK_SIZE_Y) + MAPBLOCK_SIZE_Y) % MAPBLOCK_SIZE_Y,
      ((pos.z % MAPBLOCK_SIZE_Z) + MAPBLOCK_SIZE_Z) % MAPBLOCK_SIZE_Z);
  
  Mapblock *mb = get_mapblock(mb_pos);
  Node node = mb->get_node_rel(rel_pos);
  delete mb;
  return node;
}

//High-level setting of a node.
//Everything is handled automatically, including lighting.
void Map::set_node(Vector3<int> pos, Node node) {
  Vector3<int> mb_pos(
      (pos.x < 0 && pos.x % MAPBLOCK_SIZE_X != 0) ? (pos.x / MAPBLOCK_SIZE_X - 1) : (pos.x / MAPBLOCK_SIZE_X),
      (pos.y < 0 && pos.y % MAPBLOCK_SIZE_Y != 0) ? (pos.y / MAPBLOCK_SIZE_Y - 1) : (pos.y / MAPBLOCK_SIZE_Y),
      (pos.z < 0 && pos.z % MAPBLOCK_SIZE_Z != 0) ? (pos.z / MAPBLOCK_SIZE_Z - 1) : (pos.z / MAPBLOCK_SIZE_Z));
  Vector3<int> rel_pos(
      ((pos.x % MAPBLOCK_SIZE_X) + MAPBLOCK_SIZE_X) % MAPBLOCK_SIZE_X,
      ((pos.y % MAPBLOCK_SIZE_Y) + MAPBLOCK_SIZE_Y) % MAPBLOCK_SIZE_Y,
      ((pos.z % MAPBLOCK_SIZE_Z) + MAPBLOCK_SIZE_Z) % MAPBLOCK_SIZE_Z);
  
  Mapblock *mb = get_mapblock(mb_pos);
  mb->set_node_rel(rel_pos, node);
  set_mapblock(mb_pos, mb);
  delete mb;
}

Mapblock* Map::get_mapblock(Vector3<int> mb_pos) {
  Mapblock *mb = db.get_mapblock(mb_pos);
  if(!mb->is_nil) { return mb; }
  
  //The mapblock is not held by the database, so we got an empty one.
  //We must generate some data to fill it.
  //The resultant mapblock will be sent to the database for caching,
  //but because it isn't marked as 'dirty', it shouldn't be stored to disk (it could be regenerated at any time).
  
  Mapblock *new_mb = mapgen.generate_at(mb_pos);
  db.set_mapblock(mb_pos, new_mb);
  return new_mb;
}

void Map::set_mapblock(Vector3<int> mb_pos, Mapblock *mb) {
  mb->light_needs_update = 2;
  mb->update_num++;
  mb->dirty = true;
  db.set_mapblock(mb_pos, mb);
  
  update_mapblock_light(MapblockUpdateInfo(mb));
}

void Map::update_mapblock_light(MapblockUpdateInfo info) {
  if(info.light_needs_update == 1) {
    update_mapblock_light(info.pos, info.pos);
  } else if(info.light_needs_update == 2) {
    update_mapblock_light(info.pos - Vector3<int>(1, 1, 1), info.pos + Vector3<int>(1, 1, 1));
  } else {
    return; //should never happen
  }
  
  MapblockUpdateInfo new_info = db.get_mapblockupdateinfo(info.pos);
  new_info.light_needs_update = 0;
  db.set_mapblockupdateinfo(info.pos, new_info);
}

void Map::update_mapblock_light(Vector3<int> min_pos, Vector3<int> max_pos) {
  std::set<Vector3<int>> mapblock_list;
  for(int x = min_pos.x; x <= max_pos.x; x++) {
    for(int y = min_pos.y; y <= max_pos.y; y++) {
      for(int z = min_pos.z; z <= max_pos.z; z++) {
        mapblock_list.insert(Vector3<int>(x, y, z));
      }
    }
  }
  update_mapblock_light(mapblock_list);
}

enum LightCascadeType {LC_NORM, LC_SUN};
Vector3<int> faces[6] = {
  Vector3<int>(1, 0, 0),
  Vector3<int>(0, 1, 0),
  Vector3<int>(0, 0, 1),
  Vector3<int>(-1, 0, 0),
  Vector3<int>(0, -1, 0),
  Vector3<int>(0, 0, -1)
};
Vector3<int> rev_faces[6] = {
  Vector3<int>(-1, 0, 0),
  Vector3<int>(0, -1, 0),
  Vector3<int>(0, 0, -1),
  Vector3<int>(1, 0, 0),
  Vector3<int>(0, 1, 0),
  Vector3<int>(0, 0, 1)
};
void light_cascade_fast(std::map<Vector3<int>, Mapblock*>& mapblocks, std::set<Vector3<int>>& mapblocks_to_update, Mapblock *mb, Vector3<int> mb_pos, Vector3<int> rel_pos, unsigned int light_level, LightCascadeType type, Vector3<int> skip_face, bool bleed_mode = false) {
  if(light_level == 0 && !bleed_mode) { return; }
  
  uint64_t val = mb->data[rel_pos.x][rel_pos.y][rel_pos.z];
  unsigned int light = (val >> 23) & 255;
  
  //Stop if node is opaque.
  if((light & 15) == 0) { return; }
  
  if(bleed_mode) {
    if(type == LC_NORM) {
      light_level = light & 0x0F;
    } else if(type == LC_SUN) {
      light_level = (light >> 4) & 0x0F;
    }
  }
  
  if(light_level == 0) { return; }
  
  bool save = false;
  if(type == LC_NORM) {
    if(light_level > (light & 0x0F)) {
      light = (light & 0xF0) | (light_level & 0x0F);
      save = true;
    }
  } else if(type == LC_SUN) {
    if(light_level > ((light >> 4) & 0x0F)) {
      light = (light & 0x0F) | ((light_level << 4) & 0xF0);
      save = true;
    }
  }
  
  if(save || bleed_mode) {
    //if(in_update) {
    mb->data[rel_pos.x][rel_pos.y][rel_pos.z] = (mb->data[rel_pos.x][rel_pos.y][rel_pos.z] & 0b1111111111111111111111111111111110000000011111111111111111111111UL) | (light << 23);
    //}
    
    light_level--;
    if(light_level > 0) {
      //TODO only try one direction for bleed_mode?
      for(int i = 0; i < 6; i++) {
        if(faces[i] == skip_face) { continue; }
        
        Vector3<int> adj = rel_pos + faces[i];
        Vector3<int> mb_pos_n(mb_pos);
        if(adj.x >= 0 && adj.x < MAPBLOCK_SIZE_X &&
           adj.y >= 0 && adj.y < MAPBLOCK_SIZE_Y &&
           adj.z >= 0 && adj.z < MAPBLOCK_SIZE_Z) {
          light_cascade_fast(mapblocks, mapblocks_to_update, mb, mb_pos_n, adj, light_level, type, rev_faces[i]);
        } else {
          if(adj.x < 0               ) { adj.x += MAPBLOCK_SIZE_X; mb_pos_n.x--; }
          if(adj.x >= MAPBLOCK_SIZE_X) { adj.x -= MAPBLOCK_SIZE_X; mb_pos_n.x++; }
          if(adj.y < 0               ) { adj.y += MAPBLOCK_SIZE_Y; mb_pos_n.y--; }
          if(adj.y >= MAPBLOCK_SIZE_Y) { adj.y -= MAPBLOCK_SIZE_Y; mb_pos_n.y++; }
          if(adj.z < 0               ) { adj.z += MAPBLOCK_SIZE_Z; mb_pos_n.z--; }
          if(adj.z >= MAPBLOCK_SIZE_Z) { adj.z -= MAPBLOCK_SIZE_Z; mb_pos_n.z++; }
          
          bool in_update = mapblocks_to_update.find(mb_pos_n) != mapblocks_to_update.end();
          if(!in_update) { continue; }
          
          auto search = mapblocks.find(mb_pos_n);
          if(search == mapblocks.end()) { continue; }
          Mapblock *mb_n = search->second;
          
          light_cascade_fast(mapblocks, mapblocks_to_update, mb_n, mb_pos_n, adj, light_level, type, rev_faces[i]);
        }
      }
    }
  }
}
void light_cascade(std::map<Vector3<int>, Mapblock*>& mapblocks, std::set<Vector3<int>>& mapblocks_to_update, Vector3<int> pos, unsigned int light_level, LightCascadeType type, bool bleed_mode = false) {
  if(light_level == 0 && !bleed_mode) { return; }
  
  Vector3<int> mb_pos(
      (pos.x < 0 && pos.x % MAPBLOCK_SIZE_X != 0) ? (pos.x / MAPBLOCK_SIZE_X - 1) : (pos.x / MAPBLOCK_SIZE_X),
      (pos.y < 0 && pos.y % MAPBLOCK_SIZE_Y != 0) ? (pos.y / MAPBLOCK_SIZE_Y - 1) : (pos.y / MAPBLOCK_SIZE_Y),
      (pos.z < 0 && pos.z % MAPBLOCK_SIZE_Z != 0) ? (pos.z / MAPBLOCK_SIZE_Z - 1) : (pos.z / MAPBLOCK_SIZE_Z));
  
  bool in_update = false;
  if(!bleed_mode) {
    in_update = mapblocks_to_update.find(mb_pos) != mapblocks_to_update.end();
    if(!in_update) { return; }
  }
  
  auto search = mapblocks.find(mb_pos);
  if(search == mapblocks.end()) { return; }
  Mapblock *mb = search->second;
  
  Vector3<int> rel_pos(
      ((pos.x % MAPBLOCK_SIZE_X) + MAPBLOCK_SIZE_X) % MAPBLOCK_SIZE_X,
      ((pos.y % MAPBLOCK_SIZE_Y) + MAPBLOCK_SIZE_Y) % MAPBLOCK_SIZE_Y,
      ((pos.z % MAPBLOCK_SIZE_Z) + MAPBLOCK_SIZE_Z) % MAPBLOCK_SIZE_Z);
  
  uint64_t val = mb->data[rel_pos.x][rel_pos.y][rel_pos.z];
  unsigned int light = (val >> 23) & 255;
  
  //Stop if node is opaque.
  if((light & 15) == 0) { return; }
  
  if(bleed_mode) {
    if(type == LC_NORM) {
      light_level = light & 0x0F;
    } else if(type == LC_SUN) {
      light_level = (light >> 4) & 0x0F;
    }
  }
  
  if(light_level == 0) { return; }
  
  bool save = false;
  if(type == LC_NORM) {
    if(light_level > (light & 0x0F)) {
      light = (light & 0xF0) | (light_level & 0x0F);
      save = true;
    }
  } else if(type == LC_SUN) {
    if(light_level > ((light >> 4) & 0x0F)) {
      light = (light & 0x0F) | ((light_level << 4) & 0xF0);
      save = true;
    }
  }
  
  if(save || bleed_mode) {
    if(in_update) {
      mb->data[rel_pos.x][rel_pos.y][rel_pos.z] = (mb->data[rel_pos.x][rel_pos.y][rel_pos.z] & 0b1111111111111111111111111111111110000000011111111111111111111111UL) | (light << 23);
    }
    
    light_level--;
    if(light_level > 0) {
      //TODO only try one direction for bleed_mode?
      /*light_cascade(mapblocks, mapblocks_to_update, pos + Vector3<int>(1, 0, 0), light_level, type);
      light_cascade(mapblocks, mapblocks_to_update, pos + Vector3<int>(0, 1, 0), light_level, type);
      light_cascade(mapblocks, mapblocks_to_update, pos + Vector3<int>(0, 0, 1), light_level, type);
      light_cascade(mapblocks, mapblocks_to_update, pos + Vector3<int>(-1, 0, 0), light_level, type);
      light_cascade(mapblocks, mapblocks_to_update, pos + Vector3<int>(0, -1, 0), light_level, type);
      light_cascade(mapblocks, mapblocks_to_update, pos + Vector3<int>(0, 0, -1), light_level, type);*/
      for(int i = 0; i < 6; i++) {
        Vector3<int> adj = rel_pos + faces[i];
        if(adj.x >= 0 && adj.x < MAPBLOCK_SIZE_X &&
           adj.y >= 0 && adj.y < MAPBLOCK_SIZE_Y &&
           adj.z >= 0 && adj.z < MAPBLOCK_SIZE_Z) {
          light_cascade_fast(mapblocks, mapblocks_to_update, mb, mb_pos, adj, light_level, type, rev_faces[i]);
        } else {
          light_cascade(mapblocks, mapblocks_to_update, pos + faces[i], light_level, type);
        }
      }
    }
  }
}

void Map::update_mapblock_light(std::set<Vector3<int>> mapblocks_to_update) {
  std::map<Vector3<int>, Mapblock*> mapblocks;
  std::map<Vector3<int>, std::map<unsigned int, NodeDef>> def_tables;
  
  //Load each requested mapblock plus any adjacent ones (adjacent meaning +/- 1 away on any axis).
  //Also fetch mapblocks up to SUNLIGHT_CHECK_DISTANCE above the target (for sunlight).
  //Also precompute tables of definitions.
  for(auto i : mapblocks_to_update) {
    Vector3<int> center_pos = i;
    for(int x = center_pos.x - 1; x <= center_pos.x + 1; x++) {
      for(int z = center_pos.z - 1; z <= center_pos.z + 1; z++) {
        for(int y = center_pos.y - 1; y <= center_pos.y + ((x == center_pos.x && z == center_pos.z) ? SUNLIGHT_CHECK_DISTANCE : 1); y++) {
          Vector3<int> pos(x, y, z);
          if(mapblocks.find(pos) != mapblocks.end()) { continue; }
          
          mapblocks[pos] = get_mapblock(pos);
          
          std::map<unsigned int, NodeDef> def_table;
          for(unsigned n = 0; n < mapblocks[pos]->IDtoIS.size(); n++) {
            std::string itemstring = mapblocks[pos]->id_to_itemstring(n);
            NodeDef def = get_node_def(itemstring);
            def_table[n] = def;
          }
          
          def_tables[pos] = def_table;
        }
      }
    }
  }
  
  std::vector<std::pair<Vector3<int>, unsigned int>> light_sources;
  std::vector<std::pair<Vector3<int>, unsigned int>> sunlight_sources;
  
  //Clear light in the mapblocks being updated.
  //Compute sunlight and find light sources as well.
  for(auto i : mapblocks_to_update) {
    Vector3<int> mb_pos = i;
    Mapblock *mb = mapblocks[mb_pos];
    std::map<unsigned int, NodeDef> def_table = def_tables[mb_pos];
    
    for(int x = 0; x < MAPBLOCK_SIZE_X; x++) {
      for(int z = 0; z < MAPBLOCK_SIZE_Z; z++) {
        bool has_sun = false;
        if(mb->sunlit) {
          has_sun = true;
        } else {
          for(int mb_y = mb_pos.y + 1; mb_y <= mb_pos.y + SUNLIGHT_CHECK_DISTANCE; mb_y++) {
            Vector3<int> check_pos(mb_pos.x, mb_y, mb_pos.z);
            Mapblock *mb_above = mapblocks[check_pos];
            if(mb_above->sunlit) {
              has_sun = true;
              break;
            }
            bool stop = false;
            for(int y = 0; y < MAPBLOCK_SIZE_Y; y++) {
              unsigned int id = mb_above->data[x][y][z] & 32767;
              if(!def_tables[check_pos][id].pass_sunlight) {
                stop = true;
                break;
              }
            }
            if(stop) { break; }
          }
        }
        
        for(int y = MAPBLOCK_SIZE_Y - 1; y >= 0; y--) {
          Vector3<int> abs_pos = Vector3<int>(mb_pos.x * MAPBLOCK_SIZE_X, mb_pos.y * MAPBLOCK_SIZE_Y, mb_pos.z * MAPBLOCK_SIZE_Z) + Vector3<int>(x, y, z);
          
          unsigned int id = mb->data[x][y][z] & 32767;
          NodeDef def = def_table[id];
          
          if(def.light_level > 0) {
            light_sources.push_back(std::make_pair(abs_pos, def.light_level));
          }
          
          if(!def.pass_sunlight) { has_sun = false; }
          
          unsigned int sunlight = 0;
          if(has_sun) {
            sunlight = 14;
            sunlight_sources.push_back(std::make_pair(abs_pos, 15));
          }
          
          //Initialize the light level to 1 for transparent and 0 for opaque;
          //this will be used later to avoid looking up node definitions.
          unsigned int l = def.transparent ? 1 : 0;
          
          unsigned long light = (sunlight << 4) | l;
          
          mb->data[x][y][z] = (mb->data[x][y][z] & 0b1111111111111111111111111111111110000000011111111111111111111111UL) | (light << 23);
        }
      }
    }
  }
  
  for(size_t i = 0; i < light_sources.size(); i++) {
    light_cascade(mapblocks, mapblocks_to_update, light_sources[i].first, light_sources[i].second, LC_NORM);
  }
  for(size_t i = 0; i < sunlight_sources.size(); i++) {
    light_cascade(mapblocks, mapblocks_to_update, sunlight_sources[i].first, sunlight_sources[i].second, LC_SUN);
  }
  
  //Bleed in light from the edges of adjacent mapblocks.
  std::pair<Vector3<int>, Vector3<int>> planes[6];
  planes[0].first.set(0,                   0,                   0                  ); planes[0].second.set(0,                   MAPBLOCK_SIZE_Y - 1, MAPBLOCK_SIZE_Z - 1);
  planes[1].first.set(0,                   0,                   0                  ); planes[1].second.set(MAPBLOCK_SIZE_X - 1, 0,                   MAPBLOCK_SIZE_Z - 1);
  planes[2].first.set(0,                   0,                   0                  ); planes[2].second.set(MAPBLOCK_SIZE_X - 1, MAPBLOCK_SIZE_Y - 1, 0              );
  planes[3].first.set(MAPBLOCK_SIZE_X - 1, 0,                   0                  ); planes[3].second.set(MAPBLOCK_SIZE_X - 1, MAPBLOCK_SIZE_Y - 1, MAPBLOCK_SIZE_Z - 1);
  planes[4].first.set(0,                   MAPBLOCK_SIZE_Y - 1, 0                  ); planes[4].second.set(MAPBLOCK_SIZE_X - 1, MAPBLOCK_SIZE_Y - 1, MAPBLOCK_SIZE_Z - 1);
  planes[5].first.set(0,                   0,                   MAPBLOCK_SIZE_Z - 1); planes[5].second.set(MAPBLOCK_SIZE_X - 1, MAPBLOCK_SIZE_Y - 1, MAPBLOCK_SIZE_Z - 1);
  for(auto i : mapblocks_to_update) {
    Vector3<int> mb_pos = i;
    for(size_t n = 0; n < 6; n++) {
      Vector3<int> new_pos = mb_pos + faces[n];
      
      //No need to bleed in from mapblocks that were part of this update batch.
      bool in_update = mapblocks_to_update.find(new_pos) != mapblocks_to_update.end();
      if(in_update) { continue; }
      
      //We should have the data on hand, but if not, skip.
      auto search = mapblocks.find(new_pos);
      if(search == mapblocks.end()) { continue; }
      
      for(int x = planes[n].first.x; x <= planes[n].second.x; x++) {
        for(int y = planes[n].first.y; y <= planes[n].second.y; y++) {
          for(int z = planes[n].first.z; z <= planes[n].second.z; z++) {
            Vector3<int> abs_pos = Vector3<int>(new_pos.x * MAPBLOCK_SIZE_X, new_pos.y * MAPBLOCK_SIZE_Y, new_pos.z * MAPBLOCK_SIZE_Z) + Vector3<int>(x, y, z);
            light_cascade(mapblocks, mapblocks_to_update, abs_pos, 0, LC_NORM, true);
            light_cascade(mapblocks, mapblocks_to_update, abs_pos, 0, LC_SUN, true);
          }
        }
      }
    }
  }
  
  //Save the affected mapblocks.
  for(auto i : mapblocks_to_update) {
    Vector3<int> mb_pos = i;
    Mapblock *mb = mapblocks[mb_pos];
    mb->light_update_num++;
    if(mb->light_needs_update == 1) { mb->light_needs_update = 0; }
    db.set_mapblock(mb_pos, mb);
  }
  
  //Cleanup
  for(auto p : mapblocks) {
    delete p.second;
  }
}

Vector3<int> Map::containing_mapblock(Vector3<int> pos) {
  return Vector3<int>(
      (pos.x < 0 && pos.x % MAPBLOCK_SIZE_X != 0) ? (pos.x / MAPBLOCK_SIZE_X - 1) : (pos.x / MAPBLOCK_SIZE_X),
      (pos.y < 0 && pos.y % MAPBLOCK_SIZE_Y != 0) ? (pos.y / MAPBLOCK_SIZE_Y - 1) : (pos.y / MAPBLOCK_SIZE_Y),
      (pos.z < 0 && pos.z % MAPBLOCK_SIZE_Z != 0) ? (pos.z / MAPBLOCK_SIZE_Z - 1) : (pos.z / MAPBLOCK_SIZE_Z));
}

MapblockUpdateInfo Map::get_mapblockupdateinfo(Vector3<int> mb_pos) {
  return db.get_mapblockupdateinfo(mb_pos);
}
