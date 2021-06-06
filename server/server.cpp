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
    receiver->send(broadcast_str);
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
    receiver->send(broadcast_str);
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
  player->send(broadcast_str);
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
    receiver->send(out_str);
  }
  
  log(LogSource::SERVER, LogLevel::INFO, "Time set to " + std::to_string(hours) + ":" + (minutes < 10 ? "0" : "") + std::to_string(minutes) + ".");
}

PlayerState* Server::get_player_by_tag(std::string tag) {
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  
  for(auto p : m_players) {
    PlayerState *check = p.second;
    if(check->get_tag() == tag)
      return check;
  }
  return NULL;
}
