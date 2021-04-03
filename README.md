# mc4

Voxel building game inspired by [Minetest](https://www.minetest.net/)

[![Play](play-button.png)](https://kholland4.github.io/mc4/)

## License

The web client is licensed under GPLv3+, and the server is AGPLv3+.
See [LICENSE](LICENSE) and the top of each source code file for license information.
Library and asset license files are spread throughout the source tree, links are listed below.

## Server

### Compiling

Install build dependencies:

    # apt-get install make g++ libwebsocketpp-dev libboost-dev libsqlite3-dev libssl-dev

If your distro doesn't package `libwebsocketpp-dev`, grab it from [here](https://github.com/zaphoyd/websocketpp/releases/) and compile with something like:

    $ CPPFLAGS="-I/path/to/websocketpp-x.x.x/" make -j4

The server has optional TLS support, which is strongly recommended for production use
(the client-server protocol is inherently very insecure).
If you're running locally or just testing, this is not necessary.
The decision as to whether or not to use TLS must be made at compile time.

Compile without SSL/TLS enabled (recommended for testing or local use):

    $ make -j4

Compile with SSL/TLS enabled (strongly recommended for production use):

    $ CPPFLAGS=-DTLS make -j4

Any time you recompile with different `CPPFLAGS`, be sure to `make clean` first.

### Running

The server's runtime dependencies are `libsqlite3` and `libssl1.1`,
but these will have been installed when the server was compiled.

The server can be configured using an ini file (see [config_example.ini](server/config_example.ini)) or on the command line.
For example:

    $ ./mc4-server --config-file config_example.ini
    $ ./mc4-server -o server.port=8081
    $ ./mc4-server --config-file config_example.ini -o "server.motd=-- Welcome to the server.\n-- Have a nice day."

Note that command line options are processed in the order given,
so if you want to override an option from the config file,
put your override *after* the config file.

By default, the server will run on port `8080` and store data to an SQLite database in `test_map.sqlite`.

    $ ./mc4-server

If you're using TLS, take a look at [config_example.ini](server/config_example.ini) and
set the appropriate options pointing to certificate files and such.

## Libraries/assets/etc.

#### Libraries (client)

* [Three.js](https://threejs.org/):
  three.js authors (The MIT License).
  See [LICENSE.threejs.txt](lib/LICENSE.threejs.txt) for full license information.
* [NoiseJS](https://github.com/josephg/noisejs):
  Joseph Gentle (ISC License).
  See [LICENSE.noisejs.txt](lib/LICENSE.noisejs.txt) for full license information.

#### Libraries (server)

* [PerlinNoise.hpp](https://github.com/Reputeless/PerlinNoise):
  Ryo Suzuki.
  See [PerlinNoise.hpp](server/lib/PerlinNoise.hpp) for license information.

#### Assets

* [minetest_game/mods/default](https://github.com/minetest/minetest_game/tree/master/mods/default):
  celeron55 (Perttu Ahola), Cisoun, RealBadAngel, VanessaE, Calinou, PilzAdam,
  jojoa1997, InfinityProject, Splizard, Zeg9, paramat, TumeniNodes, BlockMen,
  Neuromancer, Gambit, asl97, Pithydon, npx, kaeza, kilbith,
  tobyplowy, CloudyProton, Mossmanikin, random-geek, Topywo, Extex101,
  An0n3m0us (CC BY-SA 3.0). sofar, Ferk, Krock (CC0 1.0).
  See [README.default.txt](mods/default/icons/README.default.txt) and
  [LICENSE.default.txt](mods/default/icons/LICENSE.default.txt) for full
  license information.
* [minetest_game/mods/wool](https://github.com/minetest/minetest_game/tree/master/mods/wool):
  Cisoun (CC BY-SA 3.0). See [README.wool.txt](mods/dye/icons/README.wool.txt) and
  [LICENSE.wool.txt](mods/dye/icons/LICENSE.wool.txt) for full
  license information.
* [minetest_game/mods/dye](https://github.com/minetest/minetest_game/tree/master/mods/dye):
  Perttu Ahola (celeron55) (CC BY-SA 3.0).
  See [README.dye.txt](mods/dye/icons/README.dye.txt) and
  [LICENSE.dye.txt](mods/dye/icons/LICENSE.dye.txt) for full
  license information.
* [minetest_game/mods/flowers](https://github.com/minetest/minetest_game/tree/master/mods/flowers):
  RHRhino, Gambit, yyt16384, paramat (CC BY-SA 3.0).
  See [README.flowers.txt](mods/flowers/icons/README.flowers.txt) and
  [LICENSE.flowers.txt](mods/flowers/icons/LICENSE.flowers.txt) for full
  license information.
* [minetest-mods/technic](https://github.com/minetest-mods/technic):
  kpoppel, Nekogloop, Nore/Ekdohibs, ShadowNinja, VanessaE, and others (LGPLv2).
  See [LICENSE.technic.txt](mods/minerals/icons/LICENSE.technic.txt) for full
  license information.
* [minetest-mods/moreores](https://github.com/minetest-mods/moreores):
  Hugo Locurcio and contributors (CC BY-SA 3.0 Unported).
  See [README.moreores.txt](mods/minerals/icons/README.moreores.txt) for full
  license information.

## NOTICE

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See notices
    at the top of each source code file for more information.

The game will probably work.
It might stop working.
It might have serious security vulnerabilities.
No guarantees.
(But if you do notice anything, please file an issue and/or pull request.)
