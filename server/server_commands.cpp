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

#include "player_util.h"

#include <regex>

std::set<std::string> allowed_privs = {"fast", "fly", "teleport", "settime", "give", "creative", "grant"};


void Server::cmd_nick(PlayerState *player, std::vector<std::string> args) {
  std::unique_lock<std::shared_mutex> player_lock_unique(player->lock);
  
  if(args.size() != 2) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "invalid command: wrong number of args, expected '/nick <new_nickname>'");
    return;
  }
  
  std::string new_nick = args[1];
  
  if(!validate_player_name(new_nick)) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", BAD_PLAYER_NAME_MESSAGE);
    return;
  }
  for(auto p : m_players) {
    PlayerState *check = p.second;
    if(check->get_name() == new_nick) {
      player_lock_unique.unlock();
      chat_send_player(player, "server", "that nickname is already in use, try another one?");
      return;
    }
  }
  if(player->data.name == new_nick) {
    //all good, it's the player's own nickname
  } else if(db.player_data_name_used(new_nick)) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "that nickname is already in use, try another one?");
    return;
  }
  
  std::string old_nick = player->get_name();
  player->set_name(new_nick);
  
  player_lock_unique.unlock();
  chat_send("server", "*** " + old_nick + " changed name to " + new_nick);
}

void Server::cmd_status(PlayerState *player, std::vector<std::string> args) {
  chat_send_player(player, "server", status());
}

void Server::cmd_time(PlayerState *player, std::vector<std::string> args) {
  if(!player->has_priv("settime")) {
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
  std::shared_lock<std::shared_mutex> player_lock(player->lock);
  
  std::string world_name = std::to_string(player->pos.world);
  auto search = map.worlds.find(player->pos.world);
  if(search != map.worlds.end()) {
    world_name = search->second->name;
  }
  chat_send_player(player, "server", "(" + std::to_string(player->pos.x) + ", " + std::to_string(player->pos.y) + ", " + std::to_string(player->pos.z) + ") in w=" +
             std::to_string(player->pos.w) + " world=" + world_name + " universe=" + std::to_string(player->pos.universe));
}

void Server::cmd_tp(PlayerState *player, std::vector<std::string> args) {
  std::unique_lock<std::shared_mutex> player_lock_unique(player->lock);
  
  if(!player->has_priv("teleport")) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "missing priv: teleport");
    return;
  }
  
  if(args.size() < 4 || args.size() > 5) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "usage '/tp <x> <y> <z> [<w>]'");
    return;
  }
  
  try {
    int x = stoi(args[1]);
    int y = stoi(args[2]);
    int z = stoi(args[3]);
    int w = 0;
    if(args.size() >= 5) {
      w = stoi(args[4]);
    }
    
    player->pos.x = x;
    player->pos.y = y;
    player->pos.z = z;
    if(args.size() >= 5) {
      player->pos.w = w;
    }
    player->just_tp = true;
    //print ints, not doubles
    player_lock_unique.unlock();
    chat_send_player(player, "server", "Teleported to " + MapPos<int>(x, y, z, player->pos.w, player->pos.world, player->pos.universe).to_string() + "!");
    player_lock_unique.lock();
    player->send_pos(m_server);
    if(player->auth) {
      db.update_player_data(player->get_data());
    }
    return;
  } catch(boost::property_tree::ptree_error const& e) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "invalid command: invalid input, expected '/tp <int x> <int y> <int z> [<int w>]'");
  }
}

void Server::cmd_tp_world(PlayerState *player, std::vector<std::string> args) {
  std::unique_lock<std::shared_mutex> player_lock_unique(player->lock);
  
  if(!player->has_priv("teleport")) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "missing priv: teleport");
    return;
  }
  
  if(args.size() != 2) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "invalid command: wrong number of args, expected '/world <name>'");
    return;
  }
  
  for(auto it : map.worlds) {
    if(it.second->name == args[1]) {
      if(player->pos.world == it.first) {
        player_lock_unique.unlock();
        chat_send_player(player, "server", "You're already here!");
        return;
      }
      player->pos.world = it.first;
      player->just_tp = true;
      player_lock_unique.unlock();
      chat_send_player(player, "server", "Welcome to " + it.second->name + "!");
      player_lock_unique.lock();
      player->send_pos(m_server);
      if(player->auth) {
        db.update_player_data(player->get_data());
      }
      return;
    }
  }
  player_lock_unique.unlock();
  chat_send_player(player, "server", "unknown world");
}

