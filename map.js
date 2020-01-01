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
    var index = pos.x + "," + pos.y + "," + pos.z;
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
    var index = pos.x + "," + pos.y + "," + pos.z;
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

function nodeID(n) {
  return n & 65535;
}
function nodeRot(n) {
  return (n >> 16) & 127;
}
function nodeLight(n) {
  return (n >> 23) & 255;
}
function nodeN(id, rot=0, light=0) {
  return (id & 65535) + ((rot & 127) << 16) + ((light & 255) << 23);
}

function globalToMapBlock(pos) {
  return new THREE.Vector3(Math.floor(pos.x / MAPBLOCK_SIZE.x), Math.floor(pos.y / MAPBLOCK_SIZE.y), Math.floor(pos.z / MAPBLOCK_SIZE.z));
}
