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

#include "json.h"

#include <sstream>

std::string json_escape(std::string s) {
  std::ostringstream res;
  for(size_t i = 0; i < s.length(); i++) {
    switch(s[i]) {
      case '\b':
        res << "\\b";
        break;
      case '\f':
        res << "\\f";
        break;
      case '\n':
        res << "\\n";
        break;
      case '\r':
        res << "\\r";
        break;
      case '\t':
        res << "\\t";
        break;
      case '\"':
        res << "\\\"";
        break;
      case '\\':
        res << "\\\\";
        break;
      default:
        res << s[i];
    }
  }
  return res.str();
}
