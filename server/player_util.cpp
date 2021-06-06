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

#include "player_util.h"
#include "creative.h"

#include <regex>

void init_player_data(PlayerData &data) {
  data.pos.set(0, 20, 0, 0, 0, 0);
  data.vel.set(0, 0, 0, 0, 0, 0);
  data.rot.set(0, 0, 0, 0);
  
  data.inventory.add("main", InvList(32));
  data.inventory.add("craft", InvList(9));
  data.inventory.add("craftOutput", InvList(1));
  data.inventory.add("hand", InvList(1));
  data.inventory.add("creative", get_creative_inventory());
  
  data.creative_mode = false;
}

bool validate_player_name(std::string name) {
  std::regex nick_allow("^[a-zA-Z0-9\\-_]{1,40}$");
  if(!std::regex_match(name, nick_allow)) {
    //not ok
    return false;
  }
  return true;
}
