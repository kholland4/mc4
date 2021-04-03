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

#include <thread>


MapPos<int> PLAYER_LIMIT_VIEW_DISTANCE(3, 3, 3, 1, 0, 0);

//Client-side reach distance is 10, max player speed is 16 m/s (fast + sprint), and the client sends position updates every 0.25 seconds.
//Minimum server-side reach distance would then be 14, more added just in case.
MapPos<int> PLAYER_LIMIT_REACH_DISTANCE(20, 20, 20, 1, 0, 0);

//Measured in nodes per 10 seconds
#define DISABLE_PLAYER_SPEED_LIMIT //too problematic, disable for now
MapPos<int> PLAYER_LIMIT_VEL(120, 120, 120, 6, 0, 0);
MapPos<int> PLAYER_LIMIT_VEL_FAST(200, 200, 200, 6, 0, 0);


Server::Server(Database& _db, std::map<int, World*> _worlds)
    : m_timer(m_io, boost::asio::chrono::milliseconds(SERVER_TICK_INTERVAL)), db(_db), map(_db, _worlds, m_io),
      mapblock_tick_counter(0), fluid_tick_counter(0), slow_tick_counter(0),
      last_tick(std::chrono::steady_clock::now()),
      halt(false)
#ifdef DEBUG_NET
    , mb_out_count(0), mb_out_len(0)
#endif
{
  //disable logging
  m_server.clear_access_channels(websocketpp::log::alevel::all);
  //m_server.clear_error_channels(websocketpp::log::elevel::all);
  
  m_server.init_asio(&m_io);
  
  m_server.set_message_handler(
      websocketpp::lib::bind(&Server::on_message, this, ::_1, ::_2));
  m_server.set_open_handler(
      websocketpp::lib::bind(&Server::on_open, this, ::_1));
  m_server.set_close_handler(
      websocketpp::lib::bind(&Server::on_close, this, ::_1));
  m_server.set_http_handler(
      websocketpp::lib::bind(&Server::on_http, this, ::_1));
#ifdef TLS
  m_server.set_tls_init_handler(
      websocketpp::lib::bind(&Server::on_tls_init, this, ::_1));
#endif
}

void Server::run(uint16_t port) {
  m_timer.async_wait(boost::bind(&Server::tick, this, boost::asio::placeholders::error));
  
  m_server.listen(port);
  m_server.start_accept();
  
  log(LogSource::SERVER, LogLevel::INFO, "Server v" + std::string(VERSION) + " starting (port " + std::to_string(port) + ")");
  
  int thread_count = get_config<int>("server.threads");
  if(thread_count < 0) {
    thread_count = std::thread::hardware_concurrency();
  }
  if(thread_count > 256) {
    if(!get_config<bool>("server.many_threads")) {
      log(LogSource::SERVER, LogLevel::EMERG, "You're using an awful lot of threads (" + std::to_string(thread_count) + "). Please set server.many_threads=true to confirm.");
      exit(1);
    }
  }
  
  if(thread_count == 0) {
    log(LogSource::SERVER, LogLevel::INFO, "Single-threaded mode.");
    m_io.run();
  } else {
    log(LogSource::SERVER, LogLevel::INFO, "Using " + std::to_string(thread_count) + " threads.");
    
    std::vector<std::thread> threads;
    for(int i = 0; i < thread_count; i++) {
      boost::asio::io_context::count_type (boost::asio::io_context::*run) () = &boost::asio::io_context::run;
      std::thread thread(websocketpp::lib::bind(run, &m_io));
      threads.emplace_back(std::move(thread));
    }
    for(size_t i = 0; i < threads.size(); i++) {
      threads[i].join();
    }
  }
}

