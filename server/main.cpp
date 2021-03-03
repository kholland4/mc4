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

#define BOOST_ERROR_CODE_HEADER_ONLY

#define VERSION "0.1.0"
#define SERVER_TICK_INTERVAL 500
#define PLAYER_ENTITY_VISIBILE_DISTANCE 200
#define PLAYER_MAPBLOCK_INTEREST_DISTANCE 2

//#define DEBUG_PERF

#include <iostream>
#include <map>
#include <sstream>
#include <set>
#include <regex>
#include <chrono>
#include <cmath>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <boost/tokenizer.hpp>

#include "vector.h"
#include "json.h"
#include "map.h"
#include "mapblock.h"
#include "node.h"
#include "database.h"
 
using WsServer = websocketpp::server<websocketpp::config::asio>;
using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

class PlayerState {
  public:
    PlayerState(connection_hdl hdl)
        : auth(false), m_connection_hdl(hdl), m_tag(boost::uuids::random_generator()()), m_name(get_tag())
    {
      
    }
    
    bool operator<(const PlayerState& other) const {
      return m_tag < other.m_tag;
    }
    bool operator==(const PlayerState& other) const {
      return m_tag == other.m_tag;
    }
    
    std::string get_tag() const {
      return boost::uuids::to_string(m_tag);
    }
    
    std::string get_name() const {
      return m_name;
    }
    
    void set_name(std::string new_name) {
      m_name = new_name;
    }
    
    std::string pos_as_json() {
      /*boost::property_tree::ptree pt;
      
      pt.put("type", "set_player_pos");
      pt.put("pos.x", pos.x);
      pt.put("pos.y", pos.y);
      pt.put("pos.z", pos.z);
      
      std::ostringstream out;
      boost::property_tree::write_json(out, pt, false);
      return out.str();*/
      
      std::ostringstream out;
      
      out << "{\"type\":\"set_player_pos\","
          <<  "\"pos\":{\"x\":" << pos.x << ",\"y\":" << pos.y << ",\"z\":" << pos.z << "},"
          <<  "\"vel\":{\"x\":" << vel.x << ",\"y\":" << vel.y << ",\"z\":" << vel.z << "},"
          <<  "\"rot\":{\"x\":" << rot.x << ",\"y\":" << rot.y << ",\"z\":" << rot.z << ",\"w\":" << rot.w << "}}";
      
      return out.str();
    }
    
    std::string entity_data_as_json() {
      std::ostringstream out;
      
      out << "{\"id\":\"" << json_escape(get_tag()) << "\","
          <<  "\"pos\":{\"x\":" << pos.x << ",\"y\":" << pos.y << ",\"z\":" << pos.z << "},"
          <<  "\"vel\":{\"x\":" << vel.x << ",\"y\":" << vel.y << ",\"z\":" << vel.z << "},"
          <<  "\"rot\":{\"x\":" << rot.x << ",\"y\":" << rot.y << ",\"z\":" << rot.z << ",\"w\":" << rot.w << "}}";
      
      return out.str();
    }
    
    bool needs_mapblock_update(MapblockUpdateInfo info) {
      auto search = known_mapblocks.find(info.pos);
      if(search != known_mapblocks.end()) {
        MapblockUpdateInfo curr_info = search->second;
        return info != curr_info;
      }
      return true;
    }
    
    void send_mapblock(Mapblock *mb, WsServer& sender) {
      known_mapblocks[mb->pos] = MapblockUpdateInfo(mb);
      
      sender.send(m_connection_hdl, mb->as_json(), websocketpp::frame::opcode::text);
    }
    
    void prepare_mapblocks(std::vector<Vector3<int>> mapblock_list, Map& map) {
      //Batch update light.
      std::set<Vector3<int>> mb_need_light;
      for(size_t i = 0; i < mapblock_list.size(); i++) {
        Vector3<int> mb_pos = mapblock_list[i];
        MapblockUpdateInfo info = map.get_mapblockupdateinfo(mb_pos);
        if(info.light_needs_update == 1) { //TODO what about 2?
          mb_need_light.insert(mb_pos);
        }
      }
      if(mb_need_light.size() > 0) {
#ifdef DEBUG_PERF
        auto start = std::chrono::steady_clock::now();
#endif
        map.update_mapblock_light(mb_need_light);
#ifdef DEBUG_PERF
        auto end = std::chrono::steady_clock::now();
        auto diff = end - start;
        double ms = std::chrono::duration<double, std::milli>(diff).count();
        std::cout << "prepped " << mb_need_light.size() << " mapblocks for " << get_name() << " in " << ms << " ms" << std::endl;
#endif
      }
    }
    void update_mapblocks(std::vector<Vector3<int>> mapblock_list, Map& map, WsServer& sender) {
      prepare_mapblocks(mapblock_list, map);
      
      for(size_t i = 0; i < mapblock_list.size(); i++) {
        Vector3<int> mb_pos = mapblock_list[i];
        MapblockUpdateInfo info = map.get_mapblockupdateinfo(mb_pos);
        if(needs_mapblock_update(info)) {
          Mapblock *mb = map.get_mapblock(mb_pos);
          send_mapblock(mb, sender);
          delete mb;
        }
      }
    }
    
