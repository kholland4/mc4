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

#include "log.h"

#include <algorithm>

#define SUNLIGHT_CHECK_DISTANCE 4

Map::Map(Database& _db, std::map<int, World*> _worlds)
    : worlds(_worlds), db(_db)
{
  
}

Node Map::get_node(MapPos<int> pos) {
  //C++ integer division always rounds "towards zero", i. e. the fractional part is discarded.
  //Unfortunately, we want to floor the result -- round it down.
  //For positive numbers, C++ gives us the correct answer.
  //For negative numbers, it may not. -9 / 4 gives -2, but we want -3.
  //However, when a negative dividend is divided evenly by its divisor, we get the desired result (i. e. -8 / 2 = -4).
  MapPos<int> mb_pos(
      (pos.x < 0 && pos.x % MAPBLOCK_SIZE_X != 0) ? (pos.x / MAPBLOCK_SIZE_X - 1) : (pos.x / MAPBLOCK_SIZE_X),
      (pos.y < 0 && pos.y % MAPBLOCK_SIZE_Y != 0) ? (pos.y / MAPBLOCK_SIZE_Y - 1) : (pos.y / MAPBLOCK_SIZE_Y),
      (pos.z < 0 && pos.z % MAPBLOCK_SIZE_Z != 0) ? (pos.z / MAPBLOCK_SIZE_Z - 1) : (pos.z / MAPBLOCK_SIZE_Z),
      pos.w, pos.world, pos.universe);
  
  //Since the modulo operation gives remainders, doing something like -9 % 5 would give -4. We wrap this around into the positive interval [0, MAPBLOCK_SIZE_*).
  MapPos<int> rel_pos(
      ((pos.x % MAPBLOCK_SIZE_X) + MAPBLOCK_SIZE_X) % MAPBLOCK_SIZE_X,
      ((pos.y % MAPBLOCK_SIZE_Y) + MAPBLOCK_SIZE_Y) % MAPBLOCK_SIZE_Y,
      ((pos.z % MAPBLOCK_SIZE_Z) + MAPBLOCK_SIZE_Z) % MAPBLOCK_SIZE_Z,
      0, 0, 0);
  
  Mapblock *mb = get_mapblock(mb_pos);
  Node node = mb->get_node_rel(rel_pos);
  delete mb;
  return node;
}

//High-level setting of a node.
//Everything is handled automatically, including lighting.
void Map::set_node(MapPos<int> pos, Node node) {
  MapPos<int> mb_pos(
      (pos.x < 0 && pos.x % MAPBLOCK_SIZE_X != 0) ? (pos.x / MAPBLOCK_SIZE_X - 1) : (pos.x / MAPBLOCK_SIZE_X),
      (pos.y < 0 && pos.y % MAPBLOCK_SIZE_Y != 0) ? (pos.y / MAPBLOCK_SIZE_Y - 1) : (pos.y / MAPBLOCK_SIZE_Y),
      (pos.z < 0 && pos.z % MAPBLOCK_SIZE_Z != 0) ? (pos.z / MAPBLOCK_SIZE_Z - 1) : (pos.z / MAPBLOCK_SIZE_Z),
      pos.w, pos.world, pos.universe);
  MapPos<int> rel_pos(
      ((pos.x % MAPBLOCK_SIZE_X) + MAPBLOCK_SIZE_X) % MAPBLOCK_SIZE_X,
      ((pos.y % MAPBLOCK_SIZE_Y) + MAPBLOCK_SIZE_Y) % MAPBLOCK_SIZE_Y,
      ((pos.z % MAPBLOCK_SIZE_Z) + MAPBLOCK_SIZE_Z) % MAPBLOCK_SIZE_Z,
      0, 0, 0);
  
  Mapblock *mb = get_mapblock(mb_pos);
  
  Node old_node = mb->get_node_rel(rel_pos);
  NodeDef old_def = get_node_def(old_node.itemstring);
  
  mb->set_node_rel(rel_pos, node);
  
  NodeDef def = get_node_def(node.itemstring);
  
  if(node == old_node) {
    //No change to anything.
  } else if(old_def.transparent == def.transparent && old_def.pass_sunlight == def.pass_sunlight && old_def.light_level == def.light_level) {
    //No change to lighting.
    mb->update_num++;
    mb->dirty = true;
    db.set_mapblock(mb_pos, mb);
  } else if(def.transparent && def.pass_sunlight && old_def.light_level == 0) {
    mb->update_num++;
    mb->dirty = true;
    db.set_mapblock(mb_pos, mb);
    
    update_mapblock_light_optimized_singlenode_transparent(mb_pos, rel_pos);
  } else {
    set_mapblock(mb_pos, mb);
  }
  delete mb;
}

