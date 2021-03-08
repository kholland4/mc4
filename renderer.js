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
    var _worker = new Worker("meshgen-worker.js?v=" + VERSION);
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
          } else if(y >= mapBlock.size.y && x >= 0 && x < mapBlock.size.x && z >= 0 && z < mapBlock.size.z) {
            s2.push(blocks["0,1,0"].data[x][0][z]);
          } else if(z < 0 && x >= 0 && x < mapBlock.size.x && y >= 0 && y < mapBlock.size.y) {
            s2.push(blocks["0,0,-1"].data[x][y][mapBlock.size.z - 1]);
          } else if(z >= mapBlock.size.z && x >= 0 && x < mapBlock.size.x && y >= 0 && y < mapBlock.size.y) {
            s2.push(blocks["0,0,1"].data[x][y][0]);
          } else if(z >= 0 && z < mapBlock.size.z && x >= 0 && x < mapBlock.size.x && y >= 0 && y < mapBlock.size.y) {
            s2.push(mapBlock.data[x][y][z]);
          } else {
            s2.push(-1);
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
      unknownDef: getNodeDef("unknown"),
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
  
  //FIXME precompute this and only update it when renderDist changes
  var updatePosList = [];
  
  for(var dist = 0; dist <= Math.max(renderDist.x, renderDist.y, renderDist.z); dist++) {
    var distX = Math.min(dist, renderDist.x);
    var distY = Math.min(dist, renderDist.y);
    var distZ = Math.min(dist, renderDist.z);
    for(var x = centerPos.x - distX; x <= centerPos.x + distX; x++) {
      for(var y = centerPos.y - distY; y <= centerPos.y + distY; y++) {
        for(var z = centerPos.z - distZ; z <= centerPos.z + distZ; z++) {
          var pos = new THREE.Vector3(x, y, z);
          
          var inList = false;
          for(var i = 0; i < updatePosList.length; i++) {
            if(pos.equals(updatePosList[i])) {
              inList = true;
              break;
            }
          }
          if(inList) { continue; }
          
          updatePosList.push(pos);
        }
      }
    }
  }
  
  for(var i = 0; i < updatePosList.length; i++) {
    var pos = updatePosList[i];
    
    var index = pos.x.toString() + "," + pos.y.toString() + "," + pos.z.toString();
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
      //Request all the data needed to render the mapblock.
      server.requestMapBlock(pos); //does nothing if we already have the mapblock
      for(var n = 0; n < 6; n++) {
        var adj = new THREE.Vector3(pos.x + stdFaces[n].x, pos.y + stdFaces[n].y, pos.z + stdFaces[n].z);
        server.requestMapBlock(adj);
      }
      
      if(schedCount < RENDER_MAX_WORKERS) { //FIXME
        renderQueueUpdate(pos);
        schedCount++;
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
        //FIXME this commented-out code may be needed for singleplayer
        /*for(var lx = -1; lx <= 1; lx++) {
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
        }*/
        for(var i = 0; i < 6; i++) {
          var adj = new THREE.Vector3(mapBlock.pos.x + stdFaces[i].x, mapBlock.pos.y + stdFaces[i].y, mapBlock.pos.z + stdFaces[i].z);
          var mb = server.getMapBlock(adj);
          if(mb == null) { continue; }
          if(mb.renderNeedsUpdate == 0) {
            renderQueueUpdate(adj);
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
    var ok_index = 0;
    var ok = false;
    if(ok_index < renderLightingUpdateQueue.length && !ok) {
      ok = true;
      for(var n = 0; n < renderWorkers.length; n++) {
        if(renderWorkers[n].pos.equals(renderLightingUpdateQueue[ok_index])) {
          ok_index++;
          ok = false;
          break;
        }
      }
    }
    if(ok) {
      var pos = renderLightingUpdateQueue[ok_index];
      renderLightingUpdateQueue.splice(ok_index, 1);
      
      var res = renderUpdateLighting(pos);
      if(res == false) {
        //WARNING: if the server never fetches the wanted mapblocks, the queue will become clogged.
        //FIXME: work on other lighting queue items while waiting
        renderQueueLightingUpdate(pos);
      }
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
    //if(blocks[index] == null) { return false; }
  }
  
  var attr = renderObj.obj.geometry.getAttribute("color");
  
  var facePos = renderObj.facePos;
  for(var i = 0; i < facePos.length; i++) {
    var rx = facePos[i][0];
    var ry = facePos[i][1];
    var rz = facePos[i][2];
    
    var d = mapBlock.data[rx][ry][rz];
    /*var d;
    if(rx < 0) { d = blocks["-1,0,0"].data[mapBlock.size.x - 1][ry][rz]; } else
    if(ry < 0) { d = blocks["0,-1,0"].data[rx][mapBlock.size.y - 1][rz]; } else
    if(rz < 0) { d = blocks["0,0,-1"].data[rx][ry][mapBlock.size.z - 1]; } else
    if(rx >= mapBlock.size.x) { d = blocks["1,0,0"].data[0][ry][rz]; } else
    if(ry >= mapBlock.size.y) { d = blocks["0,1,0"].data[rx][0][rz]; } else
    if(rz >= mapBlock.size.z) { d = blocks["0,0,1"].data[rx][ry][0]; } else
    { d = blocks["0,0,0"].data[rx][ry][rz]; }*/
    
    var id = d & 32767;
    var lightRaw = (d >> 23) & 255;
    var light = Math.max(lightRaw & 15, Math.round(((lightRaw >> 4) & 15) * sunAmount));
    light = Math.max(light, 1);
    
    var tint = facePos[i][3];
    var tLight = facePos[i][4];
    
    var colorR, colorG, colorB;
    if(tLight) {
      colorR = Math.round(lightCurve[light] * tint);
      colorG = colorR;
      colorB = colorR;
    } else {
      var total = lightCurve[light] * tint;
      var count = 1;
      
      var adjList = facePos[i][5];
      for(var n = 0; n < adjList.length; n++) {
        if(adjList[n] == null) { continue; }
        
        var fx = adjList[n][0];
        var fy = adjList[n][1];
        var fz = adjList[n][2];
        
        if(fx >= -1 && fx < MAPBLOCK_SIZE.x + 1 &&
           fy >= -1 && fy < MAPBLOCK_SIZE.y + 1 &&
           fz >= -1 && fz < MAPBLOCK_SIZE.z + 1) {
          var dimCount = 0;
          if(fx < 0 || fx >= mapBlock.size.x) { dimCount++; }
          if(fy < 0 || fy >= mapBlock.size.y) { dimCount++; }
          if(fz < 0 || fz >= mapBlock.size.z) { dimCount++; }
          if(dimCount > 1) { continue; }
          //var adjD = mapBlock.data[adjList[n][0]][adjList[n][1]][adjList[n][2]];
          var adjD = -1;
          if(fx < 0) { if(blocks["-1,0,0"] != null) { adjD = blocks["-1,0,0"].data[mapBlock.size.x - 1][fy][fz]; } } else
          if(fy < 0) { if(blocks["0,-1,0"] != null) { adjD = blocks["0,-1,0"].data[fx][mapBlock.size.y - 1][fz]; } } else
          if(fz < 0) { if(blocks["0,0-1"] != null) { adjD = blocks["0,0,-1"].data[fx][fy][mapBlock.size.z - 1]; } } else
          if(fx >= mapBlock.size.x) { if(blocks["1,0,0"] != null) { adjD = blocks["1,0,0"].data[0][fy][fz]; } } else
          if(fy >= mapBlock.size.y) { if(blocks["0,1,0"] != null) { adjD = blocks["0,1,0"].data[fx][0][fz]; } } else
          if(fz >= mapBlock.size.z) { if(blocks["0,0,1"] != null) { adjD = blocks["0,0,1"].data[fx][fy][0]; } } else
          { adjD = blocks["0,0,0"].data[fx][fy][fz]; }
          if(adjD == -1) { continue; }
          
          var adjLightRaw = (adjD >> 23) & 255;
          var adjLight = Math.max(adjLightRaw & 15, Math.round(((adjLightRaw >> 4) & 15) * sunAmount));
          if(adjLight > 0 && adjLight < light - 2) { continue; }
          if(adjLight > 0 && light < adjLight - 2) { continue; }
          if(adjLight == 0) { adjLight = Math.floor(light / 3); }
          adjLight = Math.max(adjLight, 1);
          
          total += lightCurve[adjLight] * tint;
          count++;
        }
      }
      
      var avgLight = Math.round(total / count);
      colorR = avgLight;
      colorG = colorR;
      colorB = colorR;
    }
    
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
