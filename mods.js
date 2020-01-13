var allModMeta = {};

function getModMeta(name) {
  if(name in allModMeta) {
    return allModMeta[name];
  }
  
  return null;
}

function modsLoaded() {
  for(var key in allModMeta) {
    if(!allModMeta[key].loaded && !allModMeta[key].error) { //FIXME
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
    this.error = false;
  }
}

function loadMod(meta) {
  allModMeta[meta.name] = meta;
  
  var script = document.createElement("script");
  script.dataset.name = meta.name;
  script.onload = function() {
    allModMeta[this.dataset.name].loaded = true;
  };
  script.onerror = function() {
    var meta = allModMeta[this.dataset.name];
    debug("loader", "error", "unable to load mod '" + meta.path + "' as '" + meta.name + "'");
    meta.error = true;
  };
  script.src = meta.path + "/init.js";
  document.head.appendChild(script);
}

api.getModMeta = getModMeta;
