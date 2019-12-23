var allNodes = {};

function getNodeDef(itemstring) {
  return allNodes[itemstring];
}

class NodeBase {
  constructor(itemstring, props) {
    this.itemstring = itemstring;
    
    this.desc = "";
    
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
    
    this.groups = {};
    
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

var noToolDef = new Item(":", {isTool: true, toolWear: null, toolGroups: {
  cracky: {times: [0, 2], maxlevel: 1},
  crumbly: {times: [0, 1.5, 3], maxlevel: 2}
}});
//---
api.registerItem(noToolDef);
//---

function calcDigTimeActual(node, tool) {
  if(tool == null) { return null; }
  var group = null;
  var nodeDef = node.getDef();
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
  
  return calcDigTimeActual(node, new ItemStack(":"));
}

function useTool(node, inv, listName, index) {
  var tool = inv.getStack(listName, index);
  var digTime = calcDigTimeActual(node, tool);
  if(digTime != null) {
    if(tool.wear != null) {
      tool.wear--;
      if(tool.wear <= 0) { tool = null; }
      inv.setStack(listName, index, tool);
    }
  }
}

api.Node = Node;
api.NodeData = NodeData;
api.registerNode = function(item) {
  //TODO: validation
  allNodes[item.itemstring] = item;
};
