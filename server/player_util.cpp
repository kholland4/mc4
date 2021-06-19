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
#include "config.h"

#include <regex>
#include <sstream>

std::set<std::string> allowed_privs = {"interact", "shout", "touch", "fast", "fly", "teleport", "settime", "give", "creative", "grant", "grant_basic", "admin"};

//To grant $x, a player must possess one or more of $y
//By default this is "grant"
std::map<std::string, std::set<std::string>> required_to_grant = {
  {"interact", {"grant", "grant_basic"}},
  {"shout", {"grant", "grant_basic"}},
  {"fast", {"grant", "grant_basic"}},
  {"fly", {"grant", "grant_basic"}},
  {"grant_basic", {"grant", "admin"}},
  {"grant", {"admin"}},
  {"admin", {"admin"}}
};

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
  
  std::string default_grants_str = get_config<std::string>("player.default_grants");
  std::istringstream dg_it(default_grants_str);
  std::set<std::string> default_grants(std::istream_iterator<std::string>{dg_it}, std::istream_iterator<std::string>{});
  for(const auto& grant_priv : default_grants) {
    if(allowed_privs.find(grant_priv) == allowed_privs.end()) {
      log(LogSource::PLAYER, LogLevel::ERR, "cannot grant default priv '" + grant_priv + "': not a valid priv");
      continue;
    }
    
    data.privs.insert(grant_priv);
  }
}

bool validate_player_name(std::string name) {
  std::regex nick_allow("^[a-zA-Z0-9\\-_]{1,40}$");
  if(!std::regex_match(name, nick_allow)) {
    //not ok
    return false;
  }
  return true;
}
