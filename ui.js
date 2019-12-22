function initUI() {
  controls.addEventListener("lock", uiHideWindow);
}

function uiRenderItemStack(stack) {
  var def = stack.getDef();
  
  var container = document.createElement("div");
  container.className = "uiItemStack";
  
  var iconContainer = document.createElement("div");
  iconContainer.className = "uiIS_iconContainer";
  if(def.icon != null) {
    var icon = getIcon(def.icon);
    if(icon != null) {
      var iconImg = icon.img.cloneNode(true);
      iconImg.className = "uiIS_iconImg";
      iconContainer.appendChild(iconImg);
    }
  }
  container.appendChild(iconContainer);
  
  if(stack.count != 1) {
    var count = document.createElement("div");
    count.className = "uiIS_count";
    count.innerText = stack.count.toString();
    container.appendChild(count);
  }
  
  if(stack.wear != null) {
    //TODO
  }
  
  return container;
}

function uiRenderInventoryList(inv, listName, kwargs) {
  var list = inv.getList(listName);
  
  var args = Object.assign({start: 0, count: list.length, width: list.length, interactive: true}, kwargs);
  
  var container = document.createElement("div");
  container.className = "uiInvList";
  
  for(var i = args.start; i < args.start + args.count; i++) {
    var sq = document.createElement("div");
    sq.className = "uiIL_square";
    
    sq.dataset.itemstring = "";
    
    if(list[i] != null) {
      sq.appendChild(uiRenderItemStack(list[i]));
      
      sq.dataset.itemstring = list[i].itemstring;
      sq.dataset.count = list[i].count;
      sq.dataset.wear = list[i].wear;
    }
    
    if(args.interactive) {
      sq.onclick = function() {
        uiInteractInventoryList(this.inv, this.listName, this.index, "leftclick");
        uiUpdateInventoryList(this.inv, this.listName, this.args, this.container);
      }.bind({inv: inv, listName: listName, index: i, args: args, container: container});
      sq.oncontextmenu = function(e) {
        e.preventDefault();
        uiInteractInventoryList(this.inv, this.listName, this.index, "rightclick");
        uiUpdateInventoryList(this.inv, this.listName, this.args, this.container);
      }.bind({inv: inv, listName: listName, index: i, args: args, container: container});
    }
    
    container.appendChild(sq);
    
    if(i % args.width == args.width - 1) {
      container.appendChild(document.createElement("br"));
    }
  }
  
  return container;
}

//FIXME: check that container.children.length == args.count
function uiUpdateInventoryList(inv, listName, kwargs, container) {
  var list = inv.getList(listName);
  
  var args = Object.assign({start: 0, count: list.length, width: list.length + 1, interactive: true}, kwargs);
  
  for(var i = args.start; i < args.start + args.count; i++) {
    var sq = container.querySelectorAll(".uiIL_square")[i];
    
    var oldItemstring = sq.dataset.itemstring;
    var oldCount = parseInt(sq.dataset.count);
    if(sq.dataset.count == "null") { oldCount = null; }
    var oldWear = parseInt(sq.dataset.wear);
    if(sq.dataset.wear == "null") { oldWear = null; }
    
    var needsUpdate = true;
    if(oldItemstring == "" && list[i] == null) {
      needsUpdate = false;
    } else if(list[i] != null) {
      if(list[i].itemstring == oldItemstring && list[i].count == oldCount && list[i].wear == oldWear) {
        needsUpdate = false;
      }
    }
    
    if(needsUpdate) {
      while(sq.firstChild) { sq.removeChild(sq.firstChild); }
      
      sq.dataset.itemstring = "";
      
      if(list[i] != null) {
        sq.appendChild(uiRenderItemStack(list[i]));
        
        sq.dataset.itemstring = list[i].itemstring;
        sq.dataset.count = list[i].count;
        sq.dataset.wear = list[i].wear;
      }
    }
    
    if(args.interactive) {
      
    }
  }
  
  return container;
}

