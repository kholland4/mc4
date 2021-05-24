/*
    mc4, a web voxel building game
    Copyright (C) 2019-2021 kholland4

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
  
  mods.inventory.craftOutputEl = null;
  mods.inventory.craftListEl = null;
  
  mods.inventory.isOpen = false;
  mods.inventory.show = function() {
    if(mods.inventory.isOpen && !api.uiWindowOpen()) { mods.inventory.isOpen = false; }
    if(mods.inventory.isOpen) {
      api.uiHideWindow();
      mods.inventory.isOpen = false;
    } else {
      var win = new api.UIWindow();
      
      if(mods.inventory.creative) {
        mods.inventory.creativeInvEl = api.uiRenderInventoryList(new InvRef("player", null, "creative", null), {width: 8, interactive: true, scrollHeight: 3});
        
        win.add(mods.inventory.creativeInvEl);
        
        win.add(api.uiElement("spacer"));
      } else {
        mods.inventory.craftListEl = api.uiRenderInventoryList(new InvRef("player", null, "craft", null), {width: 3, interactive: true});
        win.add(mods.inventory.craftListEl);
        
        win.add(api.uiElement("spacer"));
        
        mods.inventory.craftOutputEl = api.uiRenderInventoryList(new InvRef("player", null, "craftOutput", null), {interactive: true});
        win.add(mods.inventory.craftOutputEl);
        
        win.add(api.uiElement("spacer"));
      }
      
      var invList = api.uiRenderInventoryList(new InvRef("player", null, "main", null), {width: 8, interactive: true});
      win.add(invList);
      
      api.uiShowWindow(win);
      api.uiShowHand(new InvRef("player", null, "hand", 0));
      mods.inventory.isOpen = true;
    }
  };
  
  api.registerKey(function(key) {
    if((key.toLowerCase() == "e" && api.ingameKey()) || (mods.inventory.isOpen && (key == "e" || key == "Escape"))) {
      mods.inventory.show();
    }
  });
})();
