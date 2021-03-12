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

var SERVER_REMOTE_UPDATE_INTERVAL = 0.25; //how often to send updates about, i. e., the player position to the remote server

class ServerBase {
  constructor() {
    this.entities = {};
    this.messageHooks = {};
  }
  
  addPlayer(player) {
    
  }
  
  getMapBlock(pos, needLight=false) {
    return new MapBlock(pos);
  }
  setMapBlock(pos, mapBlock) {
    
  }
  requestMapBlock(pos) {}
  
  getNodeRaw(pos) {
    var mapBlockPos = new MapPos(Math.floor(pos.x / MAPBLOCK_SIZE.x), Math.floor(pos.y / MAPBLOCK_SIZE.y), Math.floor(pos.z / MAPBLOCK_SIZE.z), pos.w, pos.world, pos.universe);
    var mapBlock = this.getMapBlock(mapBlockPos);
    if(mapBlock == null) { return null; }
    
    var localPos = new MapPos(
      ((pos.x % MAPBLOCK_SIZE.x) + MAPBLOCK_SIZE.x) % MAPBLOCK_SIZE.x,
      ((pos.y % MAPBLOCK_SIZE.y) + MAPBLOCK_SIZE.y) % MAPBLOCK_SIZE.y,
      ((pos.z % MAPBLOCK_SIZE.z) + MAPBLOCK_SIZE.z) % MAPBLOCK_SIZE.z,
      0, 0, 0);
    
    var n = mapBlock.data[localPos.x][localPos.y][localPos.z];
    
    return n;
  }
  getNode(pos) {
    var mapBlockPos = new MapPos(Math.floor(pos.x / MAPBLOCK_SIZE.x), Math.floor(pos.y / MAPBLOCK_SIZE.y), Math.floor(pos.z / MAPBLOCK_SIZE.z), pos.w, pos.world, pos.universe);
    var mapBlock = this.getMapBlock(mapBlockPos);
    if(mapBlock == null) { return null; }
    
    var localPos = new MapPos(
      ((pos.x % MAPBLOCK_SIZE.x) + MAPBLOCK_SIZE.x) % MAPBLOCK_SIZE.x,
      ((pos.y % MAPBLOCK_SIZE.y) + MAPBLOCK_SIZE.y) % MAPBLOCK_SIZE.y,
      ((pos.z % MAPBLOCK_SIZE.z) + MAPBLOCK_SIZE.z) % MAPBLOCK_SIZE.z,
      0, 0, 0);
    
    var n = mapBlock.data[localPos.x][localPos.y][localPos.z];
    
    var id = nodeID(n);
    return new NodeData(mapBlock.getItemstring(id), nodeRot(n));
  }
  
