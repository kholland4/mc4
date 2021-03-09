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

Node get_node_rel_prefetch(std::map<MapPos<int>, Mapblock*>& input_mapblocks, MapPos<int> mb_pos, MapPos<int> rel_pos) {
  while(rel_pos.x < 0               ) { rel_pos.x += MAPBLOCK_SIZE_X; mb_pos.x--; }
  while(rel_pos.x >= MAPBLOCK_SIZE_X) { rel_pos.x -= MAPBLOCK_SIZE_X; mb_pos.x++; }
  while(rel_pos.y < 0               ) { rel_pos.y += MAPBLOCK_SIZE_Y; mb_pos.y--; }
  while(rel_pos.y >= MAPBLOCK_SIZE_Y) { rel_pos.y -= MAPBLOCK_SIZE_Y; mb_pos.y++; }
  while(rel_pos.z < 0               ) { rel_pos.z += MAPBLOCK_SIZE_Z; mb_pos.z--; }
  while(rel_pos.z >= MAPBLOCK_SIZE_Z) { rel_pos.z -= MAPBLOCK_SIZE_Z; mb_pos.z++; }
  if(rel_pos.w != 0) { mb_pos.w += rel_pos.w; rel_pos.w = 0; }
  if(rel_pos.world != 0) { mb_pos.world += rel_pos.world; rel_pos.world = 0; }
  if(rel_pos.universe != 0) { mb_pos.universe += rel_pos.universe; rel_pos.universe = 0; }
  
  auto search = input_mapblocks.find(mb_pos);
  if(search == input_mapblocks.end()) {
    return Node();
  }
  
  Mapblock *mb = search->second;
  return mb->get_node_rel(rel_pos);
}

void set_node_rel_prefetch(std::map<MapPos<int>, Mapblock*>& input_mapblocks, std::map<MapPos<int>, Mapblock*>& output_mapblocks, MapPos<int> mb_pos, MapPos<int> rel_pos, Node n) {
  while(rel_pos.x < 0               ) { rel_pos.x += MAPBLOCK_SIZE_X; mb_pos.x--; }
  while(rel_pos.x >= MAPBLOCK_SIZE_X) { rel_pos.x -= MAPBLOCK_SIZE_X; mb_pos.x++; }
  while(rel_pos.y < 0               ) { rel_pos.y += MAPBLOCK_SIZE_Y; mb_pos.y--; }
  while(rel_pos.y >= MAPBLOCK_SIZE_Y) { rel_pos.y -= MAPBLOCK_SIZE_Y; mb_pos.y++; }
  while(rel_pos.z < 0               ) { rel_pos.z += MAPBLOCK_SIZE_Z; mb_pos.z--; }
  while(rel_pos.z >= MAPBLOCK_SIZE_Z) { rel_pos.z -= MAPBLOCK_SIZE_Z; mb_pos.z++; }
  if(rel_pos.w != 0) { mb_pos.w += rel_pos.w; rel_pos.w = 0; }
  if(rel_pos.world != 0) { mb_pos.world += rel_pos.world; rel_pos.world = 0; }
  if(rel_pos.universe != 0) { mb_pos.universe += rel_pos.universe; rel_pos.universe = 0; }
  
  auto search = output_mapblocks.find(mb_pos);
  if(search != output_mapblocks.end()) {
    Mapblock *mb = search->second;
    mb->set_node_rel(rel_pos, n);
    return;
  }
  
  
  auto search_in = input_mapblocks.find(mb_pos);
  if(search_in == input_mapblocks.end()) {
    return;
  }
  
  Node old_n = search_in->second->get_node_rel(rel_pos);
  if(old_n == n) { return; }
  
  Mapblock *mb = new Mapblock(*(search_in->second));
  mb->set_node_rel(rel_pos, n);
  output_mapblocks[mb_pos] = mb;
}

