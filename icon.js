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

var allIcons = {};

function getIcon(name) {
  if(name in allIcons) {
    //if(allIcons[name].loaded) {
      return allIcons[name];
    //}
  }
  
  return null;
}

function iconsLoaded() {
  for(var key in allIcons) {
    if(!allIcons[key].loaded && !allIcons[key].error) { //FIXME
      return false;
    }
  }
  return true;
}

function iconExists(file) {
  for(var key in allIcons) {
    if(allIcons[key].file == file) { //FIXME
      return key;
    }
  }
  return false;
}

class Icon {
  constructor(name, file) {
    this.name = name;
    this.file = file;
    this.img = document.createElement("img");
    this.img.src = file;
    this.img.onload = this.onload.bind(this);
    this.img.onerror = function() {
      debug("loader", "warning", "unable to load '" + this.file + "' as icon '" + this.name + "'");
      this.error = true;
      this.img.src = "textures/unknown_item.png";
    }.bind(this);
    this.loaded = false;
    this.error = false;
  }
  
  onload() {
    this.loaded = true;
  }
}

function loadIcon(name, file) {
  allIcons[name] = new Icon(name, file);
  return name;
}
