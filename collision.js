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

function collide(box1, box2) {
  return box1.intersectsBox(box2);
}

function collideMap(box) {
  for(var x = Math.round(box.min.x); x <= Math.round(box.max.x); x++) {
    for(var y = Math.round(box.min.y); y <= Math.round(box.max.y); y++) {
      for(var z = Math.round(box.min.z); z <= Math.round(box.max.z); z++) {
        var pos = new THREE.Vector3(x, y, z);
        var nodeData = server.getNode(pos);
        if(nodeData == null) { return true; }
        var def = getNodeDef(nodeData.itemstring);
        
        if(!def.walkable) {
          if(def.boundingBox != null) {
            for(var i = 0; i < def.boundingBox.length; i++) {
              var newBox = def.boundingBox[i].clone().translate(pos);
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
