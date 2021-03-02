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

class Node {
  public:
    Node() {};
    Node(std::string _itemstring) : itemstring(_itemstring), rot(0) {};
    Node(std::string _itemstring, unsigned int _rot) : itemstring(_itemstring), rot(_rot) {};
    friend std::ostream& operator<<(std::ostream &out, const Node &n) {
      out << "Node '" << n.itemstring << "' rot=" << n.rot;
      return out;
    };
    
    std::string itemstring;
    unsigned int rot;
};

class NodeDef {
  public:
    NodeDef() : itemstring("nothing"), transparent(false), pass_sunlight(false), light_level(0) {};
    NodeDef(std::string _itemstring) : itemstring(_itemstring), transparent(false), pass_sunlight(false), light_level(0) {};
    std::string itemstring;
    bool transparent;
    bool pass_sunlight;
    int light_level;
};

NodeDef get_node_def(std::string itemstring);

#endif