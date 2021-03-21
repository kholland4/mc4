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

(function() {
  var modpath = api.getModMeta("stairs").path;
  
  mods.stairs = {};
  
  var slabVerts = [
    [
      -0.5, 0, -0.5,
      -0.5, -0.5, -0.5,
      -0.5, 0, 0.5,
      
      -0.5, -0.5, -0.5,
      -0.5, -0.5, 0.5,
      -0.5, 0, 0.5
    ],
    [
      0.5, 0, 0.5,
      0.5, -0.5, 0.5,
      0.5, 0, -0.5,
      
      0.5, -0.5, 0.5,
      0.5, -0.5, -0.5,
      0.5, 0, -0.5
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
      0.5, 0, -0.5,
      -0.5, 0, -0.5,
      0.5, 0, 0.5,
      
      -0.5, 0, -0.5,
      -0.5, 0, 0.5,
      0.5, 0, 0.5
    ],
    [
      0.5, 0, -0.5,
      0.5, -0.5, -0.5,
      -0.5, 0, -0.5,
      
      0.5, -0.5, -0.5,
      -0.5, -0.5, -0.5,
      -0.5, 0, -0.5
    ],
    [
      -0.5, 0, 0.5,
      -0.5, -0.5, 0.5,
      0.5, 0, 0.5,
      
      -0.5, -0.5, 0.5,
      0.5, -0.5, 0.5,
      0.5, 0, 0.5
    ]
  ];
  var slabUVs = [
    [
      0.0, 0.5,
      0.0, 0.0,
      1.0, 0.5,
      
      0.0, 0.0,
      1.0, 0.0,
      1.0, 0.5
    ],
    [
      0.0, 0.5,
      0.0, 0.0,
      1.0, 0.5,
      
      0.0, 0.0,
      1.0, 0.0,
      1.0, 0.5
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
      0.0, 0.5,
      0.0, 0.0,
      1.0, 0.5,
      
      0.0, 0.0,
      1.0, 0.0,
      1.0, 0.5
    ],
    [
      0.0, 0.5,
      0.0, 0.0,
      1.0, 0.5,
      
      0.0, 0.0,
      1.0, 0.0,
      1.0, 0.5
    ]
  ];
  
  var stairVerts = [
    [
      0, 0.5, -0.5,
      0, 0, -0.5,
      0, 0.5, 0.5,
      
      0, 0, -0.5,
      0, 0, 0.5,
      0, 0.5, 0.5,
      
      -0.5, 0, -0.5,
      -0.5, -0.5, -0.5,
      -0.5, 0, 0.5,
      
      -0.5, -0.5, -0.5,
      -0.5, -0.5, 0.5,
      -0.5, 0, 0.5
    ],
    [
      0.5, 0.5, 0.5,
      0.5, -0.5, 0.5,
      0.5, 0.5, -0.5,
      
      0.5, -0.5, 0.5,
      0.5, -0.5, -0.5,
      0.5, 0.5, -0.5
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
      0.5, 0.5, -0.5,
      0, 0.5, -0.5,
      0.5, 0.5, 0.5,
      
      0, 0.5, -0.5,
      0, 0.5, 0.5,
      0.5, 0.5, 0.5,
      
      0, 0, -0.5,
      -0.5, 0, -0.5,
      0, 0, 0.5,
      
      -0.5, 0, -0.5,
      -0.5, 0, 0.5,
      0, 0, 0.5
    ],
    [
      0, 0.5, -0.5,
      0.5, 0.5, -0.5,
      0, 0, -0.5,
      
      -0.5, 0, -0.5,
      0, 0, -0.5,
      -0.5, -0.5, -0.5,
      
      0.5, 0.5, -0.5,
      0.5, -0.5, -0.5,
      -0.5, -0.5, -0.5
    ],
    [
      -0.5, 0, 0.5,
      -0.5, -0.5, 0.5,
      0, 0, 0.5,
      
      0, 0.5, 0.5,
      0, 0, 0.5,
      0.5, 0.5, 0.5,
      
      -0.5, -0.5, 0.5,
      0.5, -0.5, 0.5,
      0.5, 0.5, 0.5
    ]
  ];
  var stairUVs = [
    [
      0.0, 1.0,
      0.0, 0.5,
      1.0, 1.0,
      
      0.0, 0.5,
      1.0, 0.5,
      1.0, 1.0,
      
      0.0, 0.5,
      0.0, 0.0,
      1.0, 0.5,
      
      0.0, 0.0,
      1.0, 0.0,
      1.0, 0.5
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
      0.0, 1.0,
      0.0, 0.5,
      1.0, 1.0,
      
      0.0, 0.5,
      1.0, 0.5,
      1.0, 1.0,
      
      0.0, 0.5,
      0.0, 0.0,
      1.0, 0.5,
      
      0.0, 0.0,
      1.0, 0.0,
      1.0, 0.5
    ],
    [
      0.5, 1.0,
      0.0, 1.0,
      0.5, 0.5,
      
      1.0, 0.5,
      0.5, 0.5,
      1.0, 0.0,
      
      0.0, 1.0,
      0.0, 0.0,
      1.0, 0.0
    ],
    [
      0.0, 0.5,
      0.0, 0.0,
      0.5, 0.5,
      
      0.5, 1.0,
      0.5, 0.5,
      1.0, 1.0,
      
      0.0, 0.0,
      1.0, 0.0,
      1.0, 1.0
    ]
  ];
  
  mods.stairs.registerSlab = function(itemstring) {
    var itemstringFlat = itemstring.replace(":", "_");
    
    var idef = api.getItemDef(itemstring);
    var ndef = api.getNodeDef(itemstring);
    
    var tex = {};
    var keys = ["texAll", "texTop", "texBottom", "texSides", "texFront", "texBack", "texLeft", "texRight"];
    for(var i = 0; i < keys.length; i++) {
      var key = keys[i];
      tex[key] = ndef[key];
    }
    
    //---slab---
    api.registerItem(new api.Item(itemstring + "_slab", {
      isNode: true, desc: idef.desc + " Slab",
      iconFile: modpath + "/icons/" + itemstringFlat + "_slab.png",
      inCreativeInventory: idef.inCreativeInventory
    }));
    api.registerNode(new api.Node(itemstring + "_slab", Object.assign({
        groups: ndef.groups,
        transparent: true,
        passSunlight: ndef.passSunlight,
        setRotOnPlace: true,
        limitRotOnPlace: true,
        customMesh: true,
        customMeshVerts: slabVerts,
        customMeshUVs: slabUVs,
        boundingBox: [
          new THREE.Box3(new THREE.Vector3(-0.5, -0.5, -0.5), new THREE.Vector3(0.5, 0, 0.5))
        ],
        faceIsRecessed: [false, false, false, true, false, false]
      }, tex)
    )); //api.registerNode
    
    api.registerCraft(new api.CraftEntry(itemstring + "_slab" + " 6",
      [
        itemstring, itemstring, itemstring
      ], {shape: {x: 3, y: 1}}));
    
    
    //---stair---
    api.registerItem(new api.Item(itemstring + "_stairs", {
      isNode: true, desc: idef.desc + " Stairs",
      iconFile: modpath + "/icons/" + itemstringFlat + "_stairs.png",
      inCreativeInventory: idef.inCreativeInventory
    }));
    api.registerNode(new api.Node(itemstring + "_stairs", Object.assign({
        groups: ndef.groups,
        transparent: true,
        passSunlight: ndef.passSunlight,
        setRotOnPlace: true,
        customMesh: true,
        customMeshVerts: stairVerts,
        customMeshUVs: stairUVs,
        boundingBox: [
          new THREE.Box3(new THREE.Vector3(-0.5, -0.5, -0.5), new THREE.Vector3(0.5, 0, 0.5)),
          new THREE.Box3(new THREE.Vector3(0, 0, -0.5), new THREE.Vector3(0.5, 0.5, 0.5))
        ]
        //faceIsRecessed: [false, false, false, false, false, false]
      }, tex)
    )); //api.registerNode
    
    api.registerCraft(new api.CraftEntry(itemstring + "_stairs 8",
      [
        itemstring, null, null,
        itemstring, itemstring, null,
        itemstring, itemstring, itemstring
      ], {shape: {x: 3, y: 3}}));
  };
  
  mods.stairs.registerNode = function(itemstring) {
    mods.stairs.registerSlab(itemstring);
  };
  
  api.onModLoaded("default", function() {
    var slabs_stairs_list = [
      "stone", "cobble", "stone_block", "stone_brick",
      "desert_stone", "desert_cobble", "desert_stone_block", "desert_stone_brick",
      "sandstone", "sandstone_block", "sandstone_brick",
      "desert_sandstone", "desert_sandstone_block", "desert_sandstone_brick",
      "silver_sandstone", "silver_sandstone_block", "silver_sandstone_brick",
      "snow",
      "wood", "acacia_wood", "aspen_wood", "jungle_wood", "pine_wood",
      "obsidian", "obsidian_block", "obsidian_brick",
      "coal_block", "iron_block", "gold_block", "diamond_block",
      "glass",
      "brick"
    ];
    
    for(var i = 0; i < slabs_stairs_list.length; i++) {
      var itemstring = "default:" + slabs_stairs_list[i];
      mods.stairs.registerNode(itemstring);
    }
  }); //api.onModLoaded
})();
