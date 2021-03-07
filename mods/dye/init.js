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
  var modpath = api.getModMeta("dye").path;
  
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
  
  
  var dyeList = [
    "black",
    "blue",
    "brown",
    "cyan",
    "dark_green",
    "dark_grey",
    "green",
    "grey",
    "magenta",
    "orange",
    "pink",
    "red",
    "violet",
    "white",
    "yellow"
  ];
  for(var i = 0; i < dyeList.length; i++) {
    var name = dyeList[i];
    api.registerItem(new api.Item("dye:" + name, {desc: itemstringGuessName(name) + " Dye", iconFile: modpath + "/icons/dye_" + name + ".png"}));
  }
  
  
  var woolList = [
    {name: "Wool", node: "dye:wool_white", file: "wool_white.png", dye: "dye:white"},
    {name: "Black Wool", node: "dye:wool_black", file: "wool_black.png", dye: "dye:black"},
    {name: "Blue Wool", node: "dye:wool_blue", file: "wool_blue.png", dye: "dye:blue"},
    {name: "Brown Wool", node: "dye:wool_brown", file: "wool_brown.png", dye: "dye:brown"},
    {name: "Cyan Wool", node: "dye:wool_cyan", file: "wool_cyan.png", dye: "dye:cyan"},
    {name: "Dark Green Wool", node: "dye:wool_dark_green", file: "wool_dark_green.png", dye: "dye:dark_green"},
    {name: "Dark Grey Wool", node: "dye:wool_dark_grey", file: "wool_dark_grey.png", dye: "dye:dark_grey"},
    {name: "Green Wool", node: "dye:wool_green", file: "wool_green.png", dye: "dye:green"},
    {name: "Grey Wool", node: "dye:wool_grey", file: "wool_grey.png", dye: "dye:grey"},
    {name: "Magenta Wool", node: "dye:wool_magenta", file: "wool_magenta.png", dye: "dye:magenta"},
    {name: "Orange Wool", node: "dye:wool_orange", file: "wool_orange.png", dye: "dye:orange"},
    {name: "Pink Wool", node: "dye:wool_pink", file: "wool_pink.png", dye: "dye:pink"},
    {name: "Red Wool", node: "dye:wool_red", file: "wool_red.png", dye: "dye:red"},
    {name: "Violet Wool", node: "dye:wool_violet", file: "wool_violet.png", dye: "dye:violet"},
    {name: "Yellow Wool", node: "dye:wool_yellow", file: "wool_yellow.png", dye: "dye:yellow"}
  ];
  
  for(var i = 0; i < woolList.length; i++) {
    var name = woolList[i].name;
    var node = woolList[i].node;
    var file = woolList[i].file;
    var dye = woolList[i].dye;
    
    registerNodeHelper(node, {
      desc: name,
      tex: {texAll: file},
      icon: file,
      item: {groups: {wool: null}},
      node: {groups: {snappy: 1}}
    });
    
    api.registerCraft(new api.CraftEntry(node + " 8",
     [
       "group:wool", "group:wool", "group:wool",
       "group:wool", dye, "group:wool",
       "group:wool", "group:wool", "group:wool"
     ], {shape: {x: 3, y: 3}}));
  }
})();
