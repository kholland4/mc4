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

#ifndef __PLAYER_UTIL_H__
#define __PLAYER_UTIL_H__

#include "player_data.h"

#include <set>
#include <map>

#define BAD_PLAYER_NAME_MESSAGE "invalid nickname: allowed characters are a-z A-Z 0-9 - _ and length must be 1 to 40 characters"

extern std::set<std::string> allowed_privs;
extern std::map<std::string, std::set<std::string>> required_to_grant;

void init_player_data(PlayerData &data);
bool validate_player_name(std::string name);
std::string generate_possible_guest_name();

#endif
