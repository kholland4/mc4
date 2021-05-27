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

#include "tool.h"
#include "item.h"

std::optional<double> calc_dig_time_actual(Node target, ItemDef tool_def) {
  if(!tool_def.is_tool)
    return std::nullopt;
  
  NodeDef node_def = get_node_def(target.itemstring);
  if(!node_def.breakable)
    return std::nullopt;
  
  std::string group_match;
  for(auto tool_group : tool_def.tool_groups) {
    for(auto node_group : node_def.groups) {
      if(tool_group.first != node_group.first)
        continue;
      group_match = tool_group.first;
      break;
    }
    
    if(group_match != "")
      break;
  }
  
  if(group_match == "")
    return std::nullopt;
  
  int node_level = node_def.groups[group_match];
  const std::pair<std::vector<double>, int>& tool_info =
      tool_def.tool_groups[group_match];
  
  if(node_level > tool_info.second || node_level < 0 || node_level >= (int)tool_info.first.size())
    return std::nullopt;
  
  return tool_info.first[node_level];
}

std::optional<double> calc_dig_time(Node target, InvStack tool) {
  ItemDef tool_def = get_item_def(tool.itemstring);
  std::optional<double> dig_time = calc_dig_time_actual(target, tool_def);
  if(dig_time)
    return dig_time;
  
  ItemDef no_tool_def(":");
  no_tool_def.is_tool = true;
  no_tool_def.tool_wear = 999;
  no_tool_def.tool_groups["crumbly"] =
      std::make_pair(std::vector<double>{0, 1, 2}, 2);
  no_tool_def.tool_groups["choppy"] =
      std::make_pair(std::vector<double>{0, 2, 4}, 2);
  no_tool_def.tool_groups["snappy"] =
      std::make_pair(std::vector<double>{0, 1}, 1);
  no_tool_def.tool_groups["oddly_breakable_by_hand"] =
      std::make_pair(std::vector<double>{0, 0.2, 0.5}, 2);
  
  return calc_dig_time_actual(target, no_tool_def);
}
