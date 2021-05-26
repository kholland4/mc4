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
  
  //TODO: support multiple players
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
  
  invGetList(ref) {
    if(ref.objType == "player") {
      if(ref.listName in this.playerInventory) {
        return this.playerInventory[ref.listName];
      }
      return [];
    }
    
    throw new Error("unsupported inventory object type: " + ref.objType);
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
  invGetStack(ref) {
    if(ref.objType == "player") {
      if(ref.listName in this.playerInventory) {
        var list = this.playerInventory[ref.listName];
        if(ref.index < list.length) {
          return list[ref.index];
        }
      }
      return null;
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
  
  invTryOverride(first, second, stack1, stack2) {
    //Creative inventory
    if(first.objType == "player" && first.listName == "creative") {
      if(stack2 != null)
        return false;
      this.invSetStack(second, stack1);
      return true;
    }
    if(second.objType == "player" && second.listName == "creative") {
      if(stack1 != null)
        return false;
      this.invSetStack(first, stack2);
      return true;
    }
    
    //Craft output
    if((first.objType == "player" && first.listName == "craftOutput") ||
       (second.objType == "player" && second.listName == "craftOutput")) {
      if(second.objType == "player" && second.listName == "craftOutput") {
        var temp = first;
        first = second;
        second = temp;
        temp = stack1;
        stack1 = stack2;
        stack2 = temp;
      }
      
      var craftRef = new InvRef("player", null, "craft", null);
      var craftList = this.invGetList(craftRef);
      var r = getCraftRes(craftList, {x: 3, y: 3});
      if(r == null)
        return false;
      
      if(stack1 == null)
        return false;
      
      if(stack2 != null) {
        if(!stack2.typeMatch(stack1))
          return false;
        var def = stack1.getDef();
        if(!def.stackable)
          return false;
        if(stack1.count + stack2.count > def.maxStack)
          return false;
        
        stack2.count += stack1.count;
      } else {
        stack2 = stack1;
      }
      
      craftConsumeEntry(r, craftList, {x: 3, y: 3});
      this.invSetList(craftRef, craftList);
      this.invSetStack(second, stack2);
      
      var craftRes = null;
      var rNew = getCraftRes(this.invGetList(new InvRef("player", null, "craft", null)), {x: 3, y: 3});
      if(rNew != null) {
        craftRes = r.getRes();
      }
      this.invSetStack(new InvRef("player", null, "craftOutput", 0), craftRes);
      
      return true;
    }
    
    return null;
  }
  
  invSwap(first, second) {
    var stack1 = this.invGetStack(first);
    var stack2 = this.invGetStack(second);
    
    var res = this.invTryOverride(first, second, stack1, stack2);
    if(res !== null)
      return res;
    
    
    var stack1 = this.invGetStack(first);
    var stack2 = this.invGetStack(second);
    this.invSetStack(first, stack2);
    this.invSetStack(second, stack1);
    
    //Craft grid
    if((first.objType == "player" && first.listName == "craft") ||
       (second.objType == "player" && second.listName == "craft")) {
      var craftRes = null;
      var r = getCraftRes(this.invGetList(new InvRef("player", null, "craft", null)), {x: 3, y: 3});
      if(r != null) {
        craftRes = r.getRes();
      }
      this.invSetStack(new InvRef("player", null, "craftOutput", 0), craftRes);
    }
    
    return true;
  }
  
  invDistribute(first, qty1, second, qty2) {
    var stack1 = this.invGetStack(first);
    var stack2 = this.invGetStack(second);
    
    var res = this.invTryOverride(first, second, stack1, stack2);
    if(res !== null)
      return res;
    
    
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
    this.invSetStack(first, newStack1);
    this.invSetStack(second, newStack2);
    
    //Craft grid
    if((first.objType == "player" && first.listName == "craft") ||
       (second.objType == "player" && second.listName == "craft")) {
      var craftRes = null;
      var r = getCraftRes(this.invGetList(new InvRef("player", null, "craft", null)), {x: 3, y: 3});
      if(r != null) {
        craftRes = r.getRes();
      }
      this.invSetStack(new InvRef("player", null, "craftOutput", 0), craftRes);
    }
    
    return true;
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
