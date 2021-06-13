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

#include "item.h"
#include "log.h"

std::map<std::string, ItemDef*> all_item_defs;

void load_item_defs(boost::property_tree::ptree pt) {
  size_t count = 0;
  
  for(const auto& n : pt.get_child("itemDefs")) {
    std::string itemstring = n.first;
    
    ItemDef *def = new ItemDef(itemstring);
    def->stackable = n.second.get<bool>("stackable");
    def->max_stack = n.second.get<int>("maxStack");
    
    def->is_node = n.second.get<bool>("isNode");
    def->is_tool = n.second.get<bool>("isTool");
    if(n.second.get<std::string>("toolWear") != "null")
      def->tool_wear = n.second.get<int>("toolWear");
    else
      def->tool_wear = 0;
    def->fuel = n.second.get<int>("fuel");
    
    def->in_creative_inventory = n.second.get<bool>("inCreativeInventory");
    
    const auto& groups_list = n.second.get_child("groups");
    for(const auto& group : groups_list) {
      def->groups.insert(group.first);
    }
    
    const auto& tool_groups_list = n.second.get_child("toolGroups");
    for(const auto& group : tool_groups_list) {
      std::string group_name = group.first;
      int max_level = group.second.get<int>("maxlevel");
      std::vector<double> times;
      for(const auto& time_item : group.second.get_child("times")) {
        times.push_back(std::stod(time_item.second.data()));
      }
      
      def->tool_groups[group_name] = std::make_pair(times, max_level);
    }
    
    all_item_defs[itemstring] = def;
    
    count++;
  }
  
  log(LogSource::LOADER, LogLevel::INFO, std::string("Loaded " + std::to_string(count) + " item definitions"));
}

ItemDef get_item_def(std::string itemstring) {
  auto search = all_item_defs.find(itemstring);
  if(search != all_item_defs.end()) {
    return *(search->second);
  }
  
  return ItemDef();
}