  setNode(pos, nodeData) {
    var mapBlockPos = new MapPos(Math.floor(pos.x / MAPBLOCK_SIZE.x), Math.floor(pos.y / MAPBLOCK_SIZE.y), Math.floor(pos.z / MAPBLOCK_SIZE.z), pos.w, pos.world, pos.universe);
    var mapBlock = this.getMapBlock(mapBlockPos);
    var localPos = new MapPos(
      ((pos.x % MAPBLOCK_SIZE.x) + MAPBLOCK_SIZE.x) % MAPBLOCK_SIZE.x,
      ((pos.y % MAPBLOCK_SIZE.y) + MAPBLOCK_SIZE.y) % MAPBLOCK_SIZE.y,
      ((pos.z % MAPBLOCK_SIZE.z) + MAPBLOCK_SIZE.z) % MAPBLOCK_SIZE.z,
      0, 0, 0);
    
    var val = nodeN(mapBlock.getNodeID(nodeData.itemstring), nodeData.rot);
    mapBlock.data[localPos.x][localPos.y][localPos.z] = val;
    if(nodeData.itemstring != "air") { mapBlock.props.sunlit = false; }
    mapBlock.markDirty();
    //FIXME
    /*if(localPos.x == 0) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(-1, 0, 0))).markDirty(); } else
    if(localPos.x == MAPBLOCK_SIZE.x - 1) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(1, 0, 0))).markDirty(); }
    if(localPos.y == 0) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(0, -1, 0))).markDirty(); } else
    if(localPos.y == MAPBLOCK_SIZE.y - 1) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(0, 1, 0))).markDirty(); }
    if(localPos.z == 0) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(0, 0, -1))).markDirty(); } else
    if(localPos.z == MAPBLOCK_SIZE.z - 1) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(0, 0, 1))).markDirty(); }*/
    /*if(localPos.x == 0) { var p = mapBlockPos.clone().add(new THREE.Vector3(-1, 0, 0)); this.setMapBlock(p, this.getMapBlock(p)); } else
    if(localPos.x == MAPBLOCK_SIZE.z - 1) { var p = mapBlockPos.clone().add(new THREE.Vector3(1, 0, 0)); this.setMapBlock(p, this.getMapBlock(p)); }
    if(localPos.y == 0) { var p = mapBlockPos.clone().add(new THREE.Vector3(0, -1, 0)); this.setMapBlock(p, this.getMapBlock(p)); } else
    if(localPos.y == MAPBLOCK_SIZE.z - 1) { var p = mapBlockPos.clone().add(new THREE.Vector3(0, 1, 0)); this.setMapBlock(p, this.getMapBlock(p)); }
    if(localPos.z == 0) { var p = mapBlockPos.clone().add(new THREE.Vector3(0, 0, -1)); this.setMapBlock(p, this.getMapBlock(p)); } else
    if(localPos.z == MAPBLOCK_SIZE.z - 1) { var p = mapBlockPos.clone().add(new THREE.Vector3(0, 0, 1)); this.setMapBlock(p, this.getMapBlock(p)); }*/
    //---
    this.setMapBlock(mapBlockPos, mapBlock);
  }
  
  digNode(player, pos) {
    var nodeData = this.getNode(pos);
    var stack = ItemStack.fromNodeData(nodeData);
    
    var canGive = player.inventory.canGive("main", stack);
    if(player.creativeDigPlace && player.inventory.canTake("main", stack)) {
      canGive = true;
    }
    if(canGive) {
      if(player.creativeDigPlace && player.inventory.canTake("main", stack)) {
        //player already has this item, no need to give them more
      } else {
        player.inventory.give("main", stack);
      }
      this.setNode(pos, new NodeData("air"));
      
      debug("server", "log", "dig " + nodeData.itemstring + " at " + pos.toString());
      
      return true;
    }
    
    return false;
  }
  placeNode(player, pos) {
    if(player.wield == null) { return false; }
    
    var stack = player.wield;
    if(player.wield.getDef().stackable) {
      stack = player.wield.clone();
      stack.count = 1;
    }
    
    var def = stack.getDef();
    if(!def.isNode) { return false; }
    
    if(player.inventory.canTakeIndex("main", player.wieldIndex, stack)) {
      if(!player.creativeDigPlace) {
        player.inventory.takeIndex("main", player.wieldIndex, stack);
      }
      var nodeData = NodeData.fromItemStack(stack);
      this.setNode(pos, nodeData);
      
      debug("server", "log", "place " + stack.itemstring + " at " + pos.toString());
      
      return true;
    }
    
    return false;
  }
  nodeToPlace(player) {
    if(player.wield == null) { return null; }
    
    if(!player.wield.getDef().isNode) { return null; }
    
    return NodeData.fromItemStack(player.wield);
  }
  
  getInventory(thing) {
    return new Inventory();
  }
  
  onFrame(tscale) {}
  
  get ready() { return true; }
  
  addEntity(entity) {
    this.entities[entity.id] = entity;
  }
  removeEntity(entity) {
    delete this.entities[entity.id];
  }
  getEntityById(id) {
    return this.entities[id];
  }
  
  entityTick(tscale) {
    for(var key in this.entities) {
      this.entities[key].tick(tscale);
    }
  }
  
  registerMessageHook(type, hook) {
    if(!(type in this.messageHooks)) {
      this.messageHooks[type] = [];
    }
    this.messageHooks[type].push(hook);
  }
  
  sendMessage(obj) {}
  isRemote() { return false; }
  connect() {}
}

class ServerLocal extends ServerBase {
  constructor(map) {
    super();
    
    this.map = map;
    
    this.playerInventory = {};
  }
  
