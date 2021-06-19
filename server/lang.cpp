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

#include "lang.h"

#include <sstream>

std::string lang_fmt_list(std::vector<std::string> items, std::string conj, std::string quote_char) {
  std::ostringstream out;
  size_t qty = items.size();
  size_t i = 0;
  for(const auto& item : items) {
    out << quote_char << item << quote_char;
    if(i < qty - 1) {
      if(qty >= 3)
        out << ", ";
      else
        out << " ";
    }
    
    if(qty > 1 && i == qty - 2)
      out << conj << " ";
    i++;
  }
  return out.str();
}
std::string lang_fmt_list(std::set<std::string> items, std::string conj, std::string quote_char) {
  return lang_fmt_list(std::vector<std::string>{items.begin(), items.end()}, conj, quote_char);
}
