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
  var modpath = api.getModMeta("flowers").path;
  
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
  
  var xmeshVerts = [
    [
      -0.5, 0.5, -0.5,
      -0.5, -0.5, -0.5,
      0.5, 0.5, 0.5,
      
      -0.5, -0.5, -0.5,
      0.5, -0.5, 0.5,
      0.5, 0.5, 0.5
    ],
    [
      0.5, 0.5, 0.5,
      0.5, -0.5, 0.5,
      -0.5, 0.5, -0.5,
      
      0.5, -0.5, 0.5,
      -0.5, -0.5, -0.5,
      -0.5, 0.5, -0.5
    ],
    [], [],
    [
      0.5, 0.5, -0.5,
      0.5, -0.5, -0.5,
      -0.5, 0.5, 0.5,
      
      0.5, -0.5, -0.5,
      -0.5, -0.5, 0.5,
      -0.5, 0.5, 0.5
    ],
    [
      -0.5, 0.5, 0.5,
      -0.5, -0.5, 0.5,
      0.5, 0.5, -0.5,
      
      -0.5, -0.5, 0.5,
      0.5, -0.5, -0.5,
      0.5, 0.5, -0.5
    ]
  ];
  var xmeshUVs = [
    [
      0.0, 1.0,
      0.0, 0.0,
      1.0, 1.0,
      
      0.0, 0.0,
      1.0, 0.0,
      1.0, 1.0
    ],
    [
      //back side is mirrored
      1.0, 1.0,
      1.0, 0.0,
      0.0, 1.0,
      
      1.0, 0.0,
      0.0, 0.0,
      0.0, 1.0
    ],
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
      //back side is mirrored
      1.0, 1.0,
      1.0, 0.0,
      0.0, 1.0,
      
      1.0, 0.0,
      0.0, 0.0,
      0.0, 1.0
    ]
  ];
  
  
  
  for(var i = 1; i <= 5; i++) {
    registerNodeHelper("flowers:grass_" + i, {
      desc: "Grass",
      tex: {texAll: "default_grass_" + i + ".png"},
      icon: "default_grass_" + i + ".png",
      node: {groups: {oddly_breakable_by_hand: 1}, transparent: true, passSunlight: false, useTint: false, walkable: true, customMesh: true,
        customMeshVerts: xmeshVerts,
        customMeshUVs: xmeshUVs,
        drops: "flowers:grass_3",
        canPlaceInside: true
      }
    });
  }
  
  var flowerList = [
    {name: "Green Chrysanthemum", node: "chrysanthemum_green"},
    {name: "White Dandelion", node: "dandelion_white"},
    {name: "Yellow Dandelion", node: "dandelion_yellow"},
    {name: "Blue Geranium", node: "geranium"},
    {name: "Brown Mushroom", node: "mushroom_brown"},
    {name: "Red Mushroom", node: "mushroom_red"},
    {name: "Rose", node: "rose"},
    {name: "Orange Tulip", node: "tulip"},
    {name: "Black Tulip", node: "tulip_black"},
    {name: "Viola", node: "viola"}
  ];
  
  for(var i = 0; i < flowerList.length; i++) {
    var name = flowerList[i].name;
    var node = flowerList[i].node;
    
    registerNodeHelper("flowers:" + node, {
      desc: name,
      tex: {texAll: "flowers_" + node + ".png"},
      icon: "flowers_" + node + ".png",
      node: {groups: {oddly_breakable_by_hand: 1}, transparent: true, passSunlight: false, useTint: false, walkable: true, customMesh: true,
        customMeshVerts: xmeshVerts,
        customMeshUVs: xmeshUVs,
        canPlaceInside: true
      }
    });
  }
})();
