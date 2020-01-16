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
      groups: {cracky: 1}, transparent: true, customMesh: true,
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
