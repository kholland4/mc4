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

#ifndef __ITEM_H__
#define __ITEM_H__

#include <ostream>
#include <set>
#include <map>
#include <vector>

class ItemDef {
  public:
    ItemDef() : itemstring("nothing"), stackable(false), max_stack(1), is_node(false), is_tool(false), tool_wear(0), in_creative_inventory(false) {};
    ItemDef(std::string _itemstring) : itemstring(_itemstring), stackable(false), max_stack(1), is_node(false), is_tool(false), tool_wear(0), in_creative_inventory(false) {};
    
    std::string itemstring;
    bool stackable;
    int max_stack;
    
    bool is_node;
    bool is_tool;
    int tool_wear;
    
    bool in_creative_inventory;
    
    std::set<std::string> groups;
    std::map<std::string, std::pair<std::vector<double>, int>> tool_groups;
};

void load_item_defs(std::string filename);
ItemDef get_item_def(std::string itemstring);

#endif
