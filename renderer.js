//TODO: scrap and rewrite this file

var RENDER_MAX_WORKERS = 2; //4
var RENDER_MAX_LIGHTING_UPDATES = 2;

var sunAmount = 1;

//renderDist *must* be greater that unrenderDist in all dimensions
var renderDist = new THREE.Vector3(2, 2, 2); //4, 2, 4
var unrenderDist = new THREE.Vector3(7, 4, 7);

var renderCurrentMeshes = {};

var renderUpdateQueue = [];
var renderWorkers = [];

var renderWorkersActual = [];

var renderLightingUpdateQueue = [];

function initRenderer() {
  for(var i = 0; i < RENDER_MAX_WORKERS; i++) {
    var _worker = new Worker("meshgen-worker.js");
    _worker.onmessage = renderWorkerCallback;
    renderWorkersActual.push(_worker);
  }
}

class RenderWorker {
  constructor(mapBlock, worker) {
    this.pos = mapBlock.pos;
    this.updateNum = mapBlock.updateNum;
    this.startTime = performance.now();
    this.worker = worker;
    var nodeDef = [];
    for(var i = 0; i < mapBlock.IDtoIS.length; i++) {
      var itemstring = mapBlock.IDtoIS[i];
      if(itemstring == null) {
        nodeDef.push(null);
      } else {
        nodeDef.push(getNodeDef(itemstring));
      }
    }
    
    //FIXME: will break for unavailable mapblocks
    var pos = this.pos;
    var data = [];
    var blocks = {
      "0,0,0": mapBlock,
      "-1,0,0": server.getMapBlock(new THREE.Vector3(pos.x - 1, pos.y, pos.z), true),
      "1,0,0": server.getMapBlock(new THREE.Vector3(pos.x + 1, pos.y, pos.z), true),
      "0,-1,0": server.getMapBlock(new THREE.Vector3(pos.x, pos.y - 1, pos.z), true),
      "0,1,0": server.getMapBlock(new THREE.Vector3(pos.x, pos.y + 1, pos.z), true),
      "0,0,-1": server.getMapBlock(new THREE.Vector3(pos.x, pos.y, pos.z - 1), true),
      "0,0,1": server.getMapBlock(new THREE.Vector3(pos.x, pos.y, pos.z + 1), true)
    };
    
    this.ok = true;
    for(var key in blocks) {
      if(blocks[key] == null) {
        this.ok = false;
        return;
      }
    }
    
    for(var x = -1; x < mapBlock.size.x + 1; x++) {
      var s1 = [];
      for(var y = -1; y < mapBlock.size.y + 1; y++) {
        var s2 = [];
        for(var z = -1; z < mapBlock.size.z + 1; z++) {
          if(x < 0 && y >= 0 && y < mapBlock.size.y && z >= 0 && z < mapBlock.size.z) {
            s2.push(blocks["-1,0,0"].data[mapBlock.size.x - 1][y][z]);
          } else if(x >= mapBlock.size.x && y >= 0 && y < mapBlock.size.y && z >= 0 && z < mapBlock.size.z) {
            s2.push(blocks["1,0,0"].data[0][y][z]);
          } else if(y < 0 && x >= 0 && x < mapBlock.size.x && z >= 0 && z < mapBlock.size.z) {
            s2.push(blocks["0,-1,0"].data[x][mapBlock.size.y - 1][z]);
          } else if(y >= mapBlock.size.y && x >= 0 && x < mapBlock.size.x && z >= 0 && z < mapBlock.size.z) {setSun
            s2.push(blocks["0,1,0"].data[x][0][z]);
          } else if(z < 0 && x >= 0 && x < mapBlock.size.x && y >= 0 && y < mapBlock.size.y) {
            s2.push(blocks["0,0,-1"].data[x][y][mapBlock.size.z - 1]);
          } else if(z >= mapBlock.size.z && x >= 0 && x < mapBlock.size.x && y >= 0 && y < mapBlock.size.y) {
            s2.push(blocks["0,0,1"].data[x][y][0]);
          } else if(z >= 0 && z < mapBlock.size.z && x >= 0 && x < mapBlock.size.x && y >= 0 && y < mapBlock.size.y) {
            s2.push(mapBlock.data[x][y][z]);
          } else {
            s2.push(0);
          }
        }
        s1.push(s2);
      }
      data.push(s1);
    }
    
    var nodeDefAdj = {};
    for(var key in blocks) {
      if(key == "0,0,0") { continue; }
      
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
      nodeDef: nodeDef,
      nodeDefAdj: nodeDefAdj,
      data: data,
      lightNeedsUpdate: mapBlock.lightNeedsUpdate,
      sunAmount: sunAmount
      //ldata: ldata
    });
  }
}

class RenderMesh {
  constructor(pos, updateNum, obj) {
    this.pos = pos;
    this.updateNum = updateNum;
    this.obj = obj;
    this.facePos = null;
  }
}

