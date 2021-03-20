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

#include "database.h"
#include "log.h"

void MemoryDB::store_pw_info(PlayerAuthInfo& info) {
  info.db_unique_id = pw_auth_id_counter;
  info.has_db_unique_id = true;
  PlayerAuthInfo *info_store = new PlayerAuthInfo(info);
  pw_auth_store[pw_auth_id_counter] = info_store;
  pw_auth_id_counter++;
}

std::vector<PlayerAuthInfo> MemoryDB::fetch_pw_info(std::string login_name, std::string type) {
  std::vector<PlayerAuthInfo> res_list;
  
  for(auto it : pw_auth_store) {
    if(it.second->login_name == login_name && it.second->type == type) {
      res_list.push_back(*(it.second));
    }
  }
  return res_list;
}
std::vector<PlayerAuthInfo> MemoryDB::fetch_pw_info(std::string auth_id) {
  std::vector<PlayerAuthInfo> res_list;
  
  for(auto it : pw_auth_store) {
    if(it.second->auth_id == auth_id) {
      res_list.push_back(*(it.second));
    }
  }
  return res_list;
}
void MemoryDB::update_pw_info(PlayerAuthInfo info) {
  if(!info.has_db_unique_id) {
    log(LogSource::MEMORYDB, LogLevel::ERR, "unable to update auth info for '" + info.login_name + "': no db_unique_id");
    return;
  }
  
  auto search = pw_auth_store.find(info.db_unique_id);
  if(search == pw_auth_store.end()) {
    log(LogSource::MEMORYDB, LogLevel::ERR, "unable to update auth info for '" + info.login_name + "': not found");
    return;
  }
  
  for(auto it : pw_auth_store) {
    if(it.second->login_name == info.login_name) {
      log(LogSource::MEMORYDB, LogLevel::ERR, "unable to update auth info for '" + info.login_name + "': new name is already in use");
      return;
    }
  }
  
  delete search->second;
  pw_auth_store.erase(search->first);
  
  PlayerAuthInfo *info_store = new PlayerAuthInfo(info);
  pw_auth_store[info.db_unique_id] = info_store;
}
void MemoryDB::delete_pw_info(PlayerAuthInfo& info) {
  if(!info.has_db_unique_id) {
    log(LogSource::MEMORYDB, LogLevel::ERR, "unable to delete auth info for '" + info.login_name + "': no db_unique_id");
    return;
  }
  
  auto search = pw_auth_store.find(info.db_unique_id);
  if(search == pw_auth_store.end()) {
    log(LogSource::MEMORYDB, LogLevel::ERR, "unable to delete auth info for '" + info.login_name + "': not found");
    return;
  }
  
  delete search->second;
  pw_auth_store.erase(search->first);
  
  info.has_db_unique_id = false;
}



void MemoryDB::store_player_data(PlayerData data) {
  auto search = player_data_store.find(data.auth_id);
  if(search != player_data_store.end()) {
    log(LogSource::MEMORYDB, LogLevel::ERR, "unable to store data for '" + data.auth_id + "': auth_id already in use");
    return;
  }
  
  PlayerData *data_store = new PlayerData(data);
  player_data_store[data.auth_id] = data_store;
}
PlayerData MemoryDB::fetch_player_data(std::string auth_id) {
  auto search = player_data_store.find(auth_id);
  if(search == player_data_store.end()) {
    //not found, return nil
    return PlayerData();
  }
  
  return PlayerData(*(search->second));
}
void MemoryDB::update_player_data(PlayerData data) {
  auto search = player_data_store.find(data.auth_id);
  if(search == player_data_store.end()) {
    log(LogSource::MEMORYDB, LogLevel::ERR, "unable to update data for '" + data.auth_id + "': not found");
    return;
  }
  
  delete search->second;
  player_data_store.erase(search->first);
  
  PlayerData *data_store = new PlayerData(data);
  player_data_store[data.auth_id] = data_store;
}
void MemoryDB::delete_player_data(std::string auth_id) {
  auto search = player_data_store.find(auth_id);
  if(search == player_data_store.end()) {
    log(LogSource::MEMORYDB, LogLevel::ERR, "unable to delete data for '" + auth_id + "': not found");
    return;
  }
  
  delete search->second;
  player_data_store.erase(search->first);
}
bool MemoryDB::player_data_name_used(std::string name) {
  for(auto it : player_data_store) {
    if(it.second->name == name) {
      return true;
    }
  }
  return false;
}
