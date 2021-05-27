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

#ifndef __CRAFT_H__
#define __CRAFT_H__

#include <vector>
#include <optional>

#include <boost/property_tree/ptree.hpp>

#include "item.h"
#include "inventory.h"

class CraftDef {
  public:
    CraftDef() {};
    
    InvStack result;
    InvList ingredients;
    std::optional<std::pair<int, int>> shape;
    
    std::string type;
    std::optional<double> cook_time;
};

void load_craft_defs(boost::property_tree::ptree pt);

//pair <patch to use when consuming craft, patch to use to display craft output>
std::optional<std::pair<InvPatch, InvPatch>> craft_calc_result(const InvList& craft_input, std::pair<int, int> craft_input_shape);
std::optional<std::pair<InvPatch, InvPatch>> craft_calc_result(const CraftDef& craft, const InvList& craft_input, std::pair<int, int> craft_input_shape);

#endif