  //TODO: support multiple players
  addPlayer(player) {
    var inv = this.getInventory(player);
    
    inv.setList("main", new Array(32).fill(null));
    inv.setList("craft", new Array(9).fill(null));
    inv.setList("hand", new Array(1).fill(null));
    
    var playerEntity = new Entity();
    playerEntity.updatePosVelRot(player.pos, player.vel, player.rot);
    player.registerUpdateHook(playerEntity.updatePosVelRot.bind(playerEntity));
    server.addEntity(playerEntity);
  }
  
  getMapBlock(pos, needLight=false) {
    var mapBlock = this.map.getMapBlock(pos);
    if(mapBlock.lightNeedsUpdate > 0 && needLight) {
      lightQueueUpdate(mapBlock.pos);
      return null;
    }
    return mapBlock;
  }
  setMapBlock(pos, mapBlock) {
    this.map.dirtyMapBlock(mapBlock.pos);
  }
  //No need to implement requestMapBlock, as it wouldn't do anything meaningful.
  
  getInventory(thing) {
    if(thing instanceof Player) {
      var inv = new Inventory();
      inv._getList = function(name) {
        if(name in this.playerInventory) {
          return this.playerInventory[name];
        }
        return [];
      }.bind(this);
      inv._setList = function(name, data) {
        this.playerInventory[name] = data;
        return true;
      }.bind(this);
      
      inv._getStack = function(name, index) {
        if(name in this.playerInventory) {
          var list = this.playerInventory[name];
          if(index < list.length) {
            return list[index];
          }
        }
        return null;
      }.bind(this);
      inv._setStack = function(name, index, data) {
        if(name in this.playerInventory) {
          var list = this.playerInventory[name];
          if(index < list.length) {
            list[index] = data;
            return true;
          }
        }
        return false;
      }.bind(this);
      
      return inv;
    }
  }
}

class MapBlockPatch {
  constructor(_server, _pos, _nodeData) {
    this.pos = _pos;
    
    this.mapBlockPos = new MapPos(Math.floor(this.pos.x / MAPBLOCK_SIZE.x), Math.floor(this.pos.y / MAPBLOCK_SIZE.y), Math.floor(this.pos.z / MAPBLOCK_SIZE.z), this.pos.w, this.pos.world, this.pos.universe);
    this.localPos = new MapPos(
      ((this.pos.x % MAPBLOCK_SIZE.x) + MAPBLOCK_SIZE.x) % MAPBLOCK_SIZE.x,
      ((this.pos.y % MAPBLOCK_SIZE.y) + MAPBLOCK_SIZE.y) % MAPBLOCK_SIZE.y,
      ((this.pos.z % MAPBLOCK_SIZE.z) + MAPBLOCK_SIZE.z) % MAPBLOCK_SIZE.z,
      0, 0, 0);
    
    this.server = _server;
    this.nodeData = _nodeData;
    
    var mapBlock = this.server.getMapBlock(this.mapBlockPos);
    if(mapBlock == null) {
      throw new Error("cannot patch an unloaded mapblock");
    }
    this.oldNodeData = mapBlock.getNode(this.localPos);
    this.oldLight = nodeLight(mapBlock.data[this.localPos.x][this.localPos.y][this.localPos.z]);
  }
  
  doApply() {
    var mapBlock = this.server.getMapBlock(this.mapBlockPos);
    if(mapBlock == null) {
      throw new Error("cannot patch an unloaded mapblock");
    }
    
    var oldLightRaw = nodeLight(mapBlock.data[this.localPos.x][this.localPos.y][this.localPos.z]);
    var sunlight = (oldLightRaw >> 4) & 15;
    var light = oldLightRaw & 15;
    
    //Make a rough prediction of what the light will look like so that it can be shown to the player immediately.
    var def = this.nodeData.getDef();
    if(!def.transparent) {
      sunlight = 0;
      light = 0;
    } else {
      var maxNearbySunlight = 0;
      var maxNearbyLight = 0;
      var hasSunAbove = false;
      for(var i = 0; i < stdFaces.length; i++) {
        var l = nodeLight(this.server.getNodeRaw(new MapPos(this.pos.x + stdFaces[i].x, this.pos.y + stdFaces[i].y, this.pos.z + stdFaces[i].z, this.pos.w, this.pos.world, this.pos.universe)));
        maxNearbySunlight = Math.max((l >> 4) & 15, maxNearbySunlight);
        maxNearbyLight = Math.max(l & 15, maxNearbyLight);
        
        if(stdFaces[i].x == 0 && stdFaces[i].y == 1 && stdFaces[i].z == 0 && ((l >> 4) & 15) == 15) {
          hasSunAbove = true;
        }
      }
      
      if(hasSunAbove) {
        sunlight = 15;
      } else {
        sunlight = Math.max(maxNearbySunlight - 1, sunlight);
      }
      light = Math.max(maxNearbyLight - 1, light);
    }
    if(def.lightLevel > 0) {
      light = Math.max(def.lightLevel, light);
    }
    
    var val = nodeN(mapBlock.getNodeID(this.nodeData.itemstring), this.nodeData.rot, (sunlight << 4) | light);
    mapBlock.data[this.localPos.x][this.localPos.y][this.localPos.z] = val;
    if(this.nodeData.itemstring != "air") { mapBlock.props.sunlit = false; }
  }
  
