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
#include "except.h"
#include "lang.h"
#include "config.h"

#include <regex>

void must_have_privs(PlayerState *player, std::set<std::string> privs) {
  //TODO locking
  
  std::set<std::string> need_privs;
  for(const auto& priv : privs) {
    if(!player->has_priv(priv))
      need_privs.insert(priv);
  }
  
  if(need_privs.size() > 0)
    throw CommandError((need_privs.size() > 1 ? "missing privs: " : "missing priv: ") + lang_fmt_list(need_privs, "and", "'"));
}

void Server::cmd_help(PlayerState *player, std::vector<std::string> args) {
  std::optional<std::string> topic = std::nullopt;
  if(args.size() >= 2)
    topic = args[1];
  
  std::ostringstream help_text;
  
  if(topic) {
    std::string topic_s(*topic);
    
    if(topic_s.rfind("/", 0) != 0)
      topic_s = "/" + topic_s;
    
    auto search = commands_list.find(topic_s);
    if(search == commands_list.end()) {
      help_text << "Unknown command '" + topic_s + "'\n";
      help_text << "{{Back to list|/help}}";
    } else {
      const std::string& name = search->first;
      const ServerCommand& cmd = search->second;
      
      help_text << "Help for '" << name << "'\n";
      help_text << "{{Back to list|/help}}\n";
      help_text << "=====\n";
      help_text << "\n";
      help_text << cmd.help_long;
    }
  } else {
    help_text << "Server command reference\n=====\n\n";
    help_text << "Available commands:";
    
    for(const auto& it : commands_list) {
      const std::string& name = it.first;
      const ServerCommand& cmd = it.second;
      
      help_text << "\n  {{" << name << "|/help " << name << "}} : " << cmd.help_brief;
    }
    
    help_text << "\n\nUse '/help <command>' for help on a specific command";
  }
  
  UISpec help_ui;
  help_ui.components.push_back(
      UI_TextBlock(help_text.str()).to_json());
  
  std::shared_lock<std::shared_mutex> player_lock_shared(player->lock);
  UIInstance current_ui = find_ui(player->get_tag() + " server_cmd_help");
  
  if(current_ui.is_nil) {
    UIInstance help_ui_instance(help_ui);
    help_ui_instance.what_for = player->get_tag() + " server_cmd_help";
    open_ui(player, help_ui_instance);
  } else {
    current_ui.spec = help_ui;
    update_ui(player, current_ui);
  }
}

void Server::cmd_nick(PlayerState *player, std::vector<std::string> args) {
  std::unique_lock<std::shared_mutex> player_lock_unique(player->lock);
  
  if(args.size() != 2)
    throw CommandError("invalid command: wrong number of args, expected '/nick <new_nickname>'");
  
  std::string new_nick = args[1];
  
  if(!validate_player_name(new_nick))
    throw CommandError(BAD_PLAYER_NAME_MESSAGE);
  
  for(auto p : m_players) {
    PlayerState *check = p.second;
    if(check->get_name() == new_nick)
      throw CommandError("that nickname is already in use, try another one?");
  }
  if(player->data.name == new_nick) {
    //all good, it's the player's own nickname
  } else if(db.player_data_name_used(new_nick)) {
    throw CommandError("that nickname is already in use, try another one?");
  }
  
  std::string old_nick = player->get_name();
  player->set_name(new_nick);
  
  player_lock_unique.unlock();
  chat_send("server", "*** " + old_nick + " changed name to " + new_nick);
}

void Server::cmd_status(PlayerState *player, std::vector<std::string> args) {
  chat_send_player(player, "server", status());
}

