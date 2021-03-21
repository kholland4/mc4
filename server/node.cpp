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

#include "node.h"

#include "log.h"

#include <map>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

std::map<std::string, NodeDef*> all_node_defs;

void load_node_defs(std::string filename) {
  boost::property_tree::ptree pt;
  boost::property_tree::read_json(filename, pt);
  
  size_t count = 0;
  
  for(auto& n : pt.get_child("nodeDefs")) {
    std::string itemstring = n.first;
    
    NodeDef *def = new NodeDef(itemstring);
    def->transparent = n.second.get<bool>("transparent");
    def->pass_sunlight = n.second.get<bool>("passSunlight");
    def->light_level = n.second.get<int>("lightLevel");
    def->is_fluid = n.second.get<bool>("isFluid");
    
    all_node_defs[itemstring] = def;
    
    count++;
  }
  
  log(LogSource::LOADER, LogLevel::INFO, std::string("Loaded " + std::to_string(count) + " node definitions"));
}

NodeDef get_node_def(std::string itemstring) {
  auto search = all_node_defs.find(itemstring);
  if(search != all_node_defs.end()) {
    return *(search->second);
  }
  
  return NodeDef("nothing");
}
