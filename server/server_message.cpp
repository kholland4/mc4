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
#include "tool.h"
#include "craft.h"

std::optional<std::pair<InvStack, InvStack>> inv_calc_distribute(InvStack stack1, int qty1, InvStack stack2, int qty2) {
  if(stack1.is_nil && stack2.is_nil)
    return std::nullopt; //nothing to distribute
  
  if(!stack1.is_nil && !stack2.is_nil) {
    if(stack1.itemstring != stack2.itemstring)
      return std::nullopt; //mismatched stack types
  }
  
  InvStack combined;
  if(!stack1.is_nil) {
    combined = stack1;
    if(!stack2.is_nil)
      combined.count += stack2.count;
  } else {
    combined = stack2;
    if(!stack1.is_nil)
      combined.count += stack1.count;
  }
  
  ItemDef def = get_item_def(combined.itemstring);
  if(def.itemstring == "nothing")
    return std::nullopt;
  if(!def.stackable)
    return std::nullopt;
  
  if(qty1 + qty2 != combined.count)
    return std::nullopt;
  
  if(qty1 > def.max_stack || qty2 > def.max_stack)
    return std::nullopt;
  
  InvStack new_stack1 = combined;
  new_stack1.count = qty1;
  InvStack new_stack2 = combined;
  new_stack2.count = qty2;
  
  if(new_stack1.count == 0)
    new_stack1 = InvStack();
  if(new_stack2.count == 0)
    new_stack2 = InvStack();
  
  return std::make_pair(new_stack1, new_stack2);
}

std::optional<InvPatch> inv_interact_override(const InvRef& ref1, const InvStack& orig1, const InvRef& ref2, const InvStack& orig2, const std::string& req_id, PlayerState *player)
{
  InvPatch deny_patch(req_id);
  deny_patch.diffs.push_back(
      InvDiff(ref1, orig1, orig1));
  deny_patch.diffs.push_back(
      InvDiff(ref2, orig2, orig2));
  deny_patch.make_deny();
  
  if(ref1 == ref2)
    return deny_patch;
  
  //Creative inventory.
  if(
    (ref1.obj_type == "player" && ref1.list_name == "creative") &&
    (ref2.obj_type == "player" && ref2.list_name == "creative")
  ) {
    return deny_patch;
  }
  
  if(
    (ref1.obj_type == "player" && ref1.list_name == "creative") ||
    (ref2.obj_type == "player" && ref2.list_name == "creative")
  ) {
    if(!player->has_priv("creative"))
      return deny_patch;
    
    InvRef creative_ref = ref1;
    InvRef other_ref = ref2;
    InvStack creative_stack = orig1;
    InvStack other_stack = orig2;
    
    if(ref2.obj_type == "player" && ref2.list_name == "creative") {
      creative_ref = ref2;
      other_ref = ref1;
      creative_stack = orig2;
      other_stack = orig1;
    }
    
    if(other_stack.is_nil) {
      //Put the creative item in the other ref, which is empty
      InvPatch creative_patch(req_id);
      creative_patch.diffs.push_back(
          InvDiff(other_ref, other_stack, creative_stack));
      return creative_patch;
    } else if(!creative_stack.is_nil && creative_stack.itemstring == other_stack.itemstring) {
      //Combine -> other ref
      InvStack combined_stack(other_stack);
      ItemDef def = get_item_def(combined_stack.itemstring);
      combined_stack.count += creative_stack.count;
      if(combined_stack.count > def.max_stack)
        combined_stack.count = def.max_stack;
      
      InvPatch creative_patch(req_id);
      creative_patch.diffs.push_back(
          InvDiff(other_ref, other_stack, combined_stack));
      return creative_patch;
    } else {
      //Destroy the item in the other ref
      InvPatch creative_patch(req_id);
      creative_patch.diffs.push_back(
          InvDiff(other_ref, other_stack, InvStack()));
      return creative_patch;
    }
  }
  
  //Craft output.
  if(
    (ref1.obj_type == "player" && ref1.list_name == "craftOutput") &&
    (ref2.obj_type == "player" && ref2.list_name == "craftOutput")
  ) {
    return deny_patch;
  }
  
  if(
    (ref1.obj_type == "player" && ref1.list_name == "craftOutput") ||
    (ref2.obj_type == "player" && ref2.list_name == "craftOutput")
  ) {
    InvRef output_ref = ref1;
    InvRef other_ref = ref2;
    InvStack output_stack = orig1;
    InvStack other_stack = orig2;
    
    if(ref2.obj_type == "player" && ref2.list_name == "craftOutput") {
      output_ref = ref2;
      other_ref = ref1;
      output_stack = orig2;
      other_stack = orig1;
    }
    
    InvList craft_input = player->inv_get("craft");
    InvStack craft_output = player->inv_get("craftOutput", 0);
    
    std::optional<std::pair<InvPatch, InvPatch>> craft_res
        = craft_calc_result(craft_input, std::pair<int, int>(3, 3));
    
    if(!craft_res) {
      //Deny the patch and instead empty the craft output
      InvPatch cleanup_patch(req_id);
      cleanup_patch.diffs.push_back(
          InvDiff(output_ref, output_stack, InvStack()));
      cleanup_patch.diffs.push_back(
          InvDiff(other_ref, other_stack, other_stack));
      cleanup_patch.make_deny();
      return cleanup_patch;
    }
    
    InvStack craft_res_stack = (*craft_res).second.diffs[0].current;
    if(craft_res_stack != output_stack || craft_res_stack != craft_output) {
      //Deny the patch and instead fix the craft output
      InvPatch fix_patch(req_id);
      fix_patch.diffs.push_back(
          InvDiff(output_ref, output_stack, craft_res_stack));
      fix_patch.diffs.push_back(
          InvDiff(other_ref, other_stack, other_stack));
      fix_patch.make_deny();
      return fix_patch;
    }
    
    ItemDef craft_res_def = get_item_def(craft_res_stack.itemstring);
    
    if(!other_stack.is_nil && other_stack.itemstring != output_stack.itemstring)
      return deny_patch;
    
    if(!other_stack.is_nil && (other_stack.count + output_stack.count > craft_res_def.max_stack || !craft_res_def.stackable))
      return deny_patch;
    
    //Make the patch to consume craft input and give the output to the player
    InvPatch craft_success_patch(req_id);
    if(other_stack.is_nil) {
      craft_success_patch.diffs.push_back(
          InvDiff(other_ref, other_stack, output_stack));
      craft_success_patch.diffs.push_back(
          InvDiff(output_ref, output_stack, InvStack()));
    } else {
      InvStack combined_stack(other_stack);
      combined_stack.count += output_stack.count;
      craft_success_patch.diffs.push_back(
          InvDiff(other_ref, other_stack, combined_stack));
      craft_success_patch.diffs.push_back(
          InvDiff(output_ref, output_stack, InvStack()));
    }
    
    const InvPatch& consume_patch = (*craft_res).first;
    for(auto diff : consume_patch.diffs) {
      craft_success_patch.diffs.push_back(diff);
    }
    
    return craft_success_patch;
  }
  
  return std::nullopt;
}

