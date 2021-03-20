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

#ifndef __PLAYER_AUTH_H__
#define __PLAYER_AUTH_H__

#include "database.h"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

using WsServer = websocketpp::server<websocketpp::config::asio>;
using websocketpp::connection_hdl;

class PlayerGenericAuthenticator {
  public:
    virtual ~PlayerGenericAuthenticator() {}
    virtual bool step(std::string message, WsServer& server, connection_hdl& hdl, Database& db) = 0;
    virtual std::string result() = 0;
};

class PlayerPasswordAuthenticator : public PlayerGenericAuthenticator {
  public:
    PlayerPasswordAuthenticator() : auth_success(false) {}
    virtual bool step(std::string message, WsServer& server, connection_hdl& hdl, Database& db);
    virtual std::string result();
  
  private:
    PlayerAuthInfo auth_info;
    bool auth_success;
};



class PlayerAuthenticator {
  public:
    PlayerAuthenticator() : has_backend(false), auth_backend(NULL) {}
    ~PlayerAuthenticator() { if(has_backend) { delete auth_backend; } };
    bool step(std::string message, WsServer& server, connection_hdl& hdl, Database& db);
    std::string result();
  
  private:
    bool has_backend;
    PlayerGenericAuthenticator *auth_backend;
};

#endif
