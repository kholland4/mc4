var allNodes = {};

function getNodeDef(itemstring) {
  return allNodes[itemstring];
}

class NodeBase {
  constructor(itemstring, props) {
    this.itemstring = itemstring;
    
    this.walkable = false;
    this.boundingBox = null;
    
    this.visible = true;
    this.customMesh = false;
    //this.customMeshFaces = null;
    //this.customMeshUVs = null;
    this.transparent = false;
    
    this.tex = [null, null, null, null, null, null];
    this.texAll = null;
    this.texTop = null;
    this.texBottom = null;
    this.texSides = null;
    
    Object.assign(this, props);
    
    //---
    
    if(this.texAll != null) {
      for(var i = 0; i < 6; i++) {
        this.tex[i] = this.texAll;
      }
    }
    if(this.texTop != null) {
      this.tex[3] = this.texTop;
    }
    if(this.texBottom != null) {
      this.tex[2] = this.texBottom;
    }
    if(this.texSides != null) {
      this.tex[0] = this.texSides;
      this.tex[1] = this.texSides;
      this.tex[4] = this.texSides;
      this.tex[5] = this.texSides;
    }
    
    var texLoc = [];
    for(var i = 0; i < 6; i++) {
      texLoc.push(textureLoc(this.tex[i]));
    }
    
    this.uvs = [];
    if(this.visible) {
      for(var i = 0; i < 6; i++) {
        var faceUVs = [];
        var loc = texLoc[i];
        for(var n = 0; n < stdUVs.length; n += 2) {
          if(stdUVs[n] == 0.0) { faceUVs.push(loc[0]); } else { faceUVs.push(loc[2]); }
          if(stdUVs[n + 1] == 0.0) { faceUVs.push(loc[1]); } else { faceUVs.push(loc[3]); }
        }
        this.uvs.push(faceUVs);
      }
    }
  }
}

class Node extends NodeBase {
  constructor(itemstring, props) {
    super(itemstring, props);
  }
}

api.registerNode = function(item) {
  //TODO: validation
  allNodes[item.itemstring] = item;
};