Mapblock* Map::get_mapblock(MapPos<int> mb_pos) {
  Mapblock *mb = db.get_mapblock(mb_pos);
  if(!mb->is_nil) { return mb; }
  
  //The mapblock is not held by the database, so we got an empty one.
  //We must generate some data to fill it.
  //The resultant mapblock will be sent to the database for caching,
  //but because it isn't marked as 'dirty', it shouldn't be stored to disk (it could be regenerated at any time).
  
  auto search = worlds.find(mb_pos.world);
  if(search == worlds.end()) {
    //no world exists at this position
    //return a nil mapblock
    return mb;
  }
  
  search->second->mapgen.generate_at(mb_pos, mb);
  db.set_mapblock(mb_pos, mb);
  return mb;
}

void Map::set_mapblock(MapPos<int> mb_pos, Mapblock *mb) {
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
    update_mapblock_light(info.pos - MapPos<int>(1, 1, 1, 0, 0, 0), info.pos + MapPos<int>(1, 1, 1, 0, 0, 0));
  } else {
    return; //should never happen
  }
  
  MapblockUpdateInfo new_info = db.get_mapblockupdateinfo(info.pos);
  new_info.light_needs_update = 0;
  db.set_mapblockupdateinfo(info.pos, new_info);
}

void Map::update_mapblock_light(MapPos<int> min_pos, MapPos<int> max_pos) {
  if(min_pos.w != max_pos.w || min_pos.world != max_pos.world || min_pos.universe != max_pos.universe) {
    log(LogSource::MAP, LogLevel::WARNING, "cannot update mapblock light across higher spatial dimensions");
    return;
  }
  std::set<MapPos<int>> mapblock_list;
  for(int x = min_pos.x; x <= max_pos.x; x++) {
    for(int y = min_pos.y; y <= max_pos.y; y++) {
      for(int z = min_pos.z; z <= max_pos.z; z++) {
        mapblock_list.insert(MapPos<int>(x, y, z, min_pos.w, min_pos.world, min_pos.universe));
      }
    }
  }
  update_mapblock_light(mapblock_list);
}

