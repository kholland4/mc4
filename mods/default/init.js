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

(function() {
  var modpath = api.getModMeta("default").path;
  
  mods.default = {};
  
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
  function itemstringGuessName(str) {
    if(str.indexOf(":") != -1) {
      str = str.substring(str.indexOf(":") + 1);
    }
    var words = str.split("_");
    var out = [];
    for(var i = 0; i < words.length; i++) {
      out.push(words[i].charAt(0).toUpperCase() + words[i].slice(1));
    }
    
    return out.join(" ");
  }
  
  //---DIRT---
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
  
  
  
  //---STONE---
  [
    ["stone", "cobble", "stone_block", "stone_brick"],
    ["desert_stone", "desert_cobble", "desert_stone_block", "desert_stone_brick"]
  ].forEach(function(names) {
    registerNodeHelper("default:" + names[0], {
      desc: itemstringGuessName(names[0]),
      tex: {texAll: "default_" + names[0] + ".png"},
      icon: "default_" + names[0] + ".png",
      node: {groups: {cracky: 1}, drops: "default:" + names[1]},
      item: {groups: {stone: null}}
    });
    registerNodeHelper("default:" + names[1], {
      desc: itemstringGuessName(names[1]) + "stone",
      tex: {texAll: "default_" + names[1] + ".png"},
      icon: "default_" + names[1] + ".png",
      node: {groups: {cracky: 1}},
      item: {groups: {stone: null}}
    });
    api.registerCraft(new api.CraftEntry("default:" + names[0], ["default:" + names[1]], {shape: null, type: "cook", cookTime: 4}));
    registerNodeHelper("default:" + names[2], {
      desc: itemstringGuessName(names[2]),
      tex: {texAll: "default_" + names[2] + ".png"},
      icon: "default_" + names[2] + ".png",
      node: {groups: {cracky: 2}}
    });
    api.registerCraft(new api.CraftEntry("default:" + names[2] + " 9", Array(9).fill("default:" + names[0]), {shape: {x: 3, y: 3}}));
    registerNodeHelper("default:" + names[3], {
      desc: itemstringGuessName(names[3]),
      tex: {texAll: "default_" + names[3] + ".png"},
      icon: "default_" + names[3] + ".png",
      node: {groups: {cracky: 2}}
    });
    api.registerCraft(new api.CraftEntry("default:" + names[3] + " 4", Array(4).fill("default:" + names[0]), {shape: {x: 2, y: 2}}));
  });
  
  
  
  //---TREES---
  [
    ["wood", "tree", "leaves"],
    ["acacia_wood", "acacia_tree", "acacia_leaves"],
    ["aspen_wood", "aspen_tree", "aspen_leaves"],
    ["jungle_wood", "jungle_tree", "jungle_leaves"],
    ["pine_wood", "pine_tree", "pine_needles"]
  ].forEach(function(names) {
    registerNodeHelper("default:" + names[0], {
      desc: itemstringGuessName(names[0]) + " Planks",
      tex: {texAll: "default_" + names[0] + ".png"},
      icon: "default_" + names[0] + ".png",
      node: {groups: {choppy: 1}},
      item: {groups: {wood: null}}
    });
    
    registerNodeHelper("default:" + names[1], {
      desc: itemstringGuessName(names[1]),
      tex: {texTop: "default_" + names[1] + "_top.png", texBottom: "default_" + names[1] + "_top.png", texSides: "default_" + names[1] + ".png"},
      icon: "default_" + names[1] + ".png",
      node: {groups: {choppy: 2}, setRotOnPlace: true, limitRotOnPlace: true},
      item: {groups: {tree: null}}
    });
    api.registerCraft(new api.CraftEntry("default:" + names[0] + " 4", ["default:" + names[1]], {shape: null}));
    
    registerNodeHelper("default:" + names[2], {
      desc: itemstringGuessName(names[2]),
      tex: {texAll: "default_" + names[2] + ".png"},
      icon: "default_" + names[2] + ".png",
      node: {groups: {snappy: 1}, transparent: true, passSunlight: false},
      item: {groups: {leaves: null}}
    });
  });
  
  
  
  //---ORES---
  registerNodeHelper("default:ore_coal", {
    desc: "Coal Ore",
    tex: {texAll: "default_ore_coal.png"},
    icon: "default_ore_coal.png",
    node: {groups: {cracky: 2}, drops: "default:lump_coal"}
  });
  api.registerItem(new api.Item("default:lump_coal", {desc: "Coal Lump", iconFile: modpath + "/icons/default_coal_lump.png"}));
  registerNodeHelper("default:coal_block", {
    desc: "Coal Block",
    tex: {texAll: "default_coal_block.png"},
    icon: "default_coal_block.png",
    node: {groups: {cracky: 2}}
  });
  api.registerCraft(new api.CraftEntry("default:coal_block", Array(9).fill("default:lump_coal"), {shape: {x: 3, y: 3}}));
  api.registerCraft(new api.CraftEntry("default:lump_coal 9", ["default:coal_block"], {shape: null}));
  
  var minerals = ["iron", "gold", "tin", "copper"];
  for(var i = 0; i < minerals.length; i++) {
    var m = minerals[i];
    var mCaps = itemstringGuessName(m);
    
    registerNodeHelper("default:ore_" + m, {
      desc: mCaps + " Ore",
      tex: {texAll: "default_ore_" + m + ".png"},
      icon: "default_ore_" + m + ".png",
      node: {groups: {cracky: 2}, drops: "default:lump_" + m}
    });
    api.registerItem(new api.Item("default:lump_" + m, {desc: mCaps + " Lump", iconFile: modpath + "/icons/default_" + m + "_lump.png"}));
    api.registerItem(new api.Item("default:ingot_" + m, {desc: mCaps + " Ingot", iconFile: modpath + "/icons/default_" + m + "_ingot.png"}));
    api.registerCraft(new api.CraftEntry("default:ingot_" + m, ["default:lump_" + m], {shape: null, type: "cook", cookTime: 4}));
    registerNodeHelper("default:" + m + "_block", {
      desc: mCaps + " Block",
      tex: {texAll: "default_" + m + "_block.png"},
      icon: "default_" + m + "_block.png",
      node: {groups: {cracky: 3}}
    });
    api.registerCraft(new api.CraftEntry("default:" + m + "_block", Array(9).fill("default:ingot_" + m), {shape: {x: 3, y: 3}}));
    api.registerCraft(new api.CraftEntry("default:ingot_" + m + " 9", ["default:" + m + "_block"], {shape: null}));
  }
  
  registerNodeHelper("default:ore_diamond", {
    desc: "Diamond Ore",
    tex: {texAll: "default_ore_diamond.png"},
    icon: "default_ore_diamond.png",
    node: {groups: {cracky: 3}, drops: "default:diamond"}
  });
  api.registerItem(new api.Item("default:diamond", {desc: "Diamond", iconFile: modpath + "/icons/default_diamond.png"}));
  registerNodeHelper("default:diamond_block", {
    desc: "Diamond Block",
    tex: {texAll: "default_diamond_block.png"},
    icon: "default_diamond_block.png",
    node: {groups: {cracky: 3}}
  });
  api.registerCraft(new api.CraftEntry("default:diamond_block", Array(9).fill("default:diamond"), {shape: {x: 3, y: 3}}));
  api.registerCraft(new api.CraftEntry("default:diamond 9", ["default:diamond_block"], {shape: null}));
  
  //TODO bronze crafts/alloying
  api.registerItem(new api.Item("default:ingot_bronze", {desc: "Bronze Ingot", iconFile: modpath + "/icons/default_bronze_ingot.png"}));
  registerNodeHelper("default:bronze_block", {
    desc: "Bronze Block",
    tex: {texAll: "default_bronze_block.png"},
    icon: "default_bronze_block.png",
    node: {groups: {cracky: 3}}
  });
  api.registerCraft(new api.CraftEntry("default:bronze_block", Array(9).fill("default:ingot_bronze"), {shape: {x: 3, y: 3}}));
  api.registerCraft(new api.CraftEntry("default:ingot_bronze 9", ["default:bronze_block"], {shape: null}));
  
  
  
  //---ITEMS---
  api.registerItem(new api.Item("default:stick", {desc: "Stick", iconFile: modpath + "/icons/default_stick.png"}));
  api.registerCraft(new api.CraftEntry(
    "default:stick 4",
    ["group:wood"],
    {shape: null}
  ));
  
  
  
  //---TOOLS---
  //hand tool
  api.registerItem(new Item(":", {isTool: true, toolWear: null, toolGroups: {
    crumbly: {times: [0, 1.0, 2.0], maxlevel: 2},
    //cracky: {times: [0, 2], maxlevel: 1},
    choppy: {times: [0, 2, 4], maxlevel: 2},
    snappy: {times: [0, 1.0], maxlevel: 1},
    oddly_breakable_by_hand: {times: [0, 0.2, 0.5], maxlevel: 2}
  }, inCreativeInventory: false}));
  
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
    "steel": "default:ingot_iron",
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
  
  
  
  
  
  //---TORCH---
  api.registerItem(new api.Item("default:torch", {
    isNode: true, desc: "Torch",
    iconFile: modpath + "/icons/default_torch_on_floor.png"
  }));
  api.loadTexture("default:torch:texAll", modpath + "/textures/default_torch.png");
  api.registerNode(new api.Node("default:torch", {
    texAll: "default:torch:texAll",
    groups: {oddly_breakable_by_hand: 1}, transparent: true, walkable: true, customMesh: true, lightLevel: 14,
      customMeshVerts: [
        [
          -1/16, 3/16, -1/16,
          -1/16, -0.5, -1/16,
          -1/16, 3/16, 1/16,
          
          -1/16, -0.5, -1/16,
          -1/16, -0.5, 1/16,
          -1/16, 3/16, 1/16
        ],
        [
          1/16, 3/16, 1/16,
          1/16, -0.5, 1/16,
          1/16, 3/16, -1/16,
          
          1/16, -0.5, 1/16,
          1/16, -0.5, -1/16,
          1/16, 3/16, -1/16
        ],
        [
          1/16, -0.5, 1/16,
          -1/16, -0.5, 1/16,
          1/16, -0.5, -1/16,
          
          -1/16, -0.5, 1/16,
          -1/16, -0.5, -1/16,
          1/16, -0.5, -1/16
        ],
        [
          1/16, 3/16, -1/16,
          -1/16, 3/16, -1/16,
          1/16, 3/16, 1/16,
          
          -1/16, 3/16, -1/16,
          -1/16, 3/16, 1/16,
          1/16, 3/16, 1/16
        ],
        [
          1/16, 3/16, -1/16,
          1/16, -0.5, -1/16,
          -1/16, 3/16, -1/16,
          
          1/16, -0.5, -1/16,
          -1/16, -0.5, -1/16,
          -1/16, 3/16, -1/16
        ],
        [
          -1/16, 3/16, 1/16,
          -1/16, -0.5, 1/16,
          1/16, 3/16, 1/16,
          
          -1/16, -0.5, 1/16,
          1/16, -0.5, 1/16,
          1/16, 3/16, 1/16
        ]
      ],
      customMeshUVs: [
        [
          0/16, 13/16,
          0/16, 2/16,
          2/16, 13/16,
          
          0/16, 2/16,
          2/16, 2/16,
          2/16, 13/16
        ],
        [
          4/16, 13/16,
          4/16, 2/16,
          6/16, 13/16,
          
          4/16, 2/16,
          6/16, 2/16,
          6/16, 13/16
        ],
        [
          6/16, 2/16,
          6/16, 0/16,
          8/16, 2/16,
          
          6/16, 0/16,
          8/16, 0/16,
          8/16, 2/16
        ],
        [
          6/16, 15/16,
          6/16, 13/16,
          8/16, 15/16,
          
          6/16, 13/16,
          8/16, 13/16,
          8/16, 15/16
        ],
        [
          6/16, 13/16,
          6/16, 2/16,
          8/16, 13/16,
          
          6/16, 2/16,
          8/16, 2/16,
          8/16, 13/16
        ],
        [
          2/16, 13/16,
          2/16, 2/16,
          4/16, 13/16,
          
          2/16, 2/16,
          4/16, 2/16,
          4/16, 13/16
        ]
      ],
      boundingBox: [
        new THREE.Box3(new THREE.Vector3(-1/16, -0.5, -1/16), new THREE.Vector3(1/16, 3/16, 1/16))
      ]
    }
  ));
  
  
  
  //---WATER---
  registerNodeHelper("default:water_source", {
    desc: "Water Source",
    tex: {texAll: "default_water_source.png"},
    icon: "default_water_source.png",
    node: {transparent: true, reallyTransparent: true, passSunlight: false, walkable: true, isFluid: true, fluidOverlayColor: "rgba(64, 64, 255, 0.3)", joined: true, canPlaceInside: true}
  });
  
  
  //---sand, sandstone, sandstone block, sandstone brick (+ same for silver sand)---
  ["sand", "silver_sand", "desert_sand"].forEach(nodeName => registerNodeHelper("default:" + nodeName, {
    desc: itemstringGuessName(nodeName),
    tex: {texAll: "default_" + nodeName + ".png"},
    icon: "default_" + nodeName + ".png",
    node: {groups: {crumbly: 1}},
    item: {groups: {sand: null}}
  }));
  
  var sandBlocks1 = [
    "sandstone", "silver_sandstone", "desert_sandstone"
  ];
  sandBlocks1.forEach(nodeName => registerNodeHelper("default:" + nodeName, {
    desc: itemstringGuessName(nodeName),
    tex: {texAll: "default_" + nodeName + ".png"},
    icon: "default_" + nodeName + ".png",
    node: {groups: {crumbly: 3, cracky: 1}},
    item: {groups: {sandstone: null}}
  }));
  var sandBlocks2 = [
    "sandstone_block", "sandstone_brick",
    "silver_sandstone_block", "silver_sandstone_brick",
    "desert_sandstone_block", "desert_sandstone_brick"
  ];
  sandBlocks2.forEach(nodeName => registerNodeHelper("default:" + nodeName, {
    desc: itemstringGuessName(nodeName),
    tex: {texAll: "default_" + nodeName + ".png"},
    icon: "default_" + nodeName + ".png",
    node: {groups: {crumbly: 3, cracky: 1}}
  }));
  api.registerCraft(new api.CraftEntry("default:sandstone", Array(4).fill("default:sand"), {shape: {x: 2, y: 2}}));
  api.registerCraft(new api.CraftEntry("default:sand 4", ["default:sandstone"], {shape: null}));
  api.registerCraft(new api.CraftEntry("default:sandstone_block 9", Array(9).fill("default:sandstone"), {shape: {x: 3, y: 3}}));
  api.registerCraft(new api.CraftEntry("default:sandstone_brick 4", Array(4).fill("default:sandstone"), {shape: {x: 2, y: 2}}));
  api.registerCraft(new api.CraftEntry("default:silver_sandstone", Array(4).fill("default:silver_sand"), {shape: {x: 2, y: 2}}));
  api.registerCraft(new api.CraftEntry("default:silver_sand 4", ["default:silver_sandstone"], {shape: null}));
  api.registerCraft(new api.CraftEntry("default:silver_sandstone_block 9", Array(9).fill("default:silver_sandstone"), {shape: {x: 3, y: 3}}));
  api.registerCraft(new api.CraftEntry("default:silver_sandstone_brick 4", Array(4).fill("default:silver_sandstone"), {shape: {x: 2, y: 2}}));
  api.registerCraft(new api.CraftEntry("default:desert_sandstone", Array(4).fill("default:desert_sand"), {shape: {x: 2, y: 2}}));
  api.registerCraft(new api.CraftEntry("default:desert_sand 4", ["default:desert_sandstone"], {shape: null}));
  api.registerCraft(new api.CraftEntry("default:desert_sandstone_block 9", Array(9).fill("default:desert_sandstone"), {shape: {x: 3, y: 3}}));
  api.registerCraft(new api.CraftEntry("default:desert_sandstone_brick 4", Array(4).fill("default:desert_sandstone"), {shape: {x: 2, y: 2}}));
  
  //---glass---
  registerNodeHelper("default:glass", {
    desc: "Glass",
    tex: {texAll: "default_glass.png"},
    icon: "default_glass.png",
    node: {groups: {cracky: 1}, transparent: true, reallyTransparent: true, joined: true}
  });
  api.registerCraft(new api.CraftEntry("default:glass", ["group:sand"], {shape: null, type: "cook", cookTime: 3}));
  
  //---gravel---
  registerNodeHelper("default:gravel", {
    desc: "Gravel",
    tex: {texAll: "default_gravel.png"},
    icon: "default_gravel.png",
    node: {groups: {crumbly: 2}}
  });
  
  
  
  //---clay and bricks---
  registerNodeHelper("default:clay", {
    desc: "Clay",
    tex: {texAll: "default_clay.png"},
    icon: "default_clay.png",
    node: {groups: {crumbly: 2}}
  });
  api.registerItem(new api.Item("default:clay_lump", {desc: "Clay Lump", iconFile: modpath + "/icons/default_clay_lump.png"}));
  api.registerCraft(new api.CraftEntry("default:clay", Array(4).fill("default:clay_lump"), {shape: {x: 2, y: 2}}));
  api.registerCraft(new api.CraftEntry("default:clay_lump 4", ["default:clay"], {shape: null}));
  
  registerNodeHelper("default:brick", {
    desc: "Brick Block",
    tex: {texAll: "default_brick.png"},
    icon: "default_brick.png",
    node: {groups: {cracky: 2}}
  });
  api.registerItem(new api.Item("default:clay_brick", {desc: "Clay Brick", iconFile: modpath + "/icons/default_clay_brick.png"}));
  api.registerCraft(new api.CraftEntry("default:brick", Array(4).fill("default:clay_brick"), {shape: {x: 2, y: 2}}));
  api.registerCraft(new api.CraftEntry("default:clay_brick 4", ["default:brick"], {shape: null}));
  
  api.registerCraft(new api.CraftEntry("default:clay_brick", ["default:clay_lump"], {shape: null, type: "cook", cookTime: 4}));
  
  
  //---obsidian---
  registerNodeHelper("default:obsidian", {
    desc: "Obsidian",
    tex: {texAll: "default_obsidian.png"},
    icon: "default_obsidian.png",
    node: {groups: {cracky: 3}}
  });
  registerNodeHelper("default:obsidian_block", {
    desc: "Obsidian Block",
    tex: {texAll: "default_obsidian_block.png"},
    icon: "default_obsidian_block.png",
    node: {groups: {cracky: 3}}
  });
  registerNodeHelper("default:obsidian_brick", {
    desc: "Obsidian Brick",
    tex: {texAll: "default_obsidian_brick.png"},
    icon: "default_obsidian_brick.png",
    node: {groups: {cracky: 3}}
  });
  api.registerCraft(new api.CraftEntry("default:obsidian_block 9", Array(9).fill("default:obsidian"), {shape: {x: 3, y: 3}}));
  api.registerCraft(new api.CraftEntry("default:obsidian_brick 4", Array(4).fill("default:obsidian"), {shape: {x: 2, y: 2}}));
  
  
  //---snow---
  registerNodeHelper("default:snow", {
    desc: "Snow",
    tex: {texAll: "default_snow.png"},
    icon: "default_snowball.png",
    node: {groups: {crumbly: 1}, transparent: true, passSunlight: false, customMesh: true,
      customMeshVerts: [
        [
          -0.5, -0.25, -0.5,
          -0.5, -0.5, -0.5,
          -0.5, -0.25, 0.5,
          
          -0.5, -0.5, -0.5,
          -0.5, -0.5, 0.5,
          -0.5, -0.25, 0.5
        ],
        [
          0.5, -0.25, 0.5,
          0.5, -0.5, 0.5,
          0.5, -0.25, -0.5,
          
          0.5, -0.5, 0.5,
          0.5, -0.5, -0.5,
          0.5, -0.25, -0.5
        ],
        [
          0.5, -0.5, 0.5,
          -0.5, -0.5, 0.5,
          0.5, -0.5, -0.5,
          
          -0.5, -0.5, 0.5,
          -0.5, -0.5, -0.5,
          0.5, -0.5, -0.5
        ],
        [
          0.5, -0.25, -0.5,
          -0.5, -0.25, -0.5,
          0.5, -0.25, 0.5,
          
          -0.5, -0.25, -0.5,
          -0.5, -0.25, 0.5,
          0.5, -0.25, 0.5
        ],
        [
          0.5, -0.25, -0.5,
          0.5, -0.5, -0.5,
          -0.5, -0.25, -0.5,
          
          0.5, -0.5, -0.5,
          -0.5, -0.5, -0.5,
          -0.5, -0.25, -0.5
        ],
        [
          -0.5, -0.25, 0.5,
          -0.5, -0.5, 0.5,
          0.5, -0.25, 0.5,
          
          -0.5, -0.5, 0.5,
          0.5, -0.5, 0.5,
          0.5, -0.25, 0.5
        ]
      ],
      customMeshUVs: [
        [
          0.0, 0.25,
          0.0, 0.0,
          1.0, 0.25,
          
          0.0, 0.0,
          1.0, 0.0,
          1.0, 0.25
        ],
        [
          0.0, 0.25,
          0.0, 0.0,
          1.0, 0.25,
          
          0.0, 0.0,
          1.0, 0.0,
          1.0, 0.25
        ],
        [
          0.0, 1.0,
          0.0, 0.0,
          1.0, 1.0,
          
          0.0, 0.0,
          1.0, 0.0,
          1.0, 1.0
        ],
        [
          0.0, 1.0,
          0.0, 0.0,
          1.0, 1.0,
          
          0.0, 0.0,
          1.0, 0.0,
          1.0, 1.0
        ],
        [
          0.0, 0.25,
          0.0, 0.0,
          1.0, 0.25,
          
          0.0, 0.0,
          1.0, 0.0,
          1.0, 0.25
        ],
        [
          0.0, 0.25,
          0.0, 0.0,
          1.0, 0.25,
          
          0.0, 0.0,
          1.0, 0.0,
          1.0, 0.25
        ]
      ],
      boundingBox: [
        new THREE.Box3(new THREE.Vector3(-0.5, -0.5, -0.5), new THREE.Vector3(0.5, -0.375, 0.5))
      ],
      faceIsRecessed: [false, false, false, true, false, false],
      canPlaceInside: true
    }
  });
  registerNodeHelper("default:snowblock", {
    desc: "Snow Block",
    tex: {texAll: "default_snow.png"},
    icon: "default_snowblock.png",
    node: {groups: {crumbly: 1}}
  });
  api.registerCraft(new api.CraftEntry("default:snowblock", Array(9).fill("default:snow"), {shape: {x: 3, y: 3}}));
  api.registerCraft(new api.CraftEntry("default:snow 9", ["default:snowblock"], {shape: null}));
  
  
  //---Ladder---
  registerNodeHelper("default:ladder_wood", {
    desc: "Wooden Ladder",
    tex: {texAll: "default_ladder_wood.png"},
    icon: "default_ladder_wood.png",
    node: {groups: {choppy: 1}, transparent: true, passSunlight: true, walkable: true, ladderlike: true, setRotOnPlace: true, customMesh: true,
      customMeshVerts: [
        [], [],
        [
          0.5, -0.376, 0.5,
          -0.5, -0.376, 0.5,
          0.5, -0.376, -0.5,
          
          -0.5, -0.376, 0.5,
          -0.5, -0.376, -0.5,
          0.5, -0.376, -0.5
        ],
        [
          0.5, -0.375, -0.5,
          -0.5, -0.375, -0.5,
          0.5, -0.375, 0.5,
          
          -0.5,- 0.375, -0.5,
          -0.5, -0.375, 0.5,
          0.5, -0.375, 0.5
        ], [], []
      ],
      customMeshUVs: [
        [], [],
        [
          0.0, 1.0,
          0.0, 0.0,
          1.0, 1.0,
          
          0.0, 0.0,
          1.0, 0.0,
          1.0, 1.0
        ],
        [
          0.0, 1.0,
          0.0, 0.0,
          1.0, 1.0,
          
          0.0, 0.0,
          1.0, 0.0,
          1.0, 1.0
        ], [], []
      ],
      faceIsRecessed: [null, null, null, true, null, null]}
  });
  api.registerCraft(new api.CraftEntry("default:ladder_wood 4",
    [
      "default:stick", null, "default:stick",
      "default:stick", "default:stick", "default:stick",
      "default:stick", null, "default:stick"
    ], {shape: {x: 3, y: 3}}));
  
  
  //---Various white/grey bricks---
  registerNodeHelper("default:brick_white", {
    desc: "White Bricks",
    tex: {texAll: "unifiedbricks_brickblock.png"},
    icon: "unifiedbricks_brickblock.png",
    node: {groups: {cracky: 2}}
  });
  registerNodeHelper("default:brick_light", {
    desc: "Light Bricks",
    tex: {texAll: "unifiedbricks_brickblock_multicolor_light.png"},
    icon: "unifiedbricks_brickblock_multicolor_light.png",
    node: {groups: {cracky: 2}}
  });
  registerNodeHelper("default:brick_medium", {
    desc: "Medium Bricks",
    tex: {texAll: "unifiedbricks_brickblock_multicolor_medium.png"},
    icon: "unifiedbricks_brickblock_multicolor_medium.png",
    node: {groups: {cracky: 2}}
  });
  registerNodeHelper("default:brick_dark", {
    desc: "Dark Bricks",
    tex: {texAll: "unifiedbricks_brickblock_multicolor_dark.png"},
    icon: "unifiedbricks_brickblock_multicolor_dark.png",
    node: {groups: {cracky: 2}}
  });
  
  
  //---TIME---
  mods.default.timeOffset = 1440 / 2;
  mods.default.timeOfDay = function() {
    //var time = api.debugTimestamp() + mods.default.timeOffset;
    var time = mods.default.timeOffset; //FIXME temporarily disabled due to performance issues
    return {d: Math.floor(time / 1440), h: Math.floor((time % 1440) / 60), m: Math.floor(time % 60)};
  };
  mods.default.setTimeOfDay = function(h, m) {
    var currentTime = mods.default.timeOfDay();
    
    mods.default.timeOffset += (h - currentTime.h) * 60 + (m - currentTime.m);
    mods.default.updateSun();
  };
  
  mods.default._oldTime = 0;
  mods.default.updateSun = function() {
    var tRaw = mods.default.timeOfDay();
    var t = (tRaw.h * 60 + tRaw.m) / 1440;
    
    if(t != mods.default._oldTime) {
      var sun = Math.min(Math.max(-Math.cos(6.283 * t) + 0.5, 0), 1);
      api.setSun(sun);
    }
    mods.default._oldTime = t;
  };
  
  mods.default._sunUpdateCount = 0;
  api.registerOnFrame(function(tscale) {
    mods.default._sunUpdateCount++;
    if(mods.default._sunUpdateCount > 100) {
      mods.default.updateSun();
      
      mods.default._sunUpdateCount = 0;
    }
  });
  
  if(!api.server.isRemote()) {
    api.onModLoaded("chat", function() {
      mods.chat.registerCommand(new mods.chat.ChatCommand("/time", function(args) {
        if(args.length == 1) {
          var time = mods.default.timeOfDay();
          return "time of day is " + time.h + ":" + time.m.toString().padStart(2, "0");
        } else {
          var str = args[1];
          var c = str.split(":");
          if(c.length < 2) { return "invalid time '" + str + "'"; }
          var h = parseInt(c[0]);
          var m = parseInt(c[1]);
          if(isNaN(h) || isNaN(m)) { return "invalid time '" + str + "'"; }
          if(!(h >= 0 && h < 24 && m >= 0 && m < 60)) { return "invalid time '" + str + "'"; }
          
          mods.default.setTimeOfDay(h, m);
          
          var time = mods.default.timeOfDay();
          return "time of day set to " + time.h + ":" + time.m.toString().padStart(2, "0");
        }
      }, "/time [<hh>:<mm>] : get or set the time of day"));
    });
  }
  api.server.registerMessageHook("set_time", function(data) {
    mods.default.setTimeOfDay(data["hours"], data["minutes"]);
  });
  
  
  //---Mese lamp---
  registerNodeHelper("default:meselamp", {
    desc: "Lamp",
    tex: {texAll: "default_meselamp.png"},
    icon: "default_meselamp.png",
    node: {groups: {cracky: 1}, transparent: true, lightLevel: 15}
  });
  
  
  //---Chest---
  registerNodeHelper("default:chest", {
    desc: "Chest",
    tex: {
      texTop: "default_chest_top.png",
      texBottom: "default_chest_top.png",
      texSides: "default_chest_side.png",
      texFront: "default_chest_front.png",
    },
    icon: "default_chest.png",
    node: {
      groups: {choppy: 1},
      allowInteract: true,
      setRotOnPlace: true
    }
  });
  api.registerCraft(new api.CraftEntry("default:chest", [
    "group:wood", "group:wood", "group:wood",
    "group:wood", null, "group:wood",
    "group:wood", "group:wood", "group:wood"
  ], {shape: {x: 3, y: 3}}));
  
  
  //---EXTRA KEYS---
  registerConfig("keybind_fly", "KeyK");
  registerConfig("keybind_fast", "KeyJ");
  registerConfig("keybind_sprint", "AltLeft");
  //FIXME
  mods.default._fast = false;
  mods.default._sprint = false;
  api.registerKey(function(key) {
    if(!api.ingameKey()) { return; }
    
    var keybind = mapKey(key);
    
    if(keybind.includes("keybind_fly")) {
      if(api.player.fly) {
        api.player.fly = false;
      } else {
        if(api.player.privs.includes("fly")) {
          api.player.fly = true;
        } else {
          mods.chat.print("no fly priv");
        }
      }
    }
    if(keybind.includes("keybind_fast")) {
      if(mods.default._fast) {
        api.player.controls.speed = mods.default._sprint ? 8 : 4;
        mods.default._fast = false;
      } else {
        if(api.player.privs.includes("fast")) {
          api.player.controls.speed = mods.default._sprint ? 16 : 12;
          mods.default._fast = true;
        } else {
          mods.chat.print("no fast priv");
        }
      }
    }
    
    if(keybind.includes("keybind_sprint")) { //FIXME - ease in/out
      api.player.controls.speed = mods.default._fast ? 16 : 8;
      mods.default._sprint = true;
    }
  });
  api.registerKeyUp(function(key) {
    if(!api.ingameKey()) { return; }
    var keybind = mapKey(key);
    
    if(keybind.includes("keybind_sprint") && mods.default._sprint) {
      api.player.controls.speed = mods.default._fast ? 12 : 4;
      mods.default._sprint = false;
    }
  });
})();
