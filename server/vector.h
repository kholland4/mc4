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

#include <ostream>
#include <cmath>

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
    }
    
    T x;
    T y;
    T z;
};

class Quaternion {
  public:
    Quaternion();
    Quaternion(double _x, double _y, double _z, double _w);
    void set(double _x, double _y, double _z, double _w);
    
    double x;
    double y;
    double z;
    double w;
};

#endif
