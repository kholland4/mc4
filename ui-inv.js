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

function uiRenderItemStack(stack) {
  var def = stack.getDef();
  
  var container = document.createElement("div");
  container.className = "uiItemStack";
  if(def.desc != "" && def.desc != null) {
    container.title = def.desc;
  }
  
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
    var amt = stack.wear / stack.getDef().toolWear;
    
    if(amt < 1) {
      var bar = document.createElement("div");
      bar.className = "uiIS_bar";
      bar.style.width = (amt * 90).toFixed(0) + "%";
      container.appendChild(bar);
    }
  }
  
  return container;
}

function uiRenderInventoryList(ref, kwargs) {
  var list = server.invGetList(ref);
  
  var args = Object.assign({start: 0, count: list.length, width: list.length, interactive: true, scrollHeight: null}, kwargs);
  
  var container = document.createElement("div");
  container.className = "uiInvList";
  container.dataset.inv = JSON.stringify({
    ref: ref,
    args: kwargs
  });
  
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
        uiInteractInventoryList(this.ref.cloneWithIndex(this.index), "leftclick");
      }.bind({ref: ref, index: i, args: args, container: container});
      sq.oncontextmenu = function(e) {
        e.preventDefault();
        uiInteractInventoryList(this.ref.cloneWithIndex(this.index), "rightclick");
      }.bind({ref: ref, index: i, args: args, container: container});
    }
    
    container.appendChild(sq);
    
    if(i % args.width == args.width - 1) {
      container.appendChild(document.createElement("br"));
    }
  }
  
  if(args.scrollHeight != null) {
    //Workaround for Firefox's scrollbar placement
    var inner = container;
    container = document.createElement("div");
    container.style.paddingRight = "18px";
    container.appendChild(inner);
    var width = 66;
    inner.style.width = (width * args.width) + "px";
    
    container.style.overflowY = "auto";
    
    /*var sqSample = document.createElement("div");
    sqSample.className = "uiIL_square";
    sqSample.style.visibility = "hidden";
    sqSample.style.position = "fixed";
    document.body.appendChild(sqSample);
    var height = sqSample.getBoundingClientRect().height;
    document.body.removeChild(sqSample);*/
    var height = 66;
    
    container.style.maxHeight = (args.scrollHeight * height) + "px";
  }
  
  return container;
}

//FIXME: check that container.children.length == args.count
function uiUpdateInventoryList(ref, kwargs, container) {
  var list = server.invGetList(ref);
  
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

function uiInteractInventoryList(ref, type="leftclick") {
  var handRef = new InvRef("player", null, "hand", 0);
  
  var handStack = server.invGetStack(handRef);
  var targetStack = server.invGetStack(ref);
  
  var typeMatch = false;
  var def;
  if(handStack != null && targetStack != null) {
    if(targetStack.typeMatch(handStack)) {
      typeMatch = true;
      def = targetStack.getDef();
    }
  }
  
  if(type == "leftclick") {
    if(typeMatch && def.stackable) {
      if(targetStack.count < targetStack.getDef().maxStack) {
        var amt = Math.min(handStack.count, targetStack.getDef().maxStack - targetStack.count);
        server.invDistribute(ref, targetStack.count + amt, handRef, handStack.count - amt);
      } else {
        server.invSwap(ref, handRef);
      }
    } else {
      server.invSwap(ref, handRef);
    }
  } else if(type == "rightclick") {
    if(typeMatch && def.stackable) {
      if(targetStack.count < targetStack.getDef().maxStack) {
        var amt = 1;
        server.invDistribute(ref, targetStack.count + amt, handRef, handStack.count - amt);
      } else {
        server.invSwap(ref, handRef);
      }
    } else if(targetStack == null && handStack != null) {
      if(handStack.getDef().maxStack > 1 && handStack.getDef().stackable) {
        var amt = 1;
        server.invDistribute(ref, amt, handRef, handStack.count - amt);
      } else {
        server.invSwap(ref, handRef);
      }
    } else if(targetStack != null && handStack == null) {
      if(targetStack.count > 1 && targetStack.getDef().stackable) {
        var amt = Math.ceil(targetStack.count / 2);
        server.invDistribute(ref, targetStack.count - amt, handRef, amt);
      } else {
        server.invSwap(ref, handRef);
      }
    } else {
      server.invSwap(ref, handRef);
    }
  }
}

//---
api.registerOnFrame(uiUpdateHand);
//---

var uiHandVisible = false;
var uiHandRef;

function uiShowHand(ref) {
  uiHandRef = ref;
  
  var el = document.getElementById("uiHand");
  while(el.firstChild) { el.removeChild(el.firstChild); }
  el.appendChild(uiRenderInventoryList(uiHandRef, {start: uiHandRef.index, count: 1, interactive: false}));
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
    uiUpdateInventoryList(uiHandRef, {start: uiHandRef.index, count: 1, interactive: false}, el);
    
    el.style.left = api.mouse.x + "px";
    el.style.top = api.mouse.y + "px";
  }
}


api.uiRenderInventoryList = uiRenderInventoryList;
api.uiUpdateInventoryList = uiUpdateInventoryList;
api.uiShowHand = uiShowHand;
