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
    
    
    api.registerItem(new api.Item("stairs:slab_stone", {
      isNode: true, desc: "Stone Slab",
      iconFile: modpath + "/icons/stairs_slab_stone.png"
    }));
    api.registerNode(new api.Node("stairs:slab_stone", {
      texAll: "default:stone:texAll",
      groups: {cracky: 1}, transparent: true, passSunlight: false, customMesh: true, //transparent: true, transFaces: [true, true, false, true, true, true]
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
      }
    ));
  });
})();
