/*
    mc4, a web voxel building game
    Copyright (C) 2019 kholland4

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
    this._onload = [];
  }
  
  registerOnload(f) {
    if(this.loaded) {
      f();
    } else {
      this._onload.push(f);
    }
  }
  
  onload() {
    this.loaded = true;
    for(var i = 0; i < this._onload.length; i++) {
      this._onload[i]();
    }
  }
}

function loadMod(meta) {
  allModMeta[meta.name] = meta;
  
  var script = document.createElement("script");
  script.dataset.name = meta.name;
  script.onload = function() {
    allModMeta[this.dataset.name].onload();
  };
  script.onerror = function() {
    var meta = allModMeta[this.dataset.name];
    debug("loader", "error", "unable to load mod '" + meta.path + "' as '" + meta.name + "'");
    meta.error = true;
  };
  script.src = meta.path + "/init.js?v=" + VERSION;
  document.head.appendChild(script);
}

//FIXME: allow passing an array of names
api.onModLoaded = function(name, f) {
  var meta = getModMeta(name);
  if(meta == null) { return false; }
  meta.registerOnload(f);
  return true;
};

api.getModMeta = getModMeta;