void Server::cmd_who(PlayerState *player, std::vector<std::string> args) {
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  
  if(player->has_priv("admin")) {
    std::optional<std::string> target_search_name = std::nullopt;
    if(args.size() >= 2)
      target_search_name = args[1];
    
    std::ostringstream who_text;
    
    if(!target_search_name) {
      who_text << "Online players:\n\n";
      
      for(auto const& x : m_players) {
        std::shared_lock<std::shared_mutex> player_lock(x.second->lock);
        // TODO function to escape {{ etc.?
        who_text << "{{" << x.second->get_name() << "|/who " << x.second->get_tag() << "}}";
        if(x.second->auth) {
          who_text << " [id " << x.second->data.auth_id << "]\n";
        } else {
          who_text << " [guest, tag " << x.second->get_tag() << "]\n";
        }
      }
    } else {
      PlayerState *target = find_player(*target_search_name);
      if(target != NULL) {
        std::shared_lock<std::shared_mutex> target_lock(target->lock);
        
        who_text << "Player '" << target->get_name() << "'";
        if(target->auth) {
          who_text << " [id " << target->data.auth_id << "]\n";
        } else {
          who_text << " [guest]\n";
        }
        who_text << "{{Back to list|/who}}\n";
        who_text << "\n";
        
        who_text << "tag: " << target->get_tag() << "\n";
        
        who_text << "privs:";
        for(auto p : target->data.privs) {
          who_text << " " << p;
        }
        if(target->data.privs.size() == 0)
          who_text << " (none)";
        who_text << "\n";
        
        who_text << "pos: " << target->pos << "\n";
        who_text << "creative_mode: " << std::boolalpha << target->data.creative_mode << "\n";
        who_text << "\n";
        who_text << "Actions:\n";
        who_text << "  {{/kick " << target->get_tag() << "|/kick " << target->get_tag() << "}}";
      } else {
        who_text << "Cannot find player '" << *target_search_name << "'\n";
        who_text << "Have they disconnected?";
      }
    }
    
    UISpec who_ui;
    who_ui.components.push_back(
        UI_TextBlock(who_text.str()).to_json());
    
    std::shared_lock<std::shared_mutex> player_lock_shared(player->lock);
    UIInstance current_ui = find_ui(player->get_tag() + " server_cmd_who");
    
    if(current_ui.is_nil) {
      UIInstance who_ui_instance(who_ui);
      who_ui_instance.what_for = player->get_tag() + " server_cmd_who";
      open_ui(player, who_ui_instance);
    } else {
      current_ui.spec = who_ui;
      update_ui(player, current_ui);
    }
  } else {
    std::ostringstream s;
    s << "{";
    
    bool first = true;
    for(auto const& x : m_players) {
      if(!first) { s << ", "; }
      first = false;
      
      std::shared_lock<std::shared_mutex> player_lock(x.second->lock);
      s << x.second->get_name();
    }
    
    s << "}";
    chat_send_player(player, "server", s.str());
  }
}

