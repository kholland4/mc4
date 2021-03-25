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

MapPos<int> PLAYER_LIMIT_VIEW_DISTANCE(3, 3, 3, 1, 0, 0);

//Client-side reach distance is 10, max player speed is 16 m/s (fast + sprint), and the client sends position updates every 0.25 seconds.
//Minimum server-side reach distance would then be 14, more added just in case.
MapPos<int> PLAYER_LIMIT_REACH_DISTANCE(20, 20, 20, 1, 0, 0);


Server::Server(Database& _db, std::map<int, World*> _worlds)
    : m_timer(m_io, boost::asio::chrono::milliseconds(SERVER_TICK_INTERVAL)), db(_db), map(_db, _worlds), mapblock_tick_counter(0), fluid_tick_counter(0), slow_tick_counter(0)
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
  
  m_io.run();
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

std::string Server::status() const {
  std::string s = "-- Server v" + std::string(VERSION) + "; " + std::to_string(m_players.size()) + " players {";
  
  bool first = true;
  for(auto const& x : m_players) {
    if(!first) { s += ", "; }
    first = false;
    s += x.second->get_name();
  }
  
  s += "}";
  
  if(motd != "") {
    s += "\n" + motd;
  }
  
  return s;
}

void Server::chat_send(std::string channel, std::string from, std::string message) {
  log(LogSource::SERVER, LogLevel::INFO, "[#" + channel + "] <" + from + "> " + message);
  
  std::ostringstream broadcast;
  broadcast << "{\"type\":\"send_chat\","
            << "\"from\":\"" << json_escape(from) << "\","
            << "\"channel\":\"" << json_escape(channel) << "\","
            << "\"message\":\"" << json_escape(message) << "\"}";
  std::string broadcast_str = broadcast.str();
  
  for(auto p : m_players) {
    PlayerState *receiver = p.second;
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
  
  for(auto p : m_players) {
    PlayerState *receiver = p.second;
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
  
  player->send(broadcast_str, m_server);
}

void Server::set_motd(std::string new_motd) {
  motd = new_motd;
}

void Server::set_time(int hours, int minutes) {
  //TODO: have the server track the time
  
  std::ostringstream out;
  out << "{\"type\":\"set_time\",\"hours\":" << hours << ",\"minutes\":" << minutes << "}";
  std::string out_str = out.str();
  for(auto p : m_players) {
    PlayerState *receiver = p.second;
    receiver->send(out_str, m_server);
  }
  
  log(LogSource::SERVER, LogLevel::INFO, "Time set to " + std::to_string(hours) + ":" + (minutes < 10 ? "0" : "") + std::to_string(minutes) + ".");
}

void Server::on_message(connection_hdl hdl, websocketpp::config::asio::message_type::ptr msg) {
  auto search = m_players.find(hdl);
  if(search == m_players.end()) {
    log(LogSource::SERVER, LogLevel::ERR, "Unable to find player state for connection!");
    return;
  }
  PlayerState *player = search->second;
  
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
              chat_send_player(player, "server", "ERROR: player '" + p->data.name + "' is already connected");
              websocketpp::lib::error_code ec;
              m_server.close(hdl, websocketpp::close::status::going_away, "kick", ec);
              if(ec) {
                log(LogSource::SERVER, LogLevel::ERR, "error closing connection: " + ec.message());
              }
              return;
            }
          }
          
          PlayerData pdata = db.fetch_player_data(auth_id);
          if(pdata.is_nil) {
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
        
        log(LogSource::SERVER, LogLevel::INFO, player->get_name() + " connected!");
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
      Mapblock *mb = map.get_mapblock(mb_pos);
#ifdef DEBUG_NET
      unsigned int len = player->send_mapblock(mb, m_server);
      mb_out_len += len;
      mb_out_count++;
#else
      player->send_mapblock(mb, m_server);
#endif
      delete mb;
    } else if(type == "set_player_pos") {
      MapPos<double> pos(pt.get<double>("pos.x"), pt.get<double>("pos.y"), pt.get<double>("pos.z"), pt.get<int>("pos.w"), player->pos.world, player->pos.universe);
      
      //TODO: validate that the player hasn't moved too far since their last position update
      
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
      
#ifdef DEBUG_PERF
      auto start = std::chrono::steady_clock::now();
#endif
      
      map.set_node(pos, node);
      
#ifdef DEBUG_PERF
      auto end = std::chrono::steady_clock::now();
      auto diff = end - start;
      
      std::cout << "set_node in " << std::chrono::duration<double, std::milli>(diff).count() << " ms" << std::endl;
#endif
      
      log(LogSource::SERVER, LogLevel::EXTRA, "Player '" + player->get_name() + "' places '" + node.itemstring + "' at " + pos.to_string());
      
      MapPos<int> mb_pos = global_to_mapblock(pos);
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
      player->update_mapblocks(mapblock_list, map, m_server);
    } else if(type == "send_chat") {
      std::string from = player->get_name();
      std::string channel = pt.get<std::string>("channel");
      std::string message = pt.get<std::string>("message");
      
      //TODO: validate channel, access control, etc.
      //TODO: any other anti-abuse validation (player name and channel character set restrictions, length limits, etc.)
      
      chat_send(channel, from, message);
    } else if(type == "chat_command") {
      std::string command = pt.get<std::string>("command");
      
      std::vector<std::string> tokens;
      boost::char_separator<char> sep(" ", "");
      boost::tokenizer<boost::char_separator<char>> tokenizer(command, sep);
      for(boost::tokenizer<boost::char_separator<char>>::iterator it = tokenizer.begin(); it != tokenizer.end(); ++it) {
        tokens.push_back(*it);
      }
      
      if(tokens.size() < 1) {
        chat_send_player(player, "server", "invalid command: empty input");
        return;
      }
      
      std::string command_name = tokens[0];
      
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
      }
    }
  } catch(std::exception const& e) {
    log(LogSource::SERVER, LogLevel::ERR, "JSON parse error: " + std::string(e.what()) + " payload=" + msg->get_payload());
  }
}

