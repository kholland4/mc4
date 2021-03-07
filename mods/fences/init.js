/*
    mc4, a web voxel building game
    Copyright (C) 2021 kholland4

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
  var modpath = api.getModMeta("fences").path;
  
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
  
  function combine(arr1, arr2) {
    var res = [];
    res.push.apply(res, arr1);
    res.push.apply(res, arr2);
    return res;
  }
  function transform(arr, x, y, z) {
    var res = arr.slice();
    for(var i = 0; i < res.length; i += 3) {
      res[i] += x;
      res[i + 1] += y;
      res[i + 2] += z;
    }
    return res;
  }
  function transform_2d(arr, x, y) {
    var res = arr.slice();
    for(var i = 0; i < res.length; i += 2) {
      res[i] += x;
      res[i + 1] += y;
    }
    return res;
  }
  function double(arr) {
    return combine(arr, arr);
  }
  function double_shifted(arr, x, y, z) {
    return combine(arr, transform(arr, x, y, z));
  }
  function double_shifted_2d(arr, x, y) {
    return combine(arr, transform_2d(arr, x, y));
  }
  
  var fenceList = [
    {name: "Wooden Fence", node: "fences:wood", tex: "default_fence_wood.png", icon: "Wooden_Fence.png", craft: "default:wood"},
    {name: "Acacia Wood Fence", node: "fences:acacia_wood", tex: "default_fence_acacia_wood.png", icon: "Acacia_Fence.png", craft: "default:acacia_wood"},
    {name: "Aspen Wood Fence", node: "fences:aspen_wood", tex: "default_fence_aspen_wood.png", icon: "Aspen_Fence.png", craft: "default:aspen_wood"},
    {name: "Jungle Wood Fence", node: "fences:jungle_wood", tex: "default_fence_junglewood.png", icon: "Junglewood_Fence.png", craft: "default:jungle_wood"},
    {name: "Pine Wood Fence", node: "fences:pine_wood", tex: "default_fence_pine_wood.png", icon: "Pine_Fence.png", craft: "default:pine_wood"}
  ];
  
  var fenceConnects = [];
  for(var i = 0; i < fenceList.length; i++) {
    var node = fenceList[i].node;
    var craft = fenceList[i].craft;
    fenceConnects.push(node);
    fenceConnects.push(craft);
  }
  
  for(var i = 0; i < fenceList.length; i++) {
    var name = fenceList[i].name;
    var node = fenceList[i].node;
    var tex = fenceList[i].tex;
    var icon = fenceList[i].icon;
    var craft = fenceList[i].craft;
    
    registerNodeHelper(node, {
      desc: name,
      tex: {texAll: tex},
      icon: icon,
      node: {groups: {choppy: 1}, transparent: true, passSunlight: false, customMesh: true,
      connectingMesh: true, meshConnectsTo: fenceConnects,
        customMeshVerts: [
          [
            -0.125, 0.5, -0.125,
            -0.125, -0.5, -0.125,
            -0.125, 0.5, 0.125,
            
            -0.125, -0.5, -0.125,
            -0.125, -0.5, 0.125,
            -0.125, 0.5, 0.125
          ],
          [
            0.125, 0.5, 0.125,
            0.125, -0.5, 0.125,
            0.125, 0.5, -0.125,
            
            0.125, -0.5, 0.125,
            0.125, -0.5, -0.125,
            0.125, 0.5, -0.125
          ],
          [
            0.125, -0.5, 0.125,
            -0.125, -0.5, 0.125,
            0.125, -0.5, -0.125,
            
            -0.125, -0.5, 0.125,
            -0.125, -0.5, -0.125,
            0.125, -0.5, -0.125
          ],
          [
            0.125, 0.5, -0.125,
            -0.125, 0.5, -0.125,
            0.125, 0.5, 0.125,
            
            -0.125, 0.5, -0.125,
            -0.125, 0.5, 0.125,
            0.125, 0.5, 0.125
          ],
          [
            0.125, 0.5, -0.125,
            0.125, -0.5, -0.125,
            -0.125, 0.5, -0.125,
            
            0.125, -0.5, -0.125,
            -0.125, -0.5, -0.125,
            -0.125, 0.5, -0.125
          ],
          [
            -0.125, 0.5, 0.125,
            -0.125, -0.5, 0.125,
            0.125, 0.5, 0.125,
            
            -0.125, -0.5, 0.125,
            0.125, -0.5, 0.125,
            0.125, 0.5, 0.125
          ]
        ],
        customMeshUVs: [
          [
            0.375, 1.0,
            0.375, 0.0,
            0.625, 1.0,
            
            0.375, 0.0,
            0.625, 0.0,
            0.625, 1.0
          ],
          [
            0.375, 1.0,
            0.375, 0.0,
            0.625, 1.0,
            
            0.375, 0.0,
            0.625, 0.0,
            0.625, 1.0
          ],
          [
            0.375, 0.25,
            0.375, 0.0,
            0.625, 0.25,
            
            0.375, 0.0,
            0.625, 0.0,
            0.625, 0.25
          ],
          [
            0.375, 0.25,
            0.375, 0.0,
            0.625, 0.25,
            
            0.375, 0.0,
            0.625, 0.0,
            0.625, 0.25
          ],
          [
            0.375, 1.0,
            0.375, 0.0,
            0.625, 1.0,
            
            0.375, 0.0,
            0.625, 0.0,
            0.625, 1.0
          ],
          [
            0.375, 1.0,
            0.375, 0.0,
            0.625, 1.0,
            
            0.375, 0.0,
            0.625, 0.0,
            0.625, 1.0
          ]
        ],
        connectingVerts: [
          [ //-X
            [],
            [],
            double_shifted([
              0.125, 0.1875, 0.0625,
              -0.5, 0.1875, 0.0625,
              0.125, 0.1875, -0.0625,
              
              -0.5, 0.1875, 0.0625,
              -0.5, 0.1875, -0.0625,
              0.125, 0.1875, -0.0625
            ], 0, -0.5, 0),
            double_shifted([
              0.125, 0.3125, -0.0625,
              -0.5, 0.3125, -0.0625,
              0.125, 0.3125, 0.0625,
              
              -0.5, 0.3125, -0.0625,
              -0.5, 0.3125, 0.0625,
              0.125, 0.3125, 0.0625
            ], 0, -0.5, 0),
            double_shifted([
              0.125, 0.3125, -0.0625,
              0.125, 0.1875, -0.0625,
              -0.5, 0.3125, -0.0625,
              
              0.125, 0.1875, -0.0625,
              -0.5, 0.1875, -0.0625,
              -0.5, 0.3125, -0.0625
            ], 0, -0.5, 0),
            double_shifted([
              -0.5, 0.3125, 0.0625,
              -0.5, 0.1875, 0.0625,
              0.125, 0.3125, 0.0625,
              
              -0.5, 0.1875, 0.0625,
              0.125, 0.1875, 0.0625,
              0.125, 0.3125, 0.0625
            ], 0, -0.5, 0)
          ],
          [ //+X
            [],
            [],
            double_shifted([
              0.5, 0.1875, 0.0625,
              -0.125, 0.1875, 0.0625,
              0.5, 0.1875, -0.0625,
              
              -0.125, 0.1875, 0.0625,
              -0.125, 0.1875, -0.0625,
              0.5, 0.1875, -0.0625
            ], 0, -0.5, 0),
            double_shifted([
              0.5, 0.3125, -0.0625,
              -0.125, 0.3125, -0.0625,
              0.5, 0.3125, 0.0625,
              
              -0.125, 0.3125, -0.0625,
              -0.125, 0.3125, 0.0625,
              0.5, 0.3125, 0.0625
            ], 0, -0.5, 0),
            double_shifted([
              0.5, 0.3125, -0.0625,
              0.5, 0.1875, -0.0625,
              -0.125, 0.3125, -0.0625,
              
              0.5, 0.1875, -0.0625,
              -0.125, 0.1875, -0.0625,
              -0.125, 0.3125, -0.0625
            ], 0, -0.5, 0),
            double_shifted([
              -0.125, 0.3125, 0.0625,
              -0.125, 0.1875, 0.0625,
              0.5, 0.3125, 0.0625,
              
              -0.125, 0.1875, 0.0625,
              0.5, 0.1875, 0.0625,
              0.5, 0.3125, 0.0625
            ], 0, -0.5, 0)
          ],
          null, null,
          [ //-Z
            [],
            [],
            double_shifted([
              -0.0625, 0.3125, -0.5,
              -0.0625, 0.1875, -0.5,
              -0.0625, 0.3125, 0.125,
              
              -0.0625, 0.1875, -0.5,
              -0.0625, 0.1875, 0.125,
              -0.0625, 0.3125, 0.125
            ], 0, -0.5, 0),
            double_shifted([
              0.0625, 0.3125, 0.125,
              0.0625, 0.1875, 0.125,
              0.0625, 0.3125, -0.5,
              
              0.0625, 0.1875, 0.125,
              0.0625, 0.1875, -0.5,
              0.0625, 0.3125, -0.5
            ], 0, -0.5, 0),
            double_shifted([
              0.0625, 0.1875, 0.125,
              -0.0625, 0.1875, 0.125,
              0.0625, 0.1875, -0.5,
              
              -0.0625, 0.1875, 0.125,
              -0.0625, 0.1875, -0.5,
              0.0625, 0.1875, -0.5
            ], 0, -0.5, 0),
            double_shifted([
              0.0625, 0.3125, -0.5,
              -0.0625, 0.3125, -0.5,
              0.0625, 0.3125, 0.125,
              
              -0.0625, 0.3125, -0.5,
              -0.0625, 0.3125, 0.125,
              0.0625, 0.3125, 0.125
            ], 0, -0.5, 0)
          ],
          [ //+Z
            [],
            [],
            double_shifted([
              -0.0625, 0.3125, -0.125,
              -0.0625, 0.1875, -0.125,
              -0.0625, 0.3125, 0.5,
              
              -0.0625, 0.1875, -0.125,
              -0.0625, 0.1875, 0.5,
              -0.0625, 0.3125, 0.5
            ], 0, -0.5, 0),
            double_shifted([
              0.0625, 0.3125, 0.5,
              0.0625, 0.1875, 0.5,
              0.0625, 0.3125, -0.125,
              
              0.0625, 0.1875, 0.5,
              0.0625, 0.1875, -0.125,
              0.0625, 0.3125, -0.125
            ], 0, -0.5, 0),
            double_shifted([
              0.0625, 0.1875, 0.5,
              -0.0625, 0.1875, 0.5,
              0.0625, 0.1875, -0.125,
              
              -0.0625, 0.1875, 0.5,
              -0.0625, 0.1875, -0.125,
              0.0625, 0.1875, -0.125
            ], 0, -0.5, 0),
            double_shifted([
              0.0625, 0.3125, -0.125,
              -0.0625, 0.3125, -0.125,
              0.0625, 0.3125, 0.5,
              
              -0.0625, 0.3125, -0.125,
              -0.0625, 0.3125, 0.5,
              0.0625, 0.3125, 0.5
            ], 0, -0.5, 0)
          ]
        ],
        connectingUVs: [
          [ //-X
            [],
            [],
            double_shifted_2d([
              0.0, 0.0,
              0.375, 0.0,
              0.0, 0.125,
              
              0.375, 0.0,
              0.375, 0.125,
              0.0, 0.125
            ], 0, 0.0625),
            double_shifted_2d([
              0.0, 0.0,
              0.375, 0.0,
              0.0, 0.125,
              
              0.375, 0.0,
              0.375, 0.125,
              0.0, 0.125
            ], 0, 0.0625),
            double_shifted_2d([
              0.0, 0.625,
              0.0, 0.5,
              0.375, 0.625,
              
              0.0, 0.5,
              0.375, 0.5,
              0.375, 0.625
            ], 0, 0.0625),
            double_shifted_2d([
              0.0, 0.875,
              0.0, 0.75,
              0.375, 0.875,
              
              0.0, 0.75,
              0.375, 0.75,
              0.375, 0.875
            ], 0, 0.0625)
          ],
          [ //+X
            [],
            [],
            double_shifted_2d([
              0.625, 0.0,
              1.0, 0.0,
              0.625, 0.125,
              
              1.0, 0.0,
              1.0, 0.125,
              0.625, 0.125
            ], 0, 0.0625),
            double_shifted_2d([
              0.625, 0.25,
              1.0, 0.25,
              0.625, 0.375,
              
              1.0, 0.25,
              1.0, 0.375,
              0.625, 0.375
            ], 0, 0.0625),
            double_shifted_2d([
              0.625, 0.625,
              0.625, 0.5,
              1.0, 0.625,
              
              0.625, 0.5,
              1.0, 0.5,
              1.0, 0.625
            ], 0, 0.0625),
            double_shifted_2d([
              0.625, 0.875,
              0.625, 0.75,
              1.0, 0.875,
              
              0.625, 0.75,
              1.0, 0.75,
              1.0, 0.875
            ], 0, 0.0625)
          ],
          null,
          null,
          [ //-Z
            [],
            [],
            double_shifted_2d([
              0.0, 0.125,
              0.0, 0.0,
              0.375, 0.125,
              
              0.0, 0.0,
              0.375, 0.0,
              0.375, 0.125
            ], 0, 0.0625),
            double_shifted_2d([
              0.0, 0.375,
              0.0, 0.25,
              0.375, 0.375,
              
              0.0, 0.25,
              0.375, 0.25,
              0.375, 0.375
            ], 0, 0.0625),
            double_shifted_2d([
              0.0, 0.625,
              0.0, 0.5,
              0.375, 0.625,
              
              0.0, 0.5,
              0.375, 0.5,
              0.375, 0.625
            ], 0, 0.0625),
            double_shifted_2d([
              0.0, 0.875,
              0.0, 0.75,
              0.375, 0.875,
              
              0.0, 0.75,
              0.375, 0.75,
              0.375, 0.875
            ], 0, 0.0625)
          ],
          [ //+Z
            [],
            [],
            double_shifted_2d([
              0.625, 0.125,
              0.625, 0.0,
              1.0, 0.125,
              
              0.625, 0.0,
              1.0, 0.0,
              1.0, 0.125
            ], 0, 0.0625),
            double_shifted_2d([
              0.625, 0.375,
              0.625, 0.25,
              1.0, 0.375,
              
              0.625, 0.25,
              1.0, 0.25,
              1.0, 0.375
            ], 0, 0.0625),
            double_shifted_2d([
              0.625, 0.625,
              0.625, 0.5,
              1.0, 0.625,
              
              0.625, 0.5,
              1.0, 0.5,
              1.0, 0.625
            ], 0, 0.0625),
            double_shifted_2d([
              0.625, 0.875,
              0.625, 0.75,
              1.0, 0.875,
              
              0.625, 0.75,
              1.0, 0.75,
              1.0, 0.875
            ], 0, 0.0625)
          ]
        ],
        //FIXME change bounding box based on connecting
        boundingBox: [
          new THREE.Box3(new THREE.Vector3(-0.21, -0.5, -0.21), new THREE.Vector3(0.21, 0.5, 0.21))
        ],
        faceIsRecessed: [true, true, false, false, true, true]
      }
    });
    
    api.registerCraft(new api.CraftEntry(node + " 4", [
      craft, "default:stick", craft,
      craft, "default:stick", craft
    ], {shape: {x: 3, y: 2}}));
  }
})();