void Server::cmd_time(PlayerState *player, std::vector<std::string> args) {
  if(args.size() == 1) {
    std::ostringstream time_s;
    time_s << "time of day is ";
    time_s << server_time.hours;
    time_s << ":" << std::setw(2) << std::setfill('0') << server_time.minutes;
    chat_send_player(player, "server", time_s.str());
    return;
  }
  
  //set time
  must_have_privs(player, {"settime"});
  
  if(args.size() != 2)
    throw CommandError("invalid command: wrong number of args, expected '/time' or '/time hh:mm'");
  
  std::string time_spec = args[1];
  
  std::regex time_allow("^((|0|1)[0-9]|2[0-3]):[0-5][0-9]$");
  if(!std::regex_match(time_spec, time_allow))
    throw CommandError("invalid /time command: please use '/time hh:mm' in 24-hour format");
  
  auto colon_pos = time_spec.find(":");
  std::string hours_str = time_spec.substr(0, colon_pos);
  std::string minutes_str = time_spec.substr(colon_pos + 1);
  
  int hours, minutes;
  try {
    hours = std::stoi(hours_str);
    minutes = std::stoi(minutes_str);
  } catch(std::invalid_argument const& e) {
    throw CommandError("not a number: " + hours_str + " or " + minutes_str);
  } catch(std::out_of_range const& e) {
    throw CommandError("invalid (too large) number: " + hours_str + " or " + minutes_str);
  }
  set_time(hours, minutes);
  
  chat_send_player(player, "server", "time set to " + std::to_string(hours) + ":" + (minutes < 10 ? "0" : "") + std::to_string(minutes));
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
  
  must_have_privs(player, {"teleport"});
  
  if(args.size() < 4 || args.size() > 5)
    throw CommandError("usage '/tp <x> <y> <z> [<w>]'");
  
  int args_int[4] = {0, 0, 0, 0};
  for(size_t i = 1; i < std::min((size_t)5, args.size()); i++) {
    try {
      args_int[i - 1] = stoi(args[i]);
    } catch(std::invalid_argument const& e) {
      throw CommandError("not a number: " + args[i]);
    } catch(std::out_of_range const& e) {
      throw CommandError("invalid (too large) number: " + args[i]);
    }
  }
  int x = args_int[0];
  int y = args_int[1];
  int z = args_int[2];
  int w = args_int[3];
  
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
  player->send_pos();
  if(player->auth) {
    db.update_player_data(player->get_data());
  }
}

void Server::cmd_tp_world(PlayerState *player, std::vector<std::string> args) {
  std::unique_lock<std::shared_mutex> player_lock_unique(player->lock);
  
  must_have_privs(player, {"teleport"});
  
  if(args.size() != 2)
    throw CommandError("invalid command: wrong number of args, expected '/world <name>'");
  
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
      player->send_pos();
      if(player->auth) {
        db.update_player_data(player->get_data());
      }
      return;
    }
  }
  throw CommandError("unknown world");
}

void Server::cmd_tp_universe(PlayerState *player, std::vector<std::string> args) {
  std::unique_lock<std::shared_mutex> player_lock_unique(player->lock);
  
  must_have_privs(player, {"teleport"});
  
  if(args.size() != 2)
    throw CommandError("invalid command: wrong number of args, expected '/universe <number>'");
  
  int universe;
  try {
    universe = stoi(args[1]);
  } catch(std::invalid_argument const& e) {
    throw CommandError("not a number: " + args[1]);
  } catch(std::out_of_range const& e) {
    throw CommandError("invalid (too large) number: " + args[1]);
  }
  
  if(player->pos.universe == universe) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "You're already in universe " + std::to_string(universe) + ".");
  } else {
    player->pos.universe = universe;
    player->just_tp = true;
    player_lock_unique.unlock();
    chat_send_player(player, "server", "Welcome to universe " + std::to_string(player->pos.universe) + "!");
    player_lock_unique.lock();
    player->send_pos();
    if(player->auth) {
      db.update_player_data(player->get_data());
    }
  }
}

