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
    
    api.uiShowWindow(win);
    menuIsOpen = true;
  }
}
