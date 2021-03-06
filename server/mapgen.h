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

#ifndef __MAPGEN_H__
#define __MAPGEN_H__

#include "vector.h"
#include "mapblock.h"

#include "lib/PerlinNoise.hpp"

class Mapgen {
  public:
    Mapgen(uint32_t _seed) : seed(_seed) {};
    virtual void generate_at(Vector3<int> pos, Mapblock *mb) = 0;
  
  protected:
    uint32_t seed;
};

class MapgenDefault : public Mapgen {
  public:
    virtual void generate_at(Vector3<int> pos, Mapblock *mb);
};

class MapgenAlpha : public Mapgen {
  public:
    MapgenAlpha(uint32_t _seed) : Mapgen(_seed), perlin(_seed) {};
    virtual void generate_at(Vector3<int> pos, Mapblock *mb);
  
  private:
    siv::PerlinNoise perlin;
};

#endif
