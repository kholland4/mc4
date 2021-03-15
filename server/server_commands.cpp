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

std::set<std::string> allowed_privs = {"fast", "fly", "teleport", "settime"};


void Server::cmd_nick(PlayerState *player, std::vector<std::string> args) {
  if(args.size() != 2) {
    chat_send_player(player, "server", "invalid command: wrong number of args, expected '/nick <new_nickname>'");
    return;
  }
  
  std::string new_nick = args[1];
  
  std::regex nick_allow("^[a-zA-Z0-9\\-_]{1,40}$");
  if(!std::regex_match(new_nick, nick_allow)) {
    chat_send_player(player, "server", "invalid nickname: allowed characters are a-z A-Z 0-9 - _ and length must be 1 to 40 characters");
    return;
  }
  
  std::string old_nick = player->get_name();
  player->set_name(new_nick);
  chat_send("server", "*** " + old_nick + " changed name to " + new_nick);
}

void Server::cmd_status(PlayerState *player, std::vector<std::string> args) {
  chat_send_player(player, "server", status());
}

void Server::cmd_time(PlayerState *player, std::vector<std::string> args) {
  if(player->data.privs.find("settime") == player->data.privs.end()) {
    chat_send_player(player, "server", "missing priv: settime");
    return;
  }
  
  if(args.size() != 2) {
    chat_send_player(player, "server", "invalid command: wrong number of args, expected '/time hh:mm'");
    return;
  }
  
  std::string time_spec = args[1];
  
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
}

void Server::cmd_whereami(PlayerState *player, std::vector<std::string> args) {
  std::string world_name = std::to_string(player->pos.world);
  auto search = map.worlds.find(player->pos.world);
  if(search != map.worlds.end()) {
    world_name = search->second->name;
  }
  chat_send_player(player, "server", "(" + std::to_string(player->pos.x) + ", " + std::to_string(player->pos.y) + ", " + std::to_string(player->pos.z) + ") in w=" +
             std::to_string(player->pos.w) + " world=" + world_name + " universe=" + std::to_string(player->pos.universe));
}

void Server::cmd_tp_world(PlayerState *player, std::vector<std::string> args) {
  if(player->data.privs.find("teleport") == player->data.privs.end()) {
    chat_send_player(player, "server", "missing priv: teleport");
    return;
  }
  
  if(args.size() != 2) {
    chat_send_player(player, "server", "invalid command: wrong number of args, expected '/world <name>'");
    return;
  }
  
  for(auto it : map.worlds) {
    if(it.second->name == args[1]) {
      if(player->pos.world == it.first) {
        chat_send_player(player, "server", "You're already here!");
        return;
      }
      player->pos.world = it.first;
      chat_send_player(player, "server", "Welcome to " + it.second->name + "!");
      player->send_pos(m_server);
      if(player->auth) {
        db.update_player_data(player->get_data());
      }
      return;
    }
  }
  chat_send_player(player, "server", "unknown world");
}

void Server::cmd_tp_universe(PlayerState *player, std::vector<std::string> args) {
  if(player->data.privs.find("teleport") == player->data.privs.end()) {
    chat_send_player(player, "server", "missing priv: teleport");
    return;
  }
  
  if(args.size() != 2) {
    chat_send_player(player, "server", "invalid command: wrong number of args, expected '/universe <number>'");
    return;
  }
  
  try {
    int universe = stoi(args[1]);
    
    if(player->pos.universe == universe) {
      chat_send_player(player, "server", "You're already in universe " + std::to_string(universe) + ".");
    } else {
      player->pos.universe = universe;
      chat_send_player(player, "server", "Welcome to universe " + std::to_string(player->pos.universe) + "!");
      player->send_pos(m_server);
      if(player->auth) {
        db.update_player_data(player->get_data());
      }
      return;
    }
  } catch(std::exception const& e) {
    chat_send_player(player, "server", "invalid command: invalid input, expected '/universe <number>'");
  }
}

void Server::cmd_grantme(PlayerState *player, std::vector<std::string> args) {
  if(args.size() != 2) {
    chat_send_player(player, "server", "invalid command: wrong number of args, expected '/grantme <priv>'");
    return;
  }
  
  std::string new_priv = args[1];
  
  if(allowed_privs.find(new_priv) == allowed_privs.end()) {
    chat_send_player(player, "server", "invalid priv");
    return;
  }
  
  if(player->data.privs.find(new_priv) != player->data.privs.end()) {
    chat_send_player(player, "server", "you already have '" + new_priv + "'");
    return;
  }
  
  player->data.privs.insert(new_priv);
  player->send_privs(m_server);
  
  if(player->auth) {
    db.update_player_data(player->get_data());
  }
  
  chat_send_player(player, "server", "granted '" + new_priv + "'");
}