#ifdef TLS
websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> Server::on_tls_init(connection_hdl hdl) {
  websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> ctx
      = websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(websocketpp::lib::asio::ssl::context::sslv23);
  
  ctx->set_options(websocketpp::lib::asio::ssl::context::default_workarounds |
                   websocketpp::lib::asio::ssl::context::no_sslv2 |
                   websocketpp::lib::asio::ssl::context::no_sslv3 |
                   websocketpp::lib::asio::ssl::context::no_tlsv1 |
                   websocketpp::lib::asio::ssl::context::single_dh_use);
  
  std::string chain_file = get_config<std::string>("ssl.cert_chain_file");
  std::string private_key_file = get_config<std::string>("ssl.private_key_file");
  std::string dh_file = get_config<std::string>("ssl.dhparam_file");
  
  if(chain_file == "") {
    log(LogSource::SERVER, LogLevel::EMERG, "ssl.cert_chain_file not specified");
    exit(1);
  }
  if(private_key_file == "") {
    log(LogSource::SERVER, LogLevel::EMERG, "ssl.private_key_file not specified");
    exit(1);
  }
  if(dh_file == "") {
    log(LogSource::SERVER, LogLevel::EMERG, "ssl.dhparam_file not specified -- perhaps generate with `openssl dhparam -out dhparam.pem 4096` ?");
    exit(1);
  }
  
  ctx->use_certificate_chain_file(chain_file);
  ctx->use_private_key_file(private_key_file, websocketpp::lib::asio::ssl::context::pem);
  
  ctx->use_tmp_dh_file(dh_file);
  
  std::string ciphers("ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK");
  
  if(SSL_CTX_set_cipher_list(ctx->native_handle(), ciphers.c_str()) != 1) {
    log(LogSource::SERVER, LogLevel::EMERG, "Error setting SSL/TLS cipher list");
    exit(1);
  }
  
  return ctx;
}
#endif

void Server::terminate() {
  terminate(get_config<std::string>("server.default_terminate_message"));
}
void Server::terminate(std::string message) {
  //std::unique_lock<std::shared_mutex> list_lock(m_players_lock);
  log(LogSource::SERVER, LogLevel::NOTICE, "Server terminating: " + message);
  chat_send("server", "Server terminating: " + message);
  
  std::unique_lock<std::shared_mutex> tick_info_l(tick_info_lock);
  halt = true;
  tick_info_l.unlock();
  
  m_server.stop_listening();
  
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  for(auto p : m_players) {
    connection_hdl hdl = p.first;
    PlayerState *player = p.second;
    
    {
      std::unique_lock<std::shared_mutex> player_lock(player->lock);
      
      try {
        //on_close will be run later, so no worries about deadlocks
        m_server.close(hdl, websocketpp::close::status::going_away, message);
      } catch(websocketpp::exception const& e) {
        log(LogSource::SERVER, LogLevel::ERR, "Socket error: " + std::string(e.what()));
      }
    }
  }
  
  log(LogSource::SERVER, LogLevel::NOTICE, "Server terminated. Waiting for connections to finish closing.");
}

std::string Server::status() const {
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  
  std::string s = "-- Server v" + std::string(VERSION) + "; " + std::to_string(m_players.size()) + " players {";
  
  bool first = true;
  for(auto const& x : m_players) {
    if(!first) { s += ", "; }
    first = false;
    
    std::shared_lock<std::shared_mutex> player_lock(x.second->lock);
    s += x.second->get_name();
  }
  
  s += "}";
  
  std::shared_lock<std::shared_mutex> motd_l(motd_lock);
  if(motd != "") {
    s += "\n" + motd;
  }
  
  return s;
}

std::string Server::list_status_json() const {
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  
  std::ostringstream out;
  
  out << "{\"version\":\"" << json_escape(std::string(VERSION)) << "\","
      << "\"players\":" << m_players.size() << ",";
  
  int player_limit = get_config<int>("server.max_players");
  if(player_limit < 0) {
    out << "\"player_limit\":null";
  } else {
    out << "\"player_limit\":" << player_limit;
  }
  
  out << "}";
  
  return out.str();
}

