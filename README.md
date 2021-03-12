# mc4

Voxel building game inspired by [Minetest](https://www.minetest.net/)

[Play](https://kholland4.github.io/mc4/)

## License

See [LICENSE](LICENSE) and the top of each source code file for license information.

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

## NOTICE

The way in which client-server multiplayer is implemented is insecure.
This software is NOT suitable for production use.