function renderUpdateMap(centerPos) {
  var schedCount = 0;
  
  for(var x = centerPos.x - renderDist.x; x <= centerPos.x + renderDist.x; x++) {
    for(var y = centerPos.y - renderDist.y; y <= centerPos.y + renderDist.y; y++) {
      for(var z = centerPos.z - renderDist.z; z <= centerPos.z + renderDist.z; z++) {
        var pos = new THREE.Vector3(x, y, z);
        
        var index = x.toString() + "," + y.toString() + "," + z.toString();
        if(index in renderCurrentMeshes) {
          //calling getMapBlock should cache the block, so it's not a waste of resources
          var mapBlock = server.getMapBlock(pos, true);
          if(mapBlock != null) {
            /*if(mapBlock.updateNum != renderCurrentMeshes[index].updateNum) {
              renderQueueUpdate(pos);
            }*/
            if(mapBlock.renderNeedsUpdate > 0) {
              renderQueueUpdate(pos, true);
              schedCount++;
            }
          }
        } else {
          if(schedCount < RENDER_MAX_WORKERS) { //FIXME
            renderQueueUpdate(pos);
            schedCount++;
          }
        }
      }
    }
  }
  
  //remove out-of-range ones
  for(var key in renderCurrentMeshes) {
    if(renderCurrentMeshes[key].pos.x < centerPos.x - unrenderDist.x || renderCurrentMeshes[key].pos.x > centerPos.x + unrenderDist.x ||
       renderCurrentMeshes[key].pos.y < centerPos.y - unrenderDist.y || renderCurrentMeshes[key].pos.y > centerPos.y + unrenderDist.y ||
       renderCurrentMeshes[key].pos.z < centerPos.z - unrenderDist.z || renderCurrentMeshes[key].pos.z > centerPos.z + unrenderDist.z) {
      renderCurrentMeshes[key].obj.geometry.dispose();
      renderCurrentMeshes[key].obj.material.dispose();
      renderMapGroup.remove(renderCurrentMeshes[key].obj);
      renderCurrentMeshes[key] = null;
      delete renderCurrentMeshes[key];
    }
  }
  
  while(renderUpdateQueue.length > 0 && renderWorkers.length < RENDER_MAX_WORKERS) {
    var targetPos = null;
    for(var i = 0; i < renderUpdateQueue.length; i++) {
      var ok = true;
      for(var n = 0; n < renderWorkers.length; n++) {
        if(renderWorkers[n].pos.equals(renderUpdateQueue[i])) {
          ok = false;
        }
      }
      if(ok) {
        targetPos = renderUpdateQueue[i];
        break;
      }
    }
    
    if(targetPos == null) { break; }
    
    var mapBlock = server.getMapBlock(targetPos, true);
    if(mapBlock == null) { break; }
    
    var targetWorker = null;
    for(var i = 0; i < renderWorkersActual.length; i++) {
      var ok = true;
      for(var n = 0; n < renderWorkers.length; n++) {
        if(renderWorkers[n].worker == renderWorkersActual[i]) {
          ok = false;
          break;
        }
      }
      if(ok) {
        targetWorker = renderWorkersActual[i];
        break;
      }
    }
    
    if(targetWorker == null) { break; }
    
    var worker = new RenderWorker(mapBlock, targetWorker);
    if(worker.ok) {
      if(mapBlock.renderNeedsUpdate > 1) {
        //FIXME bodge
        for(var lx = -1; lx <= 1; lx++) {
          for(var ly = -1; ly <= 1; ly++) {
            for(var lz = -1; lz <= 1; lz++) {
              var adj = new THREE.Vector3(lx, ly, lz).add(mapBlock.pos);
              var mb = server.getMapBlock(adj);
              if(mb == null) { continue; }
              if(mb.renderNeedsUpdate == 0) {
                renderQueueLightingUpdate(adj);
              }
            }
          }
        }
      }
      
      renderWorkers.push(worker);
    } else {
      break;
    }
  }
  
  //FIXME
  for(var i = 0; i < RENDER_MAX_LIGHTING_UPDATES; i++) {
    if(renderLightingUpdateQueue.length > 0) {
      var pos = renderLightingUpdateQueue[0];
      renderLightingUpdateQueue.splice(0, 1);
      renderUpdateLighting(pos);
    }
  }
}

function renderQueueUpdate(pos, priority=false) {
  for(var i = 0; i < renderUpdateQueue.length; i++) {
    if(renderUpdateQueue[i].equals(pos)) {
      return;
    }
  }
  if(priority) {
    renderUpdateQueue.unshift(pos);
  } else {
    renderUpdateQueue.push(pos);
  }
}

