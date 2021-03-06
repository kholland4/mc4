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
  
  api.onModLoaded("default", function() {
    
    /*function registerNodeHelper(itemstring, args) {
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
    }*/
    
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
      var name = slabs_stairs_list[i];
      
      var tex = {texAll: "default:" + name + ":texAll"};
      
      api.registerItem(new api.Item("stairs:slab_" + name, {
        isNode: true, desc: api.getItemDef("default:" + name).desc + " Slab",
        iconFile: modpath + "/icons/stairs_slab_" + name + ".png"
      }));
      api.registerNode(new api.Node("stairs:slab_" + name, Object.assign({
        groups: api.getNodeDef("default:" + name).groups,
        transparent: true,
        passSunlight: api.getNodeDef("default:" + name).passSunlight,
        customMesh: true,
          customMeshVerts: [
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
          ],
          customMeshUVs: [
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
          ],
          boundingBox: [
            new THREE.Box3(new THREE.Vector3(-0.5, -0.5, -0.5), new THREE.Vector3(0.5, 0, 0.5))
          ]
        }, tex)
      )); //api.registerNode
      
      api.registerCraft(new api.CraftEntry("stairs:slab_" + name + " 6",
        [
          "default:" + name, "default:" + name, "default:" + name
        ], {shape: {x: 3, y: 1}}));
    } //for loop
  }); //api.onModLoaded
})();
