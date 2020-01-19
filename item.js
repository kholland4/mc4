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
