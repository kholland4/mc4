var LIGHT_MAX_WORKERS = 8;

var lightUpdateQueue = [];
var lightWorkers = [];
var lightWorkersActual = [];

function initLightWorker() {
  for(var i = 0; i < LIGHT_MAX_WORKERS; i++) {
    var _worker = new Worker("light-worker.js");
    _worker.onmessage = lightWorkerCallback;
    lightWorkersActual.push(_worker);
  }
}

class LightWorker {
  constructor(mapBlock, worker) {
    this.pos = mapBlock.pos;
    this.updateNum = mapBlock.updateNum;
    this.startTime = performance.now();
    this.worker = worker;
    
    this.ok = true;
    
    /*var pos = this.pos;
    var data = [];
    var blocks = {
      "0,0,0": mapBlock
    };
    for(var x = -1; x <= 1; x++) {
      for(var y = -1; y <= 1; y++) {
        for(var z = -1; z <= 1; z++) {
          if(x == 0 && y == 0 && z == 0) { continue; }
          blocks[x + "," + y + "," + z] = server.getMapBlock(new THREE.Vector3(pos.x + x, pos.y + y, pos.z + z));
        }
      }
    }
    
    this.ok = true;
    for(var key in blocks) {
      if(blocks[key] == null) {
        this.ok = false;
        return;
      }
    }
    
    for(var x = -mapBlock.size.x; x < mapBlock.size.x * 2; x++) {
      var s1 = [];
      for(var y = -mapBlock.size.y; y < mapBlock.size.y * 2; y++) {
        var s2 = [];
        for(var z = -mapBlock.size.z; z < mapBlock.size.z * 2; z++) {
          var mbPos = new THREE.Vector3(Math.floor(x / mapBlock.size.x), Math.floor(y / mapBlock.size.y), Math.floor(z / mapBlock.size.z));
          var mbIndex = mbPos.x + "," + mbPos.y + "," + mbPos.z;
          
          s2.push(blocks[mbIndex].data[x - mbPos.x * mapBlock.size.x][y - mbPos.y * mapBlock.size.y][z - mbPos.z * mapBlock.size.z]);
        }
        s1.push(s2);
      }
      data.push(s1);
    }
    
    var nodeDefAdj = {};
    for(var key in blocks) {
      var def = [];
      for(var i = 0; i < blocks[key].IDtoIS.length; i++) {
        var itemstring = blocks[key].IDtoIS[i];
        if(itemstring == null) {
          def.push(null);
        } else {
          def.push(getNodeDef(itemstring));
        }
      }
      
      nodeDefAdj[key] = def;
    }
    
    this.worker.postMessage({
      pos: {x: this.pos.x, y: this.pos.y, z: this.pos.z},
      size: {x: mapBlock.size.x, y: mapBlock.size.y, z: mapBlock.size.z},
      nodeDefAdj: nodeDefAdj,
      data: data,
      lightNeedsUpdate: mapBlock.lightNeedsUpdate
    });*/
  }
  
