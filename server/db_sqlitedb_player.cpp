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

void SQLiteDB::store_pw_info(PlayerAuthInfo info) {
  
}

PlayerAuthInfo SQLiteDB::fetch_pw_info(std::string login_name) {
  return PlayerAuthInfo();
}
void SQLiteDB::update_pw_info(std::string old_login_name, PlayerAuthInfo info) {
  
}
void SQLiteDB::delete_pw_info(std::string login_name) {
  
}



void SQLiteDB::store_player_data(PlayerData data) {
  
}
PlayerData SQLiteDB::fetch_player_data(std::string auth_id) {
  return PlayerData();
}
void SQLiteDB::update_player_data(PlayerData data) {
  
}
void SQLiteDB::delete_player_data(std::string auth_id) {
  
}