    //mb_radius indicates the radius of a cube of mapblocks to prepare (i. e. mb_radius=2 yields a 5x5x5 cube centered around the player's current mapblock)
    //mb_radius_outer indicates the radius of a sort of 3-dimensional plus shape extending outward from this cube
    //  (if at least two dimensions are within the bounds of the inner cube, the mapblocks will be loaded)
    //  this is used because mapblock rendering on the client requires access all 6 mapblocks immediately adjacent to the one being rendered
    void prepare_nearby_mapblocks(int mb_radius, int mb_radius_outer, Map& map) {
      //Vector3<int> abs_pos((int)pos.x, (int)pos.y, (int)pos.z);
      
      Vector3<int> mb_pos(
          (int)std::round(pos.x / MAPBLOCK_SIZE_X),
          (int)std::round(pos.y / MAPBLOCK_SIZE_Y),
          (int)std::round(pos.z / MAPBLOCK_SIZE_Z));
      Vector3<int> min_inner(mb_pos.x - mb_radius, mb_pos.y - mb_radius, mb_pos.z - mb_radius);
      Vector3<int> max_inner(mb_pos.x + mb_radius, mb_pos.y + mb_radius, mb_pos.z + mb_radius);
      Vector3<int> min_outer(mb_pos.x - mb_radius_outer, mb_pos.y - mb_radius_outer, mb_pos.z - mb_radius_outer);
      Vector3<int> max_outer(mb_pos.x + mb_radius_outer, mb_pos.y + mb_radius_outer, mb_pos.z + mb_radius_outer);
      
      std::vector<Vector3<int>> mb_to_update;
      for(int x = min_outer.x; x <= max_outer.x; x++) {
        for(int y = min_outer.y; y <= max_outer.y; y++) {
          for(int z = min_outer.z; z <= max_outer.z; z++) {
            if(((x >= min_inner.x && x <= max_inner.x) && (y >= min_inner.y && y <= max_inner.y)) ||
               ((y >= min_inner.y && y <= max_inner.y) && (z >= min_inner.z && z <= max_inner.z)) ||
               ((x >= min_inner.x && x <= max_inner.x) && (z >= min_inner.z && z <= max_inner.z))) {
              mb_to_update.push_back(Vector3<int>(x, y, z));
            }
          }
        }
      }
      
      prepare_mapblocks(mb_to_update, map);
    }
    void update_nearby_mapblocks(int mb_radius, Map& map, WsServer& sender) {
      Vector3<int> abs_pos((int)pos.x, (int)pos.y, (int)pos.z);
      
      Vector3<int> mb_pos(
          (abs_pos.x < 0 && abs_pos.x % MAPBLOCK_SIZE_X != 0) ? (abs_pos.x / MAPBLOCK_SIZE_X - 1) : (abs_pos.x / MAPBLOCK_SIZE_X),
          (abs_pos.y < 0 && abs_pos.y % MAPBLOCK_SIZE_Y != 0) ? (abs_pos.y / MAPBLOCK_SIZE_Y - 1) : (abs_pos.y / MAPBLOCK_SIZE_Y),
          (abs_pos.z < 0 && abs_pos.z % MAPBLOCK_SIZE_Z != 0) ? (abs_pos.z / MAPBLOCK_SIZE_Z - 1) : (abs_pos.z / MAPBLOCK_SIZE_Z));
      Vector3<int> min_pos(mb_pos.x - mb_radius, mb_pos.y - mb_radius, mb_pos.z - mb_radius);
      Vector3<int> max_pos(mb_pos.x + mb_radius, mb_pos.y + mb_radius, mb_pos.z + mb_radius);
      
      std::vector<Vector3<int>> mb_to_update;
      for(int x = min_pos.x; x <= max_pos.x; x++) {
        for(int y = min_pos.y; y <= max_pos.y; y++) {
          for(int z = min_pos.z; z <= max_pos.z; z++) {
            mb_to_update.push_back(Vector3<int>(x, y, z));
          }
        }
      }
      update_mapblocks(mb_to_update, map, sender);
    }
    void update_nearby_known_mapblocks(int mb_radius, Map& map, WsServer& sender) {
      Vector3<int> abs_pos((int)pos.x, (int)pos.y, (int)pos.z);
      
      Vector3<int> mb_pos(
          (abs_pos.x < 0 && abs_pos.x % MAPBLOCK_SIZE_X != 0) ? (abs_pos.x / MAPBLOCK_SIZE_X - 1) : (abs_pos.x / MAPBLOCK_SIZE_X),
          (abs_pos.y < 0 && abs_pos.y % MAPBLOCK_SIZE_Y != 0) ? (abs_pos.y / MAPBLOCK_SIZE_Y - 1) : (abs_pos.y / MAPBLOCK_SIZE_Y),
          (abs_pos.z < 0 && abs_pos.z % MAPBLOCK_SIZE_Z != 0) ? (abs_pos.z / MAPBLOCK_SIZE_Z - 1) : (abs_pos.z / MAPBLOCK_SIZE_Z));
      Vector3<int> min_pos(mb_pos.x - mb_radius, mb_pos.y - mb_radius, mb_pos.z - mb_radius);
      Vector3<int> max_pos(mb_pos.x + mb_radius, mb_pos.y + mb_radius, mb_pos.z + mb_radius);
      
      std::vector<Vector3<int>> mb_to_update;
      for(int x = min_pos.x; x <= max_pos.x; x++) {
        for(int y = min_pos.y; y <= max_pos.y; y++) {
          for(int z = min_pos.z; z <= max_pos.z; z++) {
            Vector3<int> where(x, y, z);
            auto search = known_mapblocks.find(where);
            if(search != known_mapblocks.end()) {
              mb_to_update.push_back(where);
            }
          }
        }
      }
      update_mapblocks(mb_to_update, map, sender);
    }
    
