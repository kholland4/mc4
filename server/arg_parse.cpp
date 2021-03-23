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

#include "arg_parse.h"
#include "config.h"
#include "log.h"

#include <string>

void parse_args(int argc, char *argv[]) {
  std::string expecting_value_for = "";
  for(int i = 1; i < argc; i++) {
    std::string arg(argv[i]);
    
    if(expecting_value_for != "") {
      if(expecting_value_for == "-o") {
        size_t equals_location = arg.find("=");
        if(equals_location == std::string::npos) {
          log(LogSource::CONFIG, LogLevel::EMERG, "missing = sign in '-o " + arg + "'");
          exit(1);
        }
        
        std::string key = arg.substr(0, equals_location);
        std::string val = arg.substr(equals_location + 1);
        if(!set_config_auto(key, val)) {
          exit(1);
        }
      }
      
      if(expecting_value_for == "--config-file") {
        load_config_file(arg);
      }
      
      expecting_value_for = "";
      continue;
    }
    
    if(arg == "-o" || arg == "--config-file") { //these options have values following them
      expecting_value_for = arg;
      continue;
    }
    
    log(LogSource::CONFIG, LogLevel::EMERG, "unexpected command-line option: '" + arg + "'");
    exit(1);
  }
  
  if(expecting_value_for != "") {
    log(LogSource::CONFIG, LogLevel::EMERG, "expected value following '" + expecting_value_for + "'");
    exit(1);
  }
}