void Map::tick_fluids(std::set<MapPos<int>> mapblocks) {
  std::map<MapPos<int>, Mapblock*> input_mapblocks;
  std::map<MapPos<int>, Mapblock*> output_mapblocks;
  
  for(const MapPos<int>& mb_pos : mapblocks) {
    input_mapblocks[mb_pos] = get_mapblock(mb_pos);
  }
  
  MapPos<int> faces[4] = {
    {-1, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0},
    {0, 0, -1, 0, 0, 0},
    {0, 0, 1, 0, 0, 0}
  };
  
  //Update each fluid node
  for(auto it : input_mapblocks) {
    MapPos<int> mb_pos = it.first;
    Mapblock *mb = it.second;
    
    std::vector<bool> id_fluid;
    for(auto id_it : mb->IDtoIS) {
      id_fluid.push_back(get_node_def(id_it).is_fluid);
    }
    
    //Flowing fluids are basically a cellular automata.
    //  - For each fluid node, check the four (-X/+X/-Z/+Z) immediately adjacent nodes.
    //  - If two or more of these nodes is a fluid source, this node becomes a fluid source.
    //  - Otherwise, the height of this node is the greatest of each of the four surrounding heights (where they exist), minus 2.
    //  - If the new height of this node is zero, replace it with air.
    //  - Spread the fluid to adjacent air nodes, giving them height - 2
    
    for(size_t x = 0; x < MAPBLOCK_SIZE_X; x++) {
      for(size_t y = 0; y < MAPBLOCK_SIZE_Y; y++) {
        for(size_t z = 0; z < MAPBLOCK_SIZE_Z; z++) {
          unsigned int data = mb->data[x][y][z];
          unsigned int id = data & 32767;
          if(id >= id_fluid.size()) { continue; }
          if(!id_fluid[id]) { continue; }
          
          //it's a fluid
          MapPos<int> rel_pos(x, y, z, 0, 0, 0);
          
          Node n = mb->get_node_rel(rel_pos);
          Node n_below = get_node_rel_prefetch(input_mapblocks, mb_pos, rel_pos + MapPos<int>(0, -1, 0, 0, 0, 0));
          NodeDef n_below_def = get_node_def(n_below.itemstring);
          
          unsigned int old_rot = n.rot;
          //0 is max height (1.0 physically), 15 is min height (0.0625 physically)
          int old_height = ((old_rot >> 4) & 15);
          bool is_source = (old_rot & 8) == 0 && old_height == 0;
          
          int new_height = old_height;
          bool do_destroy = false;
          bool make_source = is_source;
          bool visual_fullheight = (old_rot & 4) == 4;
          
          if(!is_source) {
            int largest_adj_height = 16;
            int nearby_source_count = 0;
            
            for(int i = 0; i < 4; i++) {
              MapPos<int> adj_pos = rel_pos + faces[i];
              Node rel_n = get_node_rel_prefetch(input_mapblocks, mb_pos, adj_pos);
              if(rel_n.itemstring != n.itemstring) { continue; }
              int rel_height = ((rel_n.rot >> 4) & 15);
              if(rel_height < largest_adj_height) {
                largest_adj_height = rel_height;
              }
              if((rel_n.rot & 8) == 0 && rel_height == 0) {
                nearby_source_count++;
              }
            }
            
            make_source = nearby_source_count >= 2;
            int target_height = make_source ? 0 : (largest_adj_height + 2);
            
            Node rel_n_above = get_node_rel_prefetch(input_mapblocks, mb_pos, rel_pos + MapPos<int>(0, 1, 0, 0, 0, 0));
            if(rel_n_above.itemstring == n.itemstring) {
              int height_above = (rel_n_above.rot >> 4) & 15;
              if(height_above < target_height) {
                target_height = height_above;
                visual_fullheight = true;
              }
            }
            
            if(target_height > 15) {
              //empty
              do_destroy = true;
            }
            new_height = target_height;
          }
          
          unsigned int new_rot = ((new_height & 15) << 4) | (make_source ? 0 : 8) | (visual_fullheight ? 4 : 0);
          
          if(do_destroy) {
            set_node_rel_prefetch(input_mapblocks, output_mapblocks, mb_pos, rel_pos, Node("air"));
          } else if(new_rot != old_rot) {
            set_node_rel_prefetch(input_mapblocks, output_mapblocks, mb_pos, rel_pos, Node(n.itemstring, new_rot));
          }
          
          //spreading
          int spread_height = new_height + 2;
          if(spread_height <= 15) {
            for(int i = 0; i < 4; i++) {
              MapPos<int> adj_pos = rel_pos + faces[i];
              Node rel_n = get_node_rel_prefetch(input_mapblocks, mb_pos, adj_pos);
              if(rel_n.itemstring != "air") { continue; }
              
              //Don't allow spreading horizontally if we're over an air/fluid block unless we're over a fluid source block
              if(n_below.itemstring == "air" || n_below_def.is_fluid) {
                /*Node rel_n_below = get_node_rel_prefetch(input_mapblocks, mb_pos, adj_pos + Vector3<int>(0, -1, 0));
                NodeDef rel_n_below_def = get_node_def(rel_n_below.itemstring);
                if(rel_n_below.itemstring == "air" ||
                   (rel_n_below_def.isFluid && rel_n_below.itemstring != n.itemstring) ||
                   (rel_n_below.itemstring == n.itemstring && (rel_n_below.rot & 8) != 0)
                { continue; }*/
                
                if(n_below.itemstring == n.itemstring && (n_below.rot & 8) == 0 && ((n_below.rot >> 4) & 15) == 0) {
                  //fluid source: ok
                } else {
                  continue;
                }
              }
              
              int rot = ((spread_height & 15) << 4) | 8;
              
              set_node_rel_prefetch(input_mapblocks, output_mapblocks, mb_pos, adj_pos, Node(n.itemstring, rot));
            }
          }
          
          if(new_height <= 15) {
            if(n_below.itemstring == "air" || n_below.itemstring == n.itemstring) {
              //Spread down
              int height_below = new_height;
              int rot = ((height_below & 15) << 4) | 8 | 4; //flags: not source, visual_fullheight
              set_node_rel_prefetch(input_mapblocks, output_mapblocks, mb_pos, rel_pos + MapPos<int>(0, -1, 0, 0, 0, 0), Node(n.itemstring, rot));
            }
          }
        }
      }
    }
  }
  
  //Clean up, save, and update lighting.
  for(auto it : input_mapblocks) {
    delete it.second;
  }
  
  std::set<MapPos<int>> to_update;
  for(auto it : output_mapblocks) {
    MapPos<int> mb_pos = it.first;
    Mapblock *mb = it.second;
    mb->light_needs_update = 1;
    mb->update_num++;
    db.set_mapblock(mb_pos, mb);
    to_update.insert(mb_pos);
    delete mb;
  }
  
  if(to_update.size() > 0) {
    update_mapblock_light(to_update);
  }
}
