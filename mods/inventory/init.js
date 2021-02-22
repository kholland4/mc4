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
  var modpath = api.getModMeta("inventory").path;
  mods.inventory = {};
  
  mods.inventory.creative = false;
  api.onModLoaded("gamemode", function() {
    mods.gamemode.registerChangeHook(function() {
      mods.inventory.creative = mods.gamemode.creative;
      if(mods.inventory.isOpen) {
        mods.inventory.show(); //FIXME will close instead of refresh
      }
    });
  });
  
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
      
      if(mods.inventory.creative) {
        mods.inventory.creativeDatastore = {"creative": []};
        
        var allItemStrings = api.listAllItems();
        for(var i = 0; i < allItemStrings.length; i++) {
          var itemstring = allItemStrings[i];
          var def = api.getItemDef(itemstring);
          if(def == null) {
            api.modDebug("inventory", "warn", "creative inventory: unable to retrieve definition for '" + itemstring + "'");
            continue;
          }
          if(!def.inCreativeInventory) { continue; }
          
          var stack = new api.ItemStack(itemstring);
          mods.inventory.creativeDatastore["creative"].push(stack);
        }
        
        mods.inventory.creativeInv = new api.Inventory();
        var cinv = mods.inventory.creativeInv;
        cinv._getList = function(name) {
          if(name in mods.inventory.creativeDatastore) {
            return mods.inventory.creativeDatastore[name];
          }
          return [];
        };
        cinv._setList = function(name, data) { return false; };
        
        cinv._getStack = function(name, index) {
          if(name in mods.inventory.creativeDatastore) {
            var list = mods.inventory.creativeDatastore[name];
            if(index < list.length) {
              if(list[index] instanceof ItemStack) {
                return list[index].clone();
              } else {
                return list[index];
              }
            }
          }
          return null;
        };
        cinv._setStack = function(name, index, data) { return false; };
        
        mods.inventory.creativeInvEl = api.uiRenderInventoryList(mods.inventory.creativeInv, "creative", {width: 8, interactive: true, scrollHeight: 3});
        
        win.add(mods.inventory.creativeInvEl);
        
        win.add(api.uiElement("spacer"));
      } else {
        mods.inventory.craftListEl = api.uiRenderInventoryList(api.player.inventory, "craft", {width: 3, interactive: true, onUpdate: mods.inventory.updateCraftOutput});
        win.add(mods.inventory.craftListEl);
        
        win.add(api.uiElement("spacer"));
        
        mods.inventory.craftOutputEl = api.uiRenderInventoryList(mods.inventory.craftOutput, "main", {interactive: true, onUpdate: mods.inventory.onCraftOutputTake});
        win.add(mods.inventory.craftOutputEl);
        mods.inventory.updateCraftOutput();
        
        win.add(api.uiElement("spacer"));
      }
      
      var invList = api.uiRenderInventoryList(api.player.inventory, "main", {width: 8, interactive: true});
      win.add(invList);
      
      api.uiShowWindow(win);
      api.uiShowHand(api.player.inventory, "hand", 0);
      mods.inventory.isOpen = true;
    }
  };
  
  api.registerKey(function(key) {
    if((key.toLowerCase() == "e" && api.ingameKey()) || (mods.inventory.isOpen && (key == "e" || key == "Escape"))) {
      mods.inventory.show();
    }
  });
})();
