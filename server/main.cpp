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

#include "database.h"
#include "mapgen.h"
#include "server.h"
#include "config.h"
#include "arg_parse.h"

#include <malloc.h>

namespace {
  std::function<void(int)> terminate_handler;
  void signal_handler(int signum) { terminate_handler(signum); }
}

int main(int argc, char *argv[]) {
  mallopt(M_ARENA_MAX, 2);
  
  parse_args(argc, argv);
  
  load_node_defs(get_config<std::string>("loader.defs_file"));
  load_item_defs(get_config<std::string>("loader.defs_file"));
  
  Database *db;
  std::string db_backend = get_config<std::string>("database.backend");
  if(db_backend == "sqlite3") {
    std::string db_file = get_config<std::string>("database.sqlite3_file");
    if(db_file == "") {
      log(LogSource::INIT, LogLevel::EMERG, "database.sqlite3_file not specified");
      exit(1);
    }
    db = new SQLiteDB(db_file.c_str());
  } else if(db_backend == "memory") {
    db = new MemoryDB();
  } else {
    log(LogSource::INIT, LogLevel::EMERG, "invalid database backend: '" + db_backend + "'");
    exit(1);
  }
  
  uint32_t seed = get_config<int>("map.seed");
  
  MapgenAlpha mapgen0(seed);
  World *world0 = new World("Earth", mapgen0);
  MapgenHeck mapgen1(seed);
  World *world1 = new World("Heck", mapgen1);
  
  std::map<int, World*> worlds = {
    {0, world0},
    {1, world1}
  };
  
  Server server(*db, worlds);
  
  server.set_motd(get_config<std::string>("server.motd"));
  
  int port = get_config<int>("server.port");
  if(port < 0 || port > 65535) {
    log(LogSource::INIT, LogLevel::EMERG, "invalid server port: " + std::to_string(port));
    exit(1);
  }
  
  terminate_handler = [&server](int signum) {
    server.terminate();
  };
  std::signal(SIGINT, signal_handler);
  
  
  server.run(port);
}