std::optional<InvPatch> update_craft_if_needed(const InvRef& ref1, const InvRef& ref2, PlayerState *player) {
  //Craft grid output.
  if(
    (ref1.obj_type == "player" && ref1.list_name == "craft") ||
    (ref2.obj_type == "player" && ref2.list_name == "craft") ||
    (ref1.obj_type == "player" && ref1.list_name == "craftOutput") ||
    (ref2.obj_type == "player" && ref2.list_name == "craftOutput")
  ) {
    InvList craft_input = player->inv_get("craft");
    InvStack craft_output = player->inv_get("craftOutput", 0);
    
    std::optional<std::pair<InvPatch, InvPatch>> craft_res
        = craft_calc_result(craft_input, std::pair<int, int>(3, 3));
    InvStack craft_res_stack;
    if(craft_res)
      craft_res_stack = (*craft_res).second.diffs[0].current;
    
    if(craft_output != craft_res_stack) {
      InvRef craft_output_ref("player", "null", "craftOutput", 0);
      InvPatch out;
      out.diffs.push_back(
          InvDiff(craft_output_ref, craft_output, craft_res_stack));
      return out;
    }
  }
  
  return std::nullopt;
}

void Server::on_message(connection_hdl hdl, websocketpp::config::asio::message_type::ptr msg) {
  std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
  auto search = m_players.find(hdl);
  if(search == m_players.end()) {
    log(LogSource::SERVER, LogLevel::ERR, "Unable to find player state for connection!");
    return;
  }
  PlayerState *player = search->second;
  
  std::unique_lock<std::shared_mutex> player_lock_unique(player->lock);
  
  try {
    std::stringstream ss;
    ss << msg->get_payload();
    
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(ss, pt);
    
    std::string type = pt.get<std::string>("type");
    
    if(!player->auth && !player->auth_guest) {
      //TODO send reply saying unauthorized (if not an auth message)
      
      if(type == "auth_guest") {
        PlayerData new_data;
        new_data.name = player->get_tag();
        new_data.is_nil = false;
        init_player_data(new_data);
        player->load_data(new_data);
        
        player->auth_guest = true;
        
        m_server.send(hdl, "{\"type\":\"auth_guest\",\"message\":\"guest_ok\"}", websocketpp::frame::opcode::text);
      } else {
        //Authenticate to an account.
        
        bool is_auth = player->auth_state.step(msg->get_payload(), m_server, hdl, db);
        if(is_auth) {
          std::string auth_id = player->auth_state.result();
          
          for(auto it : m_players) {
            PlayerState *p = it.second;
            if(p->auth && p->data.auth_id == auth_id) {
              player_lock_unique.unlock();
              chat_send_player(player, "server", "ERROR: player '" + p->data.name + "' is already connected");
              websocketpp::lib::error_code ec;
              m_server.close(hdl, websocketpp::close::status::policy_violation, "kick", ec);
              if(ec) {
                log(LogSource::SERVER, LogLevel::ERR, "error closing connection: " + ec.message());
              }
              return;
            }
          }
          
          PlayerData pdata = db.fetch_player_data(auth_id);
          if(pdata.is_nil) {
            player_lock_unique.unlock();
            chat_send_player(player, "server", "internal error: no such player");
            log(LogSource::SERVER, LogLevel::ERR, "no such player (auth id " + auth_id + ")");
            return;
          }
          player->load_data(pdata);
          player->auth = true;
        }
      }
      
      if(player->auth || player->auth_guest) {
        player->send_pos(m_server);
        player->send_privs(m_server);
        player->send_opts(m_server);
        
        player->prepare_nearby_mapblocks(2, 3, 0, map);
        player->prepare_nearby_mapblocks(1, 2, 1, map);
        
        player_lock_unique.unlock();
        
        send_inv(player);
        
        log(LogSource::SERVER, LogLevel::INFO, player->get_name() + " (from " + player->address_and_port + ") connected!");
        chat_send_player(player, "server", status());
        chat_send("server", "*** " + player->get_name() + " joined the server!");
      }
      
      return;
    }
    
    //message starts with "auth"
    if(type.rfind("auth", 0) == 0 && player->auth) {
      bool is_auth = player->auth_state.step(msg->get_payload(), m_server, hdl, db);
      if(!is_auth) {
        db.update_player_data(player->get_data());
        player->auth = false;
      }
      
      return;
    }
    
    if(type == "req_mapblock") {
      MapPos<int> mb_pos(pt.get<int>("pos.x"), pt.get<int>("pos.y"), pt.get<int>("pos.z"), pt.get<int>("pos.w"), player->pos.world, player->pos.universe);
      
      MapPos<int> player_mb = player->containing_mapblock();
      MapBox<int> bounding(player_mb - PLAYER_LIMIT_VIEW_DISTANCE, player_mb + PLAYER_LIMIT_VIEW_DISTANCE);
      if(!bounding.contains(mb_pos)) {
        log(LogSource::SERVER, LogLevel::NOTICE, "Player '" + player->get_name() + "' requests out of bounds mapblock at " + mb_pos.to_string());
        return;
      }
      
      MapblockUpdateInfo info = map.get_mapblockupdateinfo(mb_pos);
      if(info.light_needs_update > 0) {
#ifdef DEBUG_PERF
        std::cout << "single mapblock prep for " << player->get_name() << std::endl;
#endif
        map.update_mapblock_light(info);
      }
      MapblockCompressed *mbc = map.get_mapblock_compressed(mb_pos);
#ifdef DEBUG_NET
      unsigned int len = player->send_mapblock_compressed(mbc, m_server);
      
      {
        std::unique_lock<std::shared_mutex> net_lock(net_debug_lock);
        mb_out_len += len;
        mb_out_count++;
      }
#else
      player->send_mapblock_compressed(mbc, m_server);
#endif
      delete mbc;
    } else if(type == "set_player_pos") {
      if(player->just_tp) {
        //Clear position history and ignore this position update.
        player->pos_history.clear();
        player->just_tp = false;
        return;
      }
      
      MapPos<double> pos(pt.get<double>("pos.x"), pt.get<double>("pos.y"), pt.get<double>("pos.z"), pt.get<int>("pos.w"), player->pos.world, player->pos.universe);
      
      std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
      
#ifndef DISABLE_PLAYER_SPEED_LIMIT
      //Validate that the player hasn't moved too far since their last position update
      //FIXME: doesn't handle falling players
      bool violation = false;
      
      if(player->pos_history.size() > 0) {
        MapPos<double> total_distance(0, 0, 0, 0, 0, 0);
        MapPos<double> total_distance_sign(0, 0, 0, 0, 0, 0);
        MapPos<double> previous = pos;
        for(auto it : player->pos_history) {
          total_distance += (previous - it.second).abs();
          total_distance_sign += (previous - it.second);
          previous = it.second;
        }
        
        std::chrono::time_point<std::chrono::steady_clock> oldest = player->pos_history.back().first;
        double diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - oldest).count() / 10000.0; //milliseconds -> tens of seconds
        if(diff > 0) {
          MapPos<double> vel(total_distance.x / diff, total_distance.y / diff, total_distance.z / diff,
                             total_distance.w / diff, total_distance.world / diff, total_distance.universe / diff);
          MapPos<double> vel_sign(total_distance_sign.x / diff, total_distance_sign.y / diff, total_distance_sign.z / diff,
                                  total_distance_sign.w / diff, total_distance_sign.world / diff, total_distance_sign.universe / diff);
          
          MapPos<int> limit = PLAYER_LIMIT_VEL;
          if(player->data.has_priv("fast")) {
            limit = PLAYER_LIMIT_VEL_FAST;
          }
          
          if(vel_sign.y < -limit.y) {
            //falling player, probably
            //FIXME
            limit.y *= 5;
          }
          
          if(vel.x > limit.x || vel.y > limit.y || vel.z > limit.z ||
             vel.w > limit.w || vel.world > limit.world || vel.universe > limit.universe) {
            violation = true;
          }
        }
      }
      
      if(violation) {
        log(LogSource::SERVER, LogLevel::WARNING, "player '" + player->get_name() + "' tried to move too fast");
        player->send_pos(m_server); //pull them back
        return; //don't accept the new position
      }
#endif
      
      player->pos_history.push_front(std::make_pair(now, pos));
      if(player->pos_history.size() > 8) {
        player->pos_history.pop_back();
      }
      
      player->pos = pos;
      player->vel.set(pt.get<double>("vel.x"), pt.get<double>("vel.y"), pt.get<double>("vel.z"), player->vel.w, player->vel.world, player->vel.universe);
      player->rot.set(pt.get<double>("rot.x"), pt.get<double>("rot.y"), pt.get<double>("rot.z"), pt.get<double>("rot.w"));
      
      if(player->auth) {
        db.update_player_data(player->get_data());
      }
      
      player->prepare_nearby_mapblocks(2, 3, 0, map);
      player->prepare_nearby_mapblocks(1, 2, 1, map);
    } else if(type == "dig_node") {
      MapPos<int> pos(pt.get<int>("pos.x"), pt.get<int>("pos.y"), pt.get<int>("pos.z"), pt.get<int>("pos.w"), pt.get<int>("pos.world"), pt.get<int>("pos.universe"));
      int wield_index = pt.get<int>("wield");
      Node existing(pt.get<std::string>("existing.itemstring"), pt.get<unsigned int>("existing.rot"));
      
      InvStack wield_stack = player->inv_get("main", wield_index);
      Node target = map.get_node(pos);
      
      if(existing != target) {
        log(LogSource::SERVER, LogLevel::NOTICE, "Player '" + player->get_name()
                                                 + " attempted to dig node '" + existing.itemstring + "' at " + pos.to_string() + " but it has since changed");
        //TODO tell the client
        return;
      }
      
      MapPos<int> player_pos_int((int)std::round(player->pos.x), (int)std::round(player->pos.y), (int)std::round(player->pos.z), player->pos.w, player->pos.world, player->pos.universe);
      MapBox<int> bounding(player_pos_int - PLAYER_LIMIT_REACH_DISTANCE, player_pos_int + PLAYER_LIMIT_REACH_DISTANCE);
      if(!bounding.contains(pos)) {
        log(LogSource::SERVER, LogLevel::NOTICE, "Player '" + player->get_name() + "' at " + player_pos_int.to_string()
                                                 + " attempted to dig node '" + target.itemstring + "' far away at " + pos.to_string());
        return;
      }
      
      NodeDef target_def = get_node_def(target.itemstring);
      if(target_def.itemstring == "nothing") {
        log(LogSource::SERVER, LogLevel::NOTICE, "Player '" + player->get_name() + "' attempted to dig nonexistent node '" + target.itemstring + "' at " + pos.to_string());
        return;
      }
      
      if(!target_def.breakable) {
        log(LogSource::SERVER, LogLevel::NOTICE, "Player '" + player->get_name() + "' attempted to dig unbreakable node '" + target.itemstring + "' at " + pos.to_string());
        return;
      }
      
      std::optional<double> break_time_tool = calc_dig_time(target, wield_stack);
      std::optional<double> break_time_hand = calc_dig_time(target, InvStack());
      
      if(!break_time_tool && !break_time_hand) {
        log(LogSource::SERVER, LogLevel::NOTICE, "Player '" + player->get_name() + "' attempted to dig with inadequate tools node '" + target.itemstring + "' at " + pos.to_string());
        return;
      }
      
      //Does extra stuff, like manage metadata.
      //true = must go through with the dig
      //false = abort
      bool do_continue = on_dig_node(target, pos);
      if(!do_continue)
        return;
      
      log(LogSource::SERVER, LogLevel::EXTRA, "Player '" + player->get_name() + "' digs '" + target.itemstring + "' at " + pos.to_string());
      
#ifdef DEBUG_PERF
      auto start = std::chrono::steady_clock::now();
#endif
      
      map.set_node(pos, Node("air"), target);
      
#ifdef DEBUG_PERF
      auto end = std::chrono::steady_clock::now();
      auto diff = end - start;
      
      std::cout << "dig_node in " << std::chrono::duration<double, std::milli>(diff).count() << " ms" << std::endl;
#endif
      
      //consume tool
      InvPatch use_tool_patch;
      
      if(break_time_tool) {
        bool do_consume = true;
        if(break_time_hand) {
          if(*break_time_hand <= *break_time_tool)
            do_consume = false;
        }
        if(do_consume && wield_stack.wear) {
          InvStack new_wield_stack(wield_stack);
          new_wield_stack.wear = *new_wield_stack.wear - 1;
          if(new_wield_stack.wear <= 0)
            new_wield_stack = InvStack();
          
          use_tool_patch.diffs.push_back(InvDiff(
              InvRef("player", "null", "main", wield_index),
              wield_stack,
              new_wield_stack));
        }
      }
      
      //give the dug node to the player
      InvStack to_give;
      if(target_def.drops != "")
        to_give = InvStack(target_def.drops);
      else if(get_item_def(target.itemstring).itemstring != "nothing")
        to_give = InvStack(target.itemstring, 1, std::nullopt, std::nullopt);
      //else
      //  log(LogSource::SERVER, LogLevel::NOTICE, "node '" + target.itemstring + "' drops nothing");
      
      std::optional<InvPatch> give_patch = inv_calc_give(
          InvRef("player", "null", "main", -1),
          player->inv_get("main"),
          to_give);
      
      if(!give_patch) {
        //couldn't give
        //TODO
        return;
      }
      
      InvPatch combined_patch = *give_patch + use_tool_patch;
      
      player_lock_unique.unlock();
      bool res = inv_apply_patch(combined_patch, player);
      if(!res) {
        //TODO
      }
    } else if(type == "place_node") {
      MapPos<int> pos(pt.get<int>("pos.x"), pt.get<int>("pos.y"), pt.get<int>("pos.z"), pt.get<int>("pos.w"), pt.get<int>("pos.world"), pt.get<int>("pos.universe"));
      int wield_index = pt.get<int>("wield");
      Node to_place(pt.get<std::string>("data.itemstring"), pt.get<unsigned int>("data.rot"));
      
      InvStack wield_stack = player->inv_get("main", wield_index);
      
      //FIXME this should be handled by 'expected' in set_node
      Node existing = map.get_node(pos);
      if(existing.itemstring != "air") {
        log(LogSource::SERVER, LogLevel::NOTICE, "Player '" + player->get_name()
                                                 + " attempted to place node '" + to_place.itemstring + "' over '" + existing.itemstring + "'");
        //TODO tell the client
        return;
      }
      
      if(wield_stack.is_nil) {
        log(LogSource::SERVER, LogLevel::NOTICE, "Player '" + player->get_name()
                                                 + " attempted to place node '" + to_place.itemstring + "' but they are not wielding anything");
        //TODO tell the client
        return;
      }
      if(to_place.itemstring != wield_stack.itemstring) {
        log(LogSource::SERVER, LogLevel::NOTICE, "Player '" + player->get_name()
                                                 + " attempted to place node '" + to_place.itemstring + "' but they are really wielding '" + wield_stack.itemstring + "'");
        //TODO tell the client
        return;
      }
      
      MapPos<int> player_pos_int((int)std::round(player->pos.x), (int)std::round(player->pos.y), (int)std::round(player->pos.z), player->pos.w, player->pos.world, player->pos.universe);
      MapBox<int> bounding(player_pos_int - PLAYER_LIMIT_REACH_DISTANCE, player_pos_int + PLAYER_LIMIT_REACH_DISTANCE);
      if(!bounding.contains(pos)) {
        log(LogSource::SERVER, LogLevel::NOTICE, "Player '" + player->get_name() + "' at " + player_pos_int.to_string()
                                                 + " attempted to place node '" + to_place.itemstring + "' far away at " + pos.to_string());
        return;
      }
      
      NodeDef to_place_def = get_node_def(to_place.itemstring);
      if(to_place_def.itemstring == "nothing") {
        log(LogSource::SERVER, LogLevel::NOTICE, "Player '" + player->get_name() + "' attempted to place nonexistent node '" + to_place.itemstring + "' at " + pos.to_string());
        return;
      }
      
      //take the dug node from the player
      InvStack to_take(to_place.itemstring, 1, std::nullopt, std::nullopt);
      
      std::optional<InvPatch> take_patch = inv_calc_take_at(
          InvRef("player", "null", "main", wield_index),
          player->inv_get("main"),
          to_take);
      
      if(!take_patch) {
        log(LogSource::SERVER, LogLevel::NOTICE, "Player '" + player->get_name() + "' attempted to place '" + to_place.itemstring + " but they don't have any in inventory");
        return;
      }
      
      //Does extra stuff, like manage metadata.
      //true = must go through with the place
      //false = abort
      bool do_continue = on_place_node(to_place, pos);
      if(!do_continue)
        return;
      
      log(LogSource::SERVER, LogLevel::EXTRA, "Player '" + player->get_name() + "' places '" + to_place.itemstring + "' at " + pos.to_string());
      
#ifdef DEBUG_PERF
      auto start = std::chrono::steady_clock::now();
#endif
      
      map.set_node(pos, to_place, Node("air"));
      
#ifdef DEBUG_PERF
      auto end = std::chrono::steady_clock::now();
      auto diff = end - start;
      
      std::cout << "place_node in " << std::chrono::duration<double, std::milli>(diff).count() << " ms" << std::endl;
#endif
      
      player_lock_unique.unlock();
      bool res = inv_apply_patch(*take_patch, player);
      if(!res) {
        //TODO
      }
    } else if(type == "inv_swap") {
      InvRef ref1(pt.get_child("ref1"));
      InvRef ref2(pt.get_child("ref2"));
      InvStack orig1(pt.get_child("orig1"));
      InvStack orig2(pt.get_child("orig2"));
      std::string req_id = pt.get<std::string>("reqID");
      std::string craft_patch_id = pt.get<std::string>("craftPatchID");
      
      //TODO: access control (incl. accessing only own player's inventory)
      if(ref1.obj_type == "player")
        ref1.obj_id = "null"; //prevent access to other players' inventories (FIXME)
      if(ref2.obj_type == "player")
        ref2.obj_id = "null"; //prevent access to other players' inventories (FIXME)
      
      std::optional<InvPatch> override_result
          = inv_interact_override(ref1, orig1, ref2, orig2, req_id, player);
      if(override_result) {
        player_lock_unique.unlock();
        if((*override_result).is_deny) {
          player->send((*override_result).as_json("inv_patch_deny"));
        } else {
          inv_apply_patch(*override_result, player);
        }
          
        std::optional<InvPatch> craft_patch = update_craft_if_needed(ref1, ref2, player);
        if(craft_patch) {
          (*craft_patch).req_id = craft_patch_id;
          inv_apply_patch(*craft_patch, player);
        }
        return;
      }
      
      
      
      InvPatch patch(req_id);
      patch.diffs.push_back(
          InvDiff(ref1, orig1, orig2));
      patch.diffs.push_back(
          InvDiff(ref2, orig2, orig1));
      
      player_lock_unique.unlock();
      inv_apply_patch(patch, player);
      
      std::optional<InvPatch> craft_patch = update_craft_if_needed(ref1, ref2, player);
      if(craft_patch) {
        (*craft_patch).req_id = craft_patch_id;
        inv_apply_patch(*craft_patch, player);
      }
    } else if(type == "inv_distribute") {
      InvRef ref1(pt.get_child("ref1"));
      InvRef ref2(pt.get_child("ref2"));
      InvStack orig1(pt.get_child("orig1"));
      InvStack orig2(pt.get_child("orig2"));
      int qty1 = pt.get<int>("qty1");
      int qty2 = pt.get<int>("qty2");
      std::string req_id = pt.get<std::string>("reqID");
      std::string craft_patch_id = pt.get<std::string>("craftPatchID");
      
      //TODO: access control (incl. accessing only own player's inventory)
      if(ref1.obj_type == "player")
        ref1.obj_id = "null"; //prevent access to other players' inventories (FIXME)
      if(ref2.obj_type == "player")
        ref2.obj_id = "null"; //prevent access to other players' inventories (FIXME)
      
      if(ref1 == ref2) {
        InvPatch deny_patch(req_id);
        deny_patch.diffs.push_back(
            InvDiff(ref1, orig1, orig1));
        deny_patch.diffs.push_back(
            InvDiff(ref2, orig2, orig2));
        deny_patch.make_deny();
        player_lock_unique.unlock();
        player->send(deny_patch.as_json("inv_patch_deny"));
        return;
      }
      
      std::optional<InvPatch> override_result
          = inv_interact_override(ref1, orig1, ref2, orig2, req_id, player);
      if(override_result) {
        player_lock_unique.unlock();
        if((*override_result).is_deny) {
          player->send((*override_result).as_json("inv_patch_deny"));
        } else {
          inv_apply_patch(*override_result, player);
        }
        
        std::optional<InvPatch> craft_patch = update_craft_if_needed(ref1, ref2, player);
        if(craft_patch) {
          (*craft_patch).req_id = craft_patch_id;
          inv_apply_patch(*craft_patch, player);
        }
        return;
      }
      
      
      std::optional<std::pair<InvStack, InvStack>> distributed = inv_calc_distribute(orig1, qty1, orig2, qty2);
      
      if(!distributed) {
        InvPatch deny_patch(req_id);
        deny_patch.diffs.push_back(
            InvDiff(ref1, orig1, orig1));
        deny_patch.diffs.push_back(
            InvDiff(ref2, orig2, orig2));
        deny_patch.make_deny();
        player_lock_unique.unlock();
        player->send(deny_patch.as_json("inv_patch_deny"));
        return;
      }
      
      
      InvPatch patch(req_id);
      patch.diffs.push_back(
          InvDiff(ref1, orig1, (*distributed).first));
      patch.diffs.push_back(
          InvDiff(ref2, orig2, (*distributed).second));
      
      
      player_lock_unique.unlock();
      inv_apply_patch(patch, player);
      
      std::optional<InvPatch> craft_patch = update_craft_if_needed(ref1, ref2, player);
      if(craft_patch) {
        (*craft_patch).req_id = craft_patch_id;
        inv_apply_patch(*craft_patch, player);
      }
    } else if(type == "inv_pulverize") {
      int wield_index = pt.get<int>("wield");
      InvStack wield_stack = player->inv_get("main", wield_index);
      
      if(wield_stack.is_nil) {
        player_lock_unique.unlock();
        chat_send_player(player, "server", "nothing to pulverize");
        return;
      }
      
      InvPatch pulverize_patch;
      pulverize_patch.diffs.push_back(InvDiff(
          InvRef("player", "null", "main", wield_index),
          wield_stack,
          InvStack()));
      
      player_lock_unique.unlock();
      
      bool res = inv_apply_patch(pulverize_patch);
      if(!res) {
        chat_send_player(player, "server", "failed to pulverize '" + wield_stack.spec() + "'!");
      }
      
      chat_send_player(player, "server", "pulverized '" + wield_stack.spec() + "'");
    } else if(type == "send_chat") {
      std::string from = player->get_name();
      std::string channel = pt.get<std::string>("channel");
      std::string message = pt.get<std::string>("message");
      
      //TODO: validate channel, access control, etc.
      //TODO: any other anti-abuse validation (player name and channel character set restrictions, length limits, etc.)
      
      player_lock_unique.unlock();
      chat_send(channel, from, message);
      return;
    } else if(type == "interact") {
      MapPos<int> pos(pt.get<int>("pos.x"), pt.get<int>("pos.y"), pt.get<int>("pos.z"), pt.get<int>("pos.w"), pt.get<int>("pos.world"), pt.get<int>("pos.universe"));
      Node node = map.get_node(pos);
      //TODO close ui if it gets dug?
      //TODO track ui IDs or something
      if(node.itemstring == "default:chest") {
        InvRef chest_ref("node", pos.to_json(), "chest", -1);
        
        UISpec chest_ui;
        chest_ui.components.push_back(
            UI_InvList(chest_ref).to_json());
        chest_ui.components.push_back(
            UI_Spacer().to_json());
        chest_ui.components.push_back(
            UI_InvList(InvRef("player", "null", "main", -1)).to_json());
        
        UIInstance chest_ui_instance(chest_ui);
        std::string player_tag = player->get_tag();
        
        chest_ui_instance.close_callback = [this, chest_ref, player_tag]() {
          std::shared_lock<std::shared_mutex> list_lock(m_players_lock);
          PlayerState *found_player = get_player_by_tag(player_tag);
          if(found_player == NULL)
            return;
          std::unique_lock<std::shared_mutex> found_player_lock(found_player->lock);
          
          found_player->uninterest_inventory(chest_ref);
        };
        
        player_lock_unique.unlock();
        send_inv(player, chest_ref);
        open_ui(player, chest_ui_instance);
      }
    } else if(type == "ui_close") {
      player_lock_unique.unlock();
      
      std::string id = pt.get<std::string>("id");
      UIInstance search_instance(id);
      
      std::unique_lock<std::shared_mutex> ui_list_lock(active_ui_lock);
      auto search = active_ui.find(search_instance);
      if(search == active_ui.end())
        return;
      
      if(search->close_callback)
        search->close_callback();
      
      active_ui.erase(search);
    } else if(type == "chat_command") {
      std::string command = pt.get<std::string>("command");
      
      std::vector<std::string> tokens;
      boost::char_separator<char> sep(" ", "");
      boost::tokenizer<boost::char_separator<char>> tokenizer(command, sep);
      for(boost::tokenizer<boost::char_separator<char>>::iterator it = tokenizer.begin(); it != tokenizer.end(); ++it) {
        tokens.push_back(*it);
      }
      
      if(tokens.size() < 1) {
        player_lock_unique.unlock();
        chat_send_player(player, "server", "invalid command: empty input");
        return;
      }
      
      std::string command_name = tokens[0];
      
      player_lock_unique.unlock();
      
      //TODO table pointing to these functions
      if(tokens[0] == "/nick") {
        cmd_nick(player, tokens);
      } else if(tokens[0] == "/status") {
        cmd_status(player, tokens);
      } else if(tokens[0] == "/time") {
        cmd_time(player, tokens);
      } else if(tokens[0] == "/whereami") {
        cmd_whereami(player, tokens);
      } else if(tokens[0] == "/tp") {
        cmd_tp(player, tokens);
      } else if(tokens[0] == "/world") {
        cmd_tp_world(player, tokens);
      } else if(tokens[0] == "/universe") {
        cmd_tp_universe(player, tokens);
      } else if(tokens[0] == "/grant") {
        cmd_grant(player, tokens);
      } else if(tokens[0] == "/grantme") {
        cmd_grantme(player, tokens);
      } else if(tokens[0] == "/revoke") {
        cmd_revoke(player, tokens);
      } else if(tokens[0] == "/privs") {
        cmd_privs(player, tokens);
      } else if(tokens[0] == "/giveme") {
        cmd_giveme(player, tokens);
      } else if(tokens[0] == "/clearinv") {
        cmd_clearinv(player, tokens);
      } else if(tokens[0] == "/creative") {
        cmd_creative(player, tokens);
      } else {
        chat_send_player(player, "server", "unknown command");
        return;
      }
    }
  } catch(boost::property_tree::ptree_error const& e) {
    log(LogSource::SERVER, LogLevel::ERR, "JSON parse error: " + std::string(e.what()) + " payload=" + msg->get_payload());
  }
}
