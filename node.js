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
    //this.customMeshVerts = null;
    //this.customMeshUVs = null;
    this.transparent = false;
    this.renderAdj = false;
    this.transFaces = [true, true, true, true, true, true]; //not yet used consistently for performance reasons
    this.passSunlight = false;
    this.lightLevel = 0;
    
    this.tex = [null, null, null, null, null, null];
    this.texAll = null;
    this.texTop = null;
    this.texBottom = null;
    this.texSides = null;
    
    this.drops = null;
    
    this.groups = {};
    
    Object.assign(this, props);
    
    //---
    
    if(!("passSunlight" in props)) { this.passSunlight = this.transparent; }
    if(!("renderAdj" in props)) { this.renderAdj = this.transparent; }
    
    //---
    
    if(this.customMesh) {
      this.uvs = [];
      var loc = textureLoc(this.texAll);
      
      function scale(n, min1, max1, min2, max2) {
        return (n - min1) * (max2 - min2) / (max1 - min1) + min2;
      }
      
      for(var i = 0; i < this.customMeshUVs.length; i++) {
        var faceUVs = this.customMeshUVs[i];
        var s = [];
        for(var n = 0; n < faceUVs.length; n += 2) {
          s.push(scale(faceUVs[n], 0, 1, loc[0], loc[2]));
          s.push(scale(faceUVs[n + 1], 0, 1, loc[1], loc[3]));
        }
        this.uvs.push(s);
      }
    } else {
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
      crumbly: {times: [0, 2], maxlevel: 1},
      cracky: {times: [0, 2], maxlevel: 1},
      choppy: {times: [0, 2], maxlevel: 1},
      snappy: {times: [0, 2], maxlevel: 1},
      oddly_breakable_by_hand: {times: [0, 1], maxlevel: 1}
    }});
    
    api.registerItem(noToolDef);
  }
  
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
api.getNodeDef = getNodeDef;
