(function() {
  var modpath = api.getModMeta("default").path;
  
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
    api.registerItem(new api.Item(itemstring, Object.assign({isNode: true, iconFile: modpath + "/icons/" + args.icon}, args.item)));
    api.registerNode(new api.Node(itemstring, Object.assign(texOverlay, args.node)));
  }
  
  registerNodeHelper("default:dirt", {
    desc: "Dirt",
    tex: {texAll: "default_dirt.png"},
    icon: "../textures/default_dirt.png",
    node: {groups: {crumbly: 1}}
  });
  
  registerNodeHelper("default:grass", {
    desc: "Dirt with Grass",
    tex: {texTop: "default_grass.png", texBottom: "default_dirt.png", texSides: "default_grass_side.png"},
    icon: "../textures/default_grass_side.png",
    node: {groups: {crumbly: 1}}
  });
  
  registerNodeHelper("default:stone", {
    desc: "Stone",
    tex: {texAll: "default_stone.png"},
    icon: "../textures/default_stone.png",
    node: {groups: {cracky: 3}}
  });
  
  api.registerItem(new api.Item("default:pick_diamond", {
    isTool: true, toolWear: 2000, toolGroups: {
      "cracky": {times: [0, 0.2, 0.4, 0.5, 1, 1.5, 2], maxlevel: 6
    }
  }, iconFile: modpath + "/icons/default_tool_diamondpick.png"}));
  
  api.registerCraft(new api.CraftEntry(
    new api.ItemStack("default:pick_diamond", 1),
    [new api.ItemStack("default:dirt"), null, null, new api.ItemStack("default:dirt"), null, null, null, null, null],
    {shape: {x: 3, y: 3}}
  ));
})();
