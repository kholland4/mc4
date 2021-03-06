//Used to generate 'defs.json' for the server to use.

function dumpAllDefs() {
  var nodeDefs = {};
  
  for(var itemstring in allNodes) {
    var src = allNodes[itemstring];
    nodeDefs[itemstring] = {
      walkable: src.walkable,
      visible: src.visible,
      transparent: src.transparent,
      renderAdj: src.renderAdj,
      passSunlight: src.passSunlight,
      lightLevel: src.lightLevel,
      joined: src.joined,
      isFluid: src.isFluid,
      drops: src.drops,
      breakable: src.breakable,
      groups: src.groups
    };
  }
  
  var itemDefs = {};
  
  for(var itemstring in allItems) {
    var src = allItems[itemstring];
    itemDefs[itemstring] = {
      stackable: src.stackable,
      maxStack: src.maxStack,
      isNode: src.isNode,
      isTool: src.isTool,
      toolWear: src.toolWear,
      inCreativeInventory: src.inCreativeInventory,
      groups: src.groups,
      toolGroups: src.toolGroups
    };
  }
  
  var out = {
    nodeDefs: nodeDefs,
    itemDefs: itemDefs
  };
  
  var outJSON = JSON.stringify(out);
  
  var a = document.createElement("a");
  a.download = "defs.json";
  var blob = new Blob([outJSON], {type: "application/json"});
  a.href = window.URL.createObjectURL(blob);
  a.style.display = "none";
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
}