void Server::grant_revoke_common(PlayerState *player, std::string target_search_name, std::set<std::string> grant_privs, std::set<std::string> revoke_privs) {
  if(grant_privs.size() == 0 && revoke_privs.size() == 0) {
    throw CommandError("please specify privileges");
  }
  
  // check that privs to be granted are valid
  std::set<std::string> invalid_privs;
  for(const auto& to_grant : grant_privs) {
    if(allowed_privs.find(to_grant) == allowed_privs.end()) {
      invalid_privs.insert(to_grant);
    }
  }
  if(invalid_privs.size() > 0)
    throw CommandError("cannot grant " + lang_fmt_list(invalid_privs, "or", "'") +
                       ": not " + std::string(invalid_privs.size() > 1 ? "a ": "") + "valid privilege" + std::string(invalid_privs.size() > 1 ? "s": ""));
  
  // check that player has permission to grant/revoke them
  std::string self_name;
  {
    std::shared_lock<std::shared_mutex> self_lock(player->lock);
    self_name = player->get_name();
    
    for(const auto& to_grant : grant_privs) {
      std::set<std::string> req{"grant"};
      auto req_search = required_to_grant.find(to_grant);
      if(req_search != required_to_grant.end())
        req = req_search->second;
      
      bool player_has_req = false;
      for(const auto& it : req) {
        if(player->has_priv(it)) {
          player_has_req = true;
          break;
        }
      }
      
      if(!player_has_req)
        throw CommandError("you must have " + lang_fmt_list(req, "or", "'") + " to grant '" + to_grant + "'");
    }
    
    for(const auto& to_revoke : revoke_privs) {
      if(allowed_privs.find(to_revoke) == allowed_privs.end()) {
        if(!player->has_priv("admin")) {
          throw CommandError("you must have 'admin' to revoke an unknown privilege ('" + to_revoke + "')");
        }
      }
      
      std::set<std::string> req{"grant"};
      auto req_search = required_to_grant.find(to_revoke);
      if(req_search != required_to_grant.end())
        req = req_search->second;
      
      bool player_has_req = false;
      for(const auto& it : req) {
        if(player->has_priv(it)) {
          player_has_req = true;
          break;
        }
      }
      
      if(!player_has_req)
        throw CommandError("you must have " + lang_fmt_list(req, "or", "'") + " to revoke '" + to_revoke + "'");
    }
  }
  
  //Find the requested player.
  PlayerState *target = find_player(target_search_name);
  
  //TODO offline players
  
  if(target == NULL)
    throw CommandError("unknown or offline player '" + target_search_name + "'");
  
  std::unique_lock<std::shared_mutex> player_lock_unique(target->lock);
  std::string target_name = target->get_name();
  
  // do grants
  std::set<std::string> already_have;
  std::set<std::string> do_grant;
  for(const auto& to_grant : grant_privs) {
    if(target->has_priv(to_grant))
      already_have.insert(to_grant);
    else
      do_grant.insert(to_grant);
  }
  
  if(do_grant.size() > 0) {
    for(const auto& to_grant : do_grant)
      target->data.privs.insert(to_grant);
  }
  
  // do revokes
  std::set<std::string> already_revoke;
  std::set<std::string> do_revoke;
  for(const auto& to_revoke : revoke_privs) {
    if(!target->has_priv(to_revoke))
      already_revoke.insert(to_revoke);
    else
      do_revoke.insert(to_revoke);
  }
  
  if(do_revoke.size() > 0) {
    for(const auto& to_revoke : do_revoke)
      target->data.privs.erase(to_revoke);
  }
  
  // save changes
  if(do_grant.size() > 0 || do_revoke.size() > 0) {
    target->send_privs();
    if(target->auth)
      db.update_player_data(target->get_data());
  }
  
  player_lock_unique.unlock();
  
  // show messages for grants
  if(do_grant.size() > 0) {
    std::string grants = lang_fmt_list(do_grant, "and", "'");
    if(player == target) {
      chat_send_player(player, "server", "granted " + grants);
    } else {
      chat_send_player(player, "server", "granted " + grants + " to " + target_name);
      chat_send_player(target, "server", self_name + " granted you " + grants);
    }
  }
  if(already_have.size() > 0) {
    std::string have = lang_fmt_list(already_have, "and", "'");
    if(player == target)
      chat_send_player(player, "server", "you already have " + have);
    else
      chat_send_player(player, "server", target_name + " already has " + have);
  }
  
  // show messages for revokes
  if(do_revoke.size() > 0) {
    std::string revokes = lang_fmt_list(do_revoke, "and", "'");
    if(player == target) {
      chat_send_player(player, "server", "revoked " + revokes);
    } else {
      chat_send_player(player, "server", "revoked " + revokes + " from " + target_name);
      chat_send_player(target, "server", self_name + " revoked your privilege" + std::string(do_revoke.size() > 1 ? "s ": " ") + revokes);
    }
  }
  if(already_revoke.size() > 0) {
    std::string missing = lang_fmt_list(already_revoke, "or", "'");
    if(player == target)
      chat_send_player(player, "server", "you don't have " + missing);
    else
      chat_send_player(player, "server", target_name + " doesn't have " + missing);
  }
}

