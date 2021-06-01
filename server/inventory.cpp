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
#include "log.h"
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
bool InvRef::operator==(const InvRef& other) const {
  return obj_type == other.obj_type && obj_id == other.obj_id
         && list_name == other.list_name && index == other.index;
}

std::string InvRef::as_json() const {
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
std::string InvStack::as_json() const {
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
  } else {
    ItemDef def = get_item_def(itemstring);
    if(def.is_tool)
      wear = def.tool_wear;
  }
  
  //TODO use the remaining string as 'data' if present
}

InvStack::InvStack(ItemDef def) : itemstring(def.itemstring), count(1), wear(std::nullopt), data(std::nullopt), is_nil(false) {
  if(def.is_tool)
    wear = def.tool_wear;
}

bool InvStack::operator==(const InvStack& other) const {
  if(is_nil && other.is_nil)
    return true;
  if(is_nil || other.is_nil)
    return false;
  return itemstring == other.itemstring && count == other.count
         && wear == other.wear && data == other.data;
}
bool InvStack::operator!=(const InvStack& other) const {
  return !operator==(other);
}

std::string InvDiff::as_json() const {
  std::ostringstream out;
  out << "{\"ref\":" << ref.as_json() << ",\"prev\":" << prev.as_json() << ",\"current\":" << current.as_json() << "}";
  return out.str();
}

std::string InvPatch::as_json(std::string type) const {
  std::ostringstream out;
  
  out << "{\"type\":\"" << json_escape(type) << "\",";
  if(req_id)
    out << "\"reqID\":\"" << json_escape(*req_id) << "\",";
  
  out << "\"diffs\":[";
  
  bool first = true;
  for(const auto& it : diffs) {
    if(!first)
      out << ",";
    first = false;
    out << it.as_json();
  }
  
  out << "]}";
  
  return out.str();
}

void InvPatch::make_deny() {
  for(auto& it : diffs) {
    it.current = it.prev;
  }
  is_deny = true;
}

InvPatch InvPatch::operator+(const InvPatch& other) const {
  InvPatch result(*this);
  for(const auto& diff : other.diffs) {
    result.diffs.push_back(diff);
  }
  return result;
}

InvList::InvList(boost::property_tree::ptree pt) : is_nil(false) {
  for(auto it : pt) {
    list.push_back(InvStack(it.second));
  }
}
std::string InvList::as_json() const {
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

InvStack InvList::get_at(int index) const {
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

bool InvList::is_empty() {
  for(auto it : list) {
    if(!it.is_nil)
      return false;
  }
  return true;
}

InvSet::InvSet(boost::property_tree::ptree pt) {
  for(auto it : pt) {
    std::string list_name = it.first;
    add(list_name, InvList(it.second));
  }
}

std::string InvSet::as_json(std::set<std::string> exclude_lists) const {
  std::ostringstream out;
  
  out << "{";
  
  bool first = true;
  for(auto it : inventory) {
    if(exclude_lists.find(it.first) != exclude_lists.end())
      continue;
    
    if(!first)
      out << ",";
    first = false;
    out << "\"" << json_escape(it.first) << "\":" << it.second.as_json();
  }
  
  out << "}";
  
  return out.str();
}

std::string InvSet::as_json() const {
  return as_json(std::set<std::string>());
}

InvList& InvSet::get(std::string list_name) {
  auto search = inventory.find(list_name);
  if(search == inventory.end())
    return nil_list;
  return search->second;
}
bool InvSet::set(std::string list_name, InvList list) {
  auto search = inventory.find(list_name);
  if(search == inventory.end())
    return false;
  
  inventory[list_name] = list;
  return true;
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

bool InvSet::is_empty() {
  for(auto it : inventory) {
    if(!it.second.is_empty())
      return false;
  }
  return true;
}



std::optional<InvPatch> inv_calc_give(const InvRef& list_ref, const InvList& list, InvStack stack) {
  InvPatch result_patch;
  
  if(stack.is_nil)
    return result_patch;
  
  ItemDef def = get_item_def(stack.itemstring);
  if(def.itemstring == "nothing") {
    log(LogSource::INVENTORY, LogLevel::WARNING, "cannot calculate give for '" + stack.itemstring + "': item does not exist");
    return std::nullopt;
  }
  
  int max_stack = def.max_stack;
  int needed = stack.count;
  for(size_t i = 0; i < list.list.size(); i++) {
    const InvStack& s = list.list[i];
    
    if(s.is_nil)
      continue;
    if(s.itemstring != stack.itemstring)
      continue;
    int avail = max_stack - s.count;
    if(avail <= 0)
      continue;
    
    InvStack new_stack(s);
    new_stack.count += std::min(avail, needed);
    needed -= std::min(avail, needed);
    
    InvRef stack_ref(list_ref);
    stack_ref.index = i;
    result_patch.diffs.push_back(
        InvDiff(stack_ref, s, new_stack));
    
    if(needed <= 0)
      break;
  }
  
  if(needed > 0) {
    stack.count = needed;
    
    for(size_t i = 0; i < list.list.size(); i++) {
      const InvStack& s = list.list[i];
      if(!s.is_nil)
        continue;
      
      //put stack in empty slot
      needed -= stack.count;
      
      InvRef stack_ref(list_ref);
      stack_ref.index = i;
      result_patch.diffs.push_back(
          InvDiff(stack_ref, s, stack));
      
      break;
    }
  }
  
  if(needed > 0)
    return std::nullopt;
  
  return result_patch;
}

std::optional<InvPatch> inv_calc_take_at(const InvRef& stack_ref, const InvList& list, InvStack to_take) {
  InvPatch result_patch;
  
  if(to_take.is_nil)
    return result_patch;
  
  const InvStack& orig_stack = list.get_at(stack_ref.index);
  if(orig_stack.is_nil)
    return std::nullopt;
  if(orig_stack.itemstring != to_take.itemstring)
    return std::nullopt;
  if(to_take.count > orig_stack.count)
    return std::nullopt;
  
  InvStack new_stack(orig_stack);
  new_stack.count -= to_take.count;
  if(new_stack.count <= 0)
    new_stack = InvStack();
  
  result_patch.diffs.push_back(
      InvDiff(stack_ref, orig_stack, new_stack));
  
  return result_patch;
}