  runLocal() {
    var pos = this.pos;
    var mapBlock = server.getMapBlock(pos);
    var lightNeedsUpdate = mapBlock.lightNeedsUpdate;
    
    var dataBox = new THREE.Box3(new THREE.Vector3(-1, -1, -1), new THREE.Vector3(1, 1, 1));
    var updateBox = new THREE.Box3(new THREE.Vector3(0, 0, 0), new THREE.Vector3(0, 0, 0));
    if(lightNeedsUpdate == 2) {
      dataBox = new THREE.Box3(new THREE.Vector3(-2, -2, -2), new THREE.Vector3(2, 2, 2));
      updateBox = new THREE.Box3(new THREE.Vector3(-1, -1, -1), new THREE.Vector3(1, 1, 1));
    }
    
    var blocks = {
      "0,0,0": mapBlock
    };
    for(var x = dataBox.min.x; x <= dataBox.max.x; x++) {
      for(var y = dataBox.min.y; y <= dataBox.max.y; y++) {
        for(var z = dataBox.min.z; z <= dataBox.max.z; z++) {
          if(x == 0 && y == 0 && z == 0) { continue; }
          blocks[x + "," + y + "," + z] = server.getMapBlock(new THREE.Vector3(pos.x + x, pos.y + y, pos.z + z));
        }
      }
    }
    
    this.ok = true;
    for(var key in blocks) {
      if(blocks[key] == null) {
        this.ok = false;
        return;
      }
    }
    
    var size = mapBlock.size;
    
    var nodeDef = {};
    for(var key in blocks) {
      var def = [];
      for(var i = 0; i < blocks[key].IDtoIS.length; i++) {
        var itemstring = blocks[key].IDtoIS[i];
        if(itemstring == null) {
          def.push(null);
        } else {
          def.push(getNodeDef(itemstring));
        }
      }
      
      nodeDef[key] = def;
    }
    
    var lightSources = [];
    var sunlightSources = [];
    for(var x = updateBox.min.x * size.x; x < updateBox.max.x * size.x + size.x; x++) {
      for(var z = updateBox.min.z * size.z; z < updateBox.max.z * size.z + size.z; z++) {
        var hasSun = false;
        if(blocks[Math.floor(x / size.x) + "," + updateBox.max.y + "," + Math.floor(z / size.z)].props.sunlit) {
          hasSun = true;
        } else {
          //FIXME
          for(var y = updateBox.max.y * size.y + size.y; y <= updateBox.max.y * size.y + size.y * 4; y++) {
            var p = new THREE.Vector3(x + pos.x * size.x, y + pos.y * size.y, z + pos.z * size.z);
            var def = server.getNode(p).getDef();
            if(!def.passSunlight) { break; }
            if(((y % size.y) + size.y) % size.y == 0) {
              var mb = server.getMapBlock(globalToMapBlock(p));
              if(mb.props.sunlit) { hasSun = true; break; }
            }
          }
        }
        
        for(var y = updateBox.max.y * size.y + size.y - 1; y >= updateBox.min.y * size.y; y--) {
          var mbPos = {x: Math.floor(x / size.x), y: Math.floor(y / size.y), z: Math.floor(z / size.z)};
          var blockIndex = mbPos.x + "," + mbPos.y + "," + mbPos.z;
          
          var rx = x - mbPos.x * size.x;
          var ry = y - mbPos.y * size.y;
          var rz = z - mbPos.z * size.z;
          
          var d = blocks[blockIndex].data[rx][ry][rz];
          var id = d & 65535;
          
          var def = nodeDef[blockIndex][id];
          
          if(def.lightLevel > 0) {
            lightSources.push({pos: {x: x, y: y, z: z}, lightLevel: def.lightLevel});
          }
          
          if(!def.passSunlight) { hasSun = false; }
          
          var rot = (d >> 16) & 127;
          var light = (d >> 23) & 255;
          
          var sunlight = 0;
          if(hasSun) {
            sunlight = 14;
            sunlightSources.push({pos: {x: x, y: y, z: z}, lightLevel: 15});
          }
          
          var l = 0;
          if(def.transparent) {
            l = 1;
          }
          
          light = (sunlight << 4) | l;
          
          blocks[blockIndex].data[rx][ry][rz] = id | (rot << 16) | (light << 23);
        }
      }
    }
    
    /*function lightCascade(pos, lightLevel) {
      if(lightLevel <= 0) { return; }
      
      var mbPos = new THREE.Vector3(Math.floor(pos.x / mapBlock.size.x), Math.floor(pos.y / mapBlock.size.y), Math.floor(pos.z / mapBlock.size.z));
      if(mbPos.x < dataBox.min.x || mbPos.x > dataBox.max.x || mbPos.y < dataBox.min.y || mbPos.y > dataBox.max.y || mbPos.z < dataBox.min.z || mbPos.z > dataBox.max.z) { return; }
      var blockIndex = mbPos.x + "," + mbPos.y + "," + mbPos.z;
      
      var rx = pos.x - mbPos.x * MAPBLOCK_SIZE.x;
      var ry = pos.y - mbPos.y * MAPBLOCK_SIZE.y;
      var rz = pos.z - mbPos.z * MAPBLOCK_SIZE.z;
      
      var d = blocks[blockIndex].data[rx][ry][rz];
      var id = d & 65535;
      
      var def = nodeDef[blockIndex][id];
      
      if(def.transparent) {
        var rot = (d >> 16) & 127;
        var light = (d >> 23) & 255;
        
        if(lightLevel > (light & 0xF)) {
          light &= 0xF0;
          light |= lightLevel & 0xF;
          
          blocks[blockIndex].data[rx][ry][rz] = id | (rot << 16) | (light << 23);
          
          for(var face = 0; face < 6; face++) {
            lightCascade({x: pos.x + stdFaces[face].x, y: pos.y + stdFaces[face].y, z: pos.z + stdFaces[face].z}, lightLevel - 1);
          }
        }
      }
    }*/
    
    var blocksArr = [];
    for(var x = dataBox.min.x; x <= dataBox.max.x; x++) {
      var s1 = [];
      for(var y = dataBox.min.y; y <= dataBox.max.y; y++) {
        var s2 = [];
        for(var z = dataBox.min.z; z <= dataBox.max.z; z++) {
          var mbPos = new THREE.Vector3(x, y, z);
          var blockIndex = mbPos.x + "," + mbPos.y + "," + mbPos.z;
          s2.push(blocks[blockIndex].data);
        }
        s1.push(s2);
      }
      blocksArr.push(s1);
    }
    
    function lightCascade(pos, lightLevel, type=0) {
      if(lightLevel <= 0) { return; }
      
      var mbx = Math.floor(pos.x / mapBlock.size.x) | 0;
      var mby = Math.floor(pos.y / mapBlock.size.y) | 0;
      var mbz = Math.floor(pos.z / mapBlock.size.z) | 0;
      
      if(mbx < dataBox.min.x || mbx > dataBox.max.x || mby < dataBox.min.y || mby > dataBox.max.y || mbz < dataBox.min.z || mbz > dataBox.max.z) { return; }
      
      var rx = (pos.x - mbx * MAPBLOCK_SIZE.x) | 0;
      var ry = (pos.y - mby * MAPBLOCK_SIZE.y) | 0;
      var rz = (pos.z - mbz * MAPBLOCK_SIZE.z) | 0;
      
      var mx = (mbx - dataBox.min.x) | 0;
      var my = (mby - dataBox.min.y) | 0;
      var mz = (mbz - dataBox.min.z) | 0;
      
      var d = blocksArr[mx][my][mz][rx][ry][rz] | 0;
      var id = d & 65535;
      var rot = (d >> 16) & 127;
      var light = (d >> 23) & 255;
      
      if((light & 15) >= 1) { //initialized to 1 for transparent, 0 for opaque
        var sv = false;
        if(type == 0) {
          if(lightLevel > (light & 0xF)) {
            light &= 0xF0;
            light |= lightLevel & 0xF;
            sv = true;
          }
        } else if(type == 1) {
          if(lightLevel > ((light >> 4) & 0xF)) {
            light &= 0xF;
            light |= (lightLevel << 4) & 0xF0;
            sv = true;
          }
        }
        
        if(sv) {
          blocksArr[mx][my][mz][rx][ry][rz] = id | (rot << 16) | (light << 23);
          
          if(lightLevel - 1 > 1) {
            for(var face = 0; face < 6; face++) {
              lightCascade({x: pos.x + stdFaces[face].x, y: pos.y + stdFaces[face].y, z: pos.z + stdFaces[face].z}, lightLevel - 1, type);
            }
          }
        }
      }
    }
    
    for(var i = 0; i < lightSources.length; i++) {
      lightCascade(lightSources[i].pos, lightSources[i].lightLevel);
    }
    for(var i = 0; i < sunlightSources.length; i++) {
      lightCascade(sunlightSources[i].pos, sunlightSources[i].lightLevel, 1);
    }
    
    (lightWorkerCallback.bind(this.worker))(null);
  }
}

