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

#include "player_data.h"

#include "json.h"
#include "log.h"
#include "inventory.h"
#include "creative.h"

#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

PlayerData::PlayerData(std::string json, std::string _auth_id) : is_nil(true), auth_id(_auth_id) {
  try {
    std::stringstream ss;
    ss << json;
    
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss, pt);
    
    name = pt.get<std::string>("name");
    
    pos.x = pt.get<double>("pos.x");
    pos.y = pt.get<double>("pos.y");
    pos.z = pt.get<double>("pos.z");
    pos.w = pt.get<int>("pos.w");
    pos.world = pt.get<int>("pos.world");
    pos.universe = pt.get<int>("pos.universe");
    
    vel.x = pt.get<double>("vel.x");
    vel.y = pt.get<double>("vel.y");
    vel.z = pt.get<double>("vel.z");
    vel.w = pt.get<int>("vel.w");
    vel.world = pt.get<int>("vel.world");
    vel.universe = pt.get<int>("vel.universe");
    
    rot.x = pt.get<double>("rot.x");
    rot.y = pt.get<double>("rot.y");
    rot.z = pt.get<double>("rot.z");
    rot.w = pt.get<double>("rot.w");
    
    for(auto& it : pt.get_child("privs")) {
      std::string priv = it.second.get_value<std::string>();
      privs.insert(priv);
    }
    
    if(pt.get_child_optional("inventory")) {
      inventory = InvSet(pt.get_child("inventory"));
    } else {
      inventory.add("main", InvList(32));
      inventory.add("craft", InvList(9));
      inventory.add("craftOutput", InvList(1));
      inventory.add("hand", InvList(1));
    }
    
    inventory.add("creative", get_creative_inventory());
    
    creative_mode = false;
    if(pt.get_child_optional("creative_mode"))
      creative_mode = pt.get<bool>("creative_mode");
    if(!has_priv("creative"))
      creative_mode = false;
    
    is_nil = false;
  } catch(boost::property_tree::ptree_error const& e) {
    log(LogSource::PLAYER, LogLevel::ERR, "JSON parse error: " + std::string(e.what()) + " json=" + json);
    
    is_nil = true;
  }
}

std::string PlayerData::to_json() {
  std::ostringstream out;
  
  out << "{\"name\":\"" << json_escape(name) << "\","
      << "\"pos\":" << pos.to_json() << ","
      << "\"vel\":" << vel.to_json() << ","
      << "\"rot\":" << rot.to_json() << ","
      << "\"privs\":[";
  
  bool is_first = true;
  for(auto it : privs) {
    if(!is_first) { out << ","; }
    is_first = false;
    out << "\"" << json_escape(it) << "\"";
  }
  
  out << "],";
  
  out << "\"inventory\":" << inventory.to_json(std::set<std::string>{"creative"}) << ",";
  
  out << "\"creative_mode\":" << std::boolalpha << creative_mode;
  
  out << "}";
  
  return out.str();
}

bool PlayerData::has_priv(std::string priv) {
  return privs.find(priv) != privs.end();
}