void Server::on_open(connection_hdl hdl) {
  PlayerState *player = new PlayerState(hdl);
  m_players[hdl] = player;
  
  //player is by default auth=false, auth_guest=false
  //player will be authenticated later at the client's request
}

void Server::on_close(connection_hdl hdl) {
  PlayerState *player = m_players[hdl];
  m_players.erase(hdl);
  
  if(player->auth) {
    db.update_player_data(player->get_data());
  }
  
  if(player->auth || player->auth_guest) {
    //Clean up entities.
    for(auto p : m_players) {
      PlayerState *receiver = p.second;
      
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
    
    log(LogSource::SERVER, LogLevel::INFO, player->get_name() + " disconnected.");
    chat_send("server", "*** " + player->get_name() + " left the server.");
  }
  
  delete player;
  
#ifdef EXIT_AFTER_PLAYER_DISCONNECT
  exit(0);
#endif
}

void Server::tick(const boost::system::error_code&) {
  mapblock_tick_counter++;
  fluid_tick_counter++;
  if(mapblock_tick_counter >= SERVER_MAPBLOCK_TICK_RATIO) {
    std::set<MapPos<int>> interested_mapblocks;
    
    for(auto p : m_players) {
      PlayerState *player = p.second;
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
    if(!player->auth && !player->auth_guest) { continue; }
    
    std::ostringstream out;
    out << "{\"type\":\"update_entities\",\"actions\":[";
    
    bool first = true;
    for(auto p_sub : m_players) {
      PlayerState *candidate = p_sub.second;
      if(candidate == player) { continue; }
      
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
  if(mb_out_count > 0) {
    std::cout << std::to_string(mb_out_len / mb_out_count) << " avg mb out bytes, " << std::to_string(mb_out_len / 1048576.0) << " MiB total" << std::endl;
  }
#endif

  m_timer.expires_at(m_timer.expiry() + boost::asio::chrono::milliseconds(SERVER_TICK_INTERVAL));
  m_timer.async_wait(boost::bind(&Server::tick, this, boost::asio::placeholders::error));
}

void Server::slow_tick() {
  db.clean_cache();
}
