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

NodeDef get_node_def(std::string itemstring) {
  if(itemstring == "air") {
    NodeDef def("air");
    def.transparent = true;
    def.pass_sunlight = true;
    return def;
  } else if(itemstring == "default:torch") {
    NodeDef def("default:torch");
    def.transparent = true;
    def.pass_sunlight = true;
    def.light_level = 15;
    return def;
  } else if(itemstring == "default:glass") {
    NodeDef def("default:glass");
    def.transparent = true;
    def.pass_sunlight = true;
    return def;
  } else if(itemstring == "default:leaves") {
    NodeDef def("default:leaves");
    def.transparent = true;
    return def;
  }
  
  return NodeDef("nothing");
}