  doRevert() {
    var mapBlock = this.server.getMapBlock(this.mapBlockPos);
    if(mapBlock == null) {
      throw new Exception("cannot patch an unloaded mapblock");
    }
    var val = nodeN(mapBlock.getNodeID(this.oldNodeData.itemstring), this.oldNodeData.rot, this.oldLight);
    mapBlock.data[this.localPos.x][this.localPos.y][this.localPos.z] = val;
  }
  
  doQueueUpdates() {
    if(this.localPos.x == 0) { renderQueueUpdate(this.mapBlockPos.add(new MapPos(-1, 0, 0, 0, 0, 0)), true); } else
    if(this.localPos.x == MAPBLOCK_SIZE.x - 1) { renderQueueUpdate(this.mapBlockPos.add(new MapPos(1, 0, 0, 0, 0, 0)), true); }
    if(this.localPos.y == 0) { renderQueueUpdate(this.mapBlockPos.add(new MapPos(0, -1, 0, 0, 0, 0)), true); } else
    if(this.localPos.y == MAPBLOCK_SIZE.y - 1) { renderQueueUpdate(this.mapBlockPos.add(new MapPos(0, 1, 0, 0, 0, 0)), true); }
    if(this.localPos.z == 0) { renderQueueUpdate(this.mapBlockPos.add(new MapPos(0, 0, -1, 0, 0, 0)), true); } else
    if(this.localPos.z == MAPBLOCK_SIZE.z - 1) { renderQueueUpdate(this.mapBlockPos.add(new MapPos(0, 0, 1, 0, 0, 0)), true); }
    renderQueueUpdate(this.mapBlockPos, true);
  }
  
  isAccepted() {
    var mapBlock = this.server.getMapBlock(this.mapBlockPos);
    if(mapBlock == null) {
      throw new Exception("cannot patch an unloaded mapblock");
    }
    var actualNode = mapBlock.getNode(this.localPos);
    if(this.nodeData.itemstring == actualNode.itemstring && this.nodeData.rot == actualNode.rot) {
      return true;
    }
    return false;
  }
}

//TODO
class ServerRemote extends ServerBase {
  constructor(url) {
    super();
    
    this.playerInventory = {};
    
    this.cache = {};
    this.saved = {};
    
    this.requests = new Set();
    
    this.patches = [];
    
    this.player = null;
    this.timeSinceUpdateSent = 0;
    this._socketReady = false;
    this._invReady = true;
    
    this._url = url;
  }
  
