/*
    mc4, a web voxel building game
    Copyright (C) 2019-2021 kholland4

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
      throw new Error("cannot patch an unloaded mapblock");
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
      throw new Error("cannot patch an unloaded mapblock");
    }
    var actualNode = mapBlock.getNode(this.localPos);
    if(this.nodeData.itemstring == actualNode.itemstring && this.nodeData.rot == actualNode.rot) {
      return true;
    }
    return false;
  }
}

class InvPatch {
  constructor(server, reqID=null) {
    this.server = server;
    
    this.diffs = [];
    this.reqID = reqID;
  }
  
  //'prev' and 'current' are ItemStacks or null
  add(ref, prev, current) {
    this.diffs.push({
      ref: ref,
      prev: prev,
      current: current
    });
  }
  
  doApply(use_current=true) {
    for(var diff of this.diffs) {
      if(diff.ref.objType == "player") {
        if(!(diff.ref.listName in this.server.playerInventory))
          throw new Error("invalid inventory list: player/" + diff.ref.listName);
          
        var list = this.server.playerInventory[diff.ref.listName];
        if(diff.ref.index >= list.length)
          throw new Error("out of bounds inventory reference: " + diff.ref.index);
        
        if(use_current)
          list[diff.ref.index] = diff.current;
        else
          list[diff.ref.index] = diff.prev;
        this.server.updateInvDisplay(diff.ref);
        
        continue;
      }
      
      throw new Error("unsupported inventory object type: " + diff.ref.objType);
    }
  }
  doRevert() {
    this.doApply(false);
  }
  
  equals(other) {
    var diffList1 = [...this.diffs];
    var diffList2 = [...other.diffs];
    
    if(diffList1.length != diffList2.length)
      return false;
    
    while(diffList1.length > 0) {
      var toMatch = diffList1[0];
      var didMatch = false;
      for(var i = 0; i < diffList2.length; i++) {
        if(!toMatch.ref.equals(diffList2[i].ref))
          continue;
        
        if(toMatch.prev == null && diffList2[i].prev == null) {
          //ok
        } else if(toMatch.prev == null || diffList2[i].prev == null) {
          continue;
        } else if(!toMatch.prev.equals(diffList2[i].prev)) {
          continue;
        }
        
        if(toMatch.current == null && diffList2[i].current == null) {
          //ok
        } else if(toMatch.current == null || diffList2[i].current == null) {
          continue;
        } else if(!toMatch.current.equals(diffList2[i].current)) {
          continue;
        }
        
        //match, remove elements
        diffList1.splice(0, 1);
        diffList2.splice(i, 1);
        didMatch = true;
        break;
      }
      
      //no match
      if(!didMatch)
        return false;
    }
    return true;
  }
}

class ServerRemote extends ServerBase {
  constructor(url) {
    super();
    
    this.playerInventory = {};
    
    this.cache = {};
    this.saved = {};
    
    this.requests = new Set();
    
    this.patches = [];
    this.invPatches = [];
    
    this.player = null;
    this.timeSinceUpdateSent = 0;
    this._socketReady = false;
    this._invReady = false;
    this._authReady = false;
    this._posReady = false;
    this._authCredentials = null;
    
    this._url = url;
  }
  
  connect(credentials) {
    this._authCredentials = credentials;
    
    this.socket = new WebSocket(this._url);
    this.socket.binaryType = "arraybuffer";
    this.socket.onopen = function(credentials) {
      this._socketReady = true;
      
      debug("client", "status", "connected to " + this.socket.url);
      
      if(this._authCredentials.guest) {
        this.sendMessage({
          type: "auth_guest"
        });
      } else {
        if(this._authCredentials.register) {
          this.sendMessage({
            type: "auth_step",
            step: "register",
            login_name: this._authCredentials.loginName,
            password: this._authCredentials.verifier,
            client_salt: "",
            backend: "password-plain"
          });
        } else {
          this.sendMessage({
            type: "auth_step",
            step: "login",
            login_name: this._authCredentials.loginName,
            password: this._authCredentials.verifier,
            client_salt: "",
            backend: "password-plain"
          });
        }
        this._authCredentials.verifier = null;
      }
    }.bind(this);
    this.socket.onclose = function() {
      this._socketReady = false;
      
      if(event.reason == "") {
        debug("client", "status", "disconnected from " + this.socket.url);
      } else {
        debug("client", "status", "disconnected: " + event.reason);
      }
    }.bind(this);
    this.socket.onerror = function() {
      this._socketReady = false;
      
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
        
        var oldMapBlock = null;
        if(index in this.cache) { oldMapBlock = this.cache[index]; }
        
        this.cache[index] = mapBlock;
        
        //Apply mapblock patches, oldest to newest
        var i = 0;
        var toApply = [];
        while(i < this.patches.length) {
          if(mapBlock.pos.equals(this.patches[i].mapBlockPos)) {
            //Don't use previous patches from the same position
            for(var n = toApply.length - 1; n >= 0; n--) {
              if(this.patches[i].localPos.equals(toApply[n].localPos)) {
                toApply.splice(n, 1);
              }
            }
            
            if(this.patches[i].isAccepted()) {
              var localPos = this.patches[i].localPos;
              this.patches.splice(i, 1);
              
              //Also delete previous patches for the same position
              for(var n = i - 1; n >= 0; n--) {
                if(this.patches[n].mapBlockPos.equals(mapBlock.pos) && this.patches[n].localPos.equals(localPos)) {
                  this.patches.splice(n, 1);
                  i--;
                }
              }
            } else {
              toApply.push(this.patches[i]);
              i++;
            }
          } else {
            i++;
          }
        }
        
        for(var i = 0; i < toApply.length; i++) {
          toApply[i].doApply();
        }
        
        
        if(oldMapBlock != null) {
          if(mapBlock.updateNum != oldMapBlock.updateNum) {
            //see if any existing IDtoIS entries have changed different
            var changedIDtoIS = false;
            if(mapBlock.IDtoIS.length < oldMapBlock.IDtoIS.length) {
              changedIDtoIS = true;
            }
            if(!changedIDtoIS) {
              for(var i = 0; i < oldMapBlock.IDtoIS.length; i++) {
                if(mapBlock.IDtoIS[i] != oldMapBlock.IDtoIS[i]) {
                  changedIDtoIS = true;
                  break;
                }
              }
            }
            
            if(changedIDtoIS) {
              mapBlock.renderNeedsUpdate = 2; //getMapBlock will force the lighting to be updated (if lightNeedsUpdate is set) before the render update can happen
            } else {
              //see where nodes have changed (disregarding lighting)
              var sides = [false, false, false, false, false, false];
              var anyDiff = false;
              for(var x = 0; x < mapBlock.size.x; x++) {
                for(var y = 0; y < mapBlock.size.y; y++) {
                  for(var z = 0; z < mapBlock.size.z; z++) {
                    if((mapBlock.data[x][y][z] & 0b00000000011111111111111111111111) == (oldMapBlock.data[x][y][z] & 0b00000000011111111111111111111111)) { continue; }
                    
                    if(x == 0) { sides[0] = true; }
                    if(x == mapBlock.size.x - 1) { sides[1] = true; }
                    if(y == 0) { sides[2] = true; }
                    if(y == mapBlock.size.y - 1) { sides[3] = true; }
                    if(z == 0) { sides[4] = true; }
                    if(z == mapBlock.size.z - 1) { sides[5] = true; }
                    
                    anyDiff = true;
                  }
                }
              }
              
              for(var i = 0; i < sides.length; i++) {
                if(!sides[i]) { continue; }
                
                renderQueueUpdate(new MapPos(mapBlock.pos.x + stdFaces[i].x, mapBlock.pos.y + stdFaces[i].y, mapBlock.pos.z + stdFaces[i].z, mapBlock.pos.w, mapBlock.pos.world, mapBlock.pos.universe), true);
              }
              
              if(anyDiff) {
                //mapBlock.renderNeedsUpdate = 1;
                renderQueueUpdate(mapBlock.pos, true);
              } else if(mapBlock.lightUpdateNum != oldMapBlock.lightUpdateNum) {
                //we've verified that there's no difference in mapblock content
                var hasWorker = false;
                var worker;
                for(var i = 0; i < renderWorkers.length; i++) {
                  if(renderWorkers[i].pos.equals(mapBlock.pos)) {
                    hasWorker = true;
                    worker = renderWorkers[i];
                  }
                }
                if(index in renderCurrentMeshes) {
                  if(hasWorker) {
                    worker.registerOnComplete(function(pos, updateNum, index) {
                      renderCurrentMeshes[index].updateNum = updateNum;
                      renderQueueLightingUpdate(pos);
                    }.bind(null, mapBlock.pos, mapBlock.updateNum, index));
                  } else {
                    renderCurrentMeshes[index].updateNum = updateNum;
                    renderQueueLightingUpdate(mapBlock.pos);
                  }
                } else {
                  renderQueueUpdate(mapBlock.pos, true);
                }
              }
            }
          } else if(mapBlock.lightUpdateNum != oldMapBlock.lightUpdateNum) {
            //FIXME?
            renderQueueLightingUpdate(mapBlock.pos);
          }
        } else {
          mapBlock.renderNeedsUpdate = 1;
        }
        
        this.requests.delete(index);
        
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
        camera.quaternion.copy(this.player.rot);
        
        this._posReady = true;
      } else if(data.type == "set_player_privs") {
        this.player.privs = data.privs;
      } else if(data.type == "inv_list") {
        var ref = new InvRef(data.ref.objType, data.ref.objID, data.ref.listName, data.ref.index);
        var list = data.list;
        for(var i = 0; i < list.length; i++) {
          if(list[i] == null)
            continue;
          list[i] = new ItemStack(list[i].itemstring, list[i].count, list[i].wear, list[i].data);
        }
        if(ref.objType == "player") {
          this.playerInventory[ref.listName] = list;
          this.updateInvDisplay(ref);
        } else {
          throw new Error("unsupported inventory object type: " + ref.objType);
        }
      } else if(data.type == "inv_ready") {
        this._invReady = true;
      } else if(data.type == "inv_patch_deny" || data.type == "inv_patch_accept" || data.type == "inv_patch") {
        var server_patch = new InvPatch(this);
        if("reqID" in data)
          server_patch.reqID = data.reqID;
        
        for(var diff of data.diffs) {
          var prev = null;
          if(diff.prev != null)
            prev = new ItemStack(diff.prev.itemstring, diff.prev.count, diff.prev.wear, diff.prev.data);
          var current = null;
          if(diff.current != null)
            current = new ItemStack(diff.current.itemstring, diff.current.count, diff.current.wear, diff.current.data);
          
          server_patch.add(
              new InvRef(diff.ref.objType, diff.ref.objID, diff.ref.listName, diff.ref.index),
              prev,
              current
          );
        }
        
        console.log(server_patch);
        
        var didAccept = false;
        if(data.type == "inv_patch_accept" && this.invPatches.length > 0) {
          if(this.invPatches[0].reqID == server_patch.reqID) {
            if(this.invPatches[0].equals(server_patch))
              didAccept = true;
          }
        }
        
        if(didAccept) {
          this.invPatches.splice(0, 1);
        } else {
          //either a denial, or an unreleated inv patch
          //revert all pending inv patches and apply the received one
          
          for(var i = this.invPatches.length - 1; i >= 0; i--) {
            this.invPatches[i].doRevert();
            this.invPatches.splice(i, 1);
          }
          
          server_patch.doApply();
        }
      } else if(data.type == "auth_step") {
        if(data.message == "auth_ok") {
          this._authReady = true;
          debug("client", "status", "authenticated as " + this._authCredentials.loginName);
        } else if(data.message == "register_ok") {
          this._authReady = true;
          debug("client", "status", "registered new account for " + this._authCredentials.loginName);
        }
      } else if(data.type == "auth_guest") {
        if(data.message == "guest_ok") {
          this._authReady = true;
          debug("client", "status", "connected as guest");
        }
      } else if(data.type == "auth_err") {
        debug("client", "status", "authentication failed: " + data.reason);
      }
      
      if(data.type in this.messageHooks) {
        for(var i = 0; i < this.messageHooks[data.type].length; i++) {
          this.messageHooks[data.type][i](data);
        }
      }
    }.bind(this);
    
    window.addEventListener("unload", function () {
      if(this.socket.readyState == WebSocket.OPEN) {
        this.socket.close();
      }
    }.bind(this));
  }
  
  addPlayer(player) {
    this.player = player;
    
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
        this.socket.send(JSON.stringify({
          type: "req_mapblock",
          pos: {x: pos.x, y: pos.y, z: pos.z, w: pos.w, world: pos.world, universe: pos.universe}
        }));
        this.requests.add(index);
        
        setTimeout(function(index) {
          this.requests.delete(index);
        }.bind(this, index), 8000);
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
  suggestUncacheMapBlock(pos) {
    var index = pos.x + "," + pos.y + "," + pos.z + "," + pos.w + "," + pos.world + "," + pos.universe;
    if(index in this.cache) {
      delete this.cache[index];
    }
  }
  
  setNodePatchOnly(pos, nodeData) {
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
  }
  
  setNode(pos, nodeData) {
    this.setNodePatchOnly(pos, nodeData);
      
    if(this._socketReady) {
      this.socket.send(JSON.stringify({
        type: "set_node",
        pos: {x: pos.x, y: pos.y, z: pos.z, w: pos.w, world: pos.world, universe: pos.universe},
        data: nodeData
      }));
    }
  }
  
  digNode(player, pos) {
    var nodeData = this.getNode(pos);
    var stack = ItemStack.fromNodeData(nodeData);
    
    this.setNodePatchOnly(pos, new NodeData("air"));
    
    if(this._socketReady) {
      this.socket.send(JSON.stringify({
        type: "dig_node",
        pos: {x: pos.x, y: pos.y, z: pos.z, w: pos.w, world: pos.world, universe: pos.universe},
        wield: player.wieldIndex,
        existing: nodeData
      }));
    }
    
    return true;
  }
  
  placeNode(player, pos, pos_on) {
    if(player.wield == null) { return false; }
    
    var stack = player.wield;
    if(player.wield.getDef().stackable) {
      stack = player.wield.clone();
      stack.count = 1;
    }
    
    var def = stack.getDef();
    if(!def.isNode) { return false; }
    
    var nodeData = NodeData.fromItemStack(stack);
    nodeData.rot = this.calcPlaceRot(nodeData, pos, pos_on);
    
    this.setNodePatchOnly(pos, nodeData);
    
    if(this._socketReady) {
      this.socket.send(JSON.stringify({
        type: "place_node",
        pos: {x: pos.x, y: pos.y, z: pos.z, w: pos.w, world: pos.world, universe: pos.universe},
        wield: player.wieldIndex,
        data: nodeData
      }));
    }
    
    return true;
  }
  
  get ready() {
    return this._socketReady && this._invReady && this._authReady && this._posReady && this._invReady;
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
    
    
    for(var key in this.cache) {
      var pos = this.cache[key].pos;
      if(pos.x < playerMapblock.x - serverUncacheDist.x || pos.x > playerMapblock.x + serverUncacheDist.x ||
         pos.y < playerMapblock.y - serverUncacheDist.y || pos.y > playerMapblock.y + serverUncacheDist.y ||
         pos.z < playerMapblock.z - serverUncacheDist.z || pos.z > playerMapblock.z + serverUncacheDist.z ||
         pos.w < playerMapblock.w - serverUncacheDist.w || pos.w > playerMapblock.w + serverUncacheDist.w ||
         pos.world < playerMapblock.world - serverUncacheDist.world || pos.world > playerMapblock.world + serverUncacheDist.world ||
         pos.universe < playerMapblock.universe - serverUncacheDist.universe || pos.universe > playerMapblock.universe + serverUncacheDist.universe) {
        this.suggestUncacheMapBlock(pos);
      }
    }
  }
  
  sendMessage(obj) {
    this.socket.send(JSON.stringify(obj));
  }
  isRemote() { return true; }
  
  invGetList(ref) {
    if(ref.objType == "player") {
      if(ref.listName in this.playerInventory) {
        return this.playerInventory[ref.listName];
      }
      //return [];
      throw new Error("no such inventory list: player/" + ref.listName);
    }
    
    throw new Error("unsupported inventory object type: " + ref.objType);
  }
  invGetStack(ref) {
    if(ref.objType == "player") {
      if(ref.listName in this.playerInventory) {
        var list = this.playerInventory[ref.listName];
        if(ref.index < list.length) {
          return list[ref.index];
        }
        
        throw new Error("out of bounds inventory access: player/" + ref.listName + "/" + ref.index);
      }
      //return null;
      throw new Error("no such inventory list: player/" + ref.listName);
    }
    
    throw new Error("unsupported inventory object type: " + ref.objType);
  }
  invSwap(first, second) {
    if(!this._socketReady)
      return false;
    
    //do the swap locally to display to the player
    
    var origStack1 = this.invGetStack(first);
    if(origStack1 != null)
      origStack1 = new ItemStack(origStack1.itemstring, origStack1.count, origStack1.wear, origStack1.data);
    var origStack2 = this.invGetStack(second);
    if(origStack2 != null)
      origStack2 = new ItemStack(origStack2.itemstring, origStack2.count, origStack2.wear, origStack2.data);
    
    //try override
    var overrideResult = this.invTryOverride(first, second);
    if(overrideResult === false)
      return false;
    if(overrideResult instanceof InvPatch) {
      this.socket.send(JSON.stringify({
        type: "inv_swap",
        ref1: first,
        ref2: second,
        orig1: origStack1,
        orig2: origStack2,
        reqID: overrideResult.reqID
      }));
      
      overrideResult.doApply();
      this.invPatches.push(overrideResult);
      
      //TODO this won't be matched to the associated patch from the server
      var craftPatch = this.updateCraftIfNeeded(first, second);
      if(craftPatch != null) {
        craftPatch.doApply();
        this.invPatches.push(craftPatch);
      }
      
      return true;
    }
    //---
    
    var newStack1 = origStack2;
    var newStack2 = origStack1;
    
    var patch = new InvPatch(this, hopefullyPassableUUIDv4Generator());
    patch.add(first, origStack1, newStack1);
    patch.add(second, origStack2, newStack2);
    
    this.socket.send(JSON.stringify({
      type: "inv_swap",
      ref1: first,
      ref2: second,
      orig1: origStack1,
      orig2: origStack2,
      reqID: patch.reqID
    }));
    
    patch.doApply();
    this.invPatches.push(patch);
    
    //TODO this won't be matched to the associated patch from the server
    var craftPatch = this.updateCraftIfNeeded(first, second);
    if(craftPatch != null) {
      craftPatch.doApply();
      this.invPatches.push(craftPatch);
    }
    
    return true;
  }
  invDistribute(first, qty1, second, qty2) {
    if(!this._socketReady)
      return false;
    
    //do the distribute locally to display to the player
    
    var stack1 = this.invGetStack(first);
    if(stack1 != null)
      stack1 = new ItemStack(stack1.itemstring, stack1.count, stack1.wear, stack1.data);
    var stack2 = this.invGetStack(second);
    if(stack2 != null)
      stack2 = new ItemStack(stack2.itemstring, stack2.count, stack2.wear, stack2.data);
    
    //try override
    var overrideResult = this.invTryOverride(first, second);
    if(overrideResult === false)
      return false;
    if(overrideResult instanceof InvPatch) {
      this.socket.send(JSON.stringify({
        type: "inv_distribute",
        ref1: first,
        orig1: stack1,
        qty1: qty1,
        ref2: second,
        orig2: stack2,
        qty2: qty2,
        reqID: overrideResult.reqID
      }));
      
      overrideResult.doApply();
      this.invPatches.push(overrideResult);
      
      //TODO this won't be matched to the associated patch from the server
      var craftPatch = this.updateCraftIfNeeded(first, second);
      if(craftPatch != null) {
        craftPatch.doApply();
        this.invPatches.push(craftPatch);
      }
      
      return true;
    }
    //---
    
    if(stack1 == null && stack2 == null) {
      throw new Error("nothing to distribute");
    }
    
    if(stack1 != null && stack2 != null) {
      if(!stack1.typeMatch(stack2)) {
        throw new Error("cannot distribute differing itemstacks");
      }
      
      var def = stack1.getDef();
      if(!def.stackable) {
        throw new Error("cannot distribute unstackable items");
      }
    }
    
    var def;
    var combined;
    var stack1Count = 0;
    var stack2Count = 0;
    if(stack1 != null) {
      def = stack1.getDef();
      stack1Count = stack1.count;
      combined = stack1.clone();
      if(stack2 != null) {
        combined.count += stack2.count;
        stack2Count = stack2.count;
      }
    } else {
      def = stack2.getDef();
      stack2Count = stack2.count;
      combined = stack2.clone();
      if(stack1 != null) {
        combined.count += stack1.count;
        stack1Count = stack1.count;
      }
    }
    
    if(!def.stackable) {
      throw new Error("cannot distribute unstackable items");
    }
    
    if(qty1 + qty2 != combined.count) {
      throw new Error("distribute must conserve items");
    }
    if(qty1 > Math.max(stack1Count, stack2Count, def.maxStack) || qty2 > Math.max(stack1Count, stack2Count, def.maxStack)) {
      throw new Error("cannot exceed max stack in distribute");
    }
    
    var newStack1 = null;
    if(qty1 > 0) {
      newStack1 = combined.clone();
      newStack1.count = qty1;
    }
    var newStack2 = null;
    if(qty2 > 0) {
      newStack2 = combined.clone();
      newStack2.count = qty2;
    }
    
    
    var patch = new InvPatch(this, hopefullyPassableUUIDv4Generator());
    patch.add(first, stack1, newStack1);
    patch.add(second, stack2, newStack2);
    
    this.socket.send(JSON.stringify({
      type: "inv_distribute",
      ref1: first,
      orig1: stack1,
      qty1: qty1,
      ref2: second,
      orig2: stack2,
      qty2: qty2,
      reqID: patch.reqID
    }));
    
    patch.doApply();
    this.invPatches.push(patch);
    
    //TODO this won't be matched to the associated patch from the server
    var craftPatch = this.updateCraftIfNeeded(first, second);
    if(craftPatch != null) {
      craftPatch.doApply();
      this.invPatches.push(craftPatch);
    }
    
    return true;
  }
  invAutomerge(ref, qty, target) {
    
  }
  
  //false = do nothing
  //null = proceed as normal
  //InvPatch = successful override, do this patch
  invTryOverride(first, second) {
    if(first.equals(second))
      return false;
    
    //Creative inventory.
    if(
      (first.objType == "player" && first.listName == "creative") &&
      (second.objType == "player" && second.listName == "creative")
    ) {
      return false;
    }
    
    if(
      (first.objType == "player" && first.listName == "creative") ||
      (second.objType == "player" && second.listName == "creative")
    ) {
      //TODO: check creative priv? not neccessary though.
      
      var creative_ref = first;
      var other_ref = second;
      
      if(second.objType == "player" && second.listName == "creative") {
        creative_ref = second;
        other_ref = first;
      }
      
      var creative_stack = this.invGetStack(creative_ref);
      if(creative_stack != null)
        creative_stack = new ItemStack(creative_stack.itemstring, creative_stack.count, creative_stack.wear, creative_stack.data);
      var other_stack = this.invGetStack(other_ref);
      if(other_stack != null)
        other_stack = new ItemStack(other_stack.itemstring, other_stack.count, other_stack.wear, other_stack.data);
      
      if(other_stack == null) {
        //Put the creative item in the other ref, which is empty
        var creative_patch = new InvPatch(this, hopefullyPassableUUIDv4Generator());
        creative_patch.add(other_ref, other_stack, creative_stack);
        return creative_patch;
      } else if(creative_stack != null && creative_stack.itemstring == other_stack.itemstring) {
        //Combine -> other ref
        var combined_stack = new ItemStack(other_stack.itemstring, other_stack.count, other_stack.wear, other_stack.data);
        var def = combined_stack.getDef();
        combined_stack.count += creative_stack.count;
        if(combined_stack.count > def.max_stack)
          combined_stack.count = def.max_stack;
        
        var creative_patch = new InvPatch(this, hopefullyPassableUUIDv4Generator());
        creative_patch.add(other_ref, other_stack, combined_stack);
        return creative_patch;
      } else {
        //Destroy the item in the other ref
        var creative_patch = new InvPatch(this, hopefullyPassableUUIDv4Generator());
        creative_patch.add(other_ref, other_stack, null);
        return creative_patch;
      }
    }
    
    //Craft output.
    if(
      (first.objType == "player" && first.listName == "craftOutput") &&
      (second.objType == "player" && second.listName == "craftOutput")
    ) {
      return false;
    }
    
    if(
      (first.objType == "player" && first.listName == "craftOutput") ||
      (second.objType == "player" && second.listName == "craftOutput")
    ) {
      var output_ref = first;
      var other_ref = second;
      
      if(second.objType == "player" && second.listName == "craftOutput") {
        output_ref = second;
        other_ref = first;
      }
      
      var output_stack = this.invGetStack(output_ref);
      if(output_stack != null)
        output_stack = new ItemStack(output_stack.itemstring, output_stack.count, output_stack.wear, output_stack.data);
      var other_stack = this.invGetStack(other_ref);
      if(other_stack != null)
        other_stack = new ItemStack(other_stack.itemstring, other_stack.count, other_stack.wear, other_stack.data);
      
      var craft_ref = new InvRef("player", null, "craft", null);
      var craft_list = this.invGetList(craft_ref);
      var craft_res = getCraftRes(craft_list, {x: 3, y: 3});
      
      if(craft_res == null)
        return false;
      
      if(output_stack == null)
        return false;
      
      var combined_stack;
      if(other_stack != null) {
        if(!other_stack.typeMatch(output_stack))
          return false;
        var def = output_stack.getDef();
        if(!def.stackable)
          return false;
        if(output_stack.count + other_stack.count > def.maxStack)
          return false;
        
        combined_stack = new ItemStack(other_stack.itemstring, other_stack.count, other_stack.wear, other_stack.data);
        combined_stack.count += output_stack.count;
      } else {
        combined_stack = new ItemStack(output_stack.itemstring, output_stack.count, output_stack.wear, output_stack.data);
      }
      
      var craft_success_patch = new InvPatch(this, hopefullyPassableUUIDv4Generator());
      craft_success_patch.add(other_ref, other_stack, combined_stack);
      craft_success_patch.add(output_ref, output_stack, null);
      
      //TODO craftConsumeEntry should extend the patch
      
      return craft_success_patch;
    }
    
    return null;
  }
  
  updateCraftIfNeeded(first, second) {
    if(
      (first.objType == "player" && first.listName == "craft") ||
      (second.objType == "player" && second.listName == "craft") ||
      (first.objType == "player" && first.listName == "craftOutput") ||
      (second.objType == "player" && second.listName == "craftOutput")
    ) {
      var craft_input = this.invGetList(
          new InvRef("player", null, "craft", null));
      var craft_output_ref = new InvRef("player", null, "craftOutput", 0)
      var craft_output = this.invGetStack(craft_output_ref);
      
      var r = getCraftRes(craft_input, {x: 3, y: 3});
      var craft_res_stack = null;
      if(r != null)
        craft_res_stack = r.getRes();
      
      if(craft_res_stack == null && craft_output == null)
        return null;
      if(craft_res_stack != null && craft_output != null) {
        if(craft_res_stack.equals(craft_output))
          return null;
      }
      
      var out = new InvPatch(this, hopefullyPassableUUIDv4Generator());
      out.add(craft_output_ref, craft_output, craft_res_stack);
      return out;
    }
    
    return null;
  }
}
