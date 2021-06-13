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

#include "server.h"

#include "log.h"
#include "config.h"
#include "player_util.h"

void Server::tick(const boost::system::error_code&) {
  std::unique_lock<std::shared_mutex> tick_info_l(tick_info_lock);
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  
  std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
  int diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick).count(); //milliseconds
  last_tick = now;
  
  if(diff > SERVER_TICK_INTERVAL * 2 || diff < SERVER_TICK_INTERVAL / 2) {
    log(LogSource::SERVER, LogLevel::WARNING, "Tick took " + std::to_string(diff) + " ms (expected " + std::to_string(SERVER_TICK_INTERVAL) + " ms)!");
  }
  
  mapblock_tick_counter++;
  fluid_tick_counter++;
  if(mapblock_tick_counter >= SERVER_MAPBLOCK_TICK_RATIO) {
    std::set<MapPos<int>> interested_mapblocks;
    
    for(auto p : m_players) {
      PlayerState *player = p.second;
      std::shared_lock<std::shared_mutex> player_lock(player->lock);
      
      if(!player->auth && !player->auth_guest) { continue; }
      
      std::vector<MapPos<int>> nearby_known_mapblocks_1 = player->list_nearby_known_mapblocks(PLAYER_MAPBLOCK_INTEREST_DISTANCE, PLAYER_MAPBLOCK_INTEREST_DISTANCE_W);
      std::vector<MapPos<int>> nearby_known_mapblocks_2 = player->list_nearby_known_mapblocks(PLAYER_MAPBLOCK_INTEREST_DISTANCE_SMALL, PLAYER_MAPBLOCK_INTEREST_DISTANCE_SMALL_W);
      
      std::set<MapPos<int>> nearby_known_mapblocks_set(nearby_known_mapblocks_1.begin(), nearby_known_mapblocks_1.end());
      nearby_known_mapblocks_set.insert(nearby_known_mapblocks_2.begin(), nearby_known_mapblocks_2.end());
      
      std::vector<MapPos<int>> nearby_known_mapblocks(nearby_known_mapblocks_set.begin(), nearby_known_mapblocks_set.end());
      
      player->update_nearby_known_mapblocks(nearby_known_mapblocks, map);
      
      for(size_t i = 0; i < nearby_known_mapblocks.size(); i++) {
        interested_mapblocks.insert(nearby_known_mapblocks[i]);
      }
    }
    
    if(fluid_tick_counter >= SERVER_FLUID_TICK_RATIO) {
#ifdef DEBUG_PERF
      auto start = std::chrono::steady_clock::now();
#endif
      
      map.tick_fluids(interested_mapblocks);
      
#ifdef DEBUG_PERF
      auto end = std::chrono::steady_clock::now();
      auto diff = end - start;
      
      std::cout << "tick_fluids in " << std::chrono::duration<double, std::milli>(diff).count() << " ms" << std::endl;
#endif
      fluid_tick_counter = 0;
    }
    
    mapblock_tick_counter = 0;
  }
  
  for(auto p : m_players) {
    PlayerState *player = p.second;
    std::shared_lock<std::shared_mutex> player_lock(player->lock);
    
    if(!player->auth && !player->auth_guest) { continue; }
    
    std::ostringstream out;
    out << "{\"type\":\"update_entities\",\"actions\":[";
    
    bool first = true;
    for(auto p_sub : m_players) {
      PlayerState *candidate = p_sub.second;
      if(candidate == player) { continue; }
      
      std::shared_lock<std::shared_mutex> candidate_lock(candidate->lock);
      
      std::string candidate_tag = candidate->get_tag();
      
      double distance = player->pos.distance_to(candidate->pos);
      if(distance <= PLAYER_ENTITY_VISIBILE_DISTANCE) {
        //Player should know about candidate entity
        if(player->known_player_tags.find(candidate_tag) == player->known_player_tags.end()) {
          //...but they don't
          //so create it
          
          if(!first) { out << ","; }
          first = false;
          out << "{\"type\":\"create\",\"data\":" << candidate->entity_data_as_json() << "}";
          
          player->known_player_tags.insert(candidate_tag);
        } else {
          //update it
          
          if(!first) { out << ","; }
          first = false;
          out << "{\"type\":\"update\",\"data\":" << candidate->entity_data_as_json() << "}";
        }
      } else {
        //Player should *not* know about candidate entity
        if(player->known_player_tags.find(candidate_tag) != player->known_player_tags.end()) {
          //...but they do
          //so delete it
          
          if(!first) { out << ","; }
          first = false;
          out << "{\"type\":\"delete\",\"data\":" << candidate->entity_data_as_json() << "}";
          
          player->known_player_tags.erase(candidate_tag);
        }
      }
    }
    
    out << "]}";
    
    player->send(out.str());
  }
  
  slow_tick_counter++;
  if(slow_tick_counter >= SERVER_SLOW_TICK_RATIO) {
    slow_tick();
    slow_tick_counter = 0;
  }
  
  interact_tick_counter++;
  if(interact_tick_counter >= SERVER_INTERACT_TICK_RATIO) {
    interact_tick();
    interact_tick_counter = 0;
  }
  