enum LightCascadeType {LC_NORM, LC_SUN};
MapPos<int> faces[6] = {
  MapPos<int>(1, 0, 0, 0, 0, 0),
  MapPos<int>(0, 1, 0, 0, 0, 0),
  MapPos<int>(0, 0, 1, 0, 0, 0),
  MapPos<int>(-1, 0, 0, 0, 0, 0),
  MapPos<int>(0, -1, 0, 0, 0, 0),
  MapPos<int>(0, 0, -1, 0, 0, 0)
};
MapPos<int> rev_faces[6] = {
  MapPos<int>(-1, 0, 0, 0, 0, 0),
  MapPos<int>(0, -1, 0, 0, 0, 0),
  MapPos<int>(0, 0, -1, 0, 0, 0),
  MapPos<int>(1, 0, 0, 0, 0, 0),
  MapPos<int>(0, 1, 0, 0, 0, 0),
  MapPos<int>(0, 0, 1, 0, 0, 0)
};
void light_cascade_fast(std::map<MapPos<int>, Mapblock*>& mapblocks, std::set<MapPos<int>>& mapblocks_to_update, Mapblock *mb, MapPos<int> mb_pos, MapPos<int> rel_pos, unsigned int light_level, LightCascadeType type, MapPos<int> skip_face, int bleed_mode = 0) {
  if(light_level == 0 && bleed_mode == 0) { return; }
  
  uint32_t val = mb->data[rel_pos.x][rel_pos.y][rel_pos.z];
  unsigned int light = (val >> 23) & 255;
  
  //Stop if node is opaque.
  if((light & 15) == 0) { return; }
  
  if(bleed_mode == 1) {
    if(type == LC_NORM) {
      light_level = light & 0x0F;
    } else if(type == LC_SUN) {
      light_level = (light >> 4) & 0x0F;
    }
  } else if(bleed_mode == 2) {
    if(type == LC_NORM) {
      light_level = std::max(light_level, light & 0x0F);
    } else if(type == LC_SUN) {
      light_level = std::max(light_level, (light >> 4) & 0x0F);
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
  
  if(save || bleed_mode > 0) {
    //if(in_update) {
    mb->data[rel_pos.x][rel_pos.y][rel_pos.z] = (mb->data[rel_pos.x][rel_pos.y][rel_pos.z] & 0b10000000011111111111111111111111UL) | (light << 23);
    //}
    
    light_level--;
    if(light_level > 0) {
      //TODO only try one direction for bleed_mode?
      for(int i = 0; i < 6; i++) {
        if(faces[i] == skip_face) { continue; }
        
        MapPos<int> adj = rel_pos + faces[i];
        MapPos<int> mb_pos_n(mb_pos);
        if(adj.x >= 0 && adj.x < MAPBLOCK_SIZE_X &&
           adj.y >= 0 && adj.y < MAPBLOCK_SIZE_Y &&
           adj.z >= 0 && adj.z < MAPBLOCK_SIZE_Z &&
           adj.w == 0 &&
           adj.world == 0 &&
           adj.universe == 0) {
          light_cascade_fast(mapblocks, mapblocks_to_update, mb, mb_pos_n, adj, light_level, type, rev_faces[i]);
        } else {
          if(adj.x < 0               ) { adj.x += MAPBLOCK_SIZE_X; mb_pos_n.x--; }
          if(adj.x >= MAPBLOCK_SIZE_X) { adj.x -= MAPBLOCK_SIZE_X; mb_pos_n.x++; }
          if(adj.y < 0               ) { adj.y += MAPBLOCK_SIZE_Y; mb_pos_n.y--; }
          if(adj.y >= MAPBLOCK_SIZE_Y) { adj.y -= MAPBLOCK_SIZE_Y; mb_pos_n.y++; }
          if(adj.z < 0               ) { adj.z += MAPBLOCK_SIZE_Z; mb_pos_n.z--; }
          if(adj.z >= MAPBLOCK_SIZE_Z) { adj.z -= MAPBLOCK_SIZE_Z; mb_pos_n.z++; }
          if(adj.w != 0) { mb_pos_n.w = adj.w; adj.w = 0; }
          if(adj.world != 0) { mb_pos_n.world = adj.world; adj.world = 0; }
          if(adj.universe != 0) { mb_pos_n.universe = adj.universe; adj.universe = 0; }
          
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
void light_cascade(std::map<MapPos<int>, Mapblock*>& mapblocks, std::set<MapPos<int>>& mapblocks_to_update, MapPos<int> pos, unsigned int light_level, LightCascadeType type, int bleed_mode = 0) {
  if(light_level == 0 && bleed_mode == 0) { return; }
  
  MapPos<int> mb_pos(
      (pos.x < 0 && pos.x % MAPBLOCK_SIZE_X != 0) ? (pos.x / MAPBLOCK_SIZE_X - 1) : (pos.x / MAPBLOCK_SIZE_X),
      (pos.y < 0 && pos.y % MAPBLOCK_SIZE_Y != 0) ? (pos.y / MAPBLOCK_SIZE_Y - 1) : (pos.y / MAPBLOCK_SIZE_Y),
      (pos.z < 0 && pos.z % MAPBLOCK_SIZE_Z != 0) ? (pos.z / MAPBLOCK_SIZE_Z - 1) : (pos.z / MAPBLOCK_SIZE_Z),
      pos.w, pos.world, pos.universe);
  
  bool in_update = false;
  if(bleed_mode == 0) {
    in_update = mapblocks_to_update.find(mb_pos) != mapblocks_to_update.end();
    if(!in_update) { return; }
  }
  
  auto search = mapblocks.find(mb_pos);
  if(search == mapblocks.end()) { return; }
  Mapblock *mb = search->second;
  
  MapPos<int> rel_pos(
      ((pos.x % MAPBLOCK_SIZE_X) + MAPBLOCK_SIZE_X) % MAPBLOCK_SIZE_X,
      ((pos.y % MAPBLOCK_SIZE_Y) + MAPBLOCK_SIZE_Y) % MAPBLOCK_SIZE_Y,
      ((pos.z % MAPBLOCK_SIZE_Z) + MAPBLOCK_SIZE_Z) % MAPBLOCK_SIZE_Z,
      0, 0, 0);
  
  uint32_t val = mb->data[rel_pos.x][rel_pos.y][rel_pos.z];
  unsigned int light = (val >> 23) & 255;
  
  //Stop if node is opaque.
  if((light & 15) == 0) { return; }
  
  if(bleed_mode == 1) {
    if(type == LC_NORM) {
      light_level = light & 0x0F;
    } else if(type == LC_SUN) {
      light_level = (light >> 4) & 0x0F;
    }
  } else if(bleed_mode == 2) {
    if(type == LC_NORM) {
      light_level = std::max(light_level, light & 0x0F);
    } else if(type == LC_SUN) {
      light_level = std::max(light_level, (light >> 4) & 0x0F);
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
  
  if(save || bleed_mode > 0) {
    if(in_update || bleed_mode > 1) {
      mb->data[rel_pos.x][rel_pos.y][rel_pos.z] = (mb->data[rel_pos.x][rel_pos.y][rel_pos.z] & 0b10000000011111111111111111111111UL) | (light << 23);
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
        MapPos<int> adj = rel_pos + faces[i];
        if(adj.x >= 0 && adj.x < MAPBLOCK_SIZE_X &&
           adj.y >= 0 && adj.y < MAPBLOCK_SIZE_Y &&
           adj.z >= 0 && adj.z < MAPBLOCK_SIZE_Z &&
           adj.w == 0 &&
           adj.world == 0 &&
           adj.universe == 0) {
          light_cascade_fast(mapblocks, mapblocks_to_update, mb, mb_pos, adj, light_level, type, rev_faces[i]);
        } else {
          light_cascade(mapblocks, mapblocks_to_update, pos + faces[i], light_level, type);
        }
      }
    }
  }
}

void Map::save_changed_lit_mapblocks(std::map<MapPos<int>, Mapblock*>& mapblocks, std::set<MapPos<int>>& mapblocks_to_update, bool do_clear_light_needs_update) {
  for(auto i : mapblocks_to_update) {
    MapPos<int> mb_pos = i;
    Mapblock *mb = mapblocks[mb_pos];
    bool needs_update = false;
    
    Mapblock *old_mb = db.get_mapblock(mb_pos);
    if(old_mb->is_nil) {
      needs_update = true;
    } else {
      //Compare the mapblock data
      for(size_t x = 0; x < MAPBLOCK_SIZE_X && !needs_update; x++) {
        for(size_t y = 0; y < MAPBLOCK_SIZE_Y && !needs_update; y++) {
          for(size_t z = 0; z < MAPBLOCK_SIZE_Z && !needs_update; z++) {
            if(old_mb->data[x][y][z] != mb->data[x][y][z]) {
              needs_update = true;
              break;
            }
          }
        }
      }
    }
    delete old_mb;
    
    if(needs_update) {
      mb->light_update_num++;
      if(mb->light_needs_update == 1 && do_clear_light_needs_update) { mb->light_needs_update = 0; }
      db.set_mapblock(mb_pos, mb);
    } else {
      if(mb->light_needs_update == 1 && do_clear_light_needs_update) { mb->light_needs_update = 0; }
      db.set_mapblockupdateinfo(mb_pos, MapblockUpdateInfo(mb));
    }
  }
}

void Map::update_mapblock_light(std::set<MapPos<int>> mapblocks_to_update) {
  std::map<MapPos<int>, Mapblock*> mapblocks;
  std::map<MapPos<int>, std::map<unsigned int, NodeDef>> def_tables;
  
  std::set<MapPos<int>> mapblocks_to_compute;
  
  //Load each requested mapblock plus any adjacent ones (adjacent meaning +/- 1 away on any axis).
  //Also fetch mapblocks up to SUNLIGHT_CHECK_DISTANCE above the target (for sunlight).
  //Also precompute tables of definitions.
  for(auto i : mapblocks_to_update) {
    MapPos<int> center_pos = i;
    for(int x = center_pos.x - 1; x <= center_pos.x + 1; x++) {
      for(int z = center_pos.z - 1; z <= center_pos.z + 1; z++) {
        for(int y = center_pos.y - 1; y <= center_pos.y + SUNLIGHT_CHECK_DISTANCE; y++) {
          MapPos<int> pos(x, y, z, center_pos.w, center_pos.world, center_pos.universe);
          if(y <= center_pos.y + 1) {
            auto search = mapblocks_to_compute.find(pos);
            if(search == mapblocks_to_compute.end()) {
              if(mapblocks_to_update.find(pos) != mapblocks_to_update.end()) {
                mapblocks_to_compute.insert(pos);
              } else if(get_mapblockupdateinfo(pos).light_needs_update > 0) {
                mapblocks_to_compute.insert(pos);
              }
            }
          }
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
  
  std::vector<std::pair<std::pair<Mapblock*, MapPos<int>>, unsigned int>> light_sources;
  std::vector<std::pair<std::pair<Mapblock*, MapPos<int>>, unsigned int>> sunlight_sources;
  
  //Clear light in the mapblocks being updated.
  //Compute sunlight and find light sources as well.
  for(auto i : mapblocks_to_compute) {
    MapPos<int> mb_pos = i;
    Mapblock *mb = mapblocks[mb_pos];
    std::map<unsigned int, NodeDef> def_table = def_tables[mb_pos];
    
    for(int x = 0; x < MAPBLOCK_SIZE_X; x++) {
      for(int z = 0; z < MAPBLOCK_SIZE_Z; z++) {
        bool has_sun = false;
        if(mb->sunlit) {
          has_sun = true;
        } else {
          for(int mb_y = mb_pos.y + 1; mb_y <= mb_pos.y + SUNLIGHT_CHECK_DISTANCE; mb_y++) {
            MapPos<int> check_pos(mb_pos.x, mb_y, mb_pos.z, mb_pos.w, mb_pos.world, mb_pos.universe);
            auto search = mapblocks.find(check_pos);
            if(search == mapblocks.end()) { break; }
            Mapblock *mb_above = search->second;
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
          MapPos<int> rel_pos(x, y, z, 0, 0, 0);
          
          unsigned int id = mb->data[x][y][z] & 32767;
          NodeDef def = def_table[id];
          
          if(def.light_level > 0) {
            light_sources.push_back(std::make_pair(std::make_pair(mb, rel_pos), def.light_level));
          }
          
          if(!def.pass_sunlight) { has_sun = false; }
          
          unsigned int sunlight = 0;
          if(has_sun) {
            sunlight = 14;
            sunlight_sources.push_back(std::make_pair(std::make_pair(mb, rel_pos), 15));
          }
          
          //Initialize the light level to 1 for transparent and 0 for opaque;
          //this will be used later to avoid looking up node definitions.
          unsigned int l = def.transparent ? 1 : 0;
          
          unsigned long light = (sunlight << 4) | l;
          
          mb->data[x][y][z] = (mb->data[x][y][z] & 0b10000000011111111111111111111111UL) | (light << 23);
        }
      }
    }
  }
  
  MapPos<int> no_face(0, 0, 0, 0, 0, 0);
  for(size_t i = 0; i < light_sources.size(); i++) {
    light_cascade_fast(mapblocks, mapblocks_to_compute, light_sources[i].first.first, light_sources[i].first.first->pos, light_sources[i].first.second, light_sources[i].second, LC_NORM, no_face);
  }
  for(size_t i = 0; i < sunlight_sources.size(); i++) {
    light_cascade_fast(mapblocks, mapblocks_to_compute, sunlight_sources[i].first.first, sunlight_sources[i].first.first->pos, sunlight_sources[i].first.second, sunlight_sources[i].second, LC_SUN, no_face);
  }
  
  //Bleed in light from the edges of adjacent mapblocks.
  std::pair<MapPos<int>, MapPos<int>> planes[6];
  planes[0].first.set(0,                   0,                   0,                   0, 0, 0); planes[0].second.set(0,                   MAPBLOCK_SIZE_Y - 1, MAPBLOCK_SIZE_Z - 1, 0, 0, 0);
  planes[1].first.set(0,                   0,                   0,                   0, 0, 0); planes[1].second.set(MAPBLOCK_SIZE_X - 1, 0,                   MAPBLOCK_SIZE_Z - 1, 0, 0, 0);
  planes[2].first.set(0,                   0,                   0,                   0, 0, 0); planes[2].second.set(MAPBLOCK_SIZE_X - 1, MAPBLOCK_SIZE_Y - 1, 0,                   0, 0, 0);
  planes[3].first.set(MAPBLOCK_SIZE_X - 1, 0,                   0,                   0, 0, 0); planes[3].second.set(MAPBLOCK_SIZE_X - 1, MAPBLOCK_SIZE_Y - 1, MAPBLOCK_SIZE_Z - 1, 0, 0, 0);
  planes[4].first.set(0,                   MAPBLOCK_SIZE_Y - 1, 0,                   0, 0, 0); planes[4].second.set(MAPBLOCK_SIZE_X - 1, MAPBLOCK_SIZE_Y - 1, MAPBLOCK_SIZE_Z - 1, 0, 0, 0);
  planes[5].first.set(0,                   0,                   MAPBLOCK_SIZE_Z - 1, 0, 0, 0); planes[5].second.set(MAPBLOCK_SIZE_X - 1, MAPBLOCK_SIZE_Y - 1, MAPBLOCK_SIZE_Z - 1, 0, 0, 0);
  for(auto i : mapblocks_to_compute) {
    MapPos<int> mb_pos = i;
    for(size_t n = 0; n < 6; n++) {
      MapPos<int> new_pos = mb_pos + faces[n];
      
      //No need to bleed in from mapblocks that were part of this update batch.
      bool in_update = mapblocks_to_compute.find(new_pos) != mapblocks_to_compute.end();
      if(in_update) { continue; }
      
      //If we don't have the data on hand, it's irrelevant anyways.
      auto search = mapblocks.find(new_pos);
      if(search == mapblocks.end()) { continue; }
      
      for(int x = planes[n].first.x; x <= planes[n].second.x; x++) {
        for(int y = planes[n].first.y; y <= planes[n].second.y; y++) {
          for(int z = planes[n].first.z; z <= planes[n].second.z; z++) {
            MapPos<int> rel_pos(x, y, z, 0, 0, 0);
            light_cascade_fast(mapblocks, mapblocks_to_update, search->second, new_pos, rel_pos, 0, LC_NORM, no_face, 1);
            light_cascade_fast(mapblocks, mapblocks_to_update, search->second, new_pos, rel_pos, 0, LC_SUN, no_face, 1);
          }
        }
      }
    }
  }
  
  //Save the affected mapblocks.
  save_changed_lit_mapblocks(mapblocks, mapblocks_to_update, true);
  
  //Cleanup
  for(auto p : mapblocks) {
    delete p.second;
  }
}

NodeDef get_node_def_prefetch(std::map<MapPos<int>, Mapblock*>& mapblocks, std::map<MapPos<int>, std::map<unsigned int, NodeDef>>& def_tables, MapPos<int> mb_pos, MapPos<int> rel_pos) {
  while(rel_pos.x < 0               ) { rel_pos.x += MAPBLOCK_SIZE_X; mb_pos.x--; }
  while(rel_pos.x >= MAPBLOCK_SIZE_X) { rel_pos.x -= MAPBLOCK_SIZE_X; mb_pos.x++; }
  while(rel_pos.y < 0               ) { rel_pos.y += MAPBLOCK_SIZE_Y; mb_pos.y--; }
  while(rel_pos.y >= MAPBLOCK_SIZE_Y) { rel_pos.y -= MAPBLOCK_SIZE_Y; mb_pos.y++; }
  while(rel_pos.z < 0               ) { rel_pos.z += MAPBLOCK_SIZE_Z; mb_pos.z--; }
  while(rel_pos.z >= MAPBLOCK_SIZE_Z) { rel_pos.z -= MAPBLOCK_SIZE_Z; mb_pos.z++; }
  if(rel_pos.w != 0) { mb_pos.w += rel_pos.w; rel_pos.w = 0; }
  if(rel_pos.world != 0) { mb_pos.world += rel_pos.world; rel_pos.world = 0; }
  if(rel_pos.universe != 0) { mb_pos.universe += rel_pos.universe; rel_pos.universe = 0; }
  
  auto search = mapblocks.find(mb_pos);
  if(search == mapblocks.end()) {
    return NodeDef();
  }
  
  auto search_def = def_tables.find(mb_pos);
  if(search_def == def_tables.end()) {
    return NodeDef();
  }
  std::map<unsigned int, NodeDef> def_table = search_def->second;
  
  uint32_t val = search->second->data[rel_pos.x][rel_pos.y][rel_pos.z];
  unsigned int id = val & 32767;
  
  auto search_id = def_table.find(id);
  if(search_id == def_table.end()) {
    return NodeDef();
  }
  
  return search_id->second;
}

inline unsigned int get_light_val_prefetch(std::map<MapPos<int>, Mapblock*>& mapblocks, MapPos<int> mb_pos, MapPos<int> rel_pos) {
  while(rel_pos.x < 0               ) { rel_pos.x += MAPBLOCK_SIZE_X; mb_pos.x--; }
  while(rel_pos.x >= MAPBLOCK_SIZE_X) { rel_pos.x -= MAPBLOCK_SIZE_X; mb_pos.x++; }
  while(rel_pos.y < 0               ) { rel_pos.y += MAPBLOCK_SIZE_Y; mb_pos.y--; }
  while(rel_pos.y >= MAPBLOCK_SIZE_Y) { rel_pos.y -= MAPBLOCK_SIZE_Y; mb_pos.y++; }
  while(rel_pos.z < 0               ) { rel_pos.z += MAPBLOCK_SIZE_Z; mb_pos.z--; }
  while(rel_pos.z >= MAPBLOCK_SIZE_Z) { rel_pos.z -= MAPBLOCK_SIZE_Z; mb_pos.z++; }
  if(rel_pos.w != 0) { mb_pos.w += rel_pos.w; rel_pos.w = 0; }
  if(rel_pos.world != 0) { mb_pos.world += rel_pos.world; rel_pos.world = 0; }
  if(rel_pos.universe != 0) { mb_pos.universe += rel_pos.universe; rel_pos.universe = 0; }
  
  auto search = mapblocks.find(mb_pos);
  if(search == mapblocks.end()) {
    return 0;
  }
  
  uint32_t val = search->second->data[rel_pos.x][rel_pos.y][rel_pos.z];
  unsigned int light = (val >> 23) & 255;
  return light;
}

inline void set_light_val_prefetch(std::map<MapPos<int>, Mapblock*>& mapblocks, MapPos<int> mb_pos, MapPos<int> rel_pos, unsigned int light) {
  while(rel_pos.x < 0               ) { rel_pos.x += MAPBLOCK_SIZE_X; mb_pos.x--; }
  while(rel_pos.x >= MAPBLOCK_SIZE_X) { rel_pos.x -= MAPBLOCK_SIZE_X; mb_pos.x++; }
  while(rel_pos.y < 0               ) { rel_pos.y += MAPBLOCK_SIZE_Y; mb_pos.y--; }
  while(rel_pos.y >= MAPBLOCK_SIZE_Y) { rel_pos.y -= MAPBLOCK_SIZE_Y; mb_pos.y++; }
  while(rel_pos.z < 0               ) { rel_pos.z += MAPBLOCK_SIZE_Z; mb_pos.z--; }
  while(rel_pos.z >= MAPBLOCK_SIZE_Z) { rel_pos.z -= MAPBLOCK_SIZE_Z; mb_pos.z++; }
  if(rel_pos.w != 0) { mb_pos.w += rel_pos.w; rel_pos.w = 0; }
  if(rel_pos.world != 0) { mb_pos.world += rel_pos.world; rel_pos.world = 0; }
  if(rel_pos.universe != 0) { mb_pos.universe += rel_pos.universe; rel_pos.universe = 0; }
  
  auto search = mapblocks.find(mb_pos);
  if(search == mapblocks.end()) {
    return;
  }
  
  search->second->data[rel_pos.x][rel_pos.y][rel_pos.z] = (search->second->data[rel_pos.x][rel_pos.y][rel_pos.z] & 0b10000000011111111111111111111111UL) | (light << 23);
}

void Map::update_mapblock_light_optimized_singlenode_transparent(MapPos<int> mb_pos, MapPos<int> rel_pos) {
  std::set<MapPos<int>> mapblocks_to_update;
  std::map<MapPos<int>, Mapblock*> mapblocks;
  std::map<MapPos<int>, std::map<unsigned int, NodeDef>> def_tables;
  
  //Load each requested mapblock plus any adjacent ones (adjacent meaning +/- 1 away on any axis).
  for(int x = mb_pos.x - 1; x <= mb_pos.x + 1; x++) {
    for(int z = mb_pos.z - 1; z <= mb_pos.z + 1; z++) {
      for(int y = mb_pos.y - 1; y <= mb_pos.y + 1; y++) {
        MapPos<int> pos(x, y, z, mb_pos.w, mb_pos.world, mb_pos.universe);
        if(mapblocks.find(pos) != mapblocks.end()) { continue; }
        
        mapblocks[pos] = get_mapblock(pos);
        mapblocks_to_update.insert(pos);
        
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
  
  //A light value of 1 or greater indicates the node is transparent.
  unsigned int old_light = get_light_val_prefetch(mapblocks, mb_pos, rel_pos);
  set_light_val_prefetch(mapblocks, mb_pos, rel_pos, (old_light & 0xF0) | std::max(old_light & 0x0F, 1U));
  
  std::vector<std::pair<MapPos<int>, unsigned int>> light_sources;
  std::vector<std::pair<MapPos<int>, unsigned int>> sunlight_sources;
  
  MapPos<int> abs_pos = MapPos<int>(mb_pos.x * MAPBLOCK_SIZE_X, mb_pos.y * MAPBLOCK_SIZE_Y, mb_pos.z * MAPBLOCK_SIZE_Z, mb_pos.w, mb_pos.world, mb_pos.universe) + rel_pos;
  
  //If the node above the current one is sunlit (sunlight == 15), flood the sunlight downward.
  //FIXME the sunlight won't go as deep as it should
  unsigned int sunlight_above = (get_light_val_prefetch(mapblocks, mb_pos, rel_pos + MapPos<int>(0, 1, 0, 0, 0, 0)) >> 4) & 0x0F;
  if(sunlight_above == 15) {
    unsigned int old_light_s = get_light_val_prefetch(mapblocks, mb_pos, rel_pos);
    set_light_val_prefetch(mapblocks, mb_pos, rel_pos, old_light_s | 0xF0);
    sunlight_sources.push_back(std::make_pair(abs_pos, 15));
    for(int y = -1; y >= -MAPBLOCK_SIZE_Y * 2; y--) {
      NodeDef def = get_node_def_prefetch(mapblocks, def_tables, mb_pos, rel_pos + MapPos<int>(0, y, 0, 0, 0, 0));
      if(!def.pass_sunlight || !def.transparent) {
        break;
      }
      
      unsigned int old_light_l = get_light_val_prefetch(mapblocks, mb_pos, rel_pos + MapPos<int>(0, y, 0, 0, 0, 0));
      set_light_val_prefetch(mapblocks, mb_pos, rel_pos + MapPos<int>(0, y, 0, 0, 0, 0), old_light_l | 0xF0);
      if(y < -1) {
        sunlight_sources.push_back(std::make_pair(abs_pos + MapPos<int>(0, y, 0, 0, 0, 0), 15));
      }
    }
  }
  
  
  //Register the node and immediately adjacent ones as light & sunlight sources.
  NodeDef def = get_node_def_prefetch(mapblocks, def_tables, mb_pos, rel_pos);
  light_sources.push_back(std::make_pair(abs_pos, def.light_level));
  for(size_t i = 0; i < 6; i++) {
    unsigned int adj_light_val = get_light_val_prefetch(mapblocks, mb_pos, rel_pos + faces[i]) & 0x0F;
    if(adj_light_val > 1) {
      light_sources.push_back(std::make_pair(abs_pos + faces[i], adj_light_val));
    }
    unsigned int adj_sunlight_val = (get_light_val_prefetch(mapblocks, mb_pos, rel_pos + faces[i]) >> 4) & 0x0F;
    if(adj_sunlight_val > 1) {
      sunlight_sources.push_back(std::make_pair(abs_pos + faces[i], adj_sunlight_val));
    }
  }
  
  //Flood the light.
  for(size_t i = 0; i < light_sources.size(); i++) {
    light_cascade(mapblocks, mapblocks_to_update, light_sources[i].first, light_sources[i].second, LC_NORM, 2);
  }
  for(size_t i = 0; i < sunlight_sources.size(); i++) {
    light_cascade(mapblocks, mapblocks_to_update, sunlight_sources[i].first, sunlight_sources[i].second, LC_SUN, 2);
  }
  
  
  
  //Save the affected mapblocks.
  save_changed_lit_mapblocks(mapblocks, mapblocks_to_update, false);
  
  //Cleanup
  for(auto p : mapblocks) {
    delete p.second;
  }
}

MapPos<int> Map::containing_mapblock(MapPos<int> pos) {
  return MapPos<int>(
      (pos.x < 0 && pos.x % MAPBLOCK_SIZE_X != 0) ? (pos.x / MAPBLOCK_SIZE_X - 1) : (pos.x / MAPBLOCK_SIZE_X),
      (pos.y < 0 && pos.y % MAPBLOCK_SIZE_Y != 0) ? (pos.y / MAPBLOCK_SIZE_Y - 1) : (pos.y / MAPBLOCK_SIZE_Y),
      (pos.z < 0 && pos.z % MAPBLOCK_SIZE_Z != 0) ? (pos.z / MAPBLOCK_SIZE_Z - 1) : (pos.z / MAPBLOCK_SIZE_Z),
      pos.w, pos.world, pos.universe);
}

MapblockUpdateInfo Map::get_mapblockupdateinfo(MapPos<int> mb_pos) {
  return db.get_mapblockupdateinfo(mb_pos);
}
