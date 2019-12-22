(function() {
  var modpath = api.getModMeta("inventory").path;
  mods.inventory = {};
  
  mods.inventory.isOpen = false;
  mods.inventory.show = function() {
    if(mods.inventory.isOpen) {
      api.uiHideWindow();
      mods.inventory.isOpen = false;
    } else {
      var win = new api.UIWindow();
      var listEl = api.uiRenderInventoryList(player.inventory, "main", {width: 8, interactive: true});
      win.add(listEl);
      api.uiShowWindow(win);
      api.uiShowHand(player.inventory, "hand", 0);
      mods.inventory.isOpen = true;
    }
  };
  
  api.registerKey(function(key) {
    if(key.toLowerCase() == "e") {
      mods.inventory.show();
    }
  });
})();