#ifdef DEBUG_NET
  //FIXME doesn't include *all* mapblocks sent
  {
    std::shared_lock<std::shared_mutex> net_lock(net_debug_lock);
    if(mb_out_count > 0) {
      std::cout << std::to_string(mb_out_len / mb_out_count) << " avg mb out bytes, " << std::to_string(mb_out_len / 1048576.0) << " MiB total" << std::endl;
    }
  }
#endif
  
  if(!halt) {
    m_timer.expires_at(m_timer.expiry() + boost::asio::chrono::milliseconds(SERVER_TICK_INTERVAL));
    m_timer.async_wait(boost::bind(&Server::tick, this, boost::asio::placeholders::error));
  }
}

void Server::slow_tick() {
  //db.clean_cache();
}



void Server::wake_interact_tick(MapPos<int> node_pos) {
  std::unique_lock<std::shared_mutex> list_lock(active_interact_tick_lock);
  Node node = map.get_node(node_pos);
  
  if(node.itemstring == "default:furnace" || node.itemstring == "default:furnace_active") {
    active_interact_tick.insert(node_pos);
    return;
  }
  
  active_interact_tick.erase(node_pos);
}

void Server::interact_tick() {
  std::unique_lock<std::shared_mutex> list_lock(active_interact_tick_lock);
  
  auto it = active_interact_tick.begin();
  while(it != active_interact_tick.end()) {
    auto current = it++;
    MapPos<int> node_pos = *current;
    Node node = map.get_node(node_pos);
    
    if(node.itemstring == "default:furnace" || node.itemstring == "default:furnace_active") {
      NodeMeta *meta = db.get_node_meta(node_pos);
      bool meta_is_nil = meta->is_nil;
      int f_active_fuel = 0;
      if(meta->int1)
        f_active_fuel = *meta->int1;
      int f_cook_progress = 0;
      if(meta->int2)
        f_cook_progress = *meta->int2;
      delete meta;
      if(meta_is_nil) {
        active_interact_tick.erase(current);
        continue;
      }
      
      bool do_update_meta = false;
      
      InvRef f_ref_in("node", node_pos.to_json(), "furnace_in", 0);
      InvRef f_ref_fuel("node", node_pos.to_json(), "furnace_fuel", 0);
      InvRef f_ref_out("node", node_pos.to_json(), "furnace_out", 0);
      
      InvStack f_in = get_invlist(f_ref_in, NULL).get_at(f_ref_in);
      InvStack f_fuel = get_invlist(f_ref_fuel, NULL).get_at(f_ref_fuel);
      InvStack f_out = get_invlist(f_ref_out, NULL).get_at(f_ref_out);
      
      // If we've been consuming fuel, increment cook progress
      if(f_active_fuel > 0) {
        if(f_cook_progress + 1 > f_cook_progress) // prevent integer oveflow
          f_cook_progress++;
        do_update_meta = true;
      }
      
      // Consume fuel.
      if(f_active_fuel > 0) {
        f_active_fuel--;
        do_update_meta = true;
      }
      
      if(f_active_fuel == 0) {
        ItemDef fuel_def = get_item_def(f_fuel.itemstring);
        if(fuel_def.fuel > 0) {
          InvStack f_fuel_result(f_fuel);
          f_fuel_result.count--;
          if(f_fuel_result.count == 0)
            f_fuel_result = InvStack();
          
          InvPatch fuel_consume;
          fuel_consume.diffs.push_back(
              InvDiff(f_ref_fuel, f_fuel, f_fuel_result));
          
          bool res = inv_apply_patch(fuel_consume);
          if(res) {
            f_active_fuel += fuel_def.fuel;
            do_update_meta = true;
          }
        }
      }
      
      std::optional<std::pair<InvStack, int>> cook_result
          = cook_calc_result(f_in);
      
      bool can_cook = false;
      InvStack cook_res_stack;
      int cook_time = 0;
      if(cook_result != std::nullopt) {
        cook_res_stack = (*cook_result).first;
        cook_time = (*cook_result).second;
        
        if(f_out.is_nil) {
          can_cook = true;
        } else {
          can_cook = true;
          if(f_out.itemstring != cook_res_stack.itemstring)
            can_cook = false;
          
          ItemDef f_out_def = get_item_def(f_out.itemstring);
          if(f_out.count + cook_res_stack.count > f_out_def.max_stack)
            can_cook = false;
        }
      }
      
      if(!can_cook) {
        f_cook_progress = 0;
        do_update_meta = true;
      }
      
      if(can_cook && f_cook_progress >= cook_time) {
        InvStack result_f_in(f_in);
        result_f_in.count--;
        if(result_f_in.count == 0)
          result_f_in = InvStack();
        
        InvStack result_f_out(cook_res_stack);
        if(!f_out.is_nil) {
          result_f_out = f_out;
          result_f_out.count += cook_res_stack.count;
        }
        InvPatch p;
        p.diffs.push_back(
            InvDiff(f_ref_out, f_out, result_f_out));
        p.diffs.push_back(
            InvDiff(f_ref_in, f_in, result_f_in));
        bool res = inv_apply_patch(p, NULL);
        
        if(res) {
          f_cook_progress -= cook_time;
          do_update_meta = true;
        }
      }
      
      // If no fuel, stop the furnace.
      if(f_active_fuel <= 0) {
        active_interact_tick.erase(current);
        can_cook = false;
        f_cook_progress = 0;
        do_update_meta = true;
      }
      
      // Update stored metadata.
      if(do_update_meta) {
        db.lock_node_meta(node_pos);
        NodeMeta *meta = db.get_node_meta(node_pos);
        meta->int1 = f_active_fuel;
        meta->int2 = f_cook_progress;
        db.set_node_meta(node_pos, meta);
        db.unlock_node_meta(node_pos);
        
        std::ostringstream status;
        status << "Cooking: " << std::boolalpha << can_cook << "\n"
               << "Fuel: " << f_active_fuel << "\n"
               << "Progress: " << f_cook_progress << "/" << cook_time;
        std::string status_s(status.str());
        
        std::vector<UIInstance> relevant_ui
            = find_ui_multiple("furnace " + node_pos.to_json());
        
        for(auto inst : relevant_ui) {
          inst.spec.components[8] = UI_TextBlock(status_s).to_json();
          update_ui(inst);
        }
      }
      
      if(f_active_fuel > 0 && node.itemstring == "default:furnace") {
        Node new_node(node);
        new_node.itemstring = "default:furnace_active";
        map.set_node(node_pos, new_node, node);
      } else if(f_active_fuel == 0 && node.itemstring == "default:furnace_active") {
        Node new_node(node);
        new_node.itemstring = "default:furnace";
        map.set_node(node_pos, new_node, node);
      }
      
      continue;
    }
    
    active_interact_tick.erase(current);
  }
}
