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

#include "server.h"

std::map<std::string, std::vector<std::pair<std::string, int>>> init_inventories = {
  {"default:chest", {
    {"chest", 32}
  }},
  {"default:furnace", {
    {"furnace_in", 1},
    {"furnace_fuel", 1},
    {"furnace_out", 1}
  }},
  {"default:furnace_active", {
    {"furnace_in", 1},
    {"furnace_fuel", 1},
    {"furnace_out", 1}
  }}
};

bool Server::on_place_node(Node node, MapPos<int> pos) {
  auto init_search = init_inventories.find(node.itemstring);
  if(init_search != init_inventories.end()) {
    const auto& init_list = init_search->second;
    
    db.lock_node_meta(pos);
    
    NodeMeta *meta = db.get_node_meta(pos);
    for(const auto& init_pair : init_list) {
      meta->inventory.add(init_pair.first, InvList(init_pair.second));
    }
    meta->is_nil = false;
    db.set_node_meta(pos, meta);
    delete meta;
    
    db.unlock_node_meta(pos);
    return true;
  }
  
  return true;
}
bool Server::on_dig_node(Node node, MapPos<int> pos) {
  auto init_search = init_inventories.find(node.itemstring);
  if(init_search != init_inventories.end()) {
    db.lock_node_meta(pos);
    
    //Delete meta.
    NodeMeta *meta = db.get_node_meta(pos);
    if(!meta->inventory.is_empty()) {
      //Can't dig a non-empty node.
      delete meta;
      db.unlock_node_meta(pos);
      return false;
    }
    meta->is_nil = true;
    db.set_node_meta(pos, meta);
    delete meta;
    
    db.unlock_node_meta(pos);
    return true;
  }
  
  return true;
}
