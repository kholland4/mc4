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

importScripts("geometry.js");

onmessage = function(e) {
  var pos = e.data.pos;
  var size = e.data.size;
  var nodeDef = e.data.nodeDef;
  var nodeDefAdj = e.data.nodeDefAdj;
  var unknownDef = e.data.unknownDef;
  var data = e.data.data;
  var lightNeedsUpdate = e.data.lightNeedsUpdate;
  var sunAmount = e.data.sunAmount;
  //var ldata = e.data.ldata;
  
  var verts = [];
  var uvs = [];
  var colors = [];
  var facePos = [];
  
  for(var x = 0; x < size.x + 2; x++) {
    for(var y = 0; y < size.y + 2; y++) {
      for(var z = 0; z < size.z + 2; z++) {
        var d = data[x][y][z];
        if(d == -1) { continue; }
        var id = d & 32767;
        var rot = (d >> 15) & 255;
        var lightRaw = (d >> 23) & 255;
        var light = Math.max(lightRaw & 15, ((lightRaw >> 4) & 15) * sunAmount);
        light = Math.max(light, 1);
        
        var def;
        if(x < 1) { def = nodeDefAdj["-1,0,0"][id]; } else
        if(y < 1) { def = nodeDefAdj["0,-1,0"][id]; } else
        if(z < 1) { def = nodeDefAdj["0,0,-1"][id]; } else
        if(x >= size.x + 1) { def = nodeDefAdj["1,0,0"][id]; } else
        if(y >= size.y + 1) { def = nodeDefAdj["0,1,0"][id]; } else
        if(z >= size.z + 1) { def = nodeDefAdj["0,0,1"][id]; } else
        { def = nodeDef[id]; }
        if(def == undefined) { def = unknownDef; }
        if(!def.visible) { continue; }
        
        for(var faceIndex = 0; faceIndex < 6; faceIndex++) {
          var face = stdFaces[faceIndex];
          var rx = x + face.x; var ry = y + face.y; var rz = z + face.z;
          if(rx < 0 || rx >= size.x + 2) { continue; }
          if(ry < 0 || ry >= size.y + 2) { continue; }
          if(rz < 0 || rz >= size.z + 2) { continue; }
          var relD = data[rx][ry][rz];
          if(relD == -1) { continue; }
          var relID = relD & 32767;
          var relLightRaw = (relD >> 23) & 255;
          var relLight = Math.max(relLightRaw & 15, Math.round(((relLightRaw >> 4) & 15) * sunAmount));
          relLight = Math.max(relLight, 1);
          //relLight = Math.max(relLight, def.lightLevel);
          //FIXME - not desired behavior but needed to accomodate renderUpdateLighting
          var tLight = false;
          if(def.lightLevel > 0) { relLight = light; tLight = true; }
          
          var relDef;
          if(rx < 1) { relDef = nodeDefAdj["-1,0,0"][relID]; } else
          if(ry < 1) { relDef = nodeDefAdj["0,-1,0"][relID]; } else
          if(rz < 1) { relDef = nodeDefAdj["0,0,-1"][relID]; } else
          if(rx >= size.x + 1) { relDef = nodeDefAdj["1,0,0"][relID]; } else
          if(ry >= size.y + 1) { relDef = nodeDefAdj["0,1,0"][relID]; } else
          if(rz >= size.z + 1) { relDef = nodeDefAdj["0,0,1"][relID]; } else
          { relDef = nodeDef[relID]; }
          if(relDef == undefined) { relDef = unknownDef; }
          
          if(!def.transparent && !relDef.transparent) { continue; }
          //if(!def.renderAdj && !relDef.renderAdj) { continue; }
          
          //joined nodes (ie water)
          if(def.itemstring == relDef.itemstring && def.joined) { continue; }
          
          //TODO: use transFaces?
          //if(def.transparent && def.transFaces[faceIndex] && light > relLight) { relLight = light; tLight = true; }
          if(def.transparent && light > relLight) { relLight = light; tLight = true; }
          
          //Only show faces that are lit by a node in the current mapblock.
          //Exclude self-lit nodes outside of the current mapblock.
          if(tLight && (x < 1 || y < 1 || z < 1 || x >= size.x + 1 || y >= size.y + 1 || z >= size.z + 1)) { continue; }
          //Exclude non-self-lit nodes where the current face is lit by a node outside of the current mapblock.
          if(!tLight && (rx < 1 || ry < 1 || rz < 1 || rx >= size.x + 1 || ry >= size.y + 1 || rz >= size.z + 1)) { continue; }
          
          var tint = 1;
          if(faceIndex == 3) { tint = 1; } else
          if(faceIndex == 2) { tint = 0.6; } else
          if(faceIndex == 0 || faceIndex == 1) { tint = 0.75; } else
          { tint = 0.875; }
          
          var colorR = Math.round(lightCurve[relLight] * tint);
          var colorG = colorR;
          var colorB = colorR;
          
          var arr;
          if(def.customMesh) {
            arr = def.customMeshVerts[faceIndex];
          } else {
            arr = stdVerts[faceIndex];
          }
          for(var i = 0; i < arr.length; i += 3) {
            verts.push(arr[i] + (x - 1));
            verts.push(arr[i + 1] + (y - 1));
            verts.push(arr[i + 2] + (z - 1));
            
            var adjList = [];
            var vertX = arr[i];
            var vertY = arr[i + 1];
            var vertZ = arr[i + 2];
            var xDiff = vertX >= 0 ? 1 : -1;
            var yDiff = vertY >= 0 ? 1 : -1;
            var zDiff = vertZ >= 0 ? 1 : -1;
            if(face.x == 1 || face.x == -1) {
              adjList.push([rx - 1, ry - 1 + yDiff, rz - 1]);
              adjList.push([rx - 1, ry - 1, rz - 1 + zDiff]);
              adjList.push([rx - 1, ry - 1 + yDiff, rz - 1 + zDiff]);
            } else if(face.y == 1 || face.y == -1) {
              adjList.push([rx - 1 + xDiff, ry - 1, rz - 1]);
              adjList.push([rx - 1, ry - 1, rz - 1 + zDiff]);
              adjList.push([rx - 1 + xDiff, ry - 1, rz - 1 + zDiff]);
            } else if(face.z == 1 || face.z == -1) {
              adjList.push([rx - 1, ry - 1 + yDiff, rz - 1]);
              adjList.push([rx - 1 + xDiff, ry - 1, rz - 1]);
              adjList.push([rx - 1 + xDiff, ry - 1 + yDiff, rz - 1]);
            }
            
            //FIXME - see above
            if(tLight) {
              colors.push(colorR);
              colors.push(colorG);
              colors.push(colorB);
              
              facePos.push([x - 1, y - 1, z - 1, tint, true, null]);
            } else {
              var total = lightCurve[relLight] * tint;
              var count = 1;
              
              for(var n = 0; n < adjList.length; n++) {
                if(adjList[n][0] >= -1 && adjList[n][0] < size.x + 1 &&
                   adjList[n][1] >= -1 && adjList[n][1] < size.y + 1 &&
                   adjList[n][2] >= -1 && adjList[n][2] < size.z + 1) {
                  var adjD = data[adjList[n][0] + 1][adjList[n][1] + 1][adjList[n][2] + 1];
                  if(adjD == -1) { continue; }
                  var adjLightRaw = (adjD >> 23) & 255;
                  var adjLight = Math.max(adjLightRaw & 15, Math.round(((adjLightRaw >> 4) & 15) * sunAmount));
                  if(adjLight > 0 && adjLight < relLight - 2) { continue; }
                  if(adjLight > 0 && relLight < adjLight - 2) { continue; }
                  if(adjLight == 0) { adjLight = Math.floor(relLight / 3); }
                  adjLight = Math.max(adjLight, 1);
                  
                  total += lightCurve[adjLight] * tint;
                  count++;
                }
              }
              
              var avgLight = Math.round(total / count);
              colorR = avgLight;
              colorG = colorR;
              colorB = colorR;
              
              colors.push(colorR);
              colors.push(colorG);
              colors.push(colorB);
              
              facePos.push([rx - 1, ry - 1, rz - 1, tint, false, adjList]);
            }
          }
          uvs.push.apply(uvs, def.uvs[faceIndex]);
        }
      }
    }
  }
  
  var res = {
    verts: verts,
    uvs: uvs,
    colors: colors,
    facePos: facePos
  }
  /*if(lightNeedsUpdate) {
    res.data = data;
  }*/
  this.postMessage(res);
};
