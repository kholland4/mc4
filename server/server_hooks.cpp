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

bool Server::on_place_node(Node node, MapPos<int> pos) {
  if(node.itemstring == "default:chest") {
    db.lock_node_meta(pos);
    
    NodeMeta *meta = db.get_node_meta(pos);
    meta->inventory.add("chest", InvList(32));
    meta->is_nil = false;
    db.set_node_meta(pos, meta);
    delete meta;
    
    db.unlock_node_meta(pos);
    return true;
  }
  
  return true;
}
bool Server::on_dig_node(Node node, MapPos<int> pos) {
  if(node.itemstring == "default:chest") {
    db.lock_node_meta(pos);
    
    //Delete meta.
    NodeMeta *meta = db.get_node_meta(pos);
    if(!meta->inventory.is_empty()) {
      //Can't dig a non-empty chest.
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