  connect() {
    this.socket = new WebSocket(this._url);
    this.socket.binaryType = "arraybuffer";
    this.socket.onopen = function() {
      this._socketReady = true;
      
      debug("client", "status", "connected to " + this.socket.url);
    }.bind(this);
    this.socket.onclose = function() {
      this._socketReady = false;
      
      debug("client", "status", "disconnected from " + this.socket.url);
    }.bind(this);
    this.socket.onerror = function() {
      this._socketReady = true;
      
      debug("client", "error", "unable to connect to " + this.socket.url);
    }.bind(this);
    
    this.socket.onmessage = function(e) {
      if(e.data instanceof ArrayBuffer) {
        //req_mapblock
        
        //console.log(btoa(String.fromCharCode.apply(null, new Uint8Array(e.data))));
        //location.reload();
        
        var dv = new DataView(e.data);
        var endianness = false;
        var magic = dv.getUint32(0);
        if(magic != 0xABCD5678) {
          endianness = true;
        }
        var posX = dv.getInt32(4, endianness);
        var posY = dv.getInt32(8, endianness);
        var posZ = dv.getInt32(12, endianness);
        var posW = dv.getInt32(16, endianness);
        var posWorld = dv.getInt32(20, endianness);
        var posUniverse = dv.getInt32(24, endianness);
        var updateNum = dv.getUint32(28, endianness);
        var lightUpdateNum = dv.getUint32(32, endianness);
        var lightNeedsUpdate = dv.getUint32(36, endianness);
        var sunlit = (dv.getUint32(40, endianness) & 1) == 1 ? true : false;
        var dataLen = dv.getUint32(44, endianness);
        var lightDataLen = dv.getUint32(48 + dataLen * 4, endianness) * 2;
        var lightDataLenActual = dv.getUint32(48 + dataLen * 4 + 4, endianness);
        var lightDataOffset = 48 + dataLen * 4 + 4 + 4;
        var IDtoISLen = dv.getUint32(48 + dataLen * 4 + 4 + 4 + lightDataLen * 2, endianness);
        var IDtoISOffset = 48 + dataLen * 4 + 4 + 4 + lightDataLen * 2 + 4;
        
        var index = posX + "," + posY + "," + posZ + "," + posW + "," + posWorld + "," + posUniverse;
        var mapBlock = new MapBlock(new MapPos(posX, posY, posZ, posW, posWorld, posUniverse));
        mapBlock.updateNum = updateNum;
        mapBlock.lightUpdateNum = lightUpdateNum;
        mapBlock.lightNeedsUpdate = lightNeedsUpdate;
        if(index in this.cache) {
          if(mapBlock.updateNum != this.cache[index].updateNum) {
            mapBlock.renderNeedsUpdate = 2; //getMapBlock will force the lighting to be updated (if lightNeedsUpdate is set) before the render update can happen
          } else if(mapBlock.lightUpdateNum != this.cache[index].lightUpdateNum) {
            //FIXME?
            renderQueueLightingUpdate(mapBlock.pos);
          }
        } else {
          mapBlock.renderNeedsUpdate = 1;
        }
        
        var y = 0;
        var x = 0;
        var z = 0;
        var full = false;
        for(var i = 0; i < dataLen; i++) {
          var val = dv.getUint32(48 + i * 4, endianness);
          var runVal = val & 0b00000000011111111111111111111111;
          var runLength = ((val >> 23) & 511) + 1; //Run lengths are offset by 1 to allow storing [1, 512] instead of [0, 511]
          
          for(var n = 0; n < runLength; n++) {
            if(full) {
              throw new Error("decompressed mapblock is too long");
            }
            
            mapBlock.data[x][y][z] = runVal;
            z++;
            if(z >= MAPBLOCK_SIZE.z) {
              z = 0;
              x++;
              if(x >= MAPBLOCK_SIZE.x) {
                x = 0;
                y++;
                if(y >= MAPBLOCK_SIZE.y) {
                  y = 0;
                  full = true;
                }
              }
            }
          }
        }
        if(!full) {
          throw new Error("decompressed mapblock is too short");
        }
        
        var y = 0;
        var x = 0;
        var z = 0;
        var full = false;
        for(var i = 0; i < lightDataLenActual; i++) {
          var val = dv.getUint16(lightDataOffset + i * 2, endianness);
          var runVal = val & 0b0000000011111111;
          var runLength = ((val >> 8) & 255) + 1; //Run lengths are offset by 1 to allow storing [1, 256] instead of [0, 255]
          
          for(var n = 0; n < runLength; n++) {
            if(full) {
              throw new Error("decompressed mapblock light is too long");
            }
            
            mapBlock.data[x][y][z] |= (runVal << 23);
            z++;
            if(z >= MAPBLOCK_SIZE.z) {
              z = 0;
              x++;
              if(x >= MAPBLOCK_SIZE.x) {
                x = 0;
                y++;
                if(y >= MAPBLOCK_SIZE.y) {
                  y = 0;
                  full = true;
                }
              }
            }
          }
        }
        if(!full) {
          throw new Error("decompressed mapblock light is too short");
        }
        
        var dec = new TextDecoder();
        var IDtoISStr = dec.decode(new DataView(e.data, IDtoISOffset, IDtoISLen));
        
        mapBlock.IDtoIS = JSON.parse(IDtoISStr);
        mapBlock.IStoID = {};
        for(var i = 0; i < mapBlock.IDtoIS.length; i++) {
          mapBlock.IStoID[mapBlock.IDtoIS[i]] = i;
        }
        mapBlock.props.sunlit = sunlit;
        
        this.cache[index] = mapBlock;
        
        //Since updates are only queued, they won't happen until after this code runs
        for(var i = this.patches.length - 1; i >= 0; i--) {
          if(mapBlock.pos.equals(this.patches[i].mapBlockPos)) {
            if(this.patches[i].isAccepted()) {
              this.patches.splice(i, 1);
            } else {
              this.patches[i].doApply();
            }
          }
        }
        
        //if(mapBlock.lightNeedsUpdate > 0 && needLight) {
        //  lightQueueUpdate(mapBlock.pos);
        //}
        
        //console.log("recv_mapblock (" + index + ") updateNum=" + mdata.updateNum + " lightUpdateNum=" + mdata.lightUpdateNum + " lightNeedsUpdate=" + mdata.lightNeedsUpdate);
        
        return;
      }
      var data = JSON.parse(e.data);
      
      if(data.type == "update_entities") {
        data.actions.forEach(function(action) {
          if(action.type == "create") {
            this.addEntity(new Entity(action.data));
          } else if(action.type == "delete") {
            var entity = this.getEntityById(action.data.id);
            this.removeEntity(entity);
            entity.destroy();
          } else if(action.type == "update") {
            this.getEntityById(action.data.id).update(action.data);
          }
        }.bind(this));
      } else if(data.type == "set_player_pos") {
        this.player.pos.set(data.pos.x, data.pos.y, data.pos.z, data.pos.w, data.pos.world, data.pos.universe);
        this.player.rot.set(data.rot.x, data.rot.y, data.rot.z, data.rot.w);
      }
      
      if(data.type in this.messageHooks) {
        for(var i = 0; i < this.messageHooks[data.type].length; i++) {
          this.messageHooks[data.type][i](data);
        }
      }
    }.bind(this);
  }
  
