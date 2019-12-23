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

function craftConsumeEntry(entry, inv, listName, listShape) {
  var list = inv.getList(listName);
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
    }
  }
  
  inv.setList(listName, list);
  
  return true;
}

class CraftEntry {
  constructor(res, recipie, props) {
    this.res = res;
    this.list = recipie;
    
    this.shape = null; // or {x: <x>, y: <y>}
    this.type = "craft"; // or cook
    
    Object.assign(this, props);
  }
  
  //TODO: arbitrary list shapes
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
      }
    }
    
    return false;
  }
}

api.getCraftRes = getCraftRes;
api.craftConsumeEntry = craftConsumeEntry;
api.CraftEntry = CraftEntry;
api.registerCraft = function(craft) {
  //TODO: validation
  allCrafts.push(craft);
};
