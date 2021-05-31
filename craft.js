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

var allCrafts = [];

function getCraftDef(itemstring) {
  var out = [];
  for(var i = 0; i < allCrafts.length; i++) {
    if(allCrafts[i].res.itemstring == itemstring) {
      out.push(allCrafts[i]);
    }
  }
  
  return out;
}

function getCraftRes(list, listShape) {
  for(var i = 0; i < allCrafts.length; i++) {
    var res = craftCalcResult(allCrafts[i], list, listShape);
    if(res != null)
      return res;
  }
  
  return null;
}

function craftCalcResult(entry, list, listShape) {
  var result = entry.getRes();
  
  if(entry.shape != null) {
    if(entry.shape.x > listShape.x || entry.shape.y > listShape.y)
      return null;
    
    for(var xoff = 0; xoff <= (listShape.x - entry.shape.x); xoff++) {
      for(var yoff = 0; yoff <= (listShape.y - entry.shape.y); yoff++) {
        var fit = true;
        var outPatch = new InvPatch();
        
        for(var x = 0; x < listShape.x; x++) {
          for(var y = 0; y < listShape.y; y++) {
            if(x < xoff || y < yoff || x >= (xoff + entry.shape.x) || y >= (yoff + entry.shape.y)) {
              //out of bounds of area being checked
              if(list[y * listShape.x + x] == null)
                continue;
              fit = false;
              break;
            }
            
            var recipieStack = entry.list[(y - yoff) * entry.shape.x + (x - xoff)];
            var listStack = list[y * listShape.x + x];
            
            if(recipieStack == null && listStack == null) { continue; }
            if(recipieStack == null || listStack == null) { fit = false; break; }
            if(!listStack.softTypeMatch(recipieStack)) { fit = false; break; }
            if(listStack.count < recipieStack.count) { fit = false; break; }
            
            var consumedStack = new ItemStack(listStack.itemstring, listStack.count, listStack.wear, listStack.data);
            consumedStack.count -= recipieStack.count;
            if(consumedStack.count <= 0)
              consumedStack = null;
            
            outPatch.add(
              new InvRef("player", null, "craft", y * listShape.x + x),
              listStack,
              consumedStack
            );
          }
          if(!fit)
            break;
        }
        
        if(fit)
          return {craft: entry, consume: outPatch, result: result};
      }
    }
    
    return null;
  } else {
    //shapeless recipie
    var outPatch = new InvPatch();
    
    var used = [];
    for(var i = 0; i < list.length; i++) { used.push(false); }
    
    for(var i = 0; i < entry.list.length; i++) {
      var needed = entry.list[i];
      if(needed == null) { continue; }
      var found = false;
      for(var n = 0; n < list.length; n++) {
        if(list[n] == null) { continue; }
        if(used[n]) { continue; }
        
        if(list[n].softTypeMatch(needed) && list[n].count >= needed.count) {
          found = true;
          used[n] = true;
          
          var consumedStack = new ItemStack(list[n].itemstring, list[n].count, list[n].wear, list[n].data);
          consumedStack.count -= needed.count;
          if(consumedStack.count <= 0)
            consumedStack = null;
          
          outPatch.add(
            new InvRef("player", null, "craft", n),
            list[n],
            consumedStack
          );
          break;
        }
      }
      if(!found)
        return null;
    }
    
    //check for extra items
    for(var i = 0; i < list.length; i++) {
      if(used[i] == false && list[i] != null)
        return null;
    }
    
    return {craft: entry, consume: outPatch, result: result};
  }
}

class CraftEntry {
  constructor(res, recipie, props) {
    this.res = res;
    this.list = [];
    for(var i = 0; i < recipie.length; i++) {
      if(recipie[i] == null) {
        this.list.push(null);
      } else {
        this.list.push(ItemStackV.fromString(recipie[i]));
      }
    }
    
    this.shape = null; // or {x: <x>, y: <y>}
    this.type = "craft"; // or cook
    this.cookTime = null;
    
    Object.assign(this, props);
  }
  
  getRes() {
    return ItemStack.fromString(this.res);
  }
}

api.CraftEntry = CraftEntry;
api.registerCraft = function(craft) {
  //TODO: validation
  allCrafts.push(craft);
};
