var texmap;
var textureCanvas;
var textureCtx;
var texmapWidth = 32;
var texmapHeight = 32;

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
    if(!allTextures[i].loaded) {
      return false;
    }
  }
  return true;
}

class TextureImage {
  constructor(name, file) {
    this.name = name;
    this.img = document.createElement("img");
    this.img.src = file;
    this.img.onload = this.onload.bind(this);
    this.loaded = false;
    this.w = 0;
    this.h = 0;
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
  
  updateTextureMap();
}

function loadTexture(name, file) {
  if(textureCanvas == undefined) { initTextures(); }
  
  allTextures.push(new TextureImage(name, file));
  
  var index = allTextures.length - 1;
  
  var x = (index % texmapWidth) * TEXTURE_SIZE.x;
  var y = Math.floor(index / texmapWidth) * TEXTURE_SIZE.y;
  var w = textureCanvas.width;
  var h = textureCanvas.height;
  textureCoords[name] = [x / w, 1 - ((y + TEXTURE_SIZE.y) / h), (x + TEXTURE_SIZE.x) / w, 1 - (y / h)];
}

var TEXTURE_SIZE = new THREE.Vector2(16, 16);

function updateTextureMap() {
  var textures = allTextures;
  
  var w = textureCanvas.width;
  var h = textureCanvas.height;
  
  textureCtx.fillStyle = "#ff00ff";
  textureCtx.fillRect(0, 0, w, h);
  
  for(var i = 0; i < textures.length; i++) {[];
    if(!textures[i].loaded || !textures[i].w == TEXTURE_SIZE.x || !textures[i].h == TEXTURE_SIZE.y) { continue; }
    
    var x = i % texmapWidth;
    var y = Math.floor(i / texmapWidth);
    textureCtx.drawImage(textures[i].img, x * TEXTURE_SIZE.x, y * TEXTURE_SIZE.y);
  }
  
  texmap.needsUpdate = true;
}

api.loadTexture = loadTexture;
