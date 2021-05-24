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

/*bool Server::lock_unlock_invlist(InvRef ref, bool do_lock) {
  if(ref.obj_type == "player") {
    std::string player_id = ref.obj_id;
    
    std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
    PlayerState *player = NULL;
    for(auto it : m_players) {
      PlayerState *check = it.second;
      std::shared_lock<std::shared_mutex> player_lock(check->lock);
      if(check->get_tag() != player_id)
        continue;
      player = check;
      break;
    }
    
    if(player == NULL)
      return false;
    
    std::unique_lock<std::shared_mutex> player_lock(player->lock);
    
    InvList list = player->inventory.get(ref.list_name);
    if(list.is_nil)
      return false;
    
    //will be automatically created if not present
    if(do_lock) {
      player->inventory_lock[ref.list_name].lock();
    } else {
      player->inventory_lock[ref.list_name].unlock();
    }
    return true;
  }
  
  return false;
}
bool Server::lock_invlist(InvRef ref) {
  return lock_unlock_invlist(ref, true);
}
bool Server::unlock_invlist(InvRef ref) {
  return lock_unlock_invlist(ref, false);
}

InvList Server::get_invlist(InvRef ref) {
  if(ref.obj_type == "player") {
    std::string player_id = ref.obj_id;
    
    std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
    PlayerState *player = NULL;
    for(auto it : m_players) {
      PlayerState *check = it.second;
      std::shared_lock<std::shared_mutex> player_lock(check->lock);
      if(check->get_tag() != player_id)
        continue;
      player = check;
      break;
    }
    
    if(player == NULL)
      return InvList();
    
    std::shared_lock<std::shared_mutex> player_lock(player->lock);
    return player->inventory.get(ref.list_name);
  }
  
  return InvList();
}
bool Server::put_invlist(InvRef ref, InvList list) {
  return false;
}

void Server::update_known_inventories(PlayerState *player) {
  
}*/
