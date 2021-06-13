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

#include <set>
#include <map>
#include <vector>

#include <boost/property_tree/ptree.hpp>

class ItemDef {
  public:
    ItemDef() : itemstring("nothing") {};
    ItemDef(std::string _itemstring) : itemstring(_itemstring) {};
    
    std::string itemstring;
    bool stackable = false;
    int max_stack = 1;
    
    bool is_node = false;
    bool is_tool = false;
    int tool_wear = 0;
    int fuel = 0;
    
    bool in_creative_inventory = false;
    
    std::set<std::string> groups;
    std::map<std::string, std::pair<std::vector<double>, int>> tool_groups;
};

extern std::map<std::string, ItemDef*> all_item_defs;

void load_item_defs(boost::property_tree::ptree pt);
ItemDef get_item_def(std::string itemstring);

#endif
