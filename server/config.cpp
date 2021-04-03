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

#include "config.h"
#include "log.h"

#include <map>
#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

std::map<std::string, std::string> config_keys_str = {
  {"server.motd", ""},
  
  {"database.backend", "sqlite3"},
  {"database.sqlite3_file", "test_map.sqlite"},
  
  //there are no sensible defaults for these
  {"ssl.cert_chain_file", ""},
  {"ssl.private_key_file", ""},
  {"ssl.dhparam_file", ""},
  
  {"loader.defs_file", "defs.json"}
};
std::map<std::string, std::string> config_str;

std::map<std::string, int> config_keys_int = {
  {"server.port", 8080},
  {"server.threads", 0},
  
  {"database.L1_cache_target", 10000},
  {"database.L2_cache_target", 100000},
  
  {"map.seed", 82},
  {"map.water_depth", 0},
  {"map.sand_depth", 3}
};
std::map<std::string, int> config_int;

std::map<std::string, bool> config_keys_bool = {
  {"auth.allow_register", true},
  {"server.many_threads", false}
};
std::map<std::string, bool> config_bool;

//String escape sequence parser, used in set_config<std::string> to allow the user to input newlines
std::string parse_escapes(std::string in_str) {
  std::stringstream in;
  in << in_str;
  std::ostringstream out;
  
  char c;
  enum {CH, ESC} state = CH;
  while(in.get(c)) {
    if(state == CH) {
      if(c == '\\') {
        state = ESC;
      } else {
        out << c;
      }
    } else if(state == ESC) {
      switch(c) {
        case '\\': out << '\\'; break;
        case 'n': out << '\n'; break;
        default: out << '\\' << c; break;
      }
      state = CH;
    }
  }
  
  return out.str();
}

template<> std::string get_config<std::string>(std::string key) {
  //check if the config key is valid & get its default value
  auto search = config_keys_str.find(key);
  if(search == config_keys_str.end()) {
    log(LogSource::CONFIG, LogLevel::WARNING, "invalid config key (string): '" + key + "'");
    return "";
  }
  
  //lookup the user-set value, if any
  auto search_val = config_str.find(key);
  if(search_val != config_str.end()) {
    return search_val->second;
  }
  
  //if not found, return the default value
  return search->second;
}

template<> int get_config<int>(std::string key) {
  //check if the config key is valid & get its default value
  auto search = config_keys_int.find(key);
  if(search == config_keys_int.end()) {
    log(LogSource::CONFIG, LogLevel::WARNING, "invalid config key (int): '" + key + "'");
    return -1;
  }
  
  //lookup the user-set value, if any
  auto search_val = config_int.find(key);
  if(search_val != config_int.end()) {
    return search_val->second;
  }
  
  //if not found, return the default value
  return search->second;
}

template<> bool get_config<bool>(std::string key) {
  //check if the config key is valid & get its default value
  auto search = config_keys_bool.find(key);
  if(search == config_keys_bool.end()) {
    log(LogSource::CONFIG, LogLevel::WARNING, "invalid config key (bool): '" + key + "'");
    return false;
  }
  
  //lookup the user-set value, if any
  auto search_val = config_bool.find(key);
  if(search_val != config_bool.end()) {
    return search_val->second;
  }
  
  //if not found, return the default value
  return search->second;
}



template<> void set_config<std::string>(std::string key, std::string val) {
  //check if the config key is valid
  auto search = config_keys_str.find(key);
  if(search == config_keys_str.end()) {
    log(LogSource::CONFIG, LogLevel::WARNING, "invalid config key (string): '" + key + "'");
    return;
  }
  
  //set the value
  config_str[key] = parse_escapes(val);
}

template<> void set_config<int>(std::string key, int val) {
  //check if the config key is valid
  auto search = config_keys_int.find(key);
  if(search == config_keys_int.end()) {
    log(LogSource::CONFIG, LogLevel::WARNING, "invalid config key (int): '" + key + "'");
    return;
  }
  
  //set the value
  config_int[key] = val;
}

template<> void set_config<bool>(std::string key, bool val) {
  //check if the config key is valid
  auto search = config_keys_bool.find(key);
  if(search == config_keys_bool.end()) {
    log(LogSource::CONFIG, LogLevel::WARNING, "invalid config key (bool): '" + key + "'");
    return;
  }
  
  //set the value
  config_bool[key] = val;
}

bool set_config_auto(std::string key, std::string val) {
  //check for string option
  auto search_str = config_keys_str.find(key);
  if(search_str != config_keys_str.end()) {
    set_config<std::string>(key, val);
    return true;
  }
  
  //check for int option
  auto search_int = config_keys_int.find(key);
  if(search_int != config_keys_int.end()) {
    set_config<int>(key, std::stoi(val));
    return true;
  }
  
  //check for bool option
  auto search_bool = config_keys_bool.find(key);
  if(search_bool != config_keys_bool.end()) {
    bool val_bool = false;
    std::istringstream is(val);
    is >> val_bool;
    if(is.fail()) {
      is.clear();
      is >> std::boolalpha >> val_bool;
    }
    if(is.fail()) {
      log(LogSource::CONFIG, LogLevel::WARNING, "config key '" + key + "' value '" + val + "' not convertible to bool");
      return false;
    }
    
    set_config<bool>(key, val_bool);
    return true;
  }
  
  log(LogSource::CONFIG, LogLevel::WARNING, "invalid config key: '" + key + "'");
  return false;
}



void load_config_file(std::string filename) {
  boost::property_tree::ptree pt;
  boost::property_tree::read_ini(filename, pt);
  
  for(auto& section : pt) {
    if(section.second.empty()) {
      bool ok = set_config_auto(section.first, section.second.data());
      if(!ok) { exit(1); }
      continue;
    }
    
    for(auto& key : section.second) {
      bool ok = set_config_auto(section.first + "." + key.first, key.second.data());
      if(!ok) { exit(1); }
    }
  }
}
