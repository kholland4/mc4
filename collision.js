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

"use strict";

function collide(box1, box2) {
  return box1.intersectsBox(box2);
}

function collideMap(box, boxMapPos) {
  var w = 0;
  var world = 0;
  var universe = 0;
  
  if(boxMapPos != undefined) {
    w = boxMapPos.w;
    world = boxMapPos.world;
    universe = boxMapPos.universe
  }
  
  for(var x = Math.round(box.min.x); x <= Math.round(box.max.x); x++) {
    for(var y = Math.round(box.min.y); y <= Math.round(box.max.y); y++) {
      for(var z = Math.round(box.min.z); z <= Math.round(box.max.z); z++) {
        var pos = new THREE.Vector3(x, y, z);
        var mapPos = new MapPos(x, y, z, w, world, universe);
        var nodeData = server.getNode(mapPos);
        if(nodeData == null) { return true; }
        var def = getNodeDef(nodeData.itemstring);
        
        if(!def.walkable) {
          if(def.boundingBox != null) {
            var bb = def.boundingBox;
            if(nodeData.rot != 0) {
              var map = boudingBoxRotMap[nodeData.rot];
              bb = def.rotateBoundingBox(new THREE.Euler(map.x * (Math.PI/180), map.y * (Math.PI/180), map.z * (Math.PI/180), map.order));
            }
            for(var i = 0; i < bb.length; i++) {
              var newBox = bb[i].clone().translate(pos);
              if(collide(newBox, box)) {
                return true;
              }
            }
          } else {
            return true;
          }
        }
      }
    }
  }
  return false;
}
