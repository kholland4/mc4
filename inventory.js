class ItemStack {
  constructor(itemstring, count=1, wear=null, data=null) {
    this.itemstring = itemstring;
    this.count = count;
    this.wear = wear;
    this.data = data;
    if(this.data == null) { this.data = {}; }
    if(this.getDef().isTool && this.wear == null) {
      this.wear = this.getDef().toolWear;
    }
  }
  
  getDef() {
    return getItemDef(this.itemstring);
  }
  
  typeMatch(stack) {
    return this.itemstring == stack.itemstring;
  }
  
  equals(stack) {
    return this.itemstring == stack.itemstring && this.count == stack.count && this.wear == stack.wear && JSON.stringify(this.data) == JSON.stringify(stack.data);
  }
  equalsBasic(stack) {
    return this.itemstring == stack.itemstring && this.count == stack.count && this.wear == stack.wear;
  }
  
  clone() {
    return new ItemStack(this.itemstring, this.count, this.wear); //data is omitted deliberately
  }
  
  static fromNodeData(nodeData) {
    if(nodeData == null) { return null; }
    return new ItemStack(nodeData.itemstring);
  }
}

class Inventory {
  constructor(toCreate) {
    this._getList = function(name) { return []; };
    this._setList = function(name, data) { return false; };
    
    this._getStack = function(name, index) { return null; };
    this._setStack = function(name, index, data) { return false; };
  }
  
  getList(name) {
    return this._getList(name);
  }
  setList(name, data) {
    return this._setList(name, data);
  }
  
  getStack(name, index) {
    return this._getStack(name, index);
  }
  setStack(name, index, data) {
    return this._setStack(name, index, data);
  }
  
  canGive(name, stack) {
    if(stack == null) { return true; }
    
    var list = this.getList(name);
    
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
  give(name, stack) {
    if(!this.canGive(name, stack)) { return false; }
    
    if(stack == null) { return true; }
    
    var list = this.getList(name);
    
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
    
    this.setList(name, list);
    
    return true;
  }
  
  canTake(name, stack) {
    if(stack == null) { return true; }
    
    var list = this.getList(name);
    
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
  take(name, stack) {
    if(!this.canTake(name, stack)) { return false; }
    
    if(stack == null) { return true; }
    
    var list = this.getList(name);
    
    var needed = stack.count;
    for(var i = 0; i < list.length; i++) {
      if(list[i] != null) {
        if(list[i].typeMatch(stack)) {
          list[i].count -= Math.min(list[i].count, needed);
          needed -= Math.min(list[i].count, needed);
          if(list[i].count == 0) { list[i] = null; }
        }
      }
      
      if(needed <= 0) { break; }
    }
    
    if(needed > 0) {
      //TODO
    }
    
    this.setList(name, list);
    
    return true;
  }
  
  canTakeIndex(name, index, stack) {
    var compStack = this.getStack(name, index);
    
    if(compStack == null && stack != null) { return false; }
    if(stack == null) { return true; }
    
    if(!compStack.typeMatch(stack)) { return false; }
    if(compStack.count < stack.count) { return false; }
    
    return true;
  }
  takeIndex(name, index, stack) {
    if(!this.canTakeIndex(name, index, stack)) { return false; }
    
    if(stack == null) { return true; }
    
    var compStack = this.getStack(name, index);
    
    compStack.count -= stack.count;
    if(compStack.count <= 0) { compStack = null; }
    
    this.setStack(name, index, compStack);
    
    return true;
  }
}
