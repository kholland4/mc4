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

#ifndef __VECTOR_H__
#define __VECTOR_H__

#include "log.h"

#include <ostream>
#include <cmath>
#include <stdexcept>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#define MAPBLOCK_SIZE_X 16
#define MAPBLOCK_SIZE_Y 16
#define MAPBLOCK_SIZE_Z 16

template <class T> class Vector3 {
  public:
    //Vector3();
    //Vector3(T _x, T _y, T _z);
    Vector3() : x(0), y(0), z(0) {};
    Vector3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {};
    void set(T _x, T _y, T _z) {
      x = _x;
      y = _y;
      z = _z;
    };
    friend std::ostream& operator<<(std::ostream &out, const Vector3<T> &v) {
      out << "(" << v.x << ", " << v.y << ", " << v.z << ")";
      return out;
    };
    std::string to_string() {
      return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
    };
    bool operator<(const Vector3<T>& other) const {
      if(x == other.x) {
        if(y == other.y) {
          return z < other.z;
        } else {
          return y < other.y;
        }
      } else {
        return x < other.x;
      }
    };
    bool operator==(const Vector3<T>& other) const {
      return (x == other.x) && (y == other.y) && (z == other.z);
    };
    Vector3<T> operator+(const Vector3<T>& other) {
      return Vector3<T>(x + other.x, y + other.y, z + other.z);
    };
    Vector3<T> operator-(const Vector3<T>& other) {
      return Vector3<T>(x - other.x, y - other.y, z - other.z);
    };
    T distance_to(const Vector3<T>& other) {
      return sqrt((x - other.x) * (x - other.x) + (y - other.y) * (y - other.y) + (z - other.z) * (z - other.z));
    };
    
    T x;
    T y;
    T z;
};

template <class T> class MapPos {
  public:
    MapPos() : x(0), y(0), z(0), w(0), world(0), universe(0) {};
    MapPos(T _x, T _y, T _z, int _w, int _world, int _universe) : x(_x), y(_y), z(_z), w(_w), world(_world), universe(_universe) {};
    
    //may throw std::invalid_argument
    MapPos(std::string json) {
      try {
        std::stringstream ss;
        ss << json;
        
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(ss, pt);
        
        x = pt.get<T>("x");
        y = pt.get<T>("y");
        z = pt.get<T>("z");
        w = pt.get<T>("w");
        world = pt.get<T>("world");
        universe = pt.get<T>("universe");
      } catch(boost::property_tree::ptree_error const& e) {
        log(LogSource::VECTOR, LogLevel::ERR, "JSON parse error: " + std::string(e.what()) + " json=" + json);
        
        throw std::invalid_argument("JSON parse error: " + std::string(e.what()) + " json=" + json);
      }
    };
    
    void set(T _x, T _y, T _z, int _w, int _world, int _universe) {
      x = _x;
      y = _y;
      z = _z;
      w = _w;
      world = _world;
      universe = _universe;
    };
    friend std::ostream& operator<<(std::ostream &out, const MapPos<T> &v) {
      out << "(" << v.x << ", " << v.y << ", " << v.z << ") in w=" << v.w << " world=" << v.world << " universe=" << v.universe;
      return out;
    };
    std::string to_string() {
      return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ") in w=" +
             std::to_string(w) + " world=" + std::to_string(world) + " universe=" + std::to_string(universe);
    };
    std::string to_json() {
      return "{\"x\":" + std::to_string(x) + ",\"y\":" + std::to_string(y) + ",\"z\":" + std::to_string(z) + ",\"w\":" +
             std::to_string(w) + ",\"world\":" + std::to_string(world) + ",\"universe\":" + std::to_string(universe) + "}";
    };
    bool operator<(const MapPos<T>& other) const {
      if(universe == other.universe) {
        if(world == other.world) {
          if(w == other.w) {
            if(x == other.x) {
              if(y == other.y) {
                return z < other.z;
              } else {
                return y < other.y;
              }
            } else {
              return x < other.x;
            }
          } else {
            return w < other.w;
          }
        } else {
          return world < other.world;
        }
      } else {
        return universe < other.universe;
      }
    };
    
    bool operator==(const MapPos<T>& other) const {
      return (x == other.x) && (y == other.y) && (z == other.z) && (w == other.w) && (world == other.world) && (universe == other.universe);
    };
    bool operator!=(const MapPos<T>& other) const {
      return !operator==(other);
    };
    MapPos<T> operator+(const MapPos<T>& other) {
      return MapPos<T>(x + other.x, y + other.y, z + other.z, w + other.w, world + other.world, universe + other.universe);
    };
    MapPos<T> operator-(const MapPos<T>& other) {
      return MapPos<T>(x - other.x, y - other.y, z - other.z, w - other.w, world - other.world, universe - other.universe);
    };
    MapPos<T>& operator+=(const MapPos<T>& other) {
      x += other.x;
      y += other.y;
      z += other.z;
      w += other.w;
      world += other.world;
      universe += other.universe;
      return *this;
    };
    
    //FIXME
    T distance_to(const MapPos<T>& other) {
      if(universe != other.universe) { return 1000000; }
      if(world != other.world) { return 1000000; }
      if(w != other.w) { return 1000000; }
      return sqrt((x - other.x) * (x - other.x) + (y - other.y) * (y - other.y) + (z - other.z) * (z - other.z)); // + (w - other.w) * (w - other.w));
    };
    
    MapPos<T> abs() {
      return MapPos<T>(std::abs(x), std::abs(y), std::abs(z), std::abs(w), std::abs(world), std::abs(universe));
    };
    
    T x;
    T y;
    T z;
    int w;
    int world;
    int universe;
};

template <class T> class MapBox {
  public:
    //TODO: auto-sort lower and upper?
    MapBox(MapPos<T> lower, MapPos<T> upper) : min(lower), max(upper) {}
    bool contains(MapPos<T> point) {
      return point.x >= min.x && point.x <= max.x &&
             point.y >= min.y && point.y <= max.y &&
             point.z >= min.z && point.z <= max.z &&
             point.w >= min.w && point.w <= max.w &&
             point.world >= min.world && point.world <= max.world &&
             point.universe >= min.universe && point.universe <= max.universe;
    };
    
    MapPos<T> min;
    MapPos<T> max;
};

MapPos<int> global_to_mapblock(MapPos<int> pos);
MapPos<int> global_to_relative(MapPos<int> pos);

class Quaternion {
  public:
    Quaternion();
    Quaternion(double _x, double _y, double _z, double _w);
    void set(double _x, double _y, double _z, double _w);
    
    std::string to_json() {
      return "{\"x\":" + std::to_string(x) + ",\"y\":" + std::to_string(y) + ",\"z\":" + std::to_string(z) + ",\"w\":" + std::to_string(w) + "}";
    };
    
    double x;
    double y;
    double z;
    double w;
};

#endif
