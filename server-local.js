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

class ServerLocal extends ServerBase {
  constructor(map) {
    super();
    
    this.map = map;
    
    this.playerInventory = {};
    this.playerInventory["main"] = new Array(32).fill(null);
    this.playerInventory["craft"] = new Array(9).fill(null);
    this.playerInventory["craftOutput"] = new Array(1).fill(null);
    this.playerInventory["hand"] = new Array(1).fill(null);
    
    this.playerInventory["creative"] = [];
  }
  
  postInit() {
    var allItemStrings = api.listAllItems();
    for(var i = 0; i < allItemStrings.length; i++) {
      var itemstring = allItemStrings[i];
      var def = api.getItemDef(itemstring);
      if(def == null) {
        debug("server", "warning", "creative inventory: unable to retrieve definition for '" + itemstring + "'");
        continue;
      }
      if(!def.inCreativeInventory) { continue; }
      
      var stack = new api.ItemStack(itemstring);
      this.playerInventory["creative"].push(stack);
    }
  }
  
  addPlayer(player) {
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
  
  digNode(player, pos) {
    var nodeData = this.getNode(pos);
    var stack = ItemStack.fromNodeData(nodeData);
    
    var canGive = this.invCanGive(new InvRef("player", null, "main", null), stack);
    if(player.creativeDigPlace && this.invCanTake(new InvRef("player", null, "main", null), stack)) {
      canGive = true;
    }
    if(canGive) {
      if(player.creativeDigPlace && this.invCanTake(new InvRef("player", null, "main", null), stack)) {
        //player already has this item, no need to give them more
      } else {
        this.invGive(new InvRef("player", null, "main", null), stack);
      }
      this.setNode(pos, new NodeData("air"));
      
      debug("server", "log", "dig " + nodeData.itemstring + " at " + pos.toString());
      
      return true;
    }
    
    return false;
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
    
    if(this.invCanTakeIndex(new InvRef("player", null, "main", player.wieldIndex), stack)) {
      if(!player.creativeDigPlace) {
        this.invTakeIndex(new InvRef("player", null, "main", player.wieldIndex), stack);
      }
      var nodeData = NodeData.fromItemStack(stack);
      nodeData.rot = this.calcPlaceRot(nodeData);
      this.setNode(pos, nodeData);
      
      debug("server", "log", "place " + stack.itemstring + " at " + pos.toString());
      
      return true;
    }
    
    return false;
  }
  
  invSetList(ref, list) {
    if(ref.objType == "player") {
      if(ref.listName in this.playerInventory) {
        this.playerInventory[ref.listName] = list;
        this.updateInvDisplay(ref);
        return true;
      }
      return false;
    }
    
    throw new Error("unsupported inventory object type: " + ref.objType);
  }
  invSetStack(ref, stack) {
    if(ref.objType == "player") {
      if(ref.listName in this.playerInventory) {
        var list = this.playerInventory[ref.listName];
        if(ref.index < list.length) {
          list[ref.index] = stack;
          this.updateInvDisplay(ref);
          return true;
        }
      }
      return false;
    }
    
    throw new Error("unsupported inventory object type: " + ref.objType);
  }
  
  invAutomerge(ref, qty, target) {
    
  }
  
  invCanGive(ref, stack) {
    if(stack == null) { return true; }
    
    var list = this.invGetList(ref);
    
    var def = stack.getDef();
    
    var maxStack = def.maxStack;
    var needed = stack.count;
    var found = 0;
    for(var i = 0; i < list.length; i++) {
      if(list[i] == null) {
        found += maxStack;
      } else if(list[i].typeMatch(stack)) {
        found += maxStack - list[i].count;
      }
      
      if(found >= needed) { return true; }
    }
    
    return false;
  }
  invGive(ref, stack) {
    if(!this.invCanGive(ref, stack)) { return false; }
    
    if(stack == null) { return true; }
    
    var list = this.invGetList(ref);
    
    var def = stack.getDef();
    
    var maxStack = def.maxStack;
    var needed = stack.count;
    for(var i = 0; i < list.length; i++) {
      if(list[i] != null) {
        if(list[i].typeMatch(stack)) {
          var avail = maxStack - list[i].count;
          if(avail > 0) {
            list[i].count += Math.min(avail, needed);
            needed -= Math.min(avail, needed);
          }
        }
      }
      
      if(needed <= 0) { break; }
    }
    
    if(needed > 0) {
      //FIXME: mutilates original stack
      //FIXME: canGive vs give behavior differs on stacks where size > maxStack
      stack.count = needed;
      for(var i = 0; i < list.length; i++) {
        if(list[i] == null) {
          list[i] = stack;
          needed -= list[i].count;
        }
        
        if(needed <= 0) { break; }
      }
    }
    
    if(needed > 0) {
      //TODO
    }
    
    this.invSetList(ref, list);
    
    return true;
  }
  
  invCanTake(ref, stack) {
    if(stack == null) { return true; }
    
    var list = this.invGetList(ref);
    
    var needed = stack.count;
    var found = 0;
    for(var i = 0; i < list.length; i++) {
      if(list[i] != null) {
        if(list[i].typeMatch(stack)) {
          found += list[i].count;
        }
      }
      
      if(found >= needed) { return true; }
    }
    
    return false;
  }
  
  invCanTakeIndex(ref, stack) {
    var compStack = this.invGetStack(ref);
    
    if(compStack == null && stack != null) { return false; }
    if(stack == null) { return true; }
    
    if(!compStack.typeMatch(stack)) { return false; }
    if(compStack.count < stack.count) { return false; }
    
    return true;
  }
  invTakeIndex(ref, stack) {
    if(!this.invCanTakeIndex(ref, stack)) { return false; }
    
    if(stack == null) { return true; }
    
    var compStack = this.invGetStack(ref);
    
    compStack.count -= stack.count;
    if(compStack.count <= 0) { compStack = null; }
    
    this.invSetStack(ref, compStack);
    
    return true;
  }
}
