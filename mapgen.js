var MAPBLOCK_SIZE = new THREE.Vector3(16, 16, 16);

class MapBlock {
  constructor(pos) {
    this.pos = pos;
    this.size = MAPBLOCK_SIZE;
    
    this.IDtoIS = ["default:air"];
    this.IStoID = {"default:air": 0};
    
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
    
    this.updateNum = 0;
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
    
    for(var x = 0; x < block.size.x; x++) {
      for(var y = 0; y < block.size.y; y++) {
        var h = y + (pos.y * block.size.y);
        for(var z = 0; z < block.size.z; z++) {
          var hmap = heightMap[x][z];
          if(h > hmap) {
            
          } else if(h == hmap) {
            block.data[x][y][z] = grassID;
          } else if(h >= hmap - 2) {
            block.data[x][y][z] = dirtID;
          } else if(h < hmap - 2) {
            block.data[x][y][z] = stoneID;
          }
        }
      }
    }
    
    block.markDirty();
    
    return block;
  }
}