function lightUpdate() {
  while(lightUpdateQueue.length > 0 && lightWorkers.length < LIGHT_MAX_WORKERS) {
    var targetPos = null;
    for(var i = 0; i < lightUpdateQueue.length; i++) {
      var ok = true;
      for(var n = 0; n < lightWorkers.length; n++) {
        if(lightWorkers[n].pos.equals(lightUpdateQueue[i])) {
          ok = false;
        }
      }
      if(ok) {
        targetPos = lightUpdateQueue[i];
        break;
      }
    }
    
    if(targetPos == null) { break; }
    
    var mapBlock = server.getMapBlock(targetPos);
    if(mapBlock == null) { break; }
    
    var targetWorker = null;
    for(var i = 0; i < lightWorkersActual.length; i++) {
      var ok = true;
      for(var n = 0; n < lightWorkers.length; n++) {
        if(lightWorkers[n].worker == lightWorkersActual[i]) {
          ok = false;
          break;
        }
      }
      if(ok) {
        targetWorker = lightWorkersActual[i];
        break;
      }
    }
    
    if(targetWorker == null) { break; }
    
    var worker = new LightWorker(mapBlock, targetWorker);
    if(worker.ok) {
      lightWorkers.push(worker);
    } else {
      break;
    }
  }
  
  for(var i = 0; i < lightWorkers.length; i++) {
    lightWorkers[i].runLocal();
  }
}