void Server::cmd_tp_universe(PlayerState *player, std::vector<std::string> args) {
  std::unique_lock<std::shared_mutex> player_lock_unique(player->lock);
  
  if(!player->has_priv("teleport")) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "missing priv: teleport");
    return;
  }
  
  if(args.size() != 2) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "invalid command: wrong number of args, expected '/universe <number>'");
    return;
  }
  
  try {
    int universe = stoi(args[1]);
    
    if(player->pos.universe == universe) {
      player_lock_unique.unlock();
      chat_send_player(player, "server", "You're already in universe " + std::to_string(universe) + ".");
    } else {
      player->pos.universe = universe;
      player->just_tp = true;
      player_lock_unique.unlock();
      chat_send_player(player, "server", "Welcome to universe " + std::to_string(player->pos.universe) + "!");
      player_lock_unique.lock();
      player->send_pos(m_server);
      if(player->auth) {
        db.update_player_data(player->get_data());
      }
      return;
    }
  } catch(boost::property_tree::ptree_error const& e) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "invalid command: invalid input, expected '/universe <number>'");
  }
}

void Server::cmd_grant(PlayerState *player, std::vector<std::string> args) {
  std::shared_lock<std::shared_mutex> self_lock(player->lock);
  std::string self_name = player->get_name();
  self_lock.unlock();
  
  if(args.size() != 3) {
    chat_send_player(player, "server", "invalid command: wrong number of args, expected '/grant <player> <priv>'");
    return;
  }
  
  std::string player_name = args[1];
  std::string new_priv = args[2];
  
  if(allowed_privs.find(new_priv) == allowed_privs.end()) {
    chat_send_player(player, "server", "invalid privilege '" + new_priv + "'");
    return;
  }
  
  //Find the requested player.
  PlayerState *player_found = NULL;
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  for(auto it : m_players) {
    PlayerState *player_check = it.second;
    std::shared_lock<std::shared_mutex> player_lock_shared(player->lock);
    if(player_check->get_name() == player_name) {
      player_found = player_check;
      break;
    }
  }
  
  //TODO offline players
  
  if(player_found == NULL) {
    chat_send_player(player, "server", "unknown or offline player '" + player_name + "'");
    return;
  }
  
  std::unique_lock<std::shared_mutex> player_lock_unique(player_found->lock);
  
  if(player_found->has_priv(new_priv)) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "player '" + player_name + "' already has '" + new_priv + "'");
    return;
  }
  
  player_found->data.privs.insert(new_priv);
  player_found->send_privs(m_server);
  
  if(player_found->auth) {
    db.update_player_data(player_found->get_data());
  }
  
  player_lock_unique.unlock();
  chat_send_player(player, "server", "granted '" + new_priv + "' to '" + player_name + "'");
  chat_send_player(player_found, "server", self_name + " granted you '" + new_priv + "'");
}

void Server::cmd_grantme(PlayerState *player, std::vector<std::string> args) {
  std::unique_lock<std::shared_mutex> player_lock_unique(player->lock);
  
  if(args.size() != 2) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "invalid command: wrong number of args, expected '/grantme <priv>'");
    return;
  }
  
  std::string new_priv = args[1];
  
  if(allowed_privs.find(new_priv) == allowed_privs.end()) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "invalid privilege '" + new_priv + "'");
    return;
  }
  
  if(player->has_priv(new_priv)) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "you already have '" + new_priv + "'");
    return;
  }
  
  player->data.privs.insert(new_priv);
  player->send_privs(m_server);
  
  if(player->auth) {
    db.update_player_data(player->get_data());
  }
  
  player_lock_unique.unlock();
  chat_send_player(player, "server", "granted '" + new_priv + "'");
}

void Server::cmd_revoke(PlayerState *player, std::vector<std::string> args) {
  std::shared_lock<std::shared_mutex> self_lock(player->lock);
  std::string self_name = player->get_name();
  self_lock.unlock();
  
  if(args.size() != 3) {
    chat_send_player(player, "server", "invalid command: wrong number of args, expected '/revoke <player> <priv>'");
    return;
  }
  
  std::string player_name = args[1];
  std::string new_priv = args[2];
  
  //Find the requested player.
  PlayerState *player_found = NULL;
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  for(auto it : m_players) {
    PlayerState *player_check = it.second;
    std::shared_lock<std::shared_mutex> player_lock_shared(player->lock);
    if(player_check->get_name() == player_name) {
      player_found = player_check;
      break;
    }
  }
  
  //TODO offline players
  
  if(player_found == NULL) {
    chat_send_player(player, "server", "unknown or offline player '" + player_name + "'");
    return;
  }
  
  std::unique_lock<std::shared_mutex> player_lock_unique(player_found->lock);
  
  auto search = player_found->data.privs.find(new_priv);
  if(search == player_found->data.privs.end()) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "player '" + player_name + "' does not have '" + new_priv + "'");
    return;
  }
  
  player_found->data.privs.erase(search);
  player_found->send_privs(m_server);
  
  if(player_found->auth) {
    db.update_player_data(player_found->get_data());
  }
  
  player_lock_unique.unlock();
  chat_send_player(player, "server", "revoked '" + new_priv + "' from '" + player_name + "'");
  chat_send_player(player_found, "server", self_name + " revoked your privelege '" + new_priv + "'");
}

