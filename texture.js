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

var texmap;
var textureCanvas;
var textureCtx;
var texmapWidth = 32;
var texmapHeight = 32;
var texmapUsed;

var allTextures = [];
var textureCoords = {};

function textureLoc(name) {
  if(name in textureCoords) {
    return textureCoords[name];
  } else {
    return [31 / 32, 31 / 32, 1.0, 1.0];
  }
}

function texturesLoaded() {
  for(var i = 0; i < allTextures.length; i++) {
    if(!allTextures[i].loaded && !allTextures[i].error) { //FIXME
      return false;
    }
  }
  return true;
}

function textureExists(file) {
  for(var i = 0; i < allTextures.length; i++) {
    if(allTextures[i].file == file) { //FIXME
      return allTextures[i].name;
    }
  }
  return false;
}

function getTexture(name) {
  for(var i = 0; i < allTextures.length; i++) {
    if(allTextures[i].name == name) {
      return allTextures[i];
    }
  }
  return false;
}

class TextureImage {
  constructor(name, file) {
    this.name = name;
    this.file = file;
    this.img = document.createElement("img");
    this.img.src = file;
    this.img.onload = this.onload.bind(this);
    this.img.onerror = function() {
      debug("loader", "warning", "unable to load '" + this.file + "' as texture '" + this.name + "'");
      this.error = true;
    }.bind(this);
    this.loaded = false;
    this.error = false;
    this.w = 0;
    this.h = 0;
    this.x = null;
    this.y = null;
    this.allocX = 1;
    this.allocY = 1;
  }
  
  onload() {
    this.loaded = true;
    this.w = this.img.width;
    this.h = this.img.height;
    updateTextureMap();
  }
}

function initTextures() {
  if(textureCanvas != undefined) { return; }
  
  textureCanvas = document.createElement("canvas");
  //var texmapWidth = Math.ceil(Math.sqrt(textures.length));
  textureCanvas.width = texmapWidth * TEXTURE_SIZE.x;
  textureCanvas.height = texmapHeight * TEXTURE_SIZE.y;
  textureCtx = textureCanvas.getContext("2d");
  
  texmap = new THREE.CanvasTexture(textureCanvas);
  texmap.minFilter = THREE.NearestFilter;
  texmap.magFilter = THREE.NearestFilter;
  //texmap.encoding = THREE.sRGBEncoding;
  
  texmapUsed = [];
  for(var x = 0; x < texmapWidth; x++) {
    texmapUsed.push(Array(texmapHeight).fill(false));
  }
  
  updateTextureMap();
}

function loadTexture(name, file, allocX=1, allocY=1) {
  if(textureCanvas == undefined) { initTextures(); }
  
  var indexX = null;
  var indexY = null;
  for(var searchX = 0; searchX <= texmapWidth - allocX; searchX++) {
    for(var searchY = 0; searchY <= texmapHeight - allocY; searchY++) {
      var found = true;
      for(var cx = 0; cx < allocX; cx++) {
        for(var cy = 0; cy < allocY; cy++) {
          if(texmapUsed[searchX + cx][searchY + cy]) {
            found = false;
            break;
          }
        }
        if(!found)
          break;
      }
      
      if(found) {
        indexX = searchX;
        indexY = searchY;
        break;
      }
    }
    if(indexX != null)
      break;
  }
  
  if(indexX == null)
    throw new Error("out of room on texture map; can't fit " + name + " (from " + file + ") which is " + allocX + "x" + allocY + " units");
  
  for(var cx = 0; cx < allocX; cx++) {
    for(var cy = 0; cy < allocY; cy++) {
      texmapUsed[indexX + cx][indexY + cy] = true;
    }
  }
  
  var x = indexX * TEXTURE_SIZE.x;
  var y = indexY * TEXTURE_SIZE.y;
  var w = textureCanvas.width;
  var h = textureCanvas.height;
  // +/- 0.0625 is a workaround for texture flickering
  textureCoords[name] = [(x + 0.0625) / w, 1 - ((y + TEXTURE_SIZE.y - 0.0625) / h), (x + TEXTURE_SIZE.x - 0.0625) / w, 1 - ((y + 0.0625) / h)];
  
  var texImg = new TextureImage(name, file);
  texImg.x = x;
  texImg.y = y;
  texImg.allocX = allocX;
  texImg.allocY = allocY;
  allTextures.push(texImg);
}

var TEXTURE_SIZE = new THREE.Vector2(16, 16);

function updateTextureMap() {
  var textures = allTextures;
  
  var w = textureCanvas.width;
  var h = textureCanvas.height;
  
  textureCtx.fillStyle = "#ff00fe";
  textureCtx.fillRect(0, 0, w, h);
  
  for(var i = 0; i < textures.length; i++) {
    if(!textures[i].loaded || textures[i].w > textures[i].allocX * TEXTURE_SIZE.x || textures[i].h > textures[i].allocY * TEXTURE_SIZE.y)
      continue;
    
    var x = textures[i].x;
    var y = textures[i].y;
    textureCtx.drawImage(textures[i].img, x, y);
    
    //transparency
    var lx = x;
    var ly = y;
    var lw = textures[i].w;
    var lh = textures[i].h;
    
    var imgData = textureCtx.getImageData(lx, ly, lw, lh);
    
    //FIXME - sets pixels with #ff00fe color to transparent, should really check for alpha in source image
    var pixelData = imgData.data;
    for(var n = 0; n < pixelData.length; n += 4) {
      var r = pixelData[n];
      var g = pixelData[n + 1];
      var b = pixelData[n + 2];
      
      if(r == 255 && g == 0 && b == 254) {
        pixelData[n + 2] = 0;
        pixelData[n + 3] = 0;
      } else {
        pixelData[n + 3] = 255;
      }
    }
    
    textureCtx.putImageData(imgData, lx, ly);
  }
  
  texmap.needsUpdate = true;
}

api.loadTexture = loadTexture;
api.textureExists = textureExists;
