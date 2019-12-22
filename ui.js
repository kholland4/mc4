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
      
    }
    
    container.appendChild(sq);
  }
  
  return container;
}

//FIXME: check that container.children.length == args.count
function uiUpdateInventoryList(inv, listName, kwargs, container) {
  var list = inv.getList(listName);
  
  var args = Object.assign({start: 0, count: list.length, width: list.length, interactive: true}, kwargs);
  
  for(var i = args.start; i < args.start + args.count; i++) {
    var sq = container.children[i];
    
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

api.uiRenderInventoryList = uiRenderInventoryList;
api.uiUpdateInventoryList = uiUpdateInventoryList;

api.registerHUD = function(dom) {
  document.body.appendChild(dom);
};
