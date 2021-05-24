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

class ItemStackBase {
  constructor(itemstring, count=1, wear=null, data=null) {
    this.itemstring = itemstring;
    this.count = count;
    this.wear = wear;
    this.data = data;
    if(this.data == null) { this.data = {}; }
  }
  
  getDef() {
    return getItemDef(this.itemstring);
  }
  
  typeMatch(stack) {
    return this.itemstring == stack.itemstring;
  }
  //mainly used for crafting
  softTypeMatch(needed) {
    if(this.itemstring == needed.itemstring) { return true; }
    if(needed.itemstring.startsWith("group:")) {
      if(needed.itemstring.substring(6) in this.getDef().groups) {
        return true;
      }
    }
    
    return false;
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
  
  toString() {
    var out = this.itemstring;
    if(this.count != 1) {
      out += " " + this.count.toString();
    }
    return out;
  }
  
  static fromNodeData(nodeData) {
    if(nodeData == null) { return null; }
    var def = getNodeDef(nodeData.itemstring);
    if(def == null) { return null; }
    if(def.drops != null) {
      return ItemStack.fromString(def.drops);
    }
    return new ItemStack(nodeData.itemstring);
  }
  
  static fromString(str) {
    var s = str.split(" ");
    var itemstring = s[0];
    if(getItemDef(itemstring) == null) { return null; }
    var count = 1;
    if(s.length >= 2) {
      count = parseInt(s[1]);
      if(isNaN(count)) {
        count = 1;
      }
    }
    return new ItemStack(itemstring, count);
  }
}

class ItemStack extends ItemStackBase {
  constructor(itemstring, count=1, wear=null, data=null) {
    super(itemstring, count, wear, data);
    
    if(this.getDef().isTool && this.wear == null) {
      this.wear = this.getDef().toolWear;
    }
  }
}

class ItemStackV extends ItemStackBase {
  constructor(itemstring, count=1, wear=null, data=null) {
    super(itemstring, count, wear, data);
  }
  
  static fromString(str) {
    var s = str.split(" ");
    var itemstring = s[0];
    if(getItemDef(itemstring) == null && !itemstring.startsWith("group:")) { return null; }
    var count = 1;
    if(s.length >= 2) {
      count = parseInt(s[1]);
      if(isNaN(count)) {
        count = 1;
      }
    }
    return new ItemStackV(itemstring, count);
  }
}

api.ItemStack = ItemStack;
