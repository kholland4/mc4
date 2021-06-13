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

#ifndef __NODE_META_H__
#define __NODE_META_H__

#include "vector.h"
#include "inventory.h"

#include <string>

class NodeMeta {
  public:
    NodeMeta(MapPos<int> _pos) : is_nil(true), db_error(false), parse_error(false), pos(_pos) {};
    NodeMeta(MapPos<int> _pos, std::string json);
    std::string to_json();
    
    InvSet inventory;
    std::optional<int> int1;
    std::optional<int> int2;
    
    bool is_nil;
    bool db_error;
    bool parse_error;
    MapPos<int> pos;
};

#endif
