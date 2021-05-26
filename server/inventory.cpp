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

#include "inventory.h"
#include "json.h"
#include <sstream>

bool InvRef::operator<(const InvRef& other) const {
  if(obj_type == other.obj_type) {
    if(obj_id == other.obj_id) {
      if(list_name == other.list_name) {
        return index < other.index;
      } else {
        return list_name < other.list_name;
      }
    } else {
      return obj_id < other.obj_id;
    }
  } else {
    return obj_type < other.obj_type;
  }
}

std::string InvRef::as_json() {
  std::ostringstream out;
  
  out << "{\"objType\":\"" << json_escape(obj_type) << "\",";
  
  if(obj_id != "null")
    out << "\"objID\":\"" << json_escape(obj_id) << "\",";
  else
    out << "\"objID\":null,";
  
  out << "\"listName\":\"" << json_escape(list_name) << "\","
      << "\"index\":" << std::to_string(index) << "}";
  
  return out.str();
}

InvStack::InvStack(boost::property_tree::ptree pt) : is_nil(false) {
  if(!pt.data().empty()) {
    is_nil = true;
    return;
  }
  
  itemstring = pt.get<std::string>("itemstring");
  count = pt.get<int>("count");
  if(pt.get<std::string>("wear") == "null")
    wear = std::nullopt;
  else
    wear = pt.get<int>("wear");
  
  //FIXME what if the data is the literal string "null"?
  if(pt.get<std::string>("data") == "null")
    data = std::nullopt;
  else
    data = pt.get<std::string>("data");
}
std::string InvStack::as_json() {
  if(is_nil)
    return "null";
  
  std::ostringstream out;
  
  out << "{\"itemstring\":\"" << json_escape(itemstring) << "\","
      << "\"count\":" << std::to_string(count) << ",";
  
  if(wear)
    out << "\"wear\":" << std::to_string(*wear) << ",";
  else
    out << "\"wear\":null,";
  
  if(data)
    out << "\"data\":\"" << json_escape(*data) << "\"}";
  else
    out << "\"data\":null}";
  
  return out.str();
}

InvStack::InvStack(std::string spec) : count(1), wear(std::nullopt), data(std::nullopt), is_nil(false) {
  //"default:wood 64" etc.
  
  //split by spaces
  std::vector<std::string> parts;
  size_t start = 0;
  size_t end = spec.find(" ");
  while(end != std::string::npos) {
    parts.push_back(spec.substr(start, end - start));
    start = end + 1; //length of " "
    end = spec.find(" ", start);
  }
  parts.push_back(spec.substr(start));
  
  itemstring = parts[0];
  
  if(parts.size() >= 2) {
    try {
      count = std::stoi(parts[1]);
    } catch(std::invalid_argument const& e) {
      
    } catch(std::out_of_range const& e) {
      
    }
  }
  
  if(parts.size() >= 3) {
    try {
      wear = std::stoi(parts[2]);
    } catch(std::invalid_argument const& e) {
      
    } catch(std::out_of_range const& e) {
      
    }
  }
  
  //TODO use the remaining string as 'data' if present
  
  ItemDef def = get_item_def(itemstring);
  if(def.is_tool)
    wear = def.tool_wear;
}

InvStack::InvStack(ItemDef def) : itemstring(def.itemstring), count(1), wear(std::nullopt), data(std::nullopt), is_nil(false) {
  if(def.is_tool)
    wear = def.tool_wear;
}

InvList::InvList(boost::property_tree::ptree pt) : is_nil(false) {
  for(auto it : pt) {
    list.push_back(InvStack(it.second));
  }
}
std::string InvList::as_json() {
  if(is_nil)
    return "null";
  
  std::ostringstream out;
  
  out << "[";
  
  bool first = true;
  for(auto it : list) {
    if(!first)
      out << ",";
    first = false;
    out << it.as_json();
  }
  
  out << "]";
  
  return out.str();
}
std::string InvStack::spec() {
  std::ostringstream out;
  out << itemstring << " " << count;
  if(wear)
    out << " " << std::to_string(*wear);
  if(data)
    out << " " << *data;
  return out.str();
}

bool InvList::give(InvStack stack) {
  if(!give(stack, true))
    return false;
  return give(stack, false);
}
bool InvList::give(InvStack stack, bool dry_run) {
  if(stack.is_nil)
    return true;
  
  //FIXME use actual max_stack value
  int max_stack = 64;
  int needed = stack.count;
  for(InvStack& s : list) {
    if(s.is_nil)
      continue;
    if(s.itemstring != stack.itemstring)
      continue;
    int avail = max_stack - s.count;
    if(avail <= 0)
      continue;
    
    if(!dry_run)
      s.count += std::min(avail, needed);
    needed -= std::min(avail, needed);
    
    if(needed <= 0)
      break;
  }
  
  if(needed > 0) {
    stack.count = needed;
    
    for(InvStack& s : list) {
      if(!s.is_nil)
        continue;
      if(!dry_run)
        s = stack;
      needed -= stack.count;
      break;
    }
  }
  
  if(needed > 0)
    return false;
  
  return true;
}

InvStack InvList::get_at(int index) {
  if(index < 0 || index >= (int)list.size())
    return InvStack();
  
  return list[index];
}
bool InvList::set_at(int index, InvStack stack) {
  if(index < 0 || index >= (int)list.size())
    return false;
  
  list[index] = stack;
  return true;
}

bool InvList::take_at(int index, InvStack to_take) {
  if(to_take.is_nil)
    return true;
  
  if(index < 0 || index >= (int)list.size())
    return false;
  
  InvStack& s = list[index];
  if(s.is_nil)
    return false;
  if(s.itemstring != to_take.itemstring)
    return false;
  if(to_take.count > s.count)
    return false;
  s.count -= to_take.count;
  if(s.count <= 0)
    s = InvStack();
  
  return true;
}

InvSet::InvSet(boost::property_tree::ptree pt) {
  for(auto it : pt) {
    std::string list_name = it.first;
    add(list_name, InvList(it.second));
  }
}

std::string InvSet::as_json() {
  std::ostringstream out;
  
  out << "{";
  
  bool first = true;
  for(auto it : inventory) {
    if(!first)
      out << ",";
    first = false;
    out << "\"" << json_escape(it.first) << "\":" << it.second.as_json();
  }
  
  out << "}";
  
  return out.str();
}

InvList& InvSet::get(std::string list_name) {
  auto search = inventory.find(list_name);
  if(search == inventory.end())
    return nil_list;
  return search->second;
}
void InvSet::add(std::string list_name, InvList list) {
  inventory[list_name] = list;
}
bool InvSet::has_list(std::string list_name) {
  auto search = inventory.find(list_name);
  if(search == inventory.end())
    return false;
  return true;
}

InvStack InvSet::get_at(InvRef ref) {
  InvList& list = get(ref.list_name);
  if(list.is_nil)
    return InvStack();
  
  return list.get_at(ref.index);
}
bool InvSet::set_at(InvRef ref, InvStack stack) {
  InvList& list = get(ref.list_name);
  if(list.is_nil)
    return false;
  
  return list.set_at(ref.index, stack);
}