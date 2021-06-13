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

#ifndef __UI_H__
#define __UI_H__

#include "inventory.h"

#include <string>
#include <vector>
#include <functional>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

class UIComponent {
  public:
    virtual std::string to_json() const = 0;
};

class UI_InvList : public UIComponent {
  public:
    UI_InvList(InvRef _ref) : ref(_ref) {};
    virtual std::string to_json() const;
    
    InvRef ref;
};

class UI_Spacer : public UIComponent {
  public:
    virtual std::string to_json() const;
};

class UI_TextBlock : public UIComponent {
  public:
    UI_TextBlock(std::string _content) : content(_content) {};
    virtual std::string to_json() const;
    
    std::string content;
};

class UISpec {
  public:
    std::string components_to_json() const;
    
    std::vector<std::string> components;
};

class UIInstance {
  public:
    UIInstance(UISpec _spec) : is_nil(false), spec(_spec), id(boost::uuids::to_string(boost::uuids::random_generator()())) {};
    UIInstance(std::string _id) : is_nil(false), id(_id) {};
    UIInstance() : is_nil(true) {};
    
    bool operator<(const UIInstance& other) const;
    
    std::string ui_open_json() const;
    std::string ui_update_json() const;
    std::string ui_close_json() const;
    
    bool is_nil;
    UISpec spec;
    std::string id;
    std::string what_for;
    std::string player_tag;
    std::function<void()> open_callback;
    std::function<void()> update_callback;
    std::function<void()> close_callback;
};

#endif
