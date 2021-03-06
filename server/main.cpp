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

int main() {
  load_node_defs("defs.json");
  
  SQLiteDB db("test_map.sqlite");
  //MemoryDB db;
  MapgenAlpha mapgen0(82);
  World *world0 = new World("Earth", mapgen0);
  MapgenHeck mapgen1(82);
  World *world1 = new World("Heck", mapgen1);
  
  std::map<int, World*> worlds = {
    {0, world0},
    {1, world1}
  };
  
  Server server(db, worlds);
  
  server.set_motd("-- Highly Experimental Test Server (tm)\n-- Use '/gamemode creative' for creative, '/nick new_nickname_here' to change your name, '/status' to view this message, '/help' for other commands\n-- Press 'k' to toggle fly, 'j' to toggle fast");
  
  server.run(8080);
}
