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
  constructor(name, path, deps=null) {
    this.name = name;
    this.path = path;
    this.deps = [];
    if(deps != null) { this.deps = deps; }
    this.loaded = false;
  }
}

function loadMod(meta) {
  allModMeta[meta.name] = meta;
  
  var script = document.createElement("script");
  script.dataset.name = meta.name;
  script.onload = function() {
    allModMeta[this.dataset.name].loaded = true;
  };
  script.src = meta.path + "/init.js";
  document.head.appendChild(script);
}

api.getModMeta = getModMeta;