function uiInteractInventoryList(inv, listName, index, type="leftclick") {
  var handInv = inv;
  var handListName = "hand";
  var handListIndex = 0;
  
  var handStack = handInv.getStack(handListName, handListIndex);
  var targetStack = inv.getStack(listName, index);
  
  var typeMatch = false;
  if(handStack != null && targetStack != null) {
    if(targetStack.typeMatch(handStack)) {
      typeMatch = true;
    }
  }
  
  if(type == "leftclick") {
    if(typeMatch) {
      if(targetStack.count < targetStack.getDef().maxStack) {
        var amt = Math.min(handStack.count, targetStack.getDef().maxStack - targetStack.count);
        targetStack.count += amt;
        handStack.count -= amt;
        if(handStack.count <= 0) { handStack = null; }
        handInv.setStack(handListName, handListIndex, handStack);
        inv.setStack(listName, index, targetStack);
      } else {
        handInv.setStack(handListName, handListIndex, targetStack);
        inv.setStack(listName, index, handStack);
      }
    } else {
      handInv.setStack(handListName, handListIndex, targetStack);
      inv.setStack(listName, index, handStack);
    }
  } else if(type == "rightclick") {
    if(typeMatch) {
      if(targetStack.count < targetStack.getDef().maxStack) {
        var amt = 1;
        targetStack.count += amt;
        handStack.count -= amt;
        if(handStack.count <= 0) { handStack = null; }
        handInv.setStack(handListName, handListIndex, handStack);
        inv.setStack(listName, index, targetStack);
      } else {
        handInv.setStack(handListName, handListIndex, targetStack);
        inv.setStack(listName, index, handStack);
      }
    } else if(targetStack == null && handStack != null) {
      if(handStack.getDef().maxStack > 1 && handStack.getDef().stackable) {
        var amt = 1;
        targetStack = handStack.clone();
        targetStack.count = amt;
        handStack.count -= amt;
        if(handStack.count <= 0) { handStack = null; }
        handInv.setStack(handListName, handListIndex, handStack);
        inv.setStack(listName, index, targetStack);
      } else {
        handInv.setStack(handListName, handListIndex, targetStack);
        inv.setStack(listName, index, handStack);
      }
    } else if(targetStack != null && handStack == null) {
      if(targetStack.count > 1 && targetStack.getDef().stackable) {
        var amt = Math.ceil(targetStack.count / 2);
        handStack = targetStack.clone();
        handStack.count = amt;
        targetStack.count -= amt;
        if(targetStack.count <= 0) { targetStack = null; }
        handInv.setStack(handListName, handListIndex, handStack);
        inv.setStack(listName, index, targetStack);
      } else {
        handInv.setStack(handListName, handListIndex, targetStack);
        inv.setStack(listName, index, handStack);
      }
    } else {
      handInv.setStack(handListName, handListIndex, targetStack);
      inv.setStack(listName, index, handStack);
    }
  }
}

//---
api.registerOnFrame(uiUpdateHand);
//---

var uiHandVisible = false;
var uiHandInv;
var uiHandListName;
var uiHandIndex;

function uiShowHand(inv, listName, index) {
  uiHandInv = inv;
  uiHandListName = listName;
  uiHandIndex = index;
  
  var el = document.getElementById("uiHand");
  while(el.firstChild) { el.removeChild(el.firstChild); }
  el.appendChild(uiRenderInventoryList(uiHandInv, uiHandListName, {start: uiHandIndex, count: 1, interactive: false}));
  el.children[0].children[0].className += " uiHand_square";
  
  el.style.display = "block";
  uiHandVisible = true;
}
function uiHideHand() {
  var el = document.getElementById("uiHand");
  el.style.display = "none";
  while(el.firstChild) { el.removeChild(el.firstChild); }
  uiHandVisible = false;
}
function uiUpdateHand() {
  if(uiHandVisible) {
    var el = document.getElementById("uiHand");
    uiUpdateInventoryList(uiHandInv, uiHandListName, {start: uiHandIndex, count: 1, interactive: false}, el);
    
    el.style.left = api.mouse.x + "px";
    el.style.top = api.mouse.y + "px";
  }
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
  
  if(doUnlock) { controls.unlock(); }
}
function uiHideWindow(doLock=true) {
  if(uiWindowOpen) {
    var el = document.getElementById("uiWindowContainer");
    el.style.display = "none";
    while(el.firstChild) { el.removeChild(el.firstChild); }
    uiHideHand();
    uiWindowOpen = false;
    
    if(doLock) { controls.lock(); }
  }
}

api.uiRenderInventoryList = uiRenderInventoryList;
api.uiUpdateInventoryList = uiUpdateInventoryList;

api.uiShowHand = uiShowHand;
api.uiShowWindow = uiShowWindow;
api.uiHideWindow = uiHideWindow;
api.UIWindow = UIWindow;

api.registerHUD = function(dom) {
  document.body.appendChild(dom);
};
