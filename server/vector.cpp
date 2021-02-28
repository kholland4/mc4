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
