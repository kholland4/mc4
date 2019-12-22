var allIcons = {};

function getIcon(name) {
  if(name in allIcons) {
    if(allIcons[name].loaded) {
      return allIcons[name];
    }
  }
  
  return null;
}

function iconsLoaded() {
  for(var key in allIcons) {
    if(!allIcons[key].loaded) {
      return false;
    }
  }
  return true;
}

function iconExists(file) {
  for(var key in allIcons) {
    if(allIcons[key].img.src == file) {
      return key;
    }
  }
  return false;
}

class Icon {
  constructor(name, file) {
    this.name = name;
    this.img = document.createElement("img");
    this.img.src = file;
    this.img.onload = this.onload.bind(this);
    this.loaded = false;
  }
  
  onload() {
    this.loaded = true;
  }
}

function loadIcon(name, file) {
  allIcons[name] = new Icon(name, file);
  return name;
}