  //TODO: support multiple players
  addPlayer(player) {
    this.player = player;
    
    var inv = this.getInventory(player);
    
    inv.setList("main", new Array(32).fill(null));
    inv.setList("craft", new Array(9).fill(null));
    inv.setList("hand", new Array(1).fill(null));
    
    var playerEntity = new Entity();
    playerEntity.doInterpolate = false;
    playerEntity.updatePosVelRot(player.pos, player.vel, player.rot);
    player.registerUpdateHook(playerEntity.updatePosVelRot.bind(playerEntity));
    server.addEntity(playerEntity);
  }
  
  getMapBlock(pos, needLight=false) {
    assert(Number.isFinite(pos.x), "getMapBlock pos.x is not a finite number");
    assert(Number.isFinite(pos.y), "getMapBlock pos.y is not a finite number");
    assert(Number.isFinite(pos.z), "getMapBlock pos.z is not a finite number");
    assert(Number.isFinite(pos.w), "getMapBlock pos.w is not a finite number");
    assert(Number.isFinite(pos.world), "getMapBlock pos.world is not a finite number");
    assert(Number.isFinite(pos.universe), "getMapBlock pos.universe is not a finite number");
    
    var index = pos.x + "," + pos.y + "," + pos.z + "," + pos.w + "," + pos.world + "," + pos.universe;
    if(index in this.saved) {
      var mapBlock = this.saved[index];
      if(mapBlock.lightNeedsUpdate > 0 && needLight) {
        lightQueueUpdate(mapBlock.pos);
        return null;
      }
      return mapBlock;
    } else if(index in this.cache) {
      var mapBlock = this.cache[index];
      if(mapBlock.lightNeedsUpdate > 0 && needLight) {
        lightQueueUpdate(mapBlock.pos);
        return null;
      }
      return mapBlock;
    } else {
      if(!this._socketReady) { return null; }
      
      if(!this.requests.has(index)) {
        //console.log("req " + index);
        this.socket.send(JSON.stringify({
          type: "req_mapblock",
          pos: {x: pos.x, y: pos.y, z: pos.z, w: pos.w, world: pos.world, universe: pos.universe}
        }));
        this.requests.add(index);
      }
      
      return null;
    }
  }
  setMapBlock(pos, mapBlock) {
    mapBlock.markDirty();
      
    if(this._socketReady) {
      this.socket.send(JSON.stringify({
        type: "set_mapblock",
        data: mapBlock
      }));
    }
  }
  requestMapBlock(pos) {
    var index = pos.x + "," + pos.y + "," + pos.z + "," + pos.w + "," + pos.world + "," + pos.universe;
    if(!(index in this.cache) && !(index in this.saved)) {
      this.getMapBlock(pos);
    }
  }
  
