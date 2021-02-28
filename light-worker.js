/*
    mc4, a web voxel building game
    Copyright (C) 2019 kholland4

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

importScripts("geometry.js");

onmessage = function(e) {
  var pos = e.data.pos;
  var size = e.data.size;
  var nodeDefAdj = e.data.nodeDefAdj;
  var data = e.data.data;
  var lightNeedsUpdate = e.data.lightNeedsUpdate;
  
  for(var x = 0; x < size.x; x++) {
    for(var y = 0; y < size.y; y++) {
      for(var z = 0; z < size.z; z++) {
        var rx = x + size.x;
        var ry = y + size.y;
        var rz = z + size.z;
        
        var d = data[rx][ry][rz];
        var id = d & 32767;
        var rot = (d >> 15) & 255;
        var light = (d >> 23) & 255;
        
        light = 0;
        if(x < 4 && z < 4) {
          light = 10;
        }
        
        data[rx][ry][rz] = id | (rot << 15) | (light << 23);
      }
    }
  }
  
  var res = {
    data: data
  }
  this.postMessage(res);
};
