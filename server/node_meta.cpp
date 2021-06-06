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

#include "node_meta.h"
#include "json.h"
#include "log.h"

#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

NodeMeta::NodeMeta(MapPos<int> _pos, std::string json) : is_nil(false), db_error(false), parse_error(false), pos(_pos) {
  try {
    std::stringstream ss;
    ss << json;
    
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss, pt);
    
    if(pt.get_child_optional("inventory")) {
      inventory = InvSet(pt.get_child("inventory"));
    }
  } catch(boost::property_tree::ptree_error const& e) {
    log(LogSource::PLAYER, LogLevel::ERR, "JSON parse error: " + std::string(e.what()) + " json=" + json);
    
    is_nil = true;
    parse_error = true;
  }
}

std::string NodeMeta::to_json() {
  std::ostringstream out;
  
  out << "{";
  
  if(inventory.inventory.size() > 0) {
    out << "\"inventory\":" << inventory.to_json();
  }
  
  out << "}";
  
  return out.str();
}
