#ifndef __SERVER_H__
#define __SERVER_H__

#define VERSION "0.1.0"
#define SERVER_TICK_INTERVAL 500
#define PLAYER_ENTITY_VISIBILE_DISTANCE 200
#define PLAYER_MAPBLOCK_INTEREST_DISTANCE 2

#define BOOST_ERROR_CODE_HEADER_ONLY



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

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <boost/tokenizer.hpp>

#include "vector.h"
#include "json.h"
#include "map.h"
#include "mapblock.h"
#include "node.h"
#include "database.h"

#include "player.h"

using WsServer = websocketpp::server<websocketpp::config::asio>;
using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;



class Server {
  public:
    Server(Database& _db, Mapgen& _mapgen);
    void run(uint16_t port);
    std::string status() const;
    
    void chat_send(std::string channel, std::string from, std::string message);
    void chat_send(std::string channel, std::string message);
    void chat_send_player(PlayerState *player, std::string channel, std::string message);
    
    void set_motd(std::string new_motd);
    
    void set_time(int hours, int minutes);
    
  private:
    void on_message(connection_hdl hdl, websocketpp::config::asio::message_type::ptr msg);
    
    void cmd_nick(PlayerState *player, std::vector<std::string> args);
    void cmd_status(PlayerState *player, std::vector<std::string> args);
    void cmd_time(PlayerState *player, std::vector<std::string> args);
    
    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    
    void tick(const boost::system::error_code&);
    
    typedef std::map<connection_hdl, PlayerState*, std::owner_less<connection_hdl>> player_list;
    
    WsServer m_server;
    player_list m_players;
    boost::asio::io_context m_io;
    boost::asio::steady_timer m_timer;
    Database& db;
    Map map;
    std::string motd;
};

#endif
