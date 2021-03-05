#include "server.h"

void Server::cmd_nick(PlayerState *player, std::vector<std::string> args) {
  if(args.size() != 2) {
    chat_send_player(player, "server", "invalid command: wrong number of args, expected '/nick <new_nickname>'");
    return;
  }
  
  std::string new_nick = args[1];
  
  std::regex nick_allow("^[a-zA-Z0-9\\-_]+$");
  if(!std::regex_match(new_nick, nick_allow)) {
    chat_send_player(player, "server", "invalid nickname: allowed characters are a-z A-Z 0-9 - _");
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
