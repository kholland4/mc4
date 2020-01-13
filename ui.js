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
