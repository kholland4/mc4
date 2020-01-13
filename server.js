class ServerBase {
  constructor() {
    
  }
  
  addPlayer(player) {
    
  }
  
  getMapBlock(pos, needLight=false) {
    return new MapBlock(pos);
  }
  setMapBlock(pos, mapBlock) {
    
  }
  
  getNode(pos) {
    var mapBlockPos = new THREE.Vector3(Math.floor(pos.x / MAPBLOCK_SIZE.x), Math.floor(pos.y / MAPBLOCK_SIZE.y), Math.floor(pos.z / MAPBLOCK_SIZE.z));
    var mapBlock = this.getMapBlock(mapBlockPos);
    if(mapBlock == null) { return null; }
    
    var localPos = new THREE.Vector3(
      ((pos.x % MAPBLOCK_SIZE.x) + MAPBLOCK_SIZE.x) % MAPBLOCK_SIZE.x,
      ((pos.y % MAPBLOCK_SIZE.y) + MAPBLOCK_SIZE.y) % MAPBLOCK_SIZE.y,
      ((pos.z % MAPBLOCK_SIZE.z) + MAPBLOCK_SIZE.z) % MAPBLOCK_SIZE.z);
    
    var n = mapBlock.data[localPos.x][localPos.y][localPos.z];
    var id = nodeID(n);
    return new NodeData(mapBlock.getItemstring(id), nodeRot(n));
  }
  
  setNode(pos, nodeData) {
    var mapBlockPos = new THREE.Vector3(Math.floor(pos.x / MAPBLOCK_SIZE.x), Math.floor(pos.y / MAPBLOCK_SIZE.y), Math.floor(pos.z / MAPBLOCK_SIZE.z));
    var mapBlock = this.getMapBlock(mapBlockPos);
    var localPos = new THREE.Vector3(
      ((pos.x % MAPBLOCK_SIZE.x) + MAPBLOCK_SIZE.x) % MAPBLOCK_SIZE.x,
      ((pos.y % MAPBLOCK_SIZE.y) + MAPBLOCK_SIZE.y) % MAPBLOCK_SIZE.y,
      ((pos.z % MAPBLOCK_SIZE.z) + MAPBLOCK_SIZE.z) % MAPBLOCK_SIZE.z);
    
    var val = nodeN(mapBlock.getNodeID(nodeData.itemstring), nodeData.rot);
    mapBlock.data[localPos.x][localPos.y][localPos.z] = val;
    if(nodeData.itemstring != "default:air") { mapBlock.props.sunlit = false; }
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
  
  digNode(player, sel) {
    var nodeData = this.getNode(sel);
    var stack = ItemStack.fromNodeData(nodeData);
    
    if(player.inventory.canGive("main", stack)) {
      player.inventory.give("main", stack);
      this.setNode(sel, new NodeData("default:air"));
      
      debug("server", "log", "dig " + nodeData.itemstring + " at " + fmtXYZ(sel));
      
      return true;
    }
    
    return false;
  }
  placeNode(player, sel) {
    if(player.wield == null) { return false; }
    
    var stack = player.wield;
    if(player.wield.getDef().stackable) {
      stack = player.wield.clone();
      stack.count = 1;
    }
    
    var def = stack.getDef();
    if(!def.isNode) { return false; }
    
    if(player.inventory.canTakeIndex("main", player.wieldIndex, stack)) {
      player.inventory.takeIndex("main", player.wieldIndex, stack);
      this.setNode(sel, NodeData.fromItemStack(stack));
      
      debug("server", "log", "place " + stack.itemstring + " at " + fmtXYZ(sel));
      
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
  
  get ready() { return true; }
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

//TODO
class ServerRemote extends ServerBase {
  constructor(url) {
    super();
    
    this.socket = new WebSocket(url);
    this._socketReady = false;
    this._invReady = true;
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
      var data = JSON.parse(e.data);
      
      if(data.type == "req_mapblock") {
        var mdata = data.data;
        var index = mdata.pos.x + "," + mdata.pos.y + "," + mdata.pos.z;
        
        var mapBlock = new MapBlock(new THREE.Vector3(mdata.pos.x, mdata.pos.y, mdata.pos.z));
        mapBlock.updateNum = mdata.updateNum;
        mapBlock.lightingNeedsUpdate = mdata.lightingNeedsUpdate;
        if(index in this.cache) {
          if(mapBlock.updateNum != this.cache[index].updateNum) {
            mapBlock.renderNeedsUpdate = 2;
          }
        } else {
          mapBlock.renderNeedsUpdate = 1;
        }
        mapBlock.IDtoIS = mdata.IDtoIS;
        mapBlock.IStoID = mdata.IStoID;
        Object.assign(mapBlock.props, mdata.props);
        mapBlock.data = mdata.data;
        
        this.cache[index] = mapBlock;
      }
    }.bind(this);
    
    this.playerInventory = {};
    
    this.cache = {};
    this.saved = {};
    
    this.requests = [];
  }
  
  //TODO: support multiple players
  addPlayer(player) {
    var inv = this.getInventory(player);
    
    inv.setList("main", new Array(32).fill(null));
    inv.setList("craft", new Array(9).fill(null));
    inv.setList("hand", new Array(1).fill(null));
  }
  
  getMapBlock(pos, needLight=false) {
    var index = pos.x + "," + pos.y + "," + pos.z;
    if(index in this.saved) {
      return this.saved[index];
    } else if(index in this.cache) {
      return this.cache[index];
    } else {
      if(!this._socketReady) { return null; }
      
      if(!this.requests.includes(index)) {
        this.socket.send(JSON.stringify({
          type: "req_mapblock",
          pos: {x: pos.x, y: pos.y, z: pos.z}
        }));
        this.requests.push(index);
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
  
  setNode(pos, nodeData) {
    //FIXME
    var mapBlockPos = new THREE.Vector3(Math.floor(pos.x / MAPBLOCK_SIZE.x), Math.floor(pos.y / MAPBLOCK_SIZE.y), Math.floor(pos.z / MAPBLOCK_SIZE.z));
    var mapBlock = this.getMapBlock(mapBlockPos);
    var localPos = new THREE.Vector3(
      ((pos.x % MAPBLOCK_SIZE.x) + MAPBLOCK_SIZE.x) % MAPBLOCK_SIZE.x,
      ((pos.y % MAPBLOCK_SIZE.y) + MAPBLOCK_SIZE.y) % MAPBLOCK_SIZE.y,
      ((pos.z % MAPBLOCK_SIZE.z) + MAPBLOCK_SIZE.z) % MAPBLOCK_SIZE.z);
    
    var val = nodeN(mapBlock.getNodeID(nodeData.itemstring), nodeData.rot);
    mapBlock.data[localPos.x][localPos.y][localPos.z] = val;
    if(nodeData.itemstring != "default:air") { mapBlock.props.sunlit = false; }
    mapBlock.markDirty();
    //FIXME
    /*if(localPos.x == 0) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(-1, 0, 0))).markDirty(); } else
    if(localPos.x == MAPBLOCK_SIZE.x - 1) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(1, 0, 0))).markDirty(); }
    if(localPos.y == 0) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(0, -1, 0))).markDirty(); } else
    if(localPos.y == MAPBLOCK_SIZE.y - 1) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(0, 1, 0))).markDirty(); }
    if(localPos.z == 0) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(0, 0, -1))).markDirty(); } else
    if(localPos.z == MAPBLOCK_SIZE.z - 1) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(0, 0, 1))).markDirty(); }*/
    //---
    //this.setMapBlock(mapBlockPos, mapBlock);
      
    if(this._socketReady) {
      this.socket.send(JSON.stringify({
        type: "set_node",
        pos: {x: pos.x, y: pos.y, z: pos.z},
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
}

api.getNode = function(pos) { return server.getNode(pos); };
api.setNode = function(pos, nodeData) { return server.getNode(pos, nodeData); };