  setNode(pos, nodeData) {
    //FIXME
    var patch = new MapBlockPatch(this, pos, nodeData);
    
    patch.doApply();
    patch.doQueueUpdates();
    
    this.patches.push(patch);
    
    //TODO test
    setTimeout(function(p) {
      var index = this.patches.indexOf(p);
      if(index > -1) {
        patch.doRevert();
        patch.doQueueUpdates();
        this.patches.splice(index, 1);
      }
    }.bind(this, patch), 5000);
      
    if(this._socketReady) {
      this.socket.send(JSON.stringify({
        type: "set_node",
        pos: {x: pos.x, y: pos.y, z: pos.z, w: pos.w, world: pos.world, universe: pos.universe},
        data: nodeData
      }));
    }
  }
  
  getInventory(thing) {
    if(thing instanceof Player) {
      var inv = new Inventory();
      inv._getList = function(name) {
        if(name in this.playerInventory) {
          return this.playerInventory[name];
        }
        return [];
      }.bind(this);
      inv._setList = function(name, data) {
        this.playerInventory[name] = data;
        return true;
      }.bind(this);
      
      inv._getStack = function(name, index) {
        if(name in this.playerInventory) {
          var list = this.playerInventory[name];
          if(index < list.length) {
            return list[index];
          }
        }
        return null;
      }.bind(this);
      inv._setStack = function(name, index, data) {
        if(name in this.playerInventory) {
          var list = this.playerInventory[name];
          if(index < list.length) {
            list[index] = data;
            return true;
          }
        }
        return false;
      }.bind(this);
      
      return inv;
    }
  }
  
  get ready() {
    return this._socketReady && this._invReady;
  }
  
  onFrame(tscale) {
    this.lastPlayerMapblock = this.playerMapblock;
    this.playerMapblock = new MapPos(Math.round(this.player.pos.x / MAPBLOCK_SIZE.x), Math.round(this.player.pos.y / MAPBLOCK_SIZE.y), Math.round(this.player.pos.z / MAPBLOCK_SIZE.z), this.player.pos.w, this.player.pos.world, this.player.pos.universe);
    if(this.lastPlayerMapblock == undefined) { this.lastPlayerMapblock = this.playerMapblock; }
    
    this.timeSinceUpdateSent += tscale;
    if(this.timeSinceUpdateSent > SERVER_REMOTE_UPDATE_INTERVAL || !this.playerMapblock.equals(this.lastPlayerMapblock)) {
      if(this._socketReady) {
        if(this.player != null) {
          this.socket.send(JSON.stringify({
            type: "set_player_pos",
            pos: {x: this.player.pos.x, y: this.player.pos.y, z: this.player.pos.z, w: this.player.pos.w, world: this.player.pos.world, universe: this.player.pos.universe},
            vel: {x: this.player.vel.x, y: this.player.vel.y, z: this.player.vel.z},
            rot: {x: this.player.rot.x, y: this.player.rot.y, z: this.player.rot.z, w: this.player.rot.w}
          }));
        }
      }
      
      this.timeSinceUpdateSent = 0;
    }
  }
  
  sendMessage(obj) {
    this.socket.send(JSON.stringify(obj));
  }
  isRemote() { return true; }
}

api.getNode = function(pos) { return server.getNode(pos); };
api.setNode = function(pos, nodeData) { return server.getNode(pos, nodeData); };
