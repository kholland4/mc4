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

#ifndef __INVENTORY_H__
#define __INVENTORY_H__

#include "item.h"

#include <string>
#include <map>
#include <set>
#include <vector>
#include <optional>

#include <boost/property_tree/ptree.hpp>

class InvStack {
  public:
    InvStack() : is_nil(true) {}
    InvStack(std::string _itemstring, int _count, std::optional<int> _wear, std::optional<std::string> _data) :
        itemstring(_itemstring), count(_count), wear(_wear), data(_data), is_nil(false) {}
    InvStack(std::string spec);
    InvStack(ItemDef def);
    InvStack(boost::property_tree::ptree pt);
    
    bool operator==(const InvStack& other) const;
    bool operator!=(const InvStack& other) const;
    
    std::string to_json() const;
    std::string spec();
    
    std::string itemstring;
    int count;
    std::optional<int> wear;
    std::optional<std::string> data;
    
    bool is_nil;
};

class InvRef {
  public:
    InvRef(
        std::string _obj_type,
        std::optional<std::string> _obj_id,
        std::string _list_name,
        std::optional<int> _index)
        : obj_type(_obj_type), obj_id(_obj_id), list_name(_list_name), index(_index) {};
    
    InvRef(boost::property_tree::ptree pt);
    
    bool operator<(const InvRef& other) const;
    bool operator==(const InvRef& other) const;
    std::string to_json() const;
    
    std::string obj_type;
    std::optional<std::string> obj_id;
    std::string list_name;
    std::optional<int> index;
};

class InvDiff {
  public:
    InvDiff(InvRef _ref, InvStack _prev, InvStack _current)
        : ref(_ref), prev(_prev), current(_current) {};
    
    std::string to_json() const;
    
    InvRef ref;
    InvStack prev;
    InvStack current;
};

class InvPatch {
  public:
    InvPatch()
        : req_id(std::nullopt), is_deny(false) {};
    InvPatch(std::optional<std::string> _req_id)
        : req_id(_req_id), is_deny(false) {};
    
    InvPatch operator+(const InvPatch& other) const;
    InvPatch& operator+=(const InvPatch& other);
    
    std::string to_json(std::string type) const;
    void make_deny();
    
    std::optional<std::string> req_id;
    std::vector<InvDiff> diffs;
    bool is_deny;
};

class InvList {
  public:
    InvList() : is_nil(true) {};
    InvList(int count) : list(count, InvStack()), is_nil(false) {};
    InvList(boost::property_tree::ptree pt);
    std::string to_json() const;
    
    InvStack get_at(int index) const;
    InvStack get_at(const InvRef& ref) const;
    bool set_at(int index, InvStack stack);
    bool set_at(const InvRef& ref, InvStack stack);
    
    bool is_empty();
    
    std::vector<InvStack> list;
    bool is_nil;
};

class InvSet {
  public:
    InvSet() {};
    InvSet(boost::property_tree::ptree pt);
    std::string to_json() const;
    std::string to_json(std::set<std::string> exclude_lists) const;
    
    InvList& get(std::string list_name);
    bool set(std::string list_name, InvList list);
    void add(std::string list_name, InvList list);
    bool has_list(std::string list_name);
    InvStack get_at(InvRef ref);
    bool set_at(InvRef ref, InvStack stack);
    
    bool is_empty();
    
    std::map<std::string, InvList> inventory;
    InvList nil_list;
};

std::optional<InvPatch> inv_calc_give(const InvRef& list_ref, const InvList& list, InvStack stack);
std::optional<InvPatch> inv_calc_take_at(const InvRef& stack_ref, const InvList& list, InvStack to_take);

#endif
