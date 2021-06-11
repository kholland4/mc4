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
