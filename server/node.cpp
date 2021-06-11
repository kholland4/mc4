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

std::map<std::string, NodeDef*> all_node_defs;

void load_node_defs(boost::property_tree::ptree pt) {
  size_t count = 0;
  
  for(const auto& n : pt.get_child("nodeDefs")) {
    std::string itemstring = n.first;
    
    NodeDef *def = new NodeDef(itemstring);
    
    def->transparent = n.second.get<bool>("transparent");
    def->pass_sunlight = n.second.get<bool>("passSunlight");
    def->light_level = n.second.get<int>("lightLevel");
    
    def->is_fluid = n.second.get<bool>("isFluid");
    def->can_place_inside = n.second.get<bool>("canPlaceInside");
    
    def->drops = n.second.get<std::string>("drops");
    if(def->drops == "null")
      def->drops = "";
    def->breakable = n.second.get<bool>("breakable");
    for(const auto& group : n.second.get_child("groups")) {
      def->groups[group.first] = std::stoi(group.second.data());
    }
    
    all_node_defs[itemstring] = def;
    
    count++;
  }
  
  log(LogSource::LOADER, LogLevel::INFO, std::string("Loaded " + std::to_string(count) + " node definitions"));
}

NodeDef get_node_def(Node node) {
  return get_node_def(node.itemstring);
}
NodeDef get_node_def(std::string itemstring) {
  auto search = all_node_defs.find(itemstring);
  if(search != all_node_defs.end()) {
    return *(search->second);
  }
  
  return NodeDef();
}
