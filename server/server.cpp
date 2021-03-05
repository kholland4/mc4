#include "server.h"

#include "log.h"

Server::Server(Database& _db, Mapgen& _mapgen)
    : m_timer(m_io, boost::asio::chrono::milliseconds(SERVER_TICK_INTERVAL)), db(_db), map(_db, _mapgen), slow_tick_counter(0)
{
  //disable logging
  m_server.clear_access_channels(websocketpp::log::alevel::all);
  m_server.clear_error_channels(websocketpp::log::elevel::all);
  
  m_server.init_asio(&m_io);
  
  m_server.set_message_handler(
      websocketpp::lib::bind(&Server::on_message, this, ::_1, ::_2));
  m_server.set_open_handler(
      websocketpp::lib::bind(&Server::on_open, this, ::_1));
  m_server.set_close_handler(
      websocketpp::lib::bind(&Server::on_close, this, ::_1));
}

void Server::run(uint16_t port) {
  m_timer.async_wait(boost::bind(&Server::tick, this, boost::asio::placeholders::error));
  
  m_server.listen(port);
  m_server.start_accept();
  m_io.run();
}

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
  //std::cout << "on_message: " << msg->get_payload() << std::endl;
  
  auto search = m_players.find(hdl);
  if(search == m_players.end()) {
    std::cerr << "Unable to find player state for connection!" << std::endl;
    return;
  }
  PlayerState *player = search->second;
  
  try {
    std::stringstream ss;
    ss << msg->get_payload();
    
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss, pt);
    
    std::string type = pt.get<std::string>("type");
    
    if(!player->auth) {
      //TODO handle auth messages
      //TODO send reply saying unauthorized (if not an auth message)
      return;
    }
    
    if(type == "req_mapblock") {
      Vector3<int> mb_pos(pt.get<int>("pos.x"), pt.get<int>("pos.y"), pt.get<int>("pos.z"));
      
      MapblockUpdateInfo info = map.get_mapblockupdateinfo(mb_pos);
      if(info.light_needs_update > 0) {
#ifdef DEBUG_PERF
        std::cout << "single mapblock prep for " << player->get_name() << std::endl;
#endif
        map.update_mapblock_light(info);
      }
      Mapblock *mb = map.get_mapblock(mb_pos);
      player->send_mapblock(mb, m_server);
      delete mb;
    } else if(type == "set_player_pos") {
      player->pos.set(pt.get<double>("pos.x"), pt.get<double>("pos.y"), pt.get<double>("pos.z"));
      player->vel.set(pt.get<double>("vel.x"), pt.get<double>("vel.y"), pt.get<double>("vel.z"));
      player->rot.set(pt.get<double>("rot.x"), pt.get<double>("rot.y"), pt.get<double>("rot.z"), pt.get<double>("rot.w"));
      
      player->prepare_nearby_mapblocks(2, 3, map);
    } else if(type == "set_node") {
      Vector3<int> pos(pt.get<int>("pos.x"), pt.get<int>("pos.y"), pt.get<int>("pos.z"));
      Node node(pt.get<std::string>("data.itemstring"), pt.get<unsigned int>("data.rot"));
      
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
      
      Vector3<int> mb_pos = map.containing_mapblock(pos);
      Vector3<int> min_pos = mb_pos - Vector3<int>(1, 1, 1);
      Vector3<int> max_pos = mb_pos + Vector3<int>(1, 1, 1);
      std::vector<Vector3<int>> mapblock_list;
      mapblock_list.push_back(mb_pos); //Send the main affected mapblock first.
      for(int x = min_pos.x; x <= max_pos.x; x++) {
        for(int y = min_pos.y; y <= max_pos.y; y++) {
          for(int z = min_pos.z; z <= max_pos.z; z++) {
            Vector3<int> rel_mb_pos(x, y, z);
            if(rel_mb_pos == mb_pos) { continue; }
            mapblock_list.push_back(rel_mb_pos);
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
      } else {
        chat_send_player(player, "server", "unknown command");
      }
    }
  } catch(std::exception const& e) {
    std::cerr << e.what() << std::endl;
  }
}

void Server::on_open(connection_hdl hdl) {
  PlayerState *player = new PlayerState(hdl);
  m_players[hdl] = player;
  
  player->pos.set(0, 20, 0);
  player->vel.set(0, 0, 0);
  player->rot.set(0, 0, 0, 0);
  player->auth = true; //TODO: authenticate player & set their name
  
  m_server.send(hdl, player->pos_as_json(), websocketpp::frame::opcode::text);
  
  player->prepare_nearby_mapblocks(2, 3, map);
  
  log(LogSource::SERVER, LogLevel::INFO, player->get_name() + " connected!");
  chat_send_player(player, "server", status());
  chat_send("server", "*** " + player->get_name() + " joined the server!");
}

void Server::on_close(connection_hdl hdl) {
  PlayerState *player = m_players[hdl];
  m_players.erase(hdl);
  
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
  
  delete player;
}

void Server::tick(const boost::system::error_code&) {
  //std::cout << "TICK " << status() << std::endl;
  
  for(auto p : m_players) {
    PlayerState *player = p.second;
    
    player->update_nearby_known_mapblocks(PLAYER_MAPBLOCK_INTEREST_DISTANCE, map, m_server);
  }
  
  for(auto p : m_players) {
    PlayerState *player = p.second;
    
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
  
  m_timer.expires_at(m_timer.expiry() + boost::asio::chrono::milliseconds(SERVER_TICK_INTERVAL));
  m_timer.async_wait(boost::bind(&Server::tick, this, boost::asio::placeholders::error));
}

void Server::slow_tick() {
  db.clean_cache();
}
