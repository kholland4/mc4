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
  var modpath = api.getModMeta("minerals").path;
  
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
  
  //---minerals---
  var ores = {"technic": ["lead", "zinc"], "moreores": ["mithril", "silver"]};
  for(var mp in ores) {
    for(var i = 0; i < ores[mp].length; i++) {
      var m = ores[mp][i];
      var mCaps = itemstringGuessName(m);
      
      registerNodeHelper("minerals:ore_" + m, {
        desc: mCaps + " Ore",
        tex: {texAll: "mineral_" + m + ".png"},
        icon: "mineral_" + m + ".png",
        node: {groups: {cracky: 2}, drops: "minerals:lump_" + m}
      });
      api.registerItem(new api.Item("minerals:lump_" + m, {desc: mCaps + " Lump", iconFile: modpath + "/icons/" + mp + "_" + m + "_lump.png"}));
      api.registerItem(new api.Item("minerals:ingot_" + m, {desc: mCaps + " Ingot", iconFile: modpath + "/icons/" + mp + "_" + m + "_ingot.png"}));
      api.registerCraft(new api.CraftEntry("minerals:ingot_" + m, ["minerals:lump_" + m], {shape: null, type: "cook", cookTime: 4}));
      registerNodeHelper("minerals:" + m + "_block", {
        desc: mCaps + " Block",
        tex: {texAll: mp + "_" + m + "_block.png"},
        icon: mp + "_" + m + "_block.png",
        node: {groups: {cracky: 3}}
      });
      api.registerCraft(new api.CraftEntry("minerals:" + m + "_block", Array(9).fill("minerals:ingot_" + m), {shape: {x: 3, y: 3}}));
      api.registerCraft(new api.CraftEntry("minerals:ingot_" + m + " 9", ["minerals:" + m + "_block"], {shape: null}));
    }
  }
  
  
  
  //---marble, granite---
  registerNodeHelper("minerals:marble_brown", {
    desc: "Brown Marble",
    tex: {texAll: "technic_marble.png"},
    icon: "technic_marble.png",
    node: {groups: {cracky: 2}}
  });
  registerNodeHelper("minerals:brick_marble_brown", {
    desc: "Brown Marble Bricks",
    tex: {texAll: "technic_marble_bricks.png"},
    icon: "technic_marble_bricks.png",
    node: {groups: {cracky: 2}}
  });
  api.registerCraft(new api.CraftEntry("minerals:brick_marble_brown 4", Array(4).fill("minerals:marble_brown"), {shape: {x: 2, y: 2}}));
  
  registerNodeHelper("minerals:granite_dark", {
    desc: "Granite",
    tex: {texAll: "technic_granite.png"},
    icon: "technic_granite.png",
    node: {groups: {cracky: 2}}
  });
})();
