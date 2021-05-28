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

#ifndef __SERVER_H__
#define __SERVER_H__

#define VERSION "0.4.4-dev3"
#define SERVER_TICK_INTERVAL 250
#define SERVER_MAPBLOCK_TICK_RATIO 2
#define SERVER_FLUID_TICK_RATIO 8
#define SERVER_SLOW_TICK_RATIO 1 //number of ticks to each slow tick
#define PLAYER_ENTITY_VISIBILE_DISTANCE 200
#define PLAYER_MAPBLOCK_INTEREST_DISTANCE 2
#define PLAYER_MAPBLOCK_INTEREST_DISTANCE_W 0

#include "vector.h"

#define PLAYER_LIMIT_VIEW_DISTANCE MapPos<int>(3, 3, 3, 1, 0, 0)

//Client-side reach distance is 10, max player speed is 16 m/s (fast + sprint), and the client sends position updates every 0.25 seconds.
//Minimum server-side reach distance would then be 14, more added just in case.
#define PLAYER_LIMIT_REACH_DISTANCE MapPos<int>(20, 20, 20, 1, 0, 0)

//Measured in nodes per 10 seconds
#define DISABLE_PLAYER_SPEED_LIMIT //too problematic, disable for now
#define PLAYER_LIMIT_VEL MapPos<int>(120, 120, 120, 6, 0, 0)
#define PLAYER_LIMIT_VEL_FAST MapPos<int>(200, 200, 200, 6, 0, 0)


#define BOOST_ERROR_CODE_HEADER_ONLY



#include <iostream>
#include <map>
#include <sstream>
#include <set>
#include <chrono>
#include <cmath>
#include <shared_mutex>

#ifdef TLS
#include <websocketpp/config/asio.hpp>
#else
#include <websocketpp/config/asio_no_tls.hpp>
#endif
#include <websocketpp/server.hpp>

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <boost/tokenizer.hpp>

#include "json.h"
#include "map.h"
#include "mapblock.h"
#include "node.h"
#include "item.h"
#include "craft.h"
#include "database.h"

#include "player.h"
#include "player_data.h"
#include "player_auth.h"

#ifdef TLS
using WsServer = websocketpp::server<websocketpp::config::asio_tls>;
#else
using WsServer = websocketpp::server<websocketpp::config::asio>;
#endif
using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;



class Server {
  public:
    Server(Database& _db, std::map<int, World*> _worlds);
    void run(uint16_t port);
    void terminate();
    void terminate(std::string message);
    std::string status() const;
    std::string list_status_json() const;
    
    void chat_send(std::string channel, std::string from, std::string message);
    void chat_send(std::string channel, std::string message);
    void chat_send_player(PlayerState *player, std::string channel, std::string message);
    
    bool lock_invlist(InvRef ref, PlayerState *player_hint);
    bool unlock_invlist(InvRef ref, PlayerState *player_hint);
    InvList get_invlist(InvRef ref, PlayerState *player_hint);
    bool set_invlist(InvRef ref, InvList list, PlayerState *player_hint);
    
    bool inv_apply_patch(InvPatch patch, PlayerState *requesting_player);
    bool inv_apply_patch(InvPatch patch) {
      return inv_apply_patch(patch, NULL);
    };
    
    //void update_known_inventories(PlayerState *player);
    
    void set_motd(std::string new_motd);
    void set_time(int hours, int minutes);
    
  private:
    void on_message(connection_hdl hdl, websocketpp::config::asio::message_type::ptr msg);
    
    void cmd_nick(PlayerState *player, std::vector<std::string> args);
    void cmd_status(PlayerState *player, std::vector<std::string> args);
    void cmd_time(PlayerState *player, std::vector<std::string> args);
    void cmd_whereami(PlayerState *player, std::vector<std::string> args);
    void cmd_tp(PlayerState *player, std::vector<std::string> args);
    void cmd_tp_world(PlayerState *player, std::vector<std::string> args);
    void cmd_tp_universe(PlayerState *player, std::vector<std::string> args);
    void cmd_grantme(PlayerState *player, std::vector<std::string> args);
    void cmd_privs(PlayerState *player, std::vector<std::string> args);
    void cmd_giveme(PlayerState *player, std::vector<std::string> args);
    
    std::pair<websocketpp::close::status::value, std::string> validate_connection(connection_hdl hdl);
    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    void on_http(connection_hdl hdl);
    
#ifdef TLS
    websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> on_tls_init(connection_hdl hdl);
#endif
    
    void tick(const boost::system::error_code&);
    void slow_tick();
    
    bool lock_unlock_invlist(InvRef ref, bool do_lock, PlayerState *player_hint);
    
    typedef std::map<connection_hdl, PlayerState*, std::owner_less<connection_hdl>> player_list;
    
    WsServer m_server;
    boost::asio::io_context m_io;
    boost::asio::steady_timer m_timer;
    
    player_list m_players;
    mutable std::shared_mutex m_players_lock;
    
    Database& db;
    Map map;
    
    std::string motd;
    mutable std::shared_mutex motd_lock;
    
    int mapblock_tick_counter;
    int fluid_tick_counter;
    int slow_tick_counter;
    std::chrono::time_point<std::chrono::steady_clock> last_tick;
    bool halt;
    std::shared_mutex tick_info_lock;
    
#ifdef DEBUG_NET
    unsigned int mb_out_count;
    unsigned long long mb_out_len;
    std::shared_mutex net_debug_lock;
#endif
};

#endif
