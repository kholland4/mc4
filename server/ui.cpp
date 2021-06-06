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

#include "ui.h"
#include "json.h"

#include <sstream>

std::string UI_InvList::to_json() const {
  std::ostringstream out;
  
  out << "{\"type\":\"inv_list\",\"ref\":" << ref.to_json() << "}";
  return out.str();
}

std::string UI_Spacer::to_json() const {
  return "{\"type\":\"spacer\"}";
}

std::string UISpec::components_to_json() const {
  std::ostringstream out;
  
  out << "[";
  bool first = true;
  for(const auto& c : components) {
    if(!first)
      out << ",";
    first = false;
    out << c;
  }
  out << "]";
  return out.str();
}

bool UIInstance::operator<(const UIInstance& other) const {
  return id < other.id;
}

std::string UIInstance::ui_open_json() const {
  std::ostringstream out;
  out << "{\"type\":\"ui_open\",\"ui\":" << spec.components_to_json() << ","
      << "\"id\":\"" << json_escape(id) << "\"}";
  return out.str();
}
std::string UIInstance::ui_close_json() const {
  std::ostringstream out;
  out << "{\"type\":\"ui_close\",\"id\":\"" << json_escape(id) << "\"}";
  return out.str();
}
