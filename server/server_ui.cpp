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

void Server::open_ui(PlayerState *player, const UIInstance& instance) {
  std::unique_lock<std::shared_mutex> ui_list_lock(active_ui_lock);
  active_ui.insert(instance);
  
  std::shared_lock<std::shared_mutex> player_lock_shared(player->lock);
  player->send(instance.ui_open_json());
  
  if(instance.open_callback)
    instance.open_callback();
}

void Server::update_ui(PlayerState *player, const UIInstance& instance) {
  std::unique_lock<std::shared_mutex> ui_list_lock(active_ui_lock);
  active_ui.insert(instance); // will replace existing UI because they have the same id
  
  std::shared_lock<std::shared_mutex> player_lock_shared(player->lock);
  player->send(instance.ui_update_json());
  
  if(instance.update_callback)
    instance.update_callback();
}

void Server::close_ui(PlayerState *player, const UIInstance& instance) {
  if(instance.close_callback)
    instance.close_callback();
  
  std::shared_lock<std::shared_mutex> player_lock_shared(player->lock);
  player->send(instance.ui_close_json());
  
  std::unique_lock<std::shared_mutex> ui_list_lock(active_ui_lock);
  auto search = active_ui.find(instance);
  if(search != active_ui.end())
    active_ui.erase(search);
}

UIInstance Server::find_ui(std::string what_for) {
  std::shared_lock<std::shared_mutex> ui_list_lock(active_ui_lock);
  
  for(const auto& it : active_ui) {
    if(it.what_for == what_for)
      return it;
  }
  
  return UIInstance();
}
