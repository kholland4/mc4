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

std::pair<websocketpp::close::status::value, std::string> Server::validate_connection(connection_hdl hdl) {
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  
  std::string address_and_port;
  std::string address;
  try {
    auto con = m_server.get_con_from_hdl(hdl);
    address_and_port = con->get_remote_endpoint();
    
    const auto& socket = con->get_raw_socket();
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
