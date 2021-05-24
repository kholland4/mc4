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
    if(allCrafts[i].match(list, listShape)) {
      return allCrafts[i];
    }
  }
  
  return null;
}

function craftConsumeEntry(entry, list, listShape) {
  if(!entry.match(list, listShape)) { return false; }
  
  //TODO
  if(entry.shape != null && listShape != null) {
    if(entry.shape.x == listShape.x && entry.shape.y == listShape.y) {
      for(var i = 0; i < entry.shape.x * entry.shape.y; i++) {
        if(entry.list[i] != null) {
          list[i].count -= entry.list[i].count;
          if(list[i].count <= 0) {
            list[i] = null;
          }
        }
      }
    } else if(entry.shape.x <= listShape.x && entry.shape.y <= listShape.y) {
      var done = false;
      for(var xoff = 0; xoff <= (listShape.x - entry.shape.x); xoff++) {
        for(var yoff = 0; yoff <= (listShape.y - entry.shape.y); yoff++) {
          var fit = true;
          for(var x = 0; x < listShape.x; x++) {
            for(var y = 0; y < listShape.y; y++) {
              if(x < xoff || y < yoff || x >= (xoff + entry.shape.x) || y >= (yoff + entry.shape.y)) {
                //out of bounds of area being checked
                if(list[y * listShape.x + x] != null) {
                  fit = false;
                  break;
                }
                continue;
              }
              var recipieStack = entry.list[(y - yoff) * entry.shape.x + (x - xoff)];
              var listStack = list[y * listShape.x + x];
              
              if(recipieStack == null && listStack == null) { continue; }
              if(recipieStack == null || listStack == null) { fit = false; break; }
              if(!listStack.softTypeMatch(recipieStack)) { fit = false; break; }
              if(listStack.count < recipieStack.count) { fit = false; break; }
            }
          }
          
          if(fit) {
            for(var x = 0; x < entry.shape.x; x++) {
              for(var y = 0; y < entry.shape.y; y++) {
                var recipieStack = entry.list[y * entry.shape.x + x];
                var listStack = list[(y + yoff) * listShape.x + x + xoff];
                
                if(recipieStack != null) {
                  listStack.count -= recipieStack.count;
                  if(listStack.count <= 0) {
                    list[(y + yoff) * listShape.y + x + xoff] = null;
                  }
                }
              }
            }
            
            done = true;
            break;
          }
        }
        if(done) { break; }
      }
      if(!done) {
        throw new Error("craft recipie fit not found, even though it should've been");
      }
    }
  }
    
  if(entry.shape == null) {
    for(var i = 0; i < entry.list.length; i++) {
      var needed = entry.list[i];
      if(needed == null) { continue; }
      var found = false;
      for(var n = 0; n < list.length; n++) {
        if(list[n] == null) { continue; }
        
        if(list[n].softTypeMatch(needed) && list[n].count >= needed.count) {
          found = true;
          list[n].count -= needed.count;
          if(list[n].count <= 0) { list[n] = null; }
          break;
        }
      }
    }
  }
  
  return true;
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
  
  match(list, listShape) {
    if(this.shape != null && listShape != null) {
      if(this.shape.x == listShape.x && this.shape.y == listShape.y) {
        for(var i = 0; i < this.shape.x * this.shape.y; i++) {
          if(this.list[i] == null && list[i] == null) { continue; }
          if(this.list[i] == null || list[i] == null) { return false; }
          if(!list[i].softTypeMatch(this.list[i])) { return false; }
          if(list[i].count < this.list[i].count) { return false; }
        }
        
        return true;
      } else if(this.shape.x <= listShape.x && this.shape.y <= listShape.y) {
        for(var xoff = 0; xoff <= (listShape.x - this.shape.x); xoff++) {
          for(var yoff = 0; yoff <= (listShape.y - this.shape.y); yoff++) {
            var fit = true;
            for(var x = 0; x < listShape.x; x++) {
              for(var y = 0; y < listShape.y; y++) {
                if(x < xoff || y < yoff || x >= (xoff + this.shape.x) || y >= (yoff + this.shape.y)) {
                  //out of bounds of area being checked
                  if(list[y * listShape.x + x] != null) {
                    fit = false;
                    break;
                  }
                  continue;
                }
                var recipieStack = this.list[(y - yoff) * this.shape.x + (x - xoff)];
                var listStack = list[y * listShape.x + x];
                
                if(recipieStack == null && listStack == null) { continue; }
                if(recipieStack == null || listStack == null) { fit = false; break; }
                if(!listStack.softTypeMatch(recipieStack)) { fit = false; break; }
                if(listStack.count < recipieStack.count) { fit = false; break; }
              }
              if(!fit) { break; }
            }
            
            if(fit) {
              return true;
            }
          }
        }
        return false;
      }
    }
    
    if(this.shape == null) {
      var used = [];
      for(var i = 0; i < list.length; i++) { used.push(false); }
      
      for(var i = 0; i < this.list.length; i++) {
        var needed = this.list[i];
        if(needed == null) { continue; }
        var found = false;
        for(var n = 0; n < list.length; n++) {
          if(list[n] == null) { continue; }
          if(used[n]) { continue; }
          
          if(list[n].softTypeMatch(needed) && list[n].count >= needed.count) {
            found = true;
            used[n] = true;
            break;
          }
        }
        if(!found) { return false; }
      }
      
      //check for extra items
      for(var i = 0; i < list.length; i++) {
        if(used[i] == false && list[i] != null) {
          return false;
        }
      }
      
      return true;
    }
    
    return false;
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
