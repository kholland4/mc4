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

class PlayerData {
  public:
    PlayerData() : is_nil(true) {}
    
    std::string name; //player's display name (by default it's their login username)
    
    MapPos<double> pos;
    MapPos<double> vel;
    Quaternion rot;
    
    bool is_nil;
    
    std::string auth_id; //unique identifier of player in database
};

class PlayerPasswordAuthInfo {
  public:
    PlayerPasswordAuthInfo() : is_nil(true) {}
    
    bool is_nil;
    std::string login_name;
    std::string salt;
    std::string verifier;
    std::string auth_id;
};

#endif
