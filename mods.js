var allModMeta = {};

function getModMeta(name) {
  if(name in allModMeta) {
    return allModMeta[name];
  }
  
  return null;
}

function modsLoaded() {
  for(var key in allModMeta) {
    if(!allModMeta[key].loaded) {
      return false;
    }
  }
  return true;
}

class ModMeta {
  constructor(name, path) {
    this.name = name;
    this.path = path;
    this.loaded = false;
  }
}

function loadMod(name, path) {
  var meta = new ModMeta(name, path);
  allModMeta[name] = meta;
  
  var script = document.createElement("script");
  script.dataset.name = name;
  script.onload = function() {
    allModMeta[this.dataset.name].loaded = true;
  };
  script.src = path + "/init.js";
  document.head.appendChild(script);
}

api.getModMeta = getModMeta;
