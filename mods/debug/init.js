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
  var modpath = api.getModMeta("debug").path;
  
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
  
  //rotation cube
  registerNodeHelper("debug:rot_cube", {
    desc: "Rotation Cube",
    tex: {
      texFront: "debug_rot_cube_0.png",
      texBack: "debug_rot_cube_1.png",
      texBottom: "debug_rot_cube_2.png",
      texTop: "debug_rot_cube_3.png",
      texLeft: "debug_rot_cube_4.png",
      texRight: "debug_rot_cube_5.png"
    },
    icon: "debug_rot_cube.png",
    node: {groups: {oddly_breakable_by_hand: 1}, setRotOnPlace: true},
    item: {inCreativeInventory: false}
  });
  
  api.onModLoaded("stairs", function() {
    mods.stairs.registerNode("debug:rot_cube");
  });
})();
