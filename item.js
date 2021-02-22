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

var allItems = {};

function getItemDef(itemstring) {
  var res = allItems[itemstring];
  if(res == undefined) { return null; }
  return res;
}

class ItemBase {
  constructor(itemstring, props) {
    this.itemstring = itemstring;
    
    this.desc = "";
    
    this.stackable = true;
    this.maxStack = 64;
    
    this.isNode = false;
    this.isTool = false;
    this.toolWear = 0;
    
    this.iconFile = null;
    this.icon = null;
    
    this.inCreativeInventory = true;
    
    //wood, tree, stone, sand, sandstone, flora, leaves
    this.groups = {};
    this.toolGroups = {}; //node groups that a tool "helps" with
    
    Object.assign(this, props);
    
    if(this.isTool) {
      this.stackable = false;
    }
    if(!this.stackable) {
      this.maxStack = 1;
    }
    
    if(this.iconFile != null && this.icon == null) {
      var e = iconExists(this.iconFile);
      if(e != false) {
        this.icon = e;
      } else {
        this.icon = loadIcon(this.itemstring, this.iconFile);
      }
    }
  }
}

class Item extends ItemBase {
  constructor(itemstring, props) {
    super(itemstring, props);
  }
}

api.Item = Item;
api.registerItem = function(item) {
  //TODO: validation
  allItems[item.itemstring] = item;
};
api.getItemDef = getItemDef;
api.listAllItems = function() {
  var out = [];
  for(var itemstring in allItems) {
    out.push(itemstring);
  }
  return out;
};
