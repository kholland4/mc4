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

var MAPBLOCK_SIZE = new THREE.Vector3(16, 16, 16);

class MapBlock {
  constructor(pos, content="air") {
    this.pos = pos;
    this.size = MAPBLOCK_SIZE;
    
    this.IDtoIS = [content];
    this.IStoID = {content: 0};
    
    this.data = [];
    for(var x = 0; x < this.size.x; x++) {
      var s1 = [];
      for(var y = 0; y < this.size.y; y++) {
        var s2 = [];
        for(var z = 0; z < this.size.z; z++) {
          s2.push(0);
        }
        s1.push(s2);
      }
      this.data.push(s1);
    }
    
    this.props = {sunlit: false};
    
    this.updateNum = 0;
    this.lightUpdateNum = 0;
    this.lightNeedsUpdate = 1; //0 none, 1 just this, 2 this and surrounding
    this._renderNeedsUpdate = 0;
  }
  
  get renderNeedsUpdate() { return this._renderNeedsUpdate; }
  set renderNeedsUpdate(val) {
    this._renderNeedsUpdate = val;
    //FIXME FIXME
    //cascade
    if(this._renderNeedsUpdate > 1) {
      for(var face = 0; face < 6; face++) {
        var adj = new MapPos(this.pos.x + stdFaces[face].x, this.pos.y + stdFaces[face].y, this.pos.z + stdFaces[face].z, this.pos.w, this.pos.world, this.pos.universe);
        
        var mb = server.getMapBlock(adj);
        if(mb != null) {
          if(mb.renderNeedsUpdate < this._renderNeedsUpdate - 1) {
            mb.renderNeedsUpdate = this._renderNeedsUpdate - 1;
          }
        }
      }
    }
  }
  
  getNodeID(itemstring) {
    if(!(itemstring in this.IStoID)) {
      this.IStoID[itemstring] = this.IDtoIS.length;
      this.IDtoIS.push(itemstring);
    }
    return this.IStoID[itemstring];
  }
  
  getItemstring(nodeID) {
    return this.IDtoIS[nodeID];
  }
  
  markDirty() {
    this.updateNum++;
    this.lightNeedsUpdate = 2;
    this.renderNeedsUpdate = 2;
  }
  
  mapgenComplete() {
    this.updateNum++;
    this.lightNeedsUpdate = 1;
    this.renderNeedsUpdate = 1;
  }
  
  getNode(localPos) {
    var n = this.data[localPos.x][localPos.y][localPos.z];
    return new NodeData(this.getItemstring(nodeID(n)), nodeRot(n));
  }
}

class MapgenBase {
  constructor() {
    
  }
  
  getMapBlock(pos) {
    return new MapBlock(pos);
  }
}

class MapgenDefault extends MapgenBase {
  constructor(seed) {
    super();
    this.seed = seed;
  }
  
