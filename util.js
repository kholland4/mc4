/*
    mc4, a web voxel building game
    Copyright (C) 2019-2021 kholland4

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

"use strict";

function fmtXYZ(vec) {
  return "(" + vec.x + ", " + vec.y + ", " + vec.z + ")";
}
//parses XYZ coordinates in the form "x<delim>y<delim>z" where <delim> is not 0-9, -, (, or )
function parseXYZ(str) {
  if(str.startsWith("(")) { str = str.substring(1); }
  if(str.endsWith(")")) { str = str.substring(0, str.length - 1); }
  var delim = "";
  for(var i = 0; i < str.length; i++) {
    var ch = str.charAt(i);
    if(!((ch >= '0' && ch <= '9') || ch == '-')) {
      delim += ch;
    } else {
      if(delim != "") { break; }
    }
  }
  if(delim == "") { return null; }
  
  var s = str.split(delim);
  if(s.length < 3 || s.length > 6) { return null; }
  var res = new MapPos();
  for(var i = 0; i < s.length; i++) {
    var n = parseInt(s[i]);
    if(isNaN(n)) { return null; }
    res.setComponent(i, n);
  }
  
  return res;
}

api.util = {};
api.util.fmtXYZ = fmtXYZ;
api.util.parseXYZ = parseXYZ;

class MapPos {
  constructor(x, y, z, w, world, universe) {
    this.x = x;
    this.y = y;
    this.z = z;
    this.w = w;
    this.world = world;
    this.universe = universe;
  }
  
  toString() {
    return "(" + this.x + ", " + this.y + ", " + this.z + ", w=" + this.w + ", world=" + this.world + ", universe=" + this.universe + ")";
  }
  
  add(other) {
    return new MapPos(this.x + other.x,
                      this.y + other.y,
                      this.z + other.z,
                      this.w + other.w,
                      this.world + other.world,
                      this.universe + other.universe);
  }
  
  equals(other) {
    return (this.x == other.x) &&
           (this.y == other.y) &&
           (this.z == other.z) &&
           (this.w == other.w) &&
           (this.world == other.world) &&
           (this.universe == other.universe);
  }
  
  set(x, y, z, w, world, universe) {
    this.x = x;
    this.y = y;
    this.z = z;
    this.w = w;
    this.world = world;
    this.universe = universe;
  }
  
  getComponent(n) {
    if(n >= 0 && n < 6) {
      return [this.x, this.y, this.z, this.w, this.world, this.universe][n];
    } else {
      throw new Error("can't get component " + n.toString());
    }
  }
  
  setComponent(n, val) {
    if(n == 0) {
      this.x = val;
    } else if(n == 1) {
      this.y = val;
    } else if(n == 2) {
      this.z = val;
    } else if(n == 3) {
      this.w = val;
    } else if(n == 4) {
      this.world = val;
    } else if(n == 5) {
      this.universe = val;
    } else {
      throw new Error("can't set component " + n.toString());
    }
  }
  
  clone() {
    return new MapPos(this.x, this.y, this.z, this.w, this.world, this.universe);
  }
  
  copy(other) {
    if(other.x != undefined) { this.x = other.x; }
    if(other.y != undefined) { this.y = other.y; }
    if(other.z != undefined) { this.z = other.z; }
    if(other.w != undefined) { this.w = other.w; }
    if(other.world != undefined) { this.world = other.world; }
    if(other.universe != undefined) { this.universe = other.universe; }
  }
}

function assert(val, message) {
  if(!val) {
    throw new Error("assertion failed: " + message);
  }
}