    void send(std::string msg, WsServer& sender) {
      sender.send(m_connection_hdl, msg, websocketpp::frame::opcode::text);
    }
    
    bool auth;
    
    //Stores the player's physical position.
    //Once 'auth' is set to true, these will contain valid data.
    Vector3<double> pos;
    Vector3<double> vel;
    Quaternion rot;
    
    std::map<Vector3<int>, MapblockUpdateInfo> known_mapblocks;
    std::set<std::string> known_player_tags;
  private:
    connection_hdl m_connection_hdl;
    boost::uuids::uuid m_tag;
    std::string m_name;
};

class Server {
  public:
    Server(Database& _db, Mapgen& _mapgen)
        : m_timer(m_io, boost::asio::chrono::milliseconds(SERVER_TICK_INTERVAL)), db(_db), map(_db, _mapgen)
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
    
    void run(uint16_t port) {
      //TODO: print map info
      
      m_timer.async_wait(boost::bind(&Server::tick, this, boost::asio::placeholders::error));
      
      m_server.listen(port);
      m_server.start_accept();
      m_io.run();
    }
    
    std::string status() const {
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
    
    void chat_send(std::string channel, std::string from, std::string message) {
      log("[#" + channel + "] <" + from + "> " + message);
      
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
    
    void chat_send(std::string channel, std::string message) {
      log("[#" + channel + "] " + message);
      
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
    
    void chat_send_player(PlayerState *player, std::string channel, std::string message) {
      log("(to " + player->get_name() + ") [#" + channel + "] " + message);
      
      std::ostringstream broadcast;
      broadcast << "{\"type\":\"send_chat\","
                << "\"channel\":\"" << json_escape(channel) << "\","
                << "\"message\":\"" << json_escape(message) << "\","
                << "\"private\":true}";
      std::string broadcast_str = broadcast.str();
      
      player->send(broadcast_str, m_server);
    }
    
    void set_motd(std::string new_motd) {
      motd = new_motd;
    }
    
    void set_time(int hours, int minutes) {
      //TODO: have the server track the time
      
      std::ostringstream out;
      out << "{\"type\":\"set_time\",\"hours\":" << hours << ",\"minutes\":" << minutes << "}";
      std::string out_str = out.str();
      for(auto p : m_players) {
        PlayerState *receiver = p.second;
        receiver->send(out_str, m_server);
      }
      
      log("Time set to " + std::to_string(hours) + ":" + (minutes < 10 ? "0" : "") + std::to_string(minutes) + ".");
    }
  private:
    void on_message(connection_hdl hdl, websocketpp::config::asio::message_type::ptr msg) {
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
          
          log("Player '" + player->get_name() + "' places '" + node.itemstring + "' at " + pos.to_string());
          
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
          
          //TODO use a table of class instances or something
          
          if(tokens[0] == "/nick") {
            if(tokens.size() != 2) {
              chat_send_player(player, "server", "invalid command: wrong number of args, expected '/nick <new_nickname>'");
              return;
            }
            
            std::string new_nick = tokens[1];
            
            std::regex nick_allow("^[a-zA-Z0-9\\-_]+$");
            if(!std::regex_match(new_nick, nick_allow)) {
              chat_send_player(player, "server", "invalid nickname: allowed characters are a-z A-Z 0-9 - _");
              return;
            }
            
            std::string old_nick = player->get_name();
            player->set_name(new_nick);
            chat_send("server", "*** " + old_nick + " changed name to " + new_nick);
          } else if(tokens[0] == "/status") {
            chat_send_player(player, "server", status());
          } else if(tokens[0] == "/time") {
            if(tokens.size() != 2) {
              chat_send_player(player, "server", "invalid command: wrong number of args, expected '/time hh:mm'");
              return;
            }
            
            std::string time_spec = tokens[1];
            
            std::regex time_allow("^((|0|1)[0-9]|2[0-3]):[0-5][0-9]$");
            if(!std::regex_match(time_spec, time_allow)) {
              chat_send_player(player, "server", "invalid /time command: please use '/time hh:mm' in 24-hour format");
              return;
            }
            
            auto colon_pos = time_spec.find(":");
            std::string hours_str = time_spec.substr(0, colon_pos);
            std::string minutes_str = time_spec.substr(colon_pos + 1);
            
            int hours = std::stoi(hours_str);
            int minutes = std::stoi(minutes_str);
            set_time(hours, minutes);
            
            chat_send_player(player, "server", "Time set to " + std::to_string(hours) + ":" + (minutes < 10 ? "0" : "") + std::to_string(minutes) + ".");
          } else {
            chat_send_player(player, "server", "unknown command");
          }
        }
      } catch(std::exception const& e) {
        std::cerr << e.what() << std::endl;
      }
    }

    void on_open(connection_hdl hdl) {
      PlayerState *player = new PlayerState(hdl);
      m_players[hdl] = player;
      
      player->pos.set(0, 20, 0);
      player->vel.set(0, 0, 0);
      player->rot.set(0, 0, 0, 0);
      player->auth = true; //TODO: authenticate player & set their name
      
      m_server.send(hdl, player->pos_as_json(), websocketpp::frame::opcode::text);
      
      player->prepare_nearby_mapblocks(2, 3, map);
      
      log(player->get_tag() + " connected!");
      chat_send_player(player, "server", status());
      chat_send("server", "*** " + player->get_name() + " joined the server!");
    }

    void on_close(connection_hdl hdl) {
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
      
      log(player->get_tag() + " disconnected.");
      chat_send("server", "*** " + player->get_name() + " left the server.");
      
      delete player;
    }
    
    void tick(const boost::system::error_code&) {
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
      
      m_timer.expires_at(m_timer.expiry() + boost::asio::chrono::milliseconds(SERVER_TICK_INTERVAL));
      m_timer.async_wait(boost::bind(&Server::tick, this, boost::asio::placeholders::error));
    }
    
    void log(std::string msg) {
      std::cout << msg << std::endl;
    }
    
    typedef std::map<connection_hdl, PlayerState*, std::owner_less<connection_hdl>> player_list;
    
    WsServer m_server;
    player_list m_players;
    boost::asio::io_context m_io;
    boost::asio::steady_timer m_timer;
    Database& db;
    Map map;
    std::string motd;
};

int main() {
  SQLiteDB db("test_map.sqlite");
  //MemoryDB db;
  MapgenDefault mapgen;
  Server server(db, mapgen);
  
  server.set_motd("-- Highly Experimental Test Server (tm)\n-- Use '/gamemode creative' for creative, '/nick new_nickname_here' to change your name, '/status' to view this message, '/help' for other commands\n-- Press 'k' to toggle fly, 'j' to toggle fast");
  
  server.run(8080);
}
