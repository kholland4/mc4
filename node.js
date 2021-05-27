/*
    mc4, a web voxel building game
    Copyright (C) 2019-2021 kholland4

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

var allNodes = {};

function getNodeDef(itemstring) {
  if(itemstring in allNodes) {
    return allNodes[itemstring];
  } else {
    return allNodes["unknown"];
  }
}

class NodeBase {
  constructor(itemstring, props) {
    this.itemstring = itemstring;
    
    this.desc = "";
    
    this.walkable = false;
    this.ladderlike = false;
    this.boundingBox = null;
    this.meshBox = null;
    
    this.visible = true;
    this.customMesh = false;
    //this.customMeshVerts = null;
    //this.customMeshUVs = null;
    this.transparent = false;
    this.reallyTransparent = false;
    this.renderAdj = false;
    //this.transFaces = [true, true, true, true, true, true]; //not yet used consistently for performance reasons
    this.passSunlight = false;
    this.lightLevel = 0;
    this.joined = false;
    this.faceIsRecessed = [null, null, null, null, null, null]; //true: always sunkLit, false: never sunkLit, null: no preference (renderer decides)
    this.useTint = true; //turn off for flowers and such
    this.connectingMesh = false;
    this.meshConnectsTo = [];
    this.connectingVerts = null; //6 sets (-X, +X, -Y, +Y, -Z, +Z) of 6 faces each; a set can be null; a given set will be shown if the node connects in that direction
    this.connectingUVs = null;
    
    //The 'rot' property of a node could be used to store non-rotation data (i. e. fluid information, color, etc.)
    //'rotDataType' will be set automatically based on other properties
    this.rotDataType = "rot";
    this.setRotOnPlace = false;
    this.limitRotOnPlace = false;
    this.isFluid = false;
    this.fluidOverlayColor = false;
    
    this.tex = [null, null, null, null, null, null];
    this.texAll = null;
    this.texTop = null;
    this.texBottom = null;
    this.texSides = null;
    this.texFront = null;
    this.texBack = null;
    this.texLeft = null;
    this.texRight = null;
    this.fluidTexLoc = null;
    
    this.drops = null;
    this.breakable = true;
    this.canPlaceInside = false;
    
    this.groups = {};
    
    Object.assign(this, props);
    
    if(props.isFluid) {
      this.rotDataType = "fluid";
      this.fluidTexLoc = textureLoc(this.texAll);
    }
    
    //---
    
    if(!("passSunlight" in props)) { this.passSunlight = this.transparent; }
    if(!("renderAdj" in props)) { this.renderAdj = this.transparent; }
    
    if(this.meshBox == null) {
      this.meshBox = this.boundingBox;
    }
    
    if(!("reallyTransparent" in props)) { this.reallyTransparent = !this.visible || (this.transparent && this.walkable); }
    
    //---
    
    //TODO check for nonexistent textures?
    
    
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
    if(this.texFront != null) {
      this.tex[0] = this.texFront;
    }
    if(this.texBack != null) {
      this.tex[1] = this.texBack;
    }
    if(this.texLeft != null) {
      this.tex[4] = this.texLeft;
    }
    if(this.texRight != null) {
      this.tex[5] = this.texRight;
    }
    
    var texLoc = [];
    for(var i = 0; i < 6; i++) {
      texLoc.push(textureLoc(this.tex[i]));
    }
    
    if(this.customMesh) {
      this.uvs = [];
      
      function scale(n, min1, max1, min2, max2) {
        return (n - min1) * (max2 - min2) / (max1 - min1) + min2;
      }
      
      for(var i = 0; i < 6; i++) { //i < this.customMeshUVs.length; i++) {
        var faceUVs = this.customMeshUVs[i];
        var loc = texLoc[i];
        var s = [];
        for(var n = 0; n < faceUVs.length; n += 2) {
          s.push(scale(faceUVs[n], 0, 1, loc[0], loc[2]));
          s.push(scale(faceUVs[n + 1], 0, 1, loc[1], loc[3]));
        }
        this.uvs.push(s);
      }
    } else {
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
    
    if(this.connectingUVs != null) {
      function scale(n, min1, max1, min2, max2) {
        return (n - min1) * (max2 - min2) / (max1 - min1) + min2;
      }
      
      for(var f = 0; f < 6; f++) {
        if(this.connectingUVs[f] == null) { continue; }
        
        for(var i = 0; i < 6; i++) {
          var faceUVs = this.connectingUVs[f][i];
          var loc = texLoc[i];
          for(var n = 0; n < faceUVs.length; n += 2) {
            faceUVs[n] = scale(faceUVs[n], 0, 1, loc[0], loc[2]);
            faceUVs[n + 1] = scale(faceUVs[n + 1], 0, 1, loc[1], loc[3]);
          }
        }
      }
    }
  }
  
  rotateBoundingBox(euler) {
    if(this.boundingBox == null) {
      return null;
    }
    var mat4 = new THREE.Matrix4().makeRotationFromEuler(euler);
    var out = [];
    for(var i = 0; i < this.boundingBox.length; i++) {
      var box = this.boundingBox[i].clone();
      box.applyMatrix4(mat4);
      out.push(box);
    }
    return out;
  }
  
  rotateMeshBox(euler) {
    if(this.meshBox == null) {
      return null;
    }
    var mat4 = new THREE.Matrix4().makeRotationFromEuler(euler);
    var out = [];
    for(var i = 0; i < this.meshBox.length; i++) {
      var box = this.meshBox[i].clone();
      box.applyMatrix4(mat4);
      out.push(box);
    }
    return out;
  }
}

class Node extends NodeBase {
  constructor(itemstring, props) {
    super(itemstring, props);
  }
}

class NodeData {
  constructor(itemstring, rot=0) {
    this.itemstring = itemstring;
    this.rot = rot;
  }
  
  getDef() {
    return getNodeDef(this.itemstring);
  }
  
  static fromItemStack(stack) {
    if(stack == null) { return null; }
    return new NodeData(stack.itemstring);
  }
}

//---TOOLS---
//  most of this is based on MTG
//
//  groups: crumbly, cracky, choppy, snappy, oddly_breakable_by_hand, fleshy, explody
//  non-tool groups: soil, sand
//
//  levels:
//      crumbly        cracky                      choppy        snappy
//
//  1   dirt, sand     stone, cobble               planks        leaves
//  2   gravel         ore, brick                  tree
//  3   sandstone      hard ore, mineral blocks    
//  4   
//  5   
//
//  tools:
//            crumbly          cracky           choppy           snappy
//  wood      0.6, 1.2, 1.8    1.6, x, x        1.6, 3.0, x      0.4, 1.6, x
//  stone     0.5, 1.2, 1.8    1.0, 2.0, x      1.3, 2.0, 3.0    0.4, 1.4, x
//  steel     0.4, 0.9, 1.5    0.8, 1.6, 4.0    1.0, 1.4, 2.5    0.3, 1.2, 2.0
//  diamond   0.3, 0.5, 1.1    0.5, 1.0, 2.0    0.5, 0.9, 2.1    0.3, 0.9, 1.8
//------

function calcDigTimeActual(node, tool) {
  if(tool == null) { return null; }
  var group = null;
  var nodeDef = node.getDef();
  if(!nodeDef.breakable) { return null; }
  toolDef = tool.getDef();
  
  if(!toolDef.isTool) { return null; }
  
  var groupMatch = null;
  for(var toolGroup in toolDef.toolGroups) {
    for(var nodeGroup in nodeDef.groups) {
      if(toolGroup == nodeGroup) {
        groupMatch = toolGroup;
        break;
      }
    }
    if(groupMatch != null) { break; }
  }
  
  if(groupMatch == null) { return null; }
  
  var nodeLevel = nodeDef.groups[groupMatch];
  var toolInfo = toolDef.toolGroups[groupMatch];
  
  if(nodeLevel > toolInfo.maxlevel) { return null; }
  
  return toolInfo.times[nodeLevel];
}

function calcDigTime(node, tool) {
  var digTime = calcDigTimeActual(node, tool);
  if(digTime != null) { return digTime; }
  
  if(getItemDef(":") == null) {
    var noToolDef = new Item(":", {isTool: true, toolWear: null, toolGroups: {
      crumbly: {times: [0, 1, 2], maxlevel: 2},
      choppy: {times: [0, 2, 4], maxlevel: 2},
      snappy: {times: [0, 1], maxlevel: 1},
      oddly_breakable_by_hand: {times: [0, 0.2, 0.5], maxlevel: 2}
    }});
    
    api.registerItem(noToolDef);
  }
  
  return calcDigTimeActual(node, new ItemStack(":"));
}

function useTool(node, invRef) {
  if(api.server.isRemote())
    return;
  
  var tool = server.invGetStack(invRef);
  var digTime = calcDigTimeActual(node, tool);
  if(digTime != null) {
    if(tool.wear != null) {
      tool.wear--;
      if(tool.wear <= 0) { tool = null; }
      server.invSetStack(invRef, tool);
    }
  }
}

api.Node = Node;
api.NodeData = NodeData;
api.registerNode = function(item) {
  //TODO: validation
  allNodes[item.itemstring] = item;
};
api.getNodeDef = getNodeDef;
