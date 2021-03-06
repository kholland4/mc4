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
  var modpath = api.getModMeta("hud").path;
  mods.hud = {};
  
  var hudContainer = document.createElement("div");
  hudContainer.style.position = "fixed";
  hudContainer.style.left = "50%";
  hudContainer.style.bottom = "5px";
  hudContainer.style.transform = "translate(-50%)";
  hudContainer.style.zIndex = "7";
  mods.hud.dom = hudContainer;
  api.registerHUD(hudContainer);
  
  mods.hud.lastWieldIndex = 0;
  var wieldSel = document.createElement("div");
  wieldSel.style.display = "none";
  wieldSel.style.position = "absolute";
  wieldSel.style.border = "4px solid #999";
  wieldSel.style.pointerEvents = "none";
  mods.hud.wieldSel = wieldSel;
  hudContainer.appendChild(wieldSel);
  
  mods.hud.update = function() {
    var inv = api.player.inventory;
    var el = mods.hud.domInvList;
    api.uiUpdateInventoryList(inv, "main", {count: 8, interactive: false}, el);
    
    mods.hud.lastInv = [];
    var list = inv.getList("main");
    for(var i = 0; i < 8; i++) {
      if(list[i] == null) {
        mods.hud.lastInv.push(null);
      } else {
        mods.hud.lastInv.push(list[i].clone());
      }
    }
  };
  mods.hud.updateWield = function() {
    var wieldIndex = api.player.wieldIndex;
    var wieldEl = mods.hud.domInvList.querySelectorAll(".uiIL_square")[wieldIndex];
    var rect = wieldEl.getBoundingClientRect();
    var containerRect = mods.hud.dom.getBoundingClientRect();
    mods.hud.wieldSel.style.left = (rect.left - 4 - containerRect.left) + "px";
    mods.hud.wieldSel.style.top = (rect.top - 4 - containerRect.top) + "px";
    mods.hud.wieldSel.style.width = rect.width + "px";
    mods.hud.wieldSel.style.height = rect.height + "px";
    mods.hud.wieldSel.style.display = "block";
    mods.hud.lastWieldIndex = wieldIndex;
  };
  
  var inv = api.player.inventory;
  var el = api.uiRenderInventoryList(inv, "main", {count: 8, interactive: false});
  mods.hud.domInvList = el;
  //while(mods.hud.dom.firstChild) { mods.hud.dom.removeChild(mods.hud.dom.firstChild); }
  mods.hud.dom.appendChild(el);
  
  mods.hud.update();
  mods.hud.updateWield();
  
  api.registerOnFrame(function() {
    var needsUpdate = false;
    var inv = api.player.inventory;
    var list = inv.getList("main");
    for(var i = 0; i < 8; i++) {
      if(list[i] == null || mods.hud.lastInv[i] == null) {
        if(list[i] != mods.hud.lastInv[i]) {
          needsUpdate = true;
          break;
        }
      } else {
        if(!list[i].equalsBasic(mods.hud.lastInv[i])) {
          needsUpdate = true;
          break;
        }
      }
    }
    
    if(needsUpdate) {
      mods.hud.update();
    }
    
    if(api.player.wieldIndex != mods.hud.lastWieldIndex) {
      mods.hud.updateWield();
    }
  });
  
  api.registerKey(function(key) {
    if(api.uiWindowOpen()) { return; }
    
    key = key.toLowerCase();
    
    if(!isNaN(parseInt(key))) {
      var n = parseInt(key);
      if(n >= 1 && n <= 8) {
        api.player.wieldIndex = n - 1;
      }
    }
  });
  
  window.addEventListener("wheel", function(e) {
    if(api.uiWindowOpen()) { return; }
    
    if(e.deltaY < 0) {
      api.player.wieldIndex--;
      if(api.player.wieldIndex < 0) { api.player.wieldIndex = 7; }
    } else if(e.deltaY > 0) {
      api.player.wieldIndex++;
      if(api.player.wieldIndex > 7) { api.player.wieldIndex = 0; }
    }
  });
})();
