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

class InvPatch {
  constructor(reqID=null) {
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
  
  doApply(server, use_current=true) {
    for(var diff of this.diffs) {
      if(server.isRemote()) {
        var key = diff.ref.objType + "/" + diff.ref.objID + "/" + diff.ref.listName;
        if(!(key in server.invCache))
          throw new Error("invalid inventory list: " + key);
        
        var list = server.invCache[key];
        
        if(diff.ref.index >= list.length)
          throw new Error("out of bounds inventory reference: " + key + "/" + diff.ref.index);
        
        if(use_current)
          list[diff.ref.index] = diff.current;
        else
          list[diff.ref.index] = diff.prev;
        server.updateInvDisplay(diff.ref);
        
        continue;
      }
      
      if(diff.ref.objType == "player") {
        if(!(diff.ref.listName in server.playerInventory))
          throw new Error("invalid inventory list: player/" + diff.ref.listName);
          
        var list = server.playerInventory[diff.ref.listName];
        if(diff.ref.index >= list.length)
          throw new Error("out of bounds inventory reference: " + diff.ref.index);
        
        if(use_current)
          list[diff.ref.index] = diff.current;
        else
          list[diff.ref.index] = diff.prev;
        server.updateInvDisplay(diff.ref);
        
        continue;
      }
      
      throw new Error("unsupported inventory object type: " + diff.ref.objType);
    }
  }
  doRevert(server) {
    this.doApply(server, false);
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
  
  overlaps(other) {
    var diffList1 = [...this.diffs];
    var diffList2 = [...other.diffs];
    
    for(var check1 of diffList1) {
      for(var check2 of diffList2) {
        if(check1.ref.equals(check2.ref))
          return true;
      }
    }
    
    return false;
  }
}

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
      if(pRef.index != null) {
        props.start = pRef.index;
        props.count = 1;
      }
      if(ref.sameList(pRef)) {
        uiUpdateInventoryList(pRef, props.args, el[i]);
      }
    }
  }
  
  invGetList(ref) {
    if(this.isRemote()) {
      var key = ref.objType + "/" + ref.objID + "/" + ref.listName;
      if(key in this.invCache) {
        return this.invCache[key];
      }
      throw new Error("no such inventory list: " + key);
    }
    
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
    if(this.isRemote()) {
      var key = ref.objType + "/" + ref.objID + "/" + ref.listName;
      if(key in this.invCache) {
        var list = this.invCache[key];
        if(ref.index < list.length) {
          return list[ref.index];
        }
        
        throw new Error("out of bounds inventory access: " + key + "/" + ref.index);
      }
      throw new Error("no such inventory list: " + key);
    }
    
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
    if(!this._socketReady && this.isRemote())
      return false;
    
    var craftPatchID = hopefullyPassableUUIDv4Generator();
    
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
      if(this.isRemote()) {
        this.socket.send(JSON.stringify({
          type: "inv_swap",
          ref1: first,
          ref2: second,
          orig1: origStack1,
          orig2: origStack2,
          reqID: overrideResult.reqID,
          craftPatchID: craftPatchID
        }));
      }
      
      overrideResult.doApply(this);
      if(this.isRemote())
        this.invPatches.push(overrideResult);
      
      //Update craft grid if needed
      var craftPatch = this.updateCraftIfNeeded(first, second);
      if(craftPatch != null) {
        craftPatch.reqID = craftPatchID;
        craftPatch.doApply(this);
        if(this.isRemote())
          this.invPatches.push(craftPatch);
      }
      
      return true;
    }
    //---
    
    var newStack1 = origStack2;
    var newStack2 = origStack1;
    
    var patch = new InvPatch(hopefullyPassableUUIDv4Generator());
    patch.add(first, origStack1, newStack1);
    patch.add(second, origStack2, newStack2);
    
    if(this.isRemote()) {
      this.socket.send(JSON.stringify({
        type: "inv_swap",
        ref1: first,
        ref2: second,
        orig1: origStack1,
        orig2: origStack2,
        reqID: patch.reqID,
        craftPatchID: craftPatchID
      }));
    }
    
    patch.doApply(this);
    if(this.isRemote())
      this.invPatches.push(patch);
    
    //Update craft grid if needed
    var craftPatch = this.updateCraftIfNeeded(first, second);
    if(craftPatch != null) {
      craftPatch.reqID = craftPatchID;
      craftPatch.doApply(this);
      if(this.isRemote())
        this.invPatches.push(craftPatch);
    }
    
    return true;
  }
  invDistribute(first, qty1, second, qty2) {
    if(this.isRemote() && !this._socketReady)
      return false;
    
    var craftPatchID = hopefullyPassableUUIDv4Generator();
    
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
      if(this.isRemote()) {
        this.socket.send(JSON.stringify({
          type: "inv_distribute",
          ref1: first,
          orig1: stack1,
          qty1: qty1,
          ref2: second,
          orig2: stack2,
          qty2: qty2,
          reqID: overrideResult.reqID,
          craftPatchID: craftPatchID
        }));
      }
      
      overrideResult.doApply(this);
      if(this.isRemote())
        this.invPatches.push(overrideResult);
      
      //Update craft grid if needed
      var craftPatch = this.updateCraftIfNeeded(first, second);
      if(craftPatch != null) {
        craftPatch.reqID = craftPatchID;
        craftPatch.doApply(this);
        if(this.isRemote())
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
    
    
    var patch = new InvPatch(hopefullyPassableUUIDv4Generator());
    patch.add(first, stack1, newStack1);
    patch.add(second, stack2, newStack2);
    
    if(this.isRemote()) {
      this.socket.send(JSON.stringify({
        type: "inv_distribute",
        ref1: first,
        orig1: stack1,
        qty1: qty1,
        ref2: second,
        orig2: stack2,
        qty2: qty2,
        reqID: patch.reqID,
        craftPatchID: craftPatchID
      }));
    }
    
    patch.doApply(this);
    if(this.isRemote())
      this.invPatches.push(patch);
    
    //Update craft grid if needed
    var craftPatch = this.updateCraftIfNeeded(first, second);
    if(craftPatch != null) {
      craftPatch.reqID = craftPatchID;
      craftPatch.doApply(this);
      if(this.isRemote())
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
        var creative_patch = new InvPatch(hopefullyPassableUUIDv4Generator());
        creative_patch.add(other_ref, other_stack, creative_stack);
        return creative_patch;
      } else if(creative_stack != null && creative_stack.itemstring == other_stack.itemstring) {
        //Combine -> other ref
        var combined_stack = new ItemStack(other_stack.itemstring, other_stack.count, other_stack.wear, other_stack.data);
        var def = combined_stack.getDef();
        if(!def.stackable) {
          //Do nothing
          return false;
        }
        combined_stack.count += creative_stack.count;
        if(combined_stack.count > def.max_stack)
          combined_stack.count = def.max_stack;
        
        var creative_patch = new InvPatch(hopefullyPassableUUIDv4Generator());
        creative_patch.add(other_ref, other_stack, combined_stack);
        return creative_patch;
      } else {
        //Destroy the item in the other ref
        var creative_patch = new InvPatch(hopefullyPassableUUIDv4Generator());
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
      
      var craft_success_patch = new InvPatch(hopefullyPassableUUIDv4Generator());
      craft_success_patch.add(other_ref, other_stack, combined_stack);
      craft_success_patch.add(output_ref, output_stack, null);
      
      //Consume the materials required by the craft recipie
      var consume_patch = craft_res.consume;
      if(consume_patch != null) {
        for(var diff of consume_patch.diffs) {
          craft_success_patch.diffs.push(diff);
        }
      }
      
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
        craft_res_stack = r.result;
      
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

api.getNode = function(pos) { return server.getNode(pos); };
api.setNode = function(pos, nodeData) { return server.getNode(pos, nodeData); };
