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

#ifndef __NODE_H__
#define __NODE_H__

#include <ostream>
#include <map>

#include <boost/property_tree/ptree.hpp>

class Node {
  public:
    Node() {};
    Node(std::string _itemstring) : itemstring(_itemstring), rot(0) {};
    Node(std::string _itemstring, unsigned int _rot) : itemstring(_itemstring), rot(_rot) {};
    friend std::ostream& operator<<(std::ostream &out, const Node &n) {
      out << "Node '" << n.itemstring << "' rot=" << n.rot;
      return out;
    };
    bool operator==(const Node& other) {
      return itemstring == other.itemstring && rot == other.rot;
    };
    bool operator!=(const Node& other) {
      return !operator==(other);
    };
    
    std::string itemstring;
    unsigned int rot;
};

class NodeDef {
  public:
    NodeDef() : itemstring("nothing"), breakable(false) {};
    NodeDef(std::string _itemstring) : itemstring(_itemstring), breakable(true) {};
    
    std::string itemstring;
    
    bool transparent = false;
    bool pass_sunlight = false;
    int light_level = 0;
    
    bool is_fluid = false;
    bool can_place_inside = false;
    
    std::string drops;
    bool breakable;
    std::map<std::string, int> groups;
};

void load_node_defs(boost::property_tree::ptree pt);
NodeDef get_node_def(Node node);
NodeDef get_node_def(std::string itemstring);

#endif
