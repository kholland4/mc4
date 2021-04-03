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

void Server::on_message(connection_hdl hdl, websocketpp::config::asio::message_type::ptr msg) {
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  auto search = m_players.find(hdl);
  if(search == m_players.end()) {
    log(LogSource::SERVER, LogLevel::ERR, "Unable to find player state for connection!");
    return;
  }
  PlayerState *player = search->second;
  
  std::unique_lock<std::shared_mutex> player_lock_unique(player->lock);
  
  try {
    std::stringstream ss;
    ss << msg->get_payload();
    
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss, pt);
    
    std::string type = pt.get<std::string>("type");
    
    if(!player->auth && !player->auth_guest) {
      //TODO send reply saying unauthorized (if not an auth message)
      
      if(type == "auth_guest") {
        PlayerData new_data;
        new_data.name = player->get_tag();
        new_data.is_nil = false;
        init_player_data(new_data);
        player->load_data(new_data);
        
        player->auth_guest = true;
        
        m_server.send(hdl, "{\"type\":\"auth_guest\",\"message\":\"guest_ok\"}", websocketpp::frame::opcode::text);
      } else {
        //Authenticate to an account.
        
        bool is_auth = player->auth_state.step(msg->get_payload(), m_server, hdl, db);
        if(is_auth) {
          std::string auth_id = player->auth_state.result();
          
          for(auto it : m_players) {
            PlayerState *p = it.second;
            if(p->auth && p->data.auth_id == auth_id) {
              player_lock_unique.unlock();
              chat_send_player(player, "server", "ERROR: player '" + p->data.name + "' is already connected");
              websocketpp::lib::error_code ec;
              m_server.close(hdl, websocketpp::close::status::policy_violation, "kick", ec);
              if(ec) {
                log(LogSource::SERVER, LogLevel::ERR, "error closing connection: " + ec.message());
              }
              return;
            }
          }
          
          PlayerData pdata = db.fetch_player_data(auth_id);
          if(pdata.is_nil) {
            player_lock_unique.unlock();
            chat_send_player(player, "server", "internal error: no such player");
            log(LogSource::SERVER, LogLevel::ERR, "no such player (auth id " + auth_id + ")");
            return;
          }
          player->load_data(pdata);
          player->auth = true;
        }
      }
      
      if(player->auth || player->auth_guest) {
        player->send_pos(m_server);
        player->send_privs(m_server);
        
        player->prepare_nearby_mapblocks(2, 3, 0, map);
        player->prepare_nearby_mapblocks(1, 2, 1, map);
        
        player_lock_unique.unlock();
        
        log(LogSource::SERVER, LogLevel::INFO, player->get_name() + " (from " + player->address_and_port + ") connected!");
        chat_send_player(player, "server", status());
        chat_send("server", "*** " + player->get_name() + " joined the server!");
      }
      
      return;
    }
    
    //message starts with "auth"
    if(type.rfind("auth", 0) == 0 && player->auth) {
      bool is_auth = player->auth_state.step(msg->get_payload(), m_server, hdl, db);
      if(!is_auth) {
        db.update_player_data(player->get_data());
        player->auth = false;
      }
      
      return;
    }
    
    if(type == "req_mapblock") {
      MapPos<int> mb_pos(pt.get<int>("pos.x"), pt.get<int>("pos.y"), pt.get<int>("pos.z"), pt.get<int>("pos.w"), player->pos.world, player->pos.universe);
      
      MapPos<int> player_mb = player->containing_mapblock();
      MapBox<int> bounding(player_mb - PLAYER_LIMIT_VIEW_DISTANCE, player_mb + PLAYER_LIMIT_VIEW_DISTANCE);
      if(!bounding.contains(mb_pos)) {
        log(LogSource::SERVER, LogLevel::NOTICE, "Player '" + player->get_name() + "' requests out of bounds mapblock at " + mb_pos.to_string());
        return;
      }
      
      MapblockUpdateInfo info = map.get_mapblockupdateinfo(mb_pos);
      if(info.light_needs_update > 0) {
#ifdef DEBUG_PERF
        std::cout << "single mapblock prep for " << player->get_name() << std::endl;
#endif
        map.update_mapblock_light(info);
      }
      MapblockCompressed *mbc = map.get_mapblock_compressed(mb_pos);
#ifdef DEBUG_NET
      unsigned int len = player->send_mapblock_compressed(mbc, m_server);
      
      {
        std::unique_lock<std::shared_mutex> net_lock(net_debug_lock);
        mb_out_len += len;
        mb_out_count++;
      }
#else
      player->send_mapblock_compressed(mbc, m_server);
#endif
      delete mbc;
    } else if(type == "set_player_pos") {
      if(player->just_tp) {
        //Clear position history and ignore this position update.
        player->pos_history.clear();
        player->just_tp = false;
        return;
      }
      
      MapPos<double> pos(pt.get<double>("pos.x"), pt.get<double>("pos.y"), pt.get<double>("pos.z"), pt.get<int>("pos.w"), player->pos.world, player->pos.universe);
      
      std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
      
#ifndef DISABLE_PLAYER_SPEED_LIMIT
      //Validate that the player hasn't moved too far since their last position update
      //FIXME: doesn't handle falling players
      bool violation = false;
      
      if(player->pos_history.size() > 0) {
        MapPos<double> total_distance(0, 0, 0, 0, 0, 0);
        MapPos<double> total_distance_sign(0, 0, 0, 0, 0, 0);
        MapPos<double> previous = pos;
        for(auto it : player->pos_history) {
          total_distance += (previous - it.second).abs();
          total_distance_sign += (previous - it.second);
          previous = it.second;
        }
        
        std::chrono::time_point<std::chrono::steady_clock> oldest = player->pos_history.back().first;
        double diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - oldest).count() / 10000.0; //milliseconds -> tens of seconds
        if(diff > 0) {
          MapPos<double> vel(total_distance.x / diff, total_distance.y / diff, total_distance.z / diff,
                             total_distance.w / diff, total_distance.world / diff, total_distance.universe / diff);
          MapPos<double> vel_sign(total_distance_sign.x / diff, total_distance_sign.y / diff, total_distance_sign.z / diff,
                                  total_distance_sign.w / diff, total_distance_sign.world / diff, total_distance_sign.universe / diff);
          
          MapPos<int> limit = PLAYER_LIMIT_VEL;
          if(player->data.has_priv("fast")) {
            limit = PLAYER_LIMIT_VEL_FAST;
          }
          
          if(vel_sign.y < -limit.y) {
            //falling player, probably
            //FIXME
            limit.y *= 5;
          }
          
          if(vel.x > limit.x || vel.y > limit.y || vel.z > limit.z ||
             vel.w > limit.w || vel.world > limit.world || vel.universe > limit.universe) {
            violation = true;
          }
        }
      }
      
      if(violation) {
        log(LogSource::SERVER, LogLevel::WARNING, "player '" + player->get_name() + "' tried to move too fast");
        player->send_pos(m_server); //pull them back
        return; //don't accept the new position
      }
#endif
      
      player->pos_history.push_front(std::make_pair(now, pos));
      if(player->pos_history.size() > 8) {
        player->pos_history.pop_back();
      }
      
      player->pos = pos;
      player->vel.set(pt.get<double>("vel.x"), pt.get<double>("vel.y"), pt.get<double>("vel.z"), player->vel.w, player->vel.world, player->vel.universe);
      player->rot.set(pt.get<double>("rot.x"), pt.get<double>("rot.y"), pt.get<double>("rot.z"), pt.get<double>("rot.w"));
      
      if(player->auth) {
        db.update_player_data(player->get_data());
      }
      
      player->prepare_nearby_mapblocks(2, 3, 0, map);
      player->prepare_nearby_mapblocks(1, 2, 1, map);
    } else if(type == "set_node") {
      MapPos<int> pos(pt.get<int>("pos.x"), pt.get<int>("pos.y"), pt.get<int>("pos.z"), pt.get<int>("pos.w"), pt.get<int>("pos.world"), pt.get<int>("pos.universe"));
      Node node(pt.get<std::string>("data.itemstring"), pt.get<unsigned int>("data.rot"));
      
      MapPos<int> player_pos_int((int)std::round(player->pos.x), (int)std::round(player->pos.y), (int)std::round(player->pos.z), player->pos.w, player->pos.world, player->pos.universe);
      MapBox<int> bounding(player_pos_int - PLAYER_LIMIT_REACH_DISTANCE, player_pos_int + PLAYER_LIMIT_REACH_DISTANCE);
      if(!bounding.contains(pos)) {
        log(LogSource::SERVER, LogLevel::NOTICE, "Player '" + player->get_name() + "' at " + player_pos_int.to_string()
                                                 + " attempted to set node '" + node.itemstring + "' far away at " + pos.to_string());
        return;
      }
      
      if(get_node_def(node.itemstring).itemstring == "nothing") {
        log(LogSource::SERVER, LogLevel::NOTICE, "Player '" + player->get_name() + "' attempted to place nonexistent node '" + node.itemstring + "' at " + pos.to_string());
        return;
      }
      
      Node expect("", 0);
      
      log(LogSource::SERVER, LogLevel::EXTRA, "Player '" + player->get_name() + "' places '" + node.itemstring + "' at " + pos.to_string());
      
#ifdef DEBUG_PERF
      auto start = std::chrono::steady_clock::now();
#endif
      
      map.set_node(pos, node, expect);
      
#ifdef DEBUG_PERF
      auto end = std::chrono::steady_clock::now();
      auto diff = end - start;
      
      std::cout << "set_node in " << std::chrono::duration<double, std::milli>(diff).count() << " ms" << std::endl;
#endif
      
      /*MapPos<int> mb_pos = global_to_mapblock(pos);
      MapPos<int> min_pos = mb_pos - MapPos<int>(1, 1, 1, 0, 0, 0);
      MapPos<int> max_pos = mb_pos + MapPos<int>(1, 1, 1, 0, 0, 0);
      std::vector<MapPos<int>> mapblock_list;
      mapblock_list.push_back(mb_pos); //Send the main affected mapblock first.
      for(int universe = min_pos.universe; universe <= max_pos.universe; universe++) {
        for(int world = min_pos.world; world <= max_pos.world; world++) {
          for(int w = min_pos.w; w <= max_pos.w; w++) {
            for(int x = min_pos.x; x <= max_pos.x; x++) {
              for(int y = min_pos.y; y <= max_pos.y; y++) {
                for(int z = min_pos.z; z <= max_pos.z; z++) {
                  MapPos<int> rel_mb_pos(x, y, z, w, world, universe);
                  if(rel_mb_pos == mb_pos) { continue; }
                  mapblock_list.push_back(rel_mb_pos);
                }
              }
            }
          }
        }
      }
      player->update_mapblocks(mapblock_list, map, m_server);*/
    } else if(type == "send_chat") {
      std::string from = player->get_name();
      std::string channel = pt.get<std::string>("channel");
      std::string message = pt.get<std::string>("message");
      
      //TODO: validate channel, access control, etc.
      //TODO: any other anti-abuse validation (player name and channel character set restrictions, length limits, etc.)
      
      player_lock_unique.unlock();
      chat_send(channel, from, message);
      return;
    } else if(type == "chat_command") {
      std::string command = pt.get<std::string>("command");
      
      std::vector<std::string> tokens;
      boost::char_separator<char> sep(" ", "");
      boost::tokenizer<boost::char_separator<char>> tokenizer(command, sep);
      for(boost::tokenizer<boost::char_separator<char>>::iterator it = tokenizer.begin(); it != tokenizer.end(); ++it) {
        tokens.push_back(*it);
      }
      
      if(tokens.size() < 1) {
        player_lock_unique.unlock();
        chat_send_player(player, "server", "invalid command: empty input");
        return;
      }
      
      std::string command_name = tokens[0];
      
      player_lock_unique.unlock();
      
      //TODO table pointing to these functions
      if(tokens[0] == "/nick") {
        cmd_nick(player, tokens);
      } else if(tokens[0] == "/status") {
        cmd_status(player, tokens);
      } else if(tokens[0] == "/time") {
        cmd_time(player, tokens);
      } else if(tokens[0] == "/whereami") {
        cmd_whereami(player, tokens);
      } else if(tokens[0] == "/tp") {
        cmd_tp(player, tokens);
      } else if(tokens[0] == "/world") {
        cmd_tp_world(player, tokens);
      } else if(tokens[0] == "/universe") {
        cmd_tp_universe(player, tokens);
      } else if(tokens[0] == "/grantme") {
        cmd_grantme(player, tokens);
      } else if(tokens[0] == "/privs") {
        cmd_privs(player, tokens);
      } else {
        chat_send_player(player, "server", "unknown command");
        return;
      }
    }
  } catch(boost::property_tree::ptree_error const& e) {
    log(LogSource::SERVER, LogLevel::ERR, "JSON parse error: " + std::string(e.what()) + " payload=" + msg->get_payload());
  }
}