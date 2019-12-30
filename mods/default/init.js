(function() {
  var modpath = api.getModMeta("default").path;
  
  function registerNodeHelper(itemstring, args) {
    args = Object.assign({desc: "", tex: null, icon: null, item: {}, node: {}}, args);
    
    var texOverlay = {};
    for(var key in args.tex) {
      var file = modpath + "/textures/" + args.tex[key];
      var n = api.textureExists(file);
      if(n == false) {
        api.loadTexture(itemstring + ":" + key, file);
        texOverlay[key] = itemstring + ":" + key;
      } else {
        texOverlay[key] = n;
      }
    }
    if(args.icon != null) {
      args.item.iconFile = modpath + "/icons/" + args.icon;
    }
    api.registerItem(new api.Item(itemstring, Object.assign({isNode: true, desc: args.desc}, args.item)));
    api.registerNode(new api.Node(itemstring, Object.assign(texOverlay, args.node)));
  }
  
  //---NODES---
  registerNodeHelper("default:dirt", {
    desc: "Dirt",
    tex: {texAll: "default_dirt.png"},
    icon: "default_dirt.png",
    node: {groups: {crumbly: 1}}
  });
  
  registerNodeHelper("default:grass", {
    desc: "Dirt with Grass",
    tex: {texTop: "default_grass.png", texBottom: "default_dirt.png", texSides: "default_grass_side.png"},
    icon: "default_grass.png",
    node: {groups: {crumbly: 1}, drops: "default:dirt"}
  });
  
  registerNodeHelper("default:stone", {
    desc: "Stone",
    tex: {texAll: "default_stone.png"},
    icon: "default_stone.png",
    node: {groups: {cracky: 1}, drops: "default:cobble"},
    item: {groups: {stone: null}}
  });
  registerNodeHelper("default:cobble", {
    desc: "Cobblestone",
    tex: {texAll: "default_cobble.png"},
    icon: "default_cobble.png",
    node: {groups: {cracky: 1}},
    item: {groups: {stone: null}}
  });
  
  registerNodeHelper("default:wood", {
    desc: "Wood Planks",
    tex: {texAll: "default_wood.png"},
    icon: "default_wood.png",
    node: {groups: {choppy: 1}},
    item: {groups: {wood: null}}
  });
  
  registerNodeHelper("default:tree", {
    desc: "Tree",
    tex: {texTop: "default_tree_top.png", texBottom: "default_tree_top.png", texSides: "default_tree.png"},
    icon: "default_tree.png",
    node: {groups: {choppy: 2}},
    item: {groups: {tree: null}}
  });
  api.registerCraft(new api.CraftEntry(
    "default:wood 4",
    ["default:tree"],
    {shape: null}
  ));
  
  registerNodeHelper("default:leaves", {
    desc: "Leaves",
    tex: {texAll: "default_leaves.png"},
    icon: "default_leaves.png",
    node: {groups: {snappy: 1}, transparent: true},
    item: {groups: {leaves: null}}
  });
  
  /*registerNodeHelper("default:torch", {
    desc: "Torch",
    tex: {texAll: "default_stone.png"},
    icon: "../textures/default_stone.png",
    node: {groups: {}, lightLevel: 14, transparent: true}
  });*/
  
  
  
  //---ITEMS---
  api.registerItem(new api.Item("default:stick", {desc: "Stick", iconFile: modpath + "/icons/default_stick.png"}));
  api.registerCraft(new api.CraftEntry(
    "default:stick 4",
    ["group:wood"],
    {shape: null}
  ));
  api.registerItem(new api.Item("default:diamond", {desc: "Diamond", iconFile: modpath + "/icons/default_diamond.png"}));
  api.registerItem(new api.Item("default:iron_lump", {desc: "Iron Lump", iconFile: modpath + "/icons/default_iron_lump.png"}));
  api.registerItem(new api.Item("default:steel_ingot", {desc: "Steel Ingot", iconFile: modpath + "/icons/default_steel_ingot.png"}));
  
  //hand tool
  api.registerItem(new Item(":", {isTool: true, toolWear: null, toolGroups: {
    crumbly: {times: [0, 1.0, 2.0], maxlevel: 2},
    //cracky: {times: [0, 2], maxlevel: 1},
    choppy: {times: [0, 2, 4], maxlevel: 2},
    snappy: {times: [0, 1.0], maxlevel: 1},
    oddly_breakable_by_hand: {times: [0, 0.5], maxlevel: 1}
  }}));
  
  //---PICKAXES---
  api.registerItem(new api.Item("default:pick_wood", {
    desc: "Wood Pickaxe",
    isTool: true, toolWear: 60, toolGroups: {
      "cracky": {times: [0, 1.6], maxlevel: 1}
    }, iconFile: modpath + "/icons/default_tool_woodpick.png"}));
  
  api.registerItem(new api.Item("default:pick_stone", {
    desc: "Stone Pickaxe",
    isTool: true, toolWear: 120, toolGroups: {
      "cracky": {times: [0, 1.0, 2.0], maxlevel: 2}
    }, iconFile: modpath + "/icons/default_tool_stonepick.png"}));
  
  api.registerItem(new api.Item("default:pick_steel", {
    desc: "Steel Pickaxe",
    isTool: true, toolWear: 240, toolGroups: {
      "cracky": {times: [0, 0.8, 1.6, 4.0], maxlevel: 3}
    }, iconFile: modpath + "/icons/default_tool_steelpick.png"}));
  
  api.registerItem(new api.Item("default:pick_diamond", {
    desc: "Diamond Pickaxe",
    isTool: true, toolWear: 1000, toolGroups: {
      "cracky": {times: [0, 0.5, 1.0, 2.0], maxlevel: 3}
    }, iconFile: modpath + "/icons/default_tool_diamondpick.png"}));
  
  
  //---AXES---
  api.registerItem(new api.Item("default:axe_wood", {
    desc: "Wood Axe",
    isTool: true, toolWear: 60, toolGroups: {
      "choppy": {times: [0, 1.6, 3.0], maxlevel: 2}
    }, iconFile: modpath + "/icons/default_tool_woodaxe.png"}));
  
  api.registerItem(new api.Item("default:axe_stone", {
    desc: "Stone Axe",
    isTool: true, toolWear: 120, toolGroups: {
      "choppy": {times: [0, 1.3, 2.0, 3.0], maxlevel: 3}
    }, iconFile: modpath + "/icons/default_tool_stoneaxe.png"}));
  
  api.registerItem(new api.Item("default:axe_steel", {
    desc: "Steel Axe",
    isTool: true, toolWear: 240, toolGroups: {
      "choppy": {times: [0, 1.0, 1.4, 2.5], maxlevel: 3}
    }, iconFile: modpath + "/icons/default_tool_steelaxe.png"}));
  
  api.registerItem(new api.Item("default:axe_diamond", {
    desc: "Diamond Axe",
    isTool: true, toolWear: 1000, toolGroups: {
      "choppy": {times: [0, 0.5, 0.9, 2.1], maxlevel: 3}
    }, iconFile: modpath + "/icons/default_tool_diamondaxe.png"}));
  
  
  //---SHOVELS---
  api.registerItem(new api.Item("default:shovel_wood", {
    desc: "Wood Shovel",
    isTool: true, toolWear: 60, toolGroups: {
      "crumbly": {times: [0, 0.6, 1.2, 1.8], maxlevel: 3}
    }, iconFile: modpath + "/icons/default_tool_woodshovel.png"}));
  
  api.registerItem(new api.Item("default:shovel_stone", {
    desc: "Stone Shovel",
    isTool: true, toolWear: 120, toolGroups: {
      "crumbly": {times: [0, 0.5, 1.2, 1.8], maxlevel: 3}
    }, iconFile: modpath + "/icons/default_tool_stoneshovel.png"}));
  
  api.registerItem(new api.Item("default:shovel_steel", {
    desc: "Steel Shovel",
    isTool: true, toolWear: 240, toolGroups: {
      "crumbly": {times: [0, 0.4, 0.9, 1.5], maxlevel: 3}
    }, iconFile: modpath + "/icons/default_tool_steelshovel.png"}));
  
  api.registerItem(new api.Item("default:shovel_diamond", {
    desc: "Diamond Shovel",
    isTool: true, toolWear: 1000, toolGroups: {
      "crumbly": {times: [0, 0.3, 0.5, 1.1], maxlevel: 3}
    }, iconFile: modpath + "/icons/default_tool_diamondshovel.png"}));
  
  //TODO: swords
  
  var craftMaterials = {
    "wood": "group:wood",
    "stone": "group:stone",
    "steel": "default:steel_ingot",
    "diamond": "default:diamond"
  };
  
  for(var type in craftMaterials) {
    var mat = craftMaterials[type];
    
    api.registerCraft(new api.CraftEntry(
      "default:pick_" + type,
      [mat, mat, mat, null, "default:stick", null, null, "default:stick", null],
      {shape: {x: 3, y: 3}}
    ));
    
    api.registerCraft(new api.CraftEntry(
      "default:axe_" + type,
      [mat, mat, null, mat, "default:stick", null, null, "default:stick", null],
      {shape: {x: 3, y: 3}}
    ));
    api.registerCraft(new api.CraftEntry(
      "default:axe_" + type,
      [null, mat, mat, null, "default:stick", mat, null, "default:stick", null],
      {shape: {x: 3, y: 3}}
    ));
    
    api.registerCraft(new api.CraftEntry(
      "default:shovel_" + type,
      [null, mat, null, null, "default:stick", null, null, "default:stick", null],
      {shape: {x: 3, y: 3}}
    ));
  }
})();
