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

class MapBase {
  constructor() {
    
  }
  
  getMapBlock(pos) {
    return new MapBlock(pos);
  }
}

class MapLocal extends MapBase {
  constructor(mapgen) {
    super();
    this.mapgen = mapgen;
    
    this.cache = {};
    this.saved = {};
  }
  
  getMapBlock(pos) {
    var index = pos.x + "," + pos.y + "," + pos.z + "," + pos.w + "," + pos.world + "," + pos.universe;
    if(index in this.saved) {
      return this.saved[index];
    } else if(index in this.cache) {
      return this.cache[index];
    } else {
      var mapBlock = this.mapgen.getMapBlock(pos);
      this.cache[index] = mapBlock;
      return mapBlock;
    }
  }
  
  dirtyMapBlock(pos) {
    var index = pos.x + "," + pos.y + "," + pos.z + "," + pos.w + "," + pos.world + "," + pos.universe;
    if(!(index in this.saved)) {
      if(index in this.cache) {
        this.saved[index] = this.cache[index];
        delete this.cache[index];
      } else {
        this.saved[index] = this.getMapBlock(pos);
      }
    }
    //this.saved[index].markDirty();
  }
}

//Packed node data format (8 bits for light, 8 for rot, and 15 for id):
//| light||  rot ||           id|
//322222222221111111111
//0987654321098765432109876543210

//|         light|
//|   sun|| other|
// 7 6 5 4 3 2 1 0

function nodeID(n) {
  return n & 32767;
}
function nodeRot(n) {
  return (n >> 15) & 255;
}
function nodeLight(n) {
  return (n >> 23) & 255;
}
function nodeN(id, rot=0, light=0) {
  return (id & 32767) + ((rot & 255) << 15) + ((light & 255) << 23);
}

function globalToMapBlock(pos) {
  return new THREE.Vector3(Math.floor(pos.x / MAPBLOCK_SIZE.x), Math.floor(pos.y / MAPBLOCK_SIZE.y), Math.floor(pos.z / MAPBLOCK_SIZE.z));
}
