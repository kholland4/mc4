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

#include "vector.h"


MapPos<int> global_to_mapblock(MapPos<int> pos) {
  //C++ integer division always rounds "towards zero", i. e. the fractional part is discarded.
  //Unfortunately, we want to floor the result -- round it down.
  //For positive numbers, C++ gives us the correct answer.
  //For negative numbers, it may not. -9 / 4 gives -2, but we want -3.
  //However, when a negative dividend is divided evenly by its divisor, we get the desired result (i. e. -8 / 2 = -4).
  return MapPos<int>(
      (pos.x < 0 && pos.x % MAPBLOCK_SIZE_X != 0) ? (pos.x / MAPBLOCK_SIZE_X - 1) : (pos.x / MAPBLOCK_SIZE_X),
      (pos.y < 0 && pos.y % MAPBLOCK_SIZE_Y != 0) ? (pos.y / MAPBLOCK_SIZE_Y - 1) : (pos.y / MAPBLOCK_SIZE_Y),
      (pos.z < 0 && pos.z % MAPBLOCK_SIZE_Z != 0) ? (pos.z / MAPBLOCK_SIZE_Z - 1) : (pos.z / MAPBLOCK_SIZE_Z),
      pos.w, pos.world, pos.universe);
}

MapPos<int> global_to_relative(MapPos<int> pos) {
  //Since the modulo operation gives remainders, doing something like -9 % 5 would give -4. We wrap this around into the positive interval [0, MAPBLOCK_SIZE_*).
  return MapPos<int>(
      ((pos.x % MAPBLOCK_SIZE_X) + MAPBLOCK_SIZE_X) % MAPBLOCK_SIZE_X,
      ((pos.y % MAPBLOCK_SIZE_Y) + MAPBLOCK_SIZE_Y) % MAPBLOCK_SIZE_Y,
      ((pos.z % MAPBLOCK_SIZE_Z) + MAPBLOCK_SIZE_Z) % MAPBLOCK_SIZE_Z,
      0, 0, 0);
}


Quaternion::Quaternion()
    : x(0), y(0), z(0), w(0)
{
  
}
Quaternion::Quaternion(double _x, double _y, double _z, double _w)
    : x(_x), y(_y), z(_z), w(_w)
{
  
}

void Quaternion::set(double _x, double _y, double _z, double _w) {
  x = _x;
  y = _y;
  z = _z;
  w = _w;
}