function lightQueueUpdate(pos) {
  for(var i = 0; i < lightUpdateQueue.length; i++) {
    if(lightUpdateQueue[i].equals(pos)) {
      return;
    }
  }
  lightUpdateQueue.push(pos);
  
  //lightUpdate();
}

function lightWorkerCallback(message) {
  var pos;
  var updateNum;
  for(var i = 0; i < lightWorkers.length; i++) {
    if(lightWorkers[i].worker == this) {
      pos = lightWorkers[i].pos;
      updateNum = lightWorkers[i].updateNum;
      //console.log(performance.now() - lightWorkers[i].startTime);
      lightWorkers.splice(i, 1);
      break;
    }
  }
  if(pos == undefined) { return; }
  
  for(var i = 0; i < lightUpdateQueue.length; i++) {
    if(lightUpdateQueue[i].equals(pos)) {
      lightUpdateQueue.splice(i, 1);
      break;
    }
  }
  
  var mapBlock = server.getMapBlock(pos);
  
  if(message != null) {
    var data = message.data.data;
    
    var blocks = {
      "0,0,0": mapBlock
    };
    for(var x = -1; x <= 1; x++) {
      for(var y = -1; y <= 1; y++) {
        for(var z = -1; z <= 1; z++) {
          if(x == 0 && y == 0 && z == 0) { continue; }
          blocks[x + "," + y + "," + z] = server.getMapBlock(new THREE.Vector3(pos.x + x, pos.y + y, pos.z + z));
        }
      }
    }
    
    for(var key in blocks) {
      if(blocks[key] == null) {
        //uh oh
      }
    }
    
    for(var x = -mapBlock.size.x; x < mapBlock.size.x * 2; x++) {
      for(var y = -mapBlock.size.y; y < mapBlock.size.y * 2; y++) {
        for(var z = -mapBlock.size.z; z < mapBlock.size.z * 2; z++) {
          var mbPos = new THREE.Vector3(Math.floor(x / mapBlock.size.x), Math.floor(y / mapBlock.size.y), Math.floor(z / mapBlock.size.z));
          var mbIndex = mbPos.x + "," + mbPos.y + "," + mbPos.z;
          
          var d = data[x + mapBlock.size.x][y + mapBlock.size.y][z + mapBlock.size.z];
          blocks[mbIndex].data[x - mbPos.x * mapBlock.size.x][y - mbPos.y * mapBlock.size.y][z - mbPos.z * mapBlock.size.z] = d;
        }
      }
    }
    
    //FIXME: server.setMapBlock???
  }
  
  if(updateNum == mapBlock.updateNum) {
    mapBlock.lightNeedsUpdate = 0;
  }
  
  //lightUpdate();
}