void Server::chat_send(std::string channel, std::string from, std::string message) {
  log(LogSource::SERVER, LogLevel::INFO, "[#" + channel + "] <" + from + "> " + message);
  
  std::ostringstream broadcast;
  broadcast << "{\"type\":\"send_chat\","
            << "\"from\":\"" << json_escape(from) << "\","
            << "\"channel\":\"" << json_escape(channel) << "\","
            << "\"message\":\"" << json_escape(message) << "\"}";
  std::string broadcast_str = broadcast.str();
  
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  
  for(auto p : m_players) {
    PlayerState *receiver = p.second;
    
    std::shared_lock<std::shared_mutex> player_lock(receiver->lock);
    receiver->send(broadcast_str, m_server);
  }
}

void Server::chat_send(std::string channel, std::string message) {
  log(LogSource::SERVER, LogLevel::INFO, "[#" + channel + "] " + message);
  
  std::ostringstream broadcast;
  broadcast << "{\"type\":\"send_chat\","
            << "\"channel\":\"" << json_escape(channel) << "\","
            << "\"message\":\"" << json_escape(message) << "\"}";
  std::string broadcast_str = broadcast.str();
  
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  
  for(auto p : m_players) {
    PlayerState *receiver = p.second;
    
    std::shared_lock<std::shared_mutex> player_lock(receiver->lock);
    receiver->send(broadcast_str, m_server);
  }
}

void Server::chat_send_player(PlayerState *player, std::string channel, std::string message) {
  log(LogSource::SERVER, LogLevel::INFO, "(to " + player->get_name() + ") [#" + channel + "] " + message);
  
  std::ostringstream broadcast;
  broadcast << "{\"type\":\"send_chat\","
            << "\"channel\":\"" << json_escape(channel) << "\","
            << "\"message\":\"" << json_escape(message) << "\","
            << "\"private\":true}";
  std::string broadcast_str = broadcast.str();
  
  std::shared_lock<std::shared_mutex> player_lock(player->lock);
  player->send(broadcast_str, m_server);
}

void Server::set_motd(std::string new_motd) {
  std::unique_lock<std::shared_mutex> motd_l(motd_lock);
  motd = new_motd;
}

void Server::set_time(int hours, int minutes) {
  //TODO: have the server track the time
  
  std::ostringstream out;
  out << "{\"type\":\"set_time\",\"hours\":" << hours << ",\"minutes\":" << minutes << "}";
  std::string out_str = out.str();
  
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  for(auto p : m_players) {
    PlayerState *receiver = p.second;
    
    std::shared_lock<std::shared_mutex> player_lock(receiver->lock);
    receiver->send(out_str, m_server);
  }
  
  log(LogSource::SERVER, LogLevel::INFO, "Time set to " + std::to_string(hours) + ":" + (minutes < 10 ? "0" : "") + std::to_string(minutes) + ".");
}

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

std::pair<websocketpp::close::status::value, std::string> Server::validate_connection(connection_hdl hdl) {
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  
  std::string address_and_port;
  std::string address;
  try {
    auto con = m_server.get_con_from_hdl(hdl);
    address_and_port = con->get_remote_endpoint();
    
    const boost::asio::ip::tcp::socket& socket = con->get_raw_socket();
    address = socket.remote_endpoint().address().to_string();
  } catch(websocketpp::exception const& e) {
    log(LogSource::SERVER, LogLevel::ERR, "Socket error: " + std::string(e.what()));
    return std::make_pair(websocketpp::close::status::abnormal_close, "internal error");
  }
  
  //overall player limit
  int max_players = get_config<int>("server.max_players");
  
  if(max_players > 0 && (int)m_players.size() >= max_players) {
    log(LogSource::SERVER, LogLevel::INFO, "Connection from " + address_and_port + " rejected because server is full (" +
                                           std::to_string(m_players.size()) + "/" + std::to_string(max_players) + ") players");
    
    return std::make_pair(websocketpp::close::status::try_again_later,
                          "server is full (" + std::to_string(m_players.size()) + "/" + std::to_string(max_players) + ") players, try again later.");
  }
  
  //per-ip player limit
  int max_players_from_address = get_config<int>("server.max_players_from_address");
  
  if(max_players_from_address > 0) {
    int players_with_same_address = 0;
    for(auto p : m_players) {
      PlayerState *check = p.second;
      std::shared_lock<std::shared_mutex> check_lock(check->lock);
      if(check->address == address) {
        players_with_same_address++;
      }
    }
    
    if(players_with_same_address >= max_players_from_address) {
      log(LogSource::SERVER, LogLevel::INFO, "Connection from " + address_and_port + " rejected because too many players are already connected from that address (" +
                                             std::to_string(players_with_same_address) + "/" + std::to_string(max_players_from_address) + ")");
      
      return std::make_pair(websocketpp::close::status::try_again_later,
                            "too many connections from your address (" + std::to_string(players_with_same_address) + "/" +
                            std::to_string(max_players_from_address) +"), try again later.");
    }
  }
  
  //ok
  return std::make_pair(websocketpp::close::status::normal, "");
}

