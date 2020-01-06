(function() {
  var modpath = api.getModMeta("inventory").path;
  mods.inventory = {};
  
  mods.inventory.craftOutput = api.createTempInventory();
  mods.inventory.craftOutput.setList("main", [null]);
  mods.inventory.craftOutputEl = null;
  mods.inventory.craftListEl = null;
  
  mods.inventory.updateCraftOutput = function() {
    var list = api.player.inventory.getList("craft");
    
    var craftRes = api.getCraftRes(list, {x: 3, y: 3});
    if(craftRes != null) { craftRes = craftRes.getRes(); }
    mods.inventory.craftOutput.setStack("main", 0, craftRes);
    
    api.uiUpdateInventoryList(mods.inventory.craftOutput, "main", {interactive: true}, mods.inventory.craftOutputEl);
  };
  
  mods.inventory.onCraftOutputTake = function() {
    var list = api.player.inventory.getList("craft");
    var craftEntry = api.getCraftRes(list, {x: 3, y: 3});
    if(mods.inventory.craftOutput.getStack("main", 0) == null && craftEntry != null) {
      craftConsumeEntry(craftEntry, api.player.inventory, "craft", {x: 3, y: 3});
      api.uiUpdateInventoryList(api.player.inventory, "craft", {width: 3}, mods.inventory.craftListEl);
      mods.inventory.updateCraftOutput();
    } else if(mods.inventory.craftOutput.getStack("main", 0) != null) {
      //FIXME - this is a bodge but it seems to work correctly
      
      if(api.player.inventory.getStack("hand", 0) == null) {
        var handStack = api.player.inventory.getStack("hand", 0);
        var targetStack = mods.inventory.craftOutput.getStack("main", 0);
        api.player.inventory.setStack("hand", 0, targetStack);
        mods.inventory.craftOutput.setStack("main", 0, handStack);
        
        if(craftEntry != null) {
          craftConsumeEntry(craftEntry, api.player.inventory, "craft", {x: 3, y: 3});
          api.uiUpdateInventoryList(api.player.inventory, "craft", {width: 3}, mods.inventory.craftListEl);
        }
      } else {
        var handStack = api.player.inventory.getStack("hand", 0);
        var targetStack = mods.inventory.craftOutput.getStack("main", 0);
        
        if(handStack.typeMatch(targetStack) && handStack.count + targetStack.count <= handStack.getDef().maxStack) {
          handStack.count += targetStack.count;
          targetStack = null;
          api.player.inventory.setStack("hand", 0, handStack);
          mods.inventory.craftOutput.setStack("main", 0, targetStack);
          
          if(craftEntry != null) {
            craftConsumeEntry(craftEntry, api.player.inventory, "craft", {x: 3, y: 3});
            api.uiUpdateInventoryList(api.player.inventory, "craft", {width: 3}, mods.inventory.craftListEl);
          }
        } else {
          //swap
          var handStack = api.player.inventory.getStack("hand", 0);
          var targetStack = mods.inventory.craftOutput.getStack("main", 0);
          api.player.inventory.setStack("hand", 0, targetStack);
          mods.inventory.craftOutput.setStack("main", 0, handStack);
        }
      }
      
      mods.inventory.updateCraftOutput();
    }
  };
  
  mods.inventory.isOpen = false;
  mods.inventory.show = function() {
    if(mods.inventory.isOpen && !api.uiWindowOpen()) { mods.inventory.isOpen = false; }
    if(mods.inventory.isOpen) {
      api.uiHideWindow();
      mods.inventory.isOpen = false;
    } else {
      var win = new api.UIWindow();
      
      mods.inventory.craftListEl = api.uiRenderInventoryList(api.player.inventory, "craft", {width: 3, interactive: true, onUpdate: mods.inventory.updateCraftOutput});
      win.add(mods.inventory.craftListEl);
      
      win.add(api.uiElement("spacer"));
      
      mods.inventory.craftOutputEl = api.uiRenderInventoryList(mods.inventory.craftOutput, "main", {interactive: true, onUpdate: mods.inventory.onCraftOutputTake});
      win.add(mods.inventory.craftOutputEl);
      mods.inventory.updateCraftOutput();
      
      win.add(api.uiElement("spacer"));
      
      var invList = api.uiRenderInventoryList(api.player.inventory, "main", {width: 8, interactive: true});
      win.add(invList);
      
      api.uiShowWindow(win);
      api.uiShowHand(api.player.inventory, "hand", 0);
      mods.inventory.isOpen = true;
    }
  };
  
  api.registerKey(function(key) {
    if(key.toLowerCase() == "e" || (mods.inventory.isOpen && key == "Escape")) {
      mods.inventory.show();
    }
  });
})();