void Server::cmd_privs(PlayerState *player, std::vector<std::string> args) {
  //TODO: /privs <any player nick or login name>
  
  if(args.size() == 1) {
    std::shared_lock<std::shared_mutex> player_lock(player->lock);
    
    std::ostringstream ss;
    ss << "privileges of " << player->get_name() << ":";
    for(auto p : player->data.privs) {
      ss << " " << p;
    }
    
    chat_send_player(player, "server", ss.str());
    return;
  }
  
  if(args.size() != 2) {
    chat_send_player(player, "server", "invalid command: wrong number of args, expected '/privs' or '/privs <player>'");
    return;
  }
  
  std::string player_name = args[1];
  
  //Find the requested player.
  PlayerState *player_found = NULL;
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  for(auto it : m_players) {
    PlayerState *player_check = it.second;
    std::shared_lock<std::shared_mutex> player_lock_shared(player->lock);
    if(player_check->get_name() == player_name) {
      player_found = player_check;
      break;
    }
  }
  
  //TODO offline players
  
  if(player_found == NULL) {
    chat_send_player(player, "server", "unknown or offline player '" + player_name + "'");
    return;
  }
  
  std::shared_lock<std::shared_mutex> player_lock_unique(player_found->lock);
  
  std::ostringstream ss;
  ss << "privileges of " << player_found->get_name() << ":";
  for(auto p : player_found->data.privs) {
    ss << " " << p;
  }
  
  chat_send_player(player, "server", ss.str());
}

void Server::cmd_giveme(PlayerState *player, std::vector<std::string> args) {
  std::unique_lock<std::shared_mutex> player_lock_unique(player->lock);
  
  if(!player->has_priv("give")) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "missing priv: give");
    return;
  }
  
  if(args.size() < 2) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "please specify an itemstring");
    return;
  }
  
  std::string itemstring = args[1];
  ItemDef def = get_item_def(itemstring);
  if(def.itemstring == "nothing") {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "unknown item '" + itemstring + "'");
    return;
  }
  
  std::ostringstream spec;
  for(size_t i = 1; i < args.size(); i++) {
    if(i > 1)
      spec << " ";
    spec << args[i];
  }
  
  InvStack to_give = InvStack(spec.str());
  
  std::optional<InvPatch> give_patch = inv_calc_give(
          InvRef("player", "null", "main", -1),
          player->inv_get("main"),
          to_give);
  
  player_lock_unique.unlock();
  
  if(!give_patch) {
    chat_send_player(player, "server", "unable to add '" + to_give.spec() + "' to inventory");
    return;
  }
  bool res = inv_apply_patch(*give_patch, player);
  
  if(!res) {
    chat_send_player(player, "server", "unable to add '" + to_give.spec() + "' to inventory");
    return;
  }
  
  chat_send_player(player, "server", "'" + to_give.spec() + "' added to inventory.");
}

void Server::cmd_clearinv(PlayerState *player, std::vector<std::string> args) {
  //Clear own player's inventory (main, craft, craftOutput, hand)
  std::unique_lock<std::shared_mutex> player_lock_unique(player->lock);
  
  std::vector<std::string> to_clear {
    "main", "craft", "craftOutput", "hand"
  };
  
  InvPatch clear_patch;
  
  for(const auto& list_name : to_clear) {
    InvList clear_list = player->inv_get(list_name);
    for(size_t i = 0; i < clear_list.list.size(); i++) {
      clear_patch.diffs.push_back(InvDiff(
          InvRef("player", "null", list_name, i),
          clear_list.list[i],
          InvStack()
      ));
    }
  }
  
  player_lock_unique.unlock();
  
  bool res = inv_apply_patch(clear_patch, player);
  
  if(res)
    chat_send_player(player, "server", "successfully cleared inventory");
  else
    chat_send_player(player, "server", "error when attempting to clear inventory");
}
