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

function initUI() {
  controls.addEventListener("lock", function() {
    uiHideWindow();
    
    for(var i = 0; i < uiVWCloseHandlers.length; i++) {
      uiVWCloseHandlers[i]();
    }
    uiVirtualWindows = 0; //FIXME
  });
}

//---

class UIWindow {
  constructor() {
    this.dom = document.createElement("div");
    this.dom.className = "uiWindow";
  }
  
  add(el) {
    this.dom.appendChild(el);
  }
}

var uiWindowOpen = false;

function uiShowWindow(win) {
  var doUnlock = true;
  if(uiWindowOpen) { uiHideWindow(false); doUnlock = false; }
  
  var el = document.getElementById("uiWindowContainer");
  el.appendChild(win.dom);
  el.style.display = "block";
  uiWindowOpen = true;
  
  if(doUnlock && controls) { controls.unlock(); }
}
function uiHideWindow(doLock=true) {
  if(uiWindowOpen) {
    var el = document.getElementById("uiWindowContainer");
    el.style.display = "none";
    while(el.firstChild) { el.removeChild(el.firstChild); }
    uiHideHand();
    uiWindowOpen = false;
    
    if(doLock && controls) { controls.lock(); }
  }
}

function uiElement(name, opts) {
  if(name == "container") {
    var el = document.createElement("div");
    return el;
  }
  if(name == "spacer") {
    var el = document.createElement("div");
    el.className = "uiEl_spacer";
    return el;
  }
  if(name == "text") {
    var el = document.createElement("span");
    el.className = "uiEl_text";
    if(opts != undefined) { el.innerText = opts; }
    return el;
  }
  if(name == "link") {
    var el = document.createElement("a");
    el.className = "uiEl_link";
    if(opts != undefined) { el.innerText = opts[0]; el.href = opts[1]; }
    return el;
  }
  if(name == "br") {
    var el = document.createElement("br");
    el.className = "uiEl_br";
    return el;
  }
  if(name == "button") {
    var el = document.createElement("button");
    el.className = "uiEl_button";
    return el;
  }
  if(name == "select") {
    var el = document.createElement("select");
    el.className = "uiEl_select";
    for(var i = 0; i < opts.length; i++) {
      var opt = document.createElement("option");
      opt.value = opts[i][0];
      opt.innerText = opts[i][1];
      el.appendChild(opt);
    }
    return el;
  }
  if(name == "input") {
    var el = document.createElement("input");
    el.type = "text";
    el.className = "uiEl_input_text";
    if(opts != undefined) { el.value = opts; }
    return el;
  }
  if(name == "scrollbox") {
    var el = document.createElement("div");
    el.className = "uiEl_scrollbox";
    if(opts != undefined) {
      if("height" in opts) {
        el.height = opts.height;
      }
    }
    return el;
  }
  
  if(name == "tab_container") {
    var el = document.createElement("div");
    el.className = "uiEl_tab_container";
    
    var tabBox = document.createElement("div");
    tabBox.className = "uiEl_tab_box";
    
    var contentBox = document.createElement("div");
    contentBox.className = "uiEl_tab_content_box";
    
    for(var i = 0; i < opts.length; i++) {
      var tab = document.createElement("div");
      tab.className = "uiEl_tab";
      if(i == 0) { tab.className += " active"; }
      tab.dataset.index = i;
      tab.innerText = opts[i].name;
      tab.onclick = function() {
        var tabBox = this.parentElement;
        for(var i = 0; i < tabBox.children.length; i++) {
          tabBox.children[i].className = "uiEl_tab";
        }
        this.className = "uiEl_tab active";
        
        var contentBox = this.parentElement.parentElement.children[1];
        for(var i = 0; i < contentBox.children.length; i++) {
          contentBox.children[i].className = "uiEl_tab_content";
          if(contentBox.children[i].dataset.index == this.dataset.index) {
            contentBox.children[i].className += "active";
          }
        }
      };
      tabBox.appendChild(tab);
      
      var content = document.createElement("div");
      content.className = "uiEl_tab_content";
      if(i == 0) { content.className += " active"; }
      content.dataset.index = i;
      content.appendChild(opts[i].content);
      contentBox.appendChild(content);
    }
    
    el.appendChild(tabBox);
    el.appendChild(contentBox);
    
    return el;
  }
}

api.uiShowWindow = uiShowWindow;
api.uiHideWindow = uiHideWindow;
api.UIWindow = UIWindow;
api.uiElement = uiElement;

//FIXME
var uiVirtualWindows = 0;
api.uiVWOpen = function() { uiVirtualWindows++; if(controls) { controls.unlock(); } }
api.uiVWClose = function() { uiVirtualWindows--; if(controls) { controls.lock(); } }
var uiVWCloseHandlers = [];
api.registerVWCloseHandler = function(f) { uiVWCloseHandlers.push(f); };
api.uiWindowOpen = function() { return uiWindowOpen || uiVirtualWindows > 0; }

api.registerHUD = function(dom) {
  document.body.appendChild(dom);
};
