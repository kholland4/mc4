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

//Apply an inventory patch, provided its "prev" state matches reality
//Save changes to database as needed and broadcast updates to interested players
bool Server::inv_apply_patch(InvPatch patch, PlayerState *requesting_player) {
  std::set<InvRef> locked_refs;
  bool ok = true;
  std::string err_message = "error";
  
  //References must refer to unique lists (not just indices)
  //and must be sorted (to avoid deadlocks)
  std::set<InvRef> refs_to_lock;
  
  for(const auto& diff : patch.diffs) {
    InvRef list_ref = diff.ref;
    list_ref.index = -1;
    refs_to_lock.insert(list_ref);
  }
  
  for(const auto& ref : refs_to_lock) {
    ok = lock_invlist(ref, requesting_player);
    if(!ok) {
      err_message = "invalid inventory reference: " + ref.as_json();
      break;
    }
    locked_refs.insert(ref);
  }
  if(!ok) {
    for(const auto& ref : locked_refs) {
      unlock_invlist(ref, requesting_player);
    }
    
    log(LogSource::SERVER, LogLevel::NOTICE, "inv_apply_patch: " + err_message);
    patch.make_deny();
    if(requesting_player != NULL)
      requesting_player->send(patch.as_json("inv_patch_deny"));
    return false;
  }
  
  for(const auto& diff : patch.diffs) {
    InvList list = get_invlist(diff.ref, requesting_player);
    InvStack orig_stack = list.get_at(diff.ref.index);
    if(orig_stack != diff.prev) {
      ok = false;
      err_message = "patch doesn't match current state at reference: " + diff.ref.as_json();
      break;
    }
    continue;
  }
  if(!ok) {
    for(const auto& ref : locked_refs) {
      unlock_invlist(ref, requesting_player);
    }
    
    log(LogSource::SERVER, LogLevel::NOTICE, "inv_apply_patch: " + err_message);
    patch.make_deny();
    if(requesting_player != NULL)
      requesting_player->send(patch.as_json("inv_patch_deny"));
    return false;
  }
  
  //All references are validated and locked.
  //Patch is consistent with current state.
  ok = true;
  err_message = "";
  for(const auto& diff : patch.diffs) {
    InvList list = get_invlist(diff.ref, requesting_player);
    list.set_at(diff.ref.index, diff.current);
    ok = set_invlist(diff.ref, list, requesting_player);
    if(!ok) {
      err_message = "error writing to inventory at reference: " + diff.ref.as_json();
      break;
    }
  }
  
  if(!ok) {
    //Something went wrong, revert everything
    for(const auto& diff : patch.diffs) {
      InvList list = get_invlist(diff.ref, requesting_player);
      list.set_at(diff.ref.index, diff.prev);
      set_invlist(diff.ref, list, requesting_player);
    }
    
    //and release locks
    for(const auto& ref : locked_refs) {
      unlock_invlist(ref, requesting_player);
    }
    
    log(LogSource::SERVER, LogLevel::NOTICE, "inv_apply_patch: " + err_message);
    patch.make_deny();
    if(requesting_player != NULL)
      requesting_player->send(patch.as_json("inv_patch_deny"));
    return false;
  }
  
  //Release locks
  for(const auto& ref : locked_refs) {
    unlock_invlist(ref, requesting_player);
  }
  
  if(requesting_player != NULL)
    requesting_player->send(patch.as_json("inv_patch_accept"));
  
  //TODO: inform interested players (possibly with only partial patches)
  
  return true;
}

bool Server::lock_unlock_invlist(InvRef ref, bool do_lock, PlayerState *player_hint) {
  if(ref.obj_type == "player") {
    PlayerState *player = player_hint;
    
    if(player == NULL) {
      std::string player_id = ref.obj_id;
      
      std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
      for(auto it : m_players) {
        PlayerState *check = it.second;
        std::shared_lock<std::shared_mutex> player_lock(check->lock);
        if(check->get_tag() != player_id)
          continue;
        player = check;
        break;
      }
    }
    
    if(player == NULL)
      return false;
    
    std::shared_lock<std::shared_mutex> player_lock(player->lock);
    
    InvList list = player->data.inventory.get(ref.list_name);
    if(list.is_nil)
      return false;
    
    //will be automatically created if not present
    
    auto search = player->inventory_lock.find(ref.list_name);
    if(search == player->inventory_lock.end()) {
      player_lock.unlock();
      std::unique_lock<std::shared_mutex> player_lock_unique(player->lock);
      
      std::shared_mutex* lock = new std::shared_mutex();
      player->inventory_lock[ref.list_name] = lock;
      
      if(do_lock) {
        lock->lock();
      } else {
        lock->unlock();
      }
    } else {
      if(do_lock) {
        search->second->lock();
      } else {
        search->second->unlock();
      }
    }
    return true;
  }
  
  return false;
}
bool Server::lock_invlist(InvRef ref, PlayerState *player_hint) {
  return lock_unlock_invlist(ref, true, player_hint);
}
bool Server::unlock_invlist(InvRef ref, PlayerState *player_hint) {
  return lock_unlock_invlist(ref, false, player_hint);
}

InvList Server::get_invlist(InvRef ref, PlayerState *player_hint) {
  if(ref.obj_type == "player") {
    PlayerState *player = player_hint;
    
    if(player == NULL) {
      std::string player_id = ref.obj_id;
      
      std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
      for(auto it : m_players) {
        PlayerState *check = it.second;
        std::shared_lock<std::shared_mutex> player_lock(check->lock);
        if(check->get_tag() != player_id)
          continue;
        player = check;
        break;
      }
    }
    
    if(player == NULL)
      return InvList();
    
    std::shared_lock<std::shared_mutex> player_lock(player->lock);
    return player->data.inventory.get(ref.list_name);
  }
  
  return InvList();
}
bool Server::set_invlist(InvRef ref, InvList list, PlayerState *player_hint) {
  if(ref.obj_type == "player") {
    PlayerState *player = player_hint;
    
    if(player == NULL) {
      std::string player_id = ref.obj_id;
      
      std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
      for(auto it : m_players) {
        PlayerState *check = it.second;
        std::shared_lock<std::shared_mutex> player_lock(check->lock);
        if(check->get_tag() != player_id)
          continue;
        player = check;
        break;
      }
    }
    
    if(player == NULL)
      return false;
    
    std::unique_lock<std::shared_mutex> player_lock(player->lock);
    bool res = player->data.inventory.set(ref.list_name, list);
    if(!res)
      return false;
    
    if(player->auth) {
      db.update_player_data(player->get_data());
    }
    return true;
  }
  
  return false;
}
