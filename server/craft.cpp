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

#include "craft.h"
#include "log.h"

std::vector<CraftDef*> all_craft_defs;

void load_craft_defs(boost::property_tree::ptree pt) {
  size_t count = 0;
  
  for(const auto& n : pt.get_child("craftDefs")) {
    CraftDef *def = new CraftDef();
    
    def->result = InvStack(n.second.get_child("res"));
    
    def->ingredients.is_nil = false;
    for(const auto& list_item : n.second.get_child("list"))
      def->ingredients.list.push_back(InvStack(list_item.second));
    
    const auto& shape = n.second.get_child("shape");
    if(shape.data().empty())
      def->shape = std::make_pair(shape.get<int>("x"), shape.get<int>("y"));
    else
      def->shape = std::nullopt;
    
    def->type = n.second.get<std::string>("type");
    
    if(n.second.get<std::string>("cookTime") == "null")
      def->cook_time = std::nullopt;
    else
      def->cook_time = n.second.get<int>("cookTime");
    
    all_craft_defs.push_back(def);
    
    count++;
  }
  
  log(LogSource::LOADER, LogLevel::INFO, std::string("Loaded " + std::to_string(count) + " craft definitions"));
}

std::optional<std::pair<InvPatch, InvPatch>> craft_calc_result(const CraftDef& craft, const InvList& craft_input, std::pair<int, int> craft_input_shape) {
  InvPatch result_patch;
  result_patch.diffs.push_back(InvDiff(
    InvRef("player", std::nullopt, "craftOutput", 0),
    InvStack(),
    craft.result
  ));
  
  if(craft.shape) {
    int craft_shape_x = (*craft.shape).first;
    int craft_shape_y = (*craft.shape).second;
    int input_shape_x = craft_input_shape.first;
    int input_shape_y = craft_input_shape.second;
    
    if(craft_shape_x > input_shape_x || craft_shape_y > input_shape_y)
      return std::nullopt;
    
    for(int xoff = 0; xoff <= (input_shape_x - craft_shape_x); xoff++) {
      for(int yoff = 0; yoff <= (input_shape_y - craft_shape_y); yoff++) {
        bool fit = true;
        InvPatch out_patch;
        
        for(int x = 0; x < input_shape_x; x++) {
          for(int y = 0; y < input_shape_y; y++) {
            if(
              x < xoff || y < yoff ||
              x >= (xoff + craft_shape_x) || y >= (yoff + craft_shape_y)
            ) {
              //out of bounds of the recipie, should be empty
              if(craft_input.get_at(y * input_shape_x + x).is_nil)
                continue;
              fit = false;
              break;
            }
            
            int recipie_index = (y - yoff) * craft_shape_x + (x - xoff);
            int input_index = y * input_shape_x + x;
            const InvStack& recipie_stack
                = craft.ingredients.get_at(recipie_index);
            const InvStack& input_stack
                = craft_input.get_at(input_index);
            
            if(recipie_stack.is_nil && input_stack.is_nil)
              continue;
            if(recipie_stack.is_nil || input_stack.is_nil) {
              fit = false;
              break;
            }
            
            //match exact itemstring or 'group:'
            if(recipie_stack.itemstring == input_stack.itemstring) {
              //ok
            } else if(recipie_stack.itemstring.rfind("group:", 0) == 0) {
              //recipie is looking for a group
              std::string need_group = recipie_stack.itemstring.substr(6);
              ItemDef input_def = get_item_def(input_stack.itemstring);
              if(input_def.groups.find(need_group) == input_def.groups.end()) {
                fit = false;
                break;
              }
            } else {
              fit = false;
              break;
            }
            
            if(input_stack.count < recipie_stack.count) {
              fit = false;
              break;
            }
            
            InvStack consumed_stack(input_stack);
            consumed_stack.count -= recipie_stack.count;
            if(consumed_stack.count == 0)
              consumed_stack = InvStack();
            
            out_patch.diffs.push_back(InvDiff(
              InvRef("player", std::nullopt, "craft", input_index),
              input_stack,
              consumed_stack
            ));
          }
          if(!fit)
            break;
        }
        
        if(fit)
          return std::make_pair(out_patch, result_patch);
      }
    }
    
    return std::nullopt;
  } else {
    //shapeless recipie
    InvPatch out_patch;
    
    std::vector<bool> used(craft_input.list.size(), false);
    
    for(const auto& needed : craft.ingredients.list) {
      if(needed.is_nil)
        continue;
      
      bool found = false;
      for(size_t n = 0; n < craft_input.list.size(); n++) {
        const InvStack& input_stack = craft_input.get_at(n);
        if(input_stack.is_nil)
          continue;
        if(used[n])
          continue;
        
        //match exact itemstring or 'group:'
        if(needed.itemstring == input_stack.itemstring) {
          //ok
        } else if(needed.itemstring.rfind("group:", 0) == 0) {
          //recipie is looking for a group
          std::string need_group = needed.itemstring.substr(6);
          ItemDef input_def = get_item_def(input_stack.itemstring);
          if(input_def.groups.find(need_group) == input_def.groups.end())
            continue;
        } else {
          continue;
        }
        
        if(input_stack.count < needed.count)
          continue;
        
        found = true;
        used[n] = true;
        
        InvStack consumed_stack(input_stack);
        consumed_stack.count -= needed.count;
        if(consumed_stack.count == 0)
          consumed_stack = InvStack();
        
        out_patch.diffs.push_back(InvDiff(
          InvRef("player", std::nullopt, "craft", n),
          input_stack,
          consumed_stack
        ));
        
        break;
      }
      
      if(!found)
        return std::nullopt;
    }
    
    //check for extra items
    for(size_t i = 0; i < craft_input.list.size(); i++) {
      if(!used[i] && !craft_input.get_at(i).is_nil)
        return std::nullopt;
    }
    return std::make_pair(out_patch, result_patch);
  }
}

std::optional<std::pair<InvPatch, InvPatch>> craft_calc_result(const InvList& craft_input, std::pair<int, int> craft_input_shape) {
  for(const auto& craft : all_craft_defs) {
    std::optional<std::pair<InvPatch, InvPatch>> result = craft_calc_result(*craft, craft_input, craft_input_shape);
    if(result)
      return result;
  }
  
  return std::nullopt;
}



std::optional<std::pair<InvStack, int>> cook_calc_result(const InvStack& source_stack) {
  for(const auto& craft : all_craft_defs) {
    if(craft->type != "cook")
      continue;
    if(craft->ingredients.get_at(0).itemstring != source_stack.itemstring)
      continue;
    
    int cook_time = 1;
    if(craft->cook_time)
      cook_time = *craft->cook_time;
    
    return std::make_pair(craft->result, cook_time);
  }
  return std::nullopt;
}
