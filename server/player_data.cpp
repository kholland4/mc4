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
    
    is_nil = false;
  } catch(std::exception const& e) {
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
  
  out << "]}";
  
  return out.str();
}

bool PlayerData::has_priv(std::string priv) {
  return privs.find(priv) != privs.end();
}