void Server::on_open(connection_hdl hdl) {
  std::pair<websocketpp::close::status::value, std::string> res = validate_connection(hdl);
  if(res.second != "") {
    //validate_connection will send a response and log as needed
    try {
      m_server.close(hdl, res.first, res.second);
    } catch(websocketpp::exception const& e) {
      log(LogSource::SERVER, LogLevel::ERR, "Socket error: " + std::string(e.what()));
    }
    return;
  }
  
  std::unique_lock<std::shared_mutex> list_lock(m_players_lock);
  
  PlayerState *player = new PlayerState(hdl, m_server);
  
  std::unique_lock<std::shared_mutex> player_lock(player->lock);
  m_players[hdl] = player;
  
  //player is by default auth=false, auth_guest=false
  //player will be authenticated later at the client's request
}

void Server::on_close(connection_hdl hdl) {
  std::unique_lock<std::shared_mutex> list_lock(m_players_lock);
  
  auto search = m_players.find(hdl);
  if(search == m_players.end()) {
    //player was never accepted
    return;
  }
  
  PlayerState *player = search->second;
  
  {
    std::unique_lock<std::shared_mutex> player_lock(player->lock);
    
    m_players.erase(hdl);
    
    if(player->auth) {
      db.update_player_data(player->get_data());
    }
    
    if(player->auth || player->auth_guest) {
      //Clean up entities.
      for(auto p : m_players) {
        PlayerState *receiver = p.second;
        
        std::shared_lock<std::shared_mutex> receiver_lock(receiver->lock);
        
        //Player should *not* know about candidate entity
        if(receiver->known_player_tags.find(player->get_tag()) != receiver->known_player_tags.end()) {
          //...but they do
          //so delete it
          
          std::ostringstream out;
          out << "{\"type\":\"update_entities\",\"actions\":[";
          out << "{\"type\":\"delete\",\"data\":" << player->entity_data_as_json() << "}";
          out << "]}";
          receiver->send(out.str(), m_server);
          
          receiver->known_player_tags.erase(player->get_tag());
        }
      }
      
      std::string name = player->get_name();
      
      player_lock.unlock();
      list_lock.unlock();
      
      log(LogSource::SERVER, LogLevel::INFO, name + " disconnected.");
      chat_send("server", "*** " + name + " left the server.");
    }
  }
  
  delete player;
  
#ifdef EXIT_AFTER_PLAYER_DISCONNECT
  exit(0);
#endif
}

void Server::on_http(connection_hdl hdl) {
  try {
    auto con = m_server.get_con_from_hdl(hdl);
    con->set_body(list_status_json());
    con->set_status(websocketpp::http::status_code::ok);
  } catch(websocketpp::exception const& e) {
    log(LogSource::SERVER, LogLevel::ERR, "Socket error: " + std::string(e.what()));
  }
}

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
      
      std::vector<MapPos<int>> nearby_known_mapblocks = player->list_nearby_known_mapblocks(PLAYER_MAPBLOCK_INTEREST_DISTANCE, PLAYER_MAPBLOCK_INTEREST_DISTANCE_W);
      
      player->update_nearby_known_mapblocks(nearby_known_mapblocks, map, m_server);
      
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
    
    player->send(out.str(), m_server);
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
