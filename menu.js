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

var menuIsOpen = false;

function initMenu() {
  api.registerKey(function(key) {
    if(key == "Escape" && (!api.uiWindowOpen() || menuIsOpen)) {
      showMenu();
    }
  });
}

function showMenu() {
  if(menuIsOpen && !api.uiWindowOpen()) { menuIsOpen = false; }
  
  if(menuIsOpen) {
    api.uiHideWindow();
    menuIsOpen = false;
  } else {
    var win = new api.UIWindow();
    
    win.add(api.uiElement("text", "mc4 v" + api.version() + "\na voxel building game by kholland4"));
    
    win.add(api.uiElement("spacer"));
    win.add(api.uiElement("link", ["Source code on GitHub", "https://github.com/kholland4/mc4/"]));
    win.add(api.uiElement("spacer"));
    
    win.add(api.uiElement("text", "Inspired by "));
    win.add(api.uiElement("link", ["Minetest", "https://www.minetest.net/"]));
    
    win.add(api.uiElement("spacer"));
    win.add(api.uiElement("text", "Use '/help' for ingame command help."));
    
    api.uiShowWindow(win);
    menuIsOpen = true;
  }
}
