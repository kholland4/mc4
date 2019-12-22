(function() {
  var modpath = api.getModMeta("default").path;
  
  api.loadTexture("default:dirt", modpath + "/textures/default_dirt.png");
  api.registerItem(new Item("default:dirt", {isNode: true, iconFile: modpath + "/textures/default_dirt.png"}));
  api.registerNode(new Node("default:dirt", {texAll: "default:dirt"}));
  
  api.loadTexture("default:grass:top", modpath + "/textures/default_grass.png");
  api.loadTexture("default:grass:side", modpath + "/textures/default_grass_side.png");
  api.registerItem(new Item("default:grass", {isNode: true, iconFile: modpath + "/textures/default_grass_side.png"}));
  api.registerNode(new Node("default:grass", {texTop: "default:grass:top", texBottom: "default:dirt", texSides: "default:grass:side"}));
})();
