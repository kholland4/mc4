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

var SERVER_REMOTE_UPDATE_INTERVAL = 0.25; //how often to send updates about, i. e., the player position to the remote server
var serverUncacheDist = new MapPos(7, 4, 7, 2, 0, 0);

class InvRef {
  constructor(objType, objID, listName, index) {
    this.objType = objType;
    this.objID = objID;
    this.listName = listName;
    this.index = index;
  }
  
  cloneWithIndex(index) {
    return new InvRef(this.objType, this.objID, this.listName, index);
  }
  
  sameList(other) {
    return this.objType == other.objType &&
           compareObj(this.objID, other.objID) &&
           this.listName == other.listName;
  }
  
  equals(other) {
    return this.sameList(other) && this.index == other.index;
  }
}

api.InvRef = InvRef;

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
  suggestUncacheMapBlock(pos) {}
  
  getNodeRaw(pos) {
    var mapBlockPos = new MapPos(Math.floor(pos.x / MAPBLOCK_SIZE.x), Math.floor(pos.y / MAPBLOCK_SIZE.y), Math.floor(pos.z / MAPBLOCK_SIZE.z), pos.w, pos.world, pos.universe);
    var mapBlock = this.getMapBlock(mapBlockPos);
    if(mapBlock == null)
      return null;
    
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
    if(mapBlock == null)
      return null;
    
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
    
    this.setMapBlock(mapBlockPos, mapBlock);
  }
  
  calcPlaceRot(nodeData, pos, pos_on) {
    var nodeDef = nodeData.getDef();
    
    if(nodeDef.setRotOnPlace && nodeDef.rotDataType == "rot") {
      var rotAxis = 0; //the face that will be on top of the block; 0=+y, 1=-y, 2=+x, 3=+z, 4=-x, 5=-z
      var rotFace = 0;
      if(pos.equals(pos_on)) {
        //pretend we're placing on the ground
        pos_on = pos.add(new MapPos(0, -1, 0, 0, 0, 0));
      }
      
      if(pos_on.add(new MapPos(0, 1, 0, 0, 0, 0)).equals(pos) ||
         pos_on.add(new MapPos(0, -1, 0, 0, 0, 0)).equals(pos)) {
        //on ground or ceiling
        if(pos_on.add(new MapPos(0, 1, 0, 0, 0, 0)).equals(pos)) {
          //on ground
          rotAxis = 0;
        } else {
          //on ceiling
          rotAxis = 1;
        }
        if(!nodeDef.limitRotOnPlace) {
          //rotate based on player position
          var euler = new THREE.Euler().setFromQuaternion(player.rot, "YXZ");
          var yaw = euler.y;
          
          if(yaw >= (-3/4)*Math.PI && yaw <= (-1/4)*Math.PI) {
            rotFace = 0;
          } else if(yaw < (-3/4)*Math.PI || yaw > (3/4)*Math.PI) {
            rotFace = 1;
          } else if(yaw >= (1/4)*Math.PI && yaw <= (3/4)*Math.PI) {
            rotFace = 2;
          } else { //yaw > (-1/4)*Math.PI && yaw < (1/4)*Math.PI
            rotFace = 3;
          }
        }
      } else if(pos_on.add(new MapPos(-1, 0, 0, 0, 0, 0)).equals(pos)) {
        //+y face is pointing in the -x direction
        rotAxis = 2;
        rotFace = 2;
      } else if(pos_on.add(new MapPos(1, 0, 0, 0, 0, 0)).equals(pos)) {
        //+y face is pointing in the +x direction
        rotAxis = 2;
        rotFace = 0;
      } else if(pos_on.add(new MapPos(0, 0, -1, 0, 0, 0)).equals(pos)) {
        //+y face is pointing in the -z direction
        rotAxis = 2;
        rotFace = 3;
      } else if(pos_on.add(new MapPos(0, 0, 1, 0, 0, 0)).equals(pos)) {
        //+y face is pointing in the +z direction
        rotAxis = 2;
        rotFace = 1;
      }
      
      assert(rotAxis >= 0 && rotAxis < 6, "0 <= rotAxis < 6");
      assert(rotFace >= 0 && rotFace < 4, "0 <= rotFace < 4");
      return (rotAxis << 2) | rotFace;
    }
    
    return 0;
  }
  
  nodeToPlace(player) {
    if(player.wield == null) { return null; }
    
    if(!player.wield.getDef().isNode) { return null; }
    
    return NodeData.fromItemStack(player.wield);
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
  postInit() {}
  
  updateInvDisplay(ref) {
    var el = document.getElementsByClassName("uiInvList");
    for(var i = 0; i < el.length; i++) {
      var props = JSON.parse(el[i].dataset.inv);
      var pRef = new InvRef(props.ref.objType, props.ref.objID, props.ref.listName, props.ref.index);
      if(ref.sameList(pRef)) {
        uiUpdateInventoryList(pRef, props.args, el[i]);
      }
    }
  }
}

api.getNode = function(pos) { return server.getNode(pos); };
api.setNode = function(pos, nodeData) { return server.getNode(pos, nodeData); };