  getMapBlock(pos) {
    noise.seed(this.seed);
    
    var block = new MapBlock(pos);
    
    function heightAt(x, z) {
      var val = noise.simplex2(x / 50, z / 50);
      
      val = Math.round(val * 10);
      
      return val;
    }
    
    var heightMap = [];
    for(var x = pos.x * block.size.x; x < (pos.x + 1) * block.size.x; x++) {
      var s1 = [];
      for(var z = pos.z * block.size.z; z < (pos.z + 1) * block.size.z; z++) {
        var val = noise.simplex2(x / 50, z / 50);
        
        val = Math.round(val * 10);
        
        s1.push(val);
      }
      heightMap.push(s1);
    }
    
    var grassID = block.getNodeID("default:grass");
    var dirtID = block.getNodeID("default:dirt");
    var stoneID = block.getNodeID("default:stone");
    
    var oreC = [2, 3];
    var ores = [
      {id: block.getNodeID("default:ore_coal"), maxY: 0, minN: 0.51, maxN: 1.0, c: 3},
      {id: block.getNodeID("default:ore_iron"), maxY: -32, minN: -1.0, maxN: -0.64, c: 3},
      {id: block.getNodeID("default:ore_diamond"), maxY: -128, minN: 0.74, maxN: 1.0, c: 2}
    ];
    
    for(var x = 0; x < block.size.x; x++) {
      var ax = x + pos.x * block.size.x;
      for(var y = 0; y < block.size.y; y++) {
        var ay = y + pos.y * block.size.y;
        var h = ay;
        for(var z = 0; z < block.size.z; z++) {
          var az = z + pos.z * block.size.z;
          
          var hmap = heightMap[x][z];
          if(h > hmap) {
            
          } else if(h == hmap) {
            block.data[x][y][z] = grassID;
          } else if(h >= hmap - 2) {
            block.data[x][y][z] = dirtID;
          } else if(h < hmap - 2) {
            var id = stoneID;
            
            for(var ci = 0; ci < oreC.length; ci++) {
              var c = oreC[ci];
              
              var oreNoise = noise.perlin3(ax / c, ay / c, az / c);
              
              for(var i = 0; i < ores.length; i++) {
                var ore = ores[i];
                if(ore.c != c) { continue; }
                if("minY" in ore) { if(ay < ore.minY) { continue; } }
                if("maxY" in ore) { if(ay > ore.maxY) { continue; } }
                
                if(ore.minN < oreNoise && oreNoise < ore.maxN) {
                  id = ore.id;
                  break;
                }
              }
              if(id != stoneID) { break; }
            }
            
            block.data[x][y][z] = id;
          }
        }
      }
    }
    
    //Trees
    var treePos = [];
    var treeMaxSpread = new THREE.Vector3(7, 10, 7);
    var basePos = new THREE.Vector3(pos.x * block.size.x, pos.y * block.size.y, pos.z * block.size.z);
    for(var x = basePos.x - treeMaxSpread.x; x < basePos.x + block.size.x + treeMaxSpread.x; x++) {
      for(var z = basePos.z - treeMaxSpread.z; z < basePos.z + block.size.z + treeMaxSpread.z; z++) {
        var hmap;
        if(x >= basePos.x && x < basePos.x + block.size.x && z >= basePos.z && z < basePos.z + block.size.z) {
          hmap = heightMap[x - basePos.x][z - basePos.z];
        } else {
          hmap = heightAt(x, z);
        }
        
        if(hmap < basePos.y - treeMaxSpread.y || hmap > basePos.y + block.size.y + treeMaxSpread.y) { continue; }
        
        var n = noise.simplex2(x / 4, z / 4);
        if(n > 0.85) {
          treePos.push(new THREE.Vector3(x, hmap + 1, z));
        }
      }
    }
    
    var trunkID = block.getNodeID("default:tree");
    var leafID = block.getNodeID("default:leaves");
    
    for(var i = 0; i < treePos.length; i++) {
      var p = treePos[i];
      
      var nodes = [];
      
      for(var n = 0; n < 6; n++) {
        nodes.push({pos: new THREE.Vector3(0, n, 0).add(p), data: trunkID});
      }
      
      for(var x = -3; x <= 3; x++) {
        for(var y = 2; y <= 6; y++) {
          for(var z = -3; z <= 3; z++) {
            if(x == 0 && z == 0 && y >= 0 && y < 6) { continue; }
            
            var r = 3;
            if(y >= 4) { r = 2; }
            if(y >= 5) { r = 1; }
            if(x < -r || x > r || z < -r || z > r || (x == -r && z == -r) || (x == r && z == -r) || (x == -r && z == r) || (x == r && z == r)) { continue; }
            if(y == 6 && x != 0 && z != 0) { continue; }
            
            var r = 2;
            if(y >= 4) { r = 1; }
            if(y >= 6) { r = 0; }
            if(!(x >= -r && x <= r && z >= -r && z <= r)) {
              if(!(noise.simplex3((p.x + x) / 15, (p.y + y) / 15, (p.z + z) / 15) > 0.2)) {
                continue;
              }
            }
            
            nodes.push({pos: new THREE.Vector3(x, y, z).add(p), data: leafID});
          }
        }
      }
      
      for(var n = 0; n < nodes.length; n++) {
        var nodePos = nodes[n].pos;
        if(nodePos.x >= basePos.x && nodePos.x < basePos.x + block.size.x && nodePos.y >= basePos.y && nodePos.y < basePos.y + block.size.y && nodePos.z >= basePos.z && nodePos.z < basePos.z + block.size.z) {
          block.data[nodePos.x - basePos.x][nodePos.y - basePos.y][nodePos.z - basePos.z] = nodes[n].data;
        }
      }
    }
    
    if(pos.y >= 1) { block.props.sunlit = true; }
    block.mapgenComplete();
    
    return block;
  }
}