function renderWorkerCallback(message) {
  var pos;
  var updateNum;
  for(var i = 0; i < renderWorkers.length; i++) {
    if(renderWorkers[i].worker == this) {
      pos = renderWorkers[i].pos;
      updateNum = renderWorkers[i].updateNum;
      //console.log(performance.now() - renderWorkers[i].startTime);
      renderWorkers.splice(i, 1);
      break;
    }
  }
  if(pos == undefined) { return; }
  
  for(var i = 0; i < renderUpdateQueue.length; i++) {
    if(renderUpdateQueue[i].equals(pos)) {
      renderUpdateQueue.splice(i, 1);
      break;
    }
  }
  
  var index = pos.x.toString() + "," + pos.y.toString() + "," + pos.z.toString();
  var current = renderCurrentMeshes[index];
  if(current != undefined && current != null) {
    current.obj.geometry.dispose();
    current.obj.material.dispose();
    renderMapGroup.remove(current.obj);
    current = null;
    delete renderCurrentMeshes[index];
  }
  
  var verts = message.data.verts;
  var uvs = message.data.uvs;
  var colors = message.data.colors;
  var facePos = message.data.facePos;
  
  var geometry = new THREE.BufferGeometry();
  geometry.setAttribute("position", new THREE.BufferAttribute(new Float32Array(verts), 3));
  geometry.setAttribute("uv", new THREE.BufferAttribute(new Float32Array(uvs), 2));
  geometry.setAttribute("color", new THREE.BufferAttribute(new Uint8Array(colors), 3, true));
  var material = new THREE.MeshBasicMaterial({map: texmap, vertexColors: THREE.VertexColors, alphaTest: 0.5});
  var mesh = new THREE.Mesh(geometry, material);
  mesh.position.x = pos.x * MAPBLOCK_SIZE.x;
  mesh.position.y = pos.y * MAPBLOCK_SIZE.y;
  mesh.position.z = pos.z * MAPBLOCK_SIZE.z;
  
  renderMapGroup.add(mesh);
  
  var renderObj = new RenderMesh(pos, updateNum, mesh);
  renderObj.facePos = facePos;
  var index = pos.x.toString() + "," + pos.y.toString() + "," + pos.z.toString();
  renderCurrentMeshes[index] = renderObj;
  
  var mapBlock = server.getMapBlock(pos);
  if(updateNum == mapBlock.updateNum) {
    mapBlock.renderNeedsUpdate = 0;
  }
}

function renderQueueLightingUpdate(pos) {
  for(var i = 0; i < renderLightingUpdateQueue.length; i++) {
    if(renderLightingUpdateQueue[i].equals(pos)) {
      return;
    }
  }
  renderLightingUpdateQueue.push(pos);
}

function renderUpdateLighting(pos) {
  var index = pos.x.toString() + "," + pos.y.toString() + "," + pos.z.toString();
  if(!(index in renderCurrentMeshes)) { return false; }
  
  var renderObj = renderCurrentMeshes[index];
  
  var mapBlock = server.getMapBlock(pos, true);
  if(mapBlock == null) { return false; }
  if(mapBlock.updateNum != renderObj.updateNum) { return false; }
  
  var blocks = {
    "0,0,0": mapBlock
  };
  for(var i = 0; i < stdFaces.length; i++) {
    var index = stdFaces[i].x + "," + stdFaces[i].y + "," + stdFaces[i].z;
    blocks[index] = server.getMapBlock(new THREE.Vector3(stdFaces[i].x, stdFaces[i].y, stdFaces[i].z).add(pos));
    if(blocks[index] == null) { return false; }
  }
  
  var attr = renderObj.obj.geometry.getAttribute("color");
  
  var facePos = renderObj.facePos;
  for(var i = 0; i < facePos.length; i++) {
    var rx = facePos[i][0];
    var ry = facePos[i][1];
    var rz = facePos[i][2];
    
    var d;
    if(rx < 0) { d = blocks["-1,0,0"].data[mapBlock.size.x - 1][ry][rz]; } else
    if(ry < 0) { d = blocks["0,-1,0"].data[rx][mapBlock.size.y - 1][rz]; } else
    if(rz < 0) { d = blocks["0,0,-1"].data[rx][ry][mapBlock.size.z - 1]; } else
    if(rx >= mapBlock.size.x) { d = blocks["1,0,0"].data[0][ry][rz]; } else
    if(ry >= mapBlock.size.y) { d = blocks["0,1,0"].data[rx][0][rz]; } else
    if(rz >= mapBlock.size.z) { d = blocks["0,0,1"].data[rx][ry][0]; } else
    { d = blocks["0,0,0"].data[rx][ry][rz]; }
    
    var id = d & 65535;
    var lightRaw = (d >> 23) & 255;
    var light = Math.max(lightRaw & 15, ((lightRaw >> 4) & 15) * sunAmount);
    light = Math.max(light, 1);
    
    var tint = facePos[i][3];
    
    var colorR = Math.round(lightCurve[light] * tint);
    var colorG = colorR;
    var colorB = colorR;
    
    attr.array[i * 3] = colorR;
    attr.array[i * 3 + 1] = colorG;
    attr.array[i * 3 + 2] = colorB;
  }
  
  attr.needsUpdate = true;
  renderObj.obj.geometry.setAttribute("color", attr);
  
  return true;
}

function setSun(amount) {
  amount = Math.max(Math.min(amount, 1), 0);
  if(amount == sunAmount) { return; }
  sunAmount = amount;
  
  for(var key in renderCurrentMeshes) {
    renderQueueLightingUpdate(renderCurrentMeshes[key].pos);
  }
}

api.setSun = setSun;
api.getSun = function() { return sunAmount; }