void Server::cmd_grant(PlayerState *player, std::vector<std::string> args) {
  if(args.size() == 1) {
    std::ostringstream out;
    out << "valid privileges:";
    for(const auto& p : allowed_privs) {
      out << " " << p;
    }
    
    chat_send_player(player, "server", out.str());
    return;
  }
  
  if(args.size() < 3)
    throw CommandError("invalid command: wrong number of args, expected '/grant <player> <priv>...'");
  
  std::string target_name = args[1];
  std::set<std::string> new_privs{args.begin() + 2, args.end()};
  grant_revoke_common(player, target_name, new_privs, std::set<std::string>());
}

void Server::cmd_grantme(PlayerState *player, std::vector<std::string> args) {
  std::string self_name;
  {
    std::shared_lock<std::shared_mutex> player_lock_shared(player->lock);
    self_name = player->get_name();
  }
  
  if(args.size() < 2)
    throw CommandError("invalid command: wrong number of args, expected '/grantme <priv>...'");
  
  std::string target_name = self_name;
  std::set<std::string> new_privs{args.begin() + 1, args.end()};
  try {
    grant_revoke_common(player, target_name, new_privs, std::set<std::string>());
  } catch(const CommandError& e) {
    chat_send_player(player, "server", e.what());
  }
}

void Server::cmd_revoke(PlayerState *player, std::vector<std::string> args) {
  if(args.size() < 3)
    throw CommandError("invalid command: wrong number of args, expected '/revoke <player> <priv>...'");
  
  std::string target_name = args[1];
  std::set<std::string> revoke_privs{args.begin() + 2, args.end()};
  grant_revoke_common(player, target_name, std::set<std::string>(), revoke_privs);
}

void Server::cmd_revokeme(PlayerState *player, std::vector<std::string> args) {
  std::string self_name;
  {
    std::shared_lock<std::shared_mutex> player_lock_shared(player->lock);
    self_name = player->get_name();
  }
  
  if(args.size() < 2)
    throw CommandError("invalid command: wrong number of args, expected '/revokeme <priv>...'");
  
  std::string target_name = self_name;
  std::set<std::string> revoke_privs{args.begin() + 1, args.end()};
  grant_revoke_common(player, target_name, std::set<std::string>(), revoke_privs);
}

