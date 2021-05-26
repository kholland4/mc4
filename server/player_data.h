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

#ifndef __PLAYER_DATA_H__
#define __PLAYER_DATA_H__

#include "vector.h"
#include "inventory.h"

#include <set>
#include <shared_mutex>

class PlayerData {
  public:
    PlayerData() : is_nil(true) {}
    PlayerData(std::string json, std::string _auth_id);
    
    bool has_priv(std::string priv);
    
    std::string to_json();
    
    std::string name; //player's display name (by default it's their login username)
    
    MapPos<double> pos;
    MapPos<double> vel;
    Quaternion rot;
    
    std::set<std::string> privs;
    InvSet inventory;
    
    bool is_nil;
    
    std::string auth_id; //unique identifier of player in database
};

class PlayerAuthInfo {
  public:
    PlayerAuthInfo() : is_nil(true), has_db_unique_id(false), db_unique_id(0) {}
    
    bool is_nil;
    bool has_db_unique_id;
    int64_t db_unique_id;
    std::string type;
    std::string login_name;
    std::string auth_id;
    
    std::string data; //other auth data as JSON
};

#endif
