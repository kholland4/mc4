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

#include <map>

#include "lib/PerlinNoise.hpp"

class Mapgen {
  public:
    Mapgen(uint32_t _seed) : seed(_seed) {};
    virtual std::map<MapPos<int>, Mapblock*> generate_near(MapPos<int> pos) = 0;
  
  protected:
    uint32_t seed;
};

class MapgenDefault : public Mapgen {
  public:
    MapgenDefault(uint32_t _seed) : Mapgen(_seed) {};
    virtual std::map<MapPos<int>, Mapblock*> generate_near(MapPos<int> pos);
};

class MapgenAlpha : public Mapgen {
  public:
    MapgenAlpha(uint32_t _seed) : Mapgen(_seed), perlin(_seed) {};
    virtual std::map<MapPos<int>, Mapblock*> generate_near(MapPos<int> pos);
  
  private:
    siv::PerlinNoise perlin;
};

class MapgenHeck : public Mapgen {
  public:
    MapgenHeck(uint32_t _seed) : Mapgen(_seed), perlin(_seed) {};
    virtual std::map<MapPos<int>, Mapblock*> generate_near(MapPos<int> pos);
  
  private:
    siv::PerlinNoise perlin;
};



class World {
  public:
    World(std::string _name, Mapgen& _mapgen) : name(_name), mapgen(_mapgen) {}
    
    std::string name;
    Mapgen& mapgen;
};

#endif
