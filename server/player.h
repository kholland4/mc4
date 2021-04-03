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

#ifndef __PLAYER_H__
#define __PLAYER_H__

#define BOOST_ERROR_CODE_HEADER_ONLY

#include "vector.h"
#include "mapblock.h"
#include "map.h"
#include "json.h"
#include "player_data.h"
#include "player_auth.h"
#include "log.h"

#include <chrono>
#include <shared_mutex>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef TLS
#include <websocketpp/config/asio.hpp>
#else
#include <websocketpp/config/asio_no_tls.hpp>
#endif
#include <websocketpp/server.hpp>

#ifdef TLS
using WsServer = websocketpp::server<websocketpp::config::asio_tls>;
#else
using WsServer = websocketpp::server<websocketpp::config::asio>;
#endif
using websocketpp::connection_hdl;



class PlayerState {
  public:
    PlayerState(connection_hdl hdl, WsServer& server);
    
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
    
    std::string pos_as_json();
    std::string entity_data_as_json();
    std::string privs_as_json();
    
    void send_pos(WsServer& sender);
    void send_privs(WsServer& sender);
    
    bool needs_mapblock_update(MapblockUpdateInfo info);
    unsigned int send_mapblock_compressed(MapblockCompressed *mbc, WsServer& sender);
    
    void prepare_mapblocks(std::vector<MapPos<int>> mapblock_list, Map& map);
    void prepare_nearby_mapblocks(int mb_radius, int mb_radius_outer, int mb_radius_w, Map& map);
    
    void update_mapblocks(std::vector<MapPos<int>> mapblock_list, Map& map, WsServer& sender);
    void update_nearby_mapblocks(int mb_radius, int mb_radius_w, Map& map, WsServer& sender);
    std::vector<MapPos<int>> list_nearby_known_mapblocks(int mb_radius, int mb_radius_w);
    void update_nearby_known_mapblocks(int mb_radius, int mb_radius_w, Map& map, WsServer& sender);
    void update_nearby_known_mapblocks(std::vector<MapPos<int>> mb_to_update, Map& map, WsServer& sender);
    
    void send(std::string msg, WsServer& sender) {
      try {
        sender.send(m_connection_hdl, msg, websocketpp::frame::opcode::text);
      } catch(websocketpp::exception const& e) {
        log(LogSource::PLAYER, LogLevel::ERR, "Socket send error");
      }
    }
    
    void load_data(PlayerData _data) {
      data = _data;
      
      pos = data.pos;
      vel = data.vel;
      rot = data.rot;
      m_name = data.name;
    }
    PlayerData get_data() {
      data.pos = pos;
      data.vel = vel;
      data.rot = rot;
      //don't save name
      
      return data;
    }
    
    MapPos<int> containing_mapblock();
    
    bool auth;
    bool auth_guest;
    
    //Stores the player's physical position.
    //Once 'auth' is set to true, these will contain valid data.
    MapPos<double> pos;
    MapPos<double> vel;
    Quaternion rot;
    
    std::list<std::pair<std::chrono::time_point<std::chrono::steady_clock>, MapPos<double>>> pos_history;
    bool just_tp;
    
    std::map<MapPos<int>, MapblockUpdateInfo> known_mapblocks;
    std::set<std::string> known_player_tags;
    
    PlayerAuthenticator auth_state;
    PlayerData data;
    
    std::string address_and_port;
    std::string address;
    
    std::shared_mutex lock;
  private:
    connection_hdl m_connection_hdl;
    boost::uuids::uuid m_tag;
    std::string m_name;
};

#endif
