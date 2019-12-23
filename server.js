class ServerBase {
  constructor() {
    
  }
  
  getMapBlock(pos) {
    return new MapBlock(pos);
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
  
  getMapBlock(pos) {
    return this.map.getMapBlock(pos);
  }
  
  getNode(pos) {
    var mapBlockPos = new THREE.Vector3(Math.floor(pos.x / MAPBLOCK_SIZE.x), Math.floor(pos.y / MAPBLOCK_SIZE.y), Math.floor(pos.z / MAPBLOCK_SIZE.z));
    var mapBlock = this.getMapBlock(mapBlockPos);
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
    mapBlock.markDirty();
    //FIXME
    if(localPos.x == 0) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(-1, 0, 0))).markDirty(); } else
    if(localPos.x == MAPBLOCK_SIZE.x - 1) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(1, 0, 0))).markDirty(); }
    if(localPos.y == 0) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(0, -1, 0))).markDirty(); } else
    if(localPos.y == MAPBLOCK_SIZE.y - 1) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(0, 1, 0))).markDirty(); }
    if(localPos.z == 0) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(0, 0, -1))).markDirty(); } else
    if(localPos.z == MAPBLOCK_SIZE.z - 1) { this.getMapBlock(mapBlockPos.clone().add(new THREE.Vector3(0, 0, 1))).markDirty(); }
    //---
    this.map.dirtyMapBlock(mapBlockPos);
  }
  
  digNode(player, sel) {
    var nodeData = this.getNode(sel);
    var stack = ItemStack.fromNodeData(nodeData);
    
    if(player.inventory.canGive("main", stack)) {
      player.inventory.give("main", stack);
      this.setNode(sel, new NodeData("default:air"));
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
  constructor() {
    super();
  }
}

api.getNode = function(pos) { return server.getNode(pos); };
api.setNode = function(pos, nodeData) { return server.getNode(pos, nodeData); };