void Server::cmd_privs(PlayerState *player, std::vector<std::string> args) {
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
  
  if(args.size() != 2)
    throw CommandError("invalid command: wrong number of args, expected '/privs' or '/privs <player>'");
  
  std::string player_name = args[1];
  
  //Find the requested player.
  PlayerState *player_found = find_player(player_name);
  
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
  
  must_have_privs(player, {"give"});
  
  if(args.size() < 2)
    throw CommandError("please specify an itemstring");
  
  std::string itemstring = args[1];
  ItemDef def = get_item_def(itemstring);
  if(def.itemstring == "nothing")
    throw CommandError("unknown item '" + itemstring + "'");
  
  std::ostringstream spec;
  for(size_t i = 1; i < args.size(); i++) {
    if(i > 1)
      spec << " ";
    spec << args[i];
  }
  
  InvStack to_give = InvStack(spec.str());
  
  std::optional<InvPatch> give_patch = inv_calc_give(
          InvRef("player", std::nullopt, "main", std::nullopt),
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

void Server::cmd_give(PlayerState *player, std::vector<std::string> args) {
  std::string self_name;
  {
    std::shared_lock<std::shared_mutex> player_lock_shared(player->lock);
    self_name = player->get_name();
  }
  
  must_have_privs(player, {"give"});
  
  if(args.size() < 3)
    throw CommandError("usage: '/give <player> <itemstring>'");
  
  std::string player_name = args[1];
  
  //Find the requested player.
  PlayerState *player_found = find_player(player_name);
  
  //TODO offline players?
  
  if(player_found == NULL)
    throw CommandError("unknown or offline player '" + player_name + "'");
  
  std::unique_lock<std::shared_mutex> target_lock_unique(player_found->lock);
  
  std::string itemstring = args[2];
  ItemDef def = get_item_def(itemstring);
  if(def.itemstring == "nothing")
    throw CommandError("unknown item '" + itemstring + "'");
  
  std::ostringstream spec;
  for(size_t i = 2; i < args.size(); i++) {
    if(i > 2)
      spec << " ";
    spec << args[i];
  }
  
  InvStack to_give = InvStack(spec.str());
  
  std::optional<InvPatch> give_patch = inv_calc_give(
          InvRef("player", std::nullopt, "main", std::nullopt),
          player_found->inv_get("main"),
          to_give);
  
  if(!give_patch) {
    chat_send_player(player, "server", "unable to give '" + to_give.spec() + "' to '" + player_name + "'");
    return;
  }
  
  target_lock_unique.unlock();
  bool res = inv_apply_patch(*give_patch, player_found);
  
  if(!res) {
    chat_send_player(player, "server", "unable to give '" + to_give.spec() + "' to '" + player_name + "'");
    return;
  }
  
  chat_send_player(player, "server", "gave '" + to_give.spec() + "' to '" + player_name + "'");
  chat_send_player(player_found, "server", self_name + " gave you '" + to_give.spec() + "'");
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
          InvRef("player", std::nullopt, list_name, i),
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

void Server::cmd_creative(PlayerState *player, std::vector<std::string> args) {
  std::unique_lock<std::shared_mutex> player_lock_unique(player->lock);
  
  if(args.size() < 2) {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "creative is " + std::string(player->data.creative_mode ? "on" : "off"));
    return;
  }
  
  std::string mode = args[1];
  if(mode != "on" && mode != "off")
    throw CommandError("usage: /creative [on|off]");
  
  must_have_privs(player, {"creative"});
  
  bool old_creative_mode = player->data.creative_mode;
  
  if(mode == "on")
    player->data.creative_mode = true;
  if(mode == "off")
    player->data.creative_mode = false;
  
  if(player->data.creative_mode != old_creative_mode) {
    if(player->auth) {
      db.update_player_data(player->get_data());
    }
    player->send_opts();
    
    player_lock_unique.unlock();
    chat_send_player(player, "server", "creative mode is now " + mode);
  } else {
    player_lock_unique.unlock();
    chat_send_player(player, "server", "creative mode is already " + mode);
  }
}

void Server::cmd_kick(PlayerState *player, std::vector<std::string> args) {
  must_have_privs(player, {"kick"});
  
  if(args.size() < 2 || args.size() > 3)
    throw CommandError("usage: '/kick <player> [<message>]");
  
  std::string target_search_name = args[1];
  std::string message = get_config<std::string>("player.default_kick_message");
  if(args.size() >= 3)
    message = args[2];
  
  PlayerState *target = find_player(target_search_name);
  
  if(target == NULL)
    throw CommandError("unknown or offline player '" + target_search_name + "'");
  
  {
    std::unique_lock<std::shared_mutex> player_lock(target->lock);
    
    connection_hdl hdl = target->m_connection_hdl;
    
    try {
      //on_close will be run later, so no worries about deadlocks
      m_server.close(hdl, websocketpp::close::status::going_away, message);
      chat_send_player(player, "server", "Kicked player '" + target->get_name() + "'");
    } catch(websocketpp::exception const& e) {
      log(LogSource::SERVER, LogLevel::ERR, "Socket error: " + std::string(e.what()));
      chat_send_player(player, "server", "Socket error while kicking player '" + target->get_name() + "', they may have disconnected");
    }
  }
}
