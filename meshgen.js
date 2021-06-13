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

function generateMapblockMesh(dataIn) {
  var pos = dataIn.pos;
  var size = dataIn.size;
  var nodeDef = dataIn.nodeDef;
  var nodeDefAdj = dataIn.nodeDefAdj;
  var unknownDef = dataIn.unknownDef;
  var data = dataIn.data;
  var lightNeedsUpdate = dataIn.lightNeedsUpdate;
  var sun = dataIn.sunAmount;
  //var ldata = dataIn.ldata;
  
  var verts = [];
  var uvs = [];
  var colors = [];
  var facePos = [];
  
  var transparentVerts = [];
  var transparentUVs = [];
  var transparentColors = [];
  var transparentFacePos = [];
  
  for(var x = 0; x < size.x + 2; x++) {
    for(var y = 0; y < size.y + 2; y++) {
      for(var z = 0; z < size.z + 2; z++) {
        var d = data[x][y][z];
        if(d == -1) { continue; }
        var id = d & 32767;
        var rot = (d >> 15) & 255;
        var lightRaw = (d >> 23) & 255;
        var light = Math.max(lightRaw & 15, Math.round(((lightRaw >> 4) & 15) * sun));
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
        
        var useTransparent = def.itemstring == "default:water_source";
        
        var connectDirs = [false, false, false, false, false, false];
        if(def.connectingMesh) {
          for(var faceIndex = 0; faceIndex < 6; faceIndex++) {
            var face = stdFaces[faceIndex];
            var rx = x + face.x; var ry = y + face.y; var rz = z + face.z;
            if(rx < 0 || rx >= size.x + 2) { continue; }
            if(ry < 0 || ry >= size.y + 2) { continue; }
            if(rz < 0 || rz >= size.z + 2) { continue; }
            var relD = data[rx][ry][rz];
            if(relD == -1) { continue; }
            var relID = relD & 32767;
            
            var relDef;
            if(rx < 1) { relDef = nodeDefAdj["-1,0,0"][relID]; } else
            if(ry < 1) { relDef = nodeDefAdj["0,-1,0"][relID]; } else
            if(rz < 1) { relDef = nodeDefAdj["0,0,-1"][relID]; } else
            if(rx >= size.x + 1) { relDef = nodeDefAdj["1,0,0"][relID]; } else
            if(ry >= size.y + 1) { relDef = nodeDefAdj["0,1,0"][relID]; } else
            if(rz >= size.z + 1) { relDef = nodeDefAdj["0,0,1"][relID]; } else
            { relDef = nodeDef[relID]; }
            if(relDef == undefined) { relDef = unknownDef; }
            
            for(var i = 0; i < def.meshConnectsTo.length; i++) {
              if(def.meshConnectsTo[i] == relDef.itemstring) {
                connectDirs[faceIndex] = true;
                break;
              }
            }
          }
        }
        
        var fluidVerts, fluidUVs;
        if(def.isFluid) {
          var nearby_heights = [0, 0, 0, 0, 0, 0, 0, 0];
          var f = [
            {x: -1, y: 0, z: 0},
            {x: 1, y: 0, z: 0},
            {x: 0, y: 0, z: -1},
            {x: 0, y: 0, z: 1},
            {x: -1, y: 0, z: -1},
            {x: 1, y: 0, z: -1},
            {x: -1, y: 0, z: 1},
            {x: 1, y: 0, z: 1}
          ];
          for(var faceIndex = 0; faceIndex < 8; faceIndex++) {
            var face = f[faceIndex];
            var rx = x + face.x; var ry = y + face.y; var rz = z + face.z;
            if(rx < 0 || rx >= size.x + 2) { continue; }
            if(ry < 0 || ry >= size.y + 2) { continue; }
            if(rz < 0 || rz >= size.z + 2) { continue; }
            var relD = data[rx][ry][rz];
            if(relD == -1) { continue; }
            var relID = relD & 32767;
            var relRot = (relD >> 15) & 255;
            
            var relDef;
            if(rx < 1) { relDef = nodeDefAdj["-1,0,0"][relID]; } else
            if(ry < 1) { relDef = nodeDefAdj["0,-1,0"][relID]; } else
            if(rz < 1) { relDef = nodeDefAdj["0,0,-1"][relID]; } else
            if(rx >= size.x + 1) { relDef = nodeDefAdj["1,0,0"][relID]; } else
            if(ry >= size.y + 1) { relDef = nodeDefAdj["0,1,0"][relID]; } else
            if(rz >= size.z + 1) { relDef = nodeDefAdj["0,0,1"][relID]; } else
            { relDef = nodeDef[relID]; }
            if(relDef == undefined) { relDef = unknownDef; }
            
            if(relDef.isFluid) {
              nearby_heights[faceIndex] = 16 - ((relRot >> 4) & 15);
              if((relRot & 4) == 4) { //visual_fullheight
                nearby_heights[faceIndex] = 16;
              }
            }
          }
          
          var baseHeight = 16 - ((rot >> 4) & 15);
          if((rot & 4) == 4) { //visual_fullheight
            baseHeight = 16;
          }
          //height in the negative-x and negative-z corner (and so on) on a scale of 0 to 16 inclusive
          var h_xmzm = Math.max(baseHeight, Math.max(nearby_heights[4], nearby_heights[0], nearby_heights[2]));
          var h_xpzm = Math.max(baseHeight, Math.max(nearby_heights[5], nearby_heights[1], nearby_heights[2]));
          var h_xmzp = Math.max(baseHeight, Math.max(nearby_heights[6], nearby_heights[0], nearby_heights[3]));
          var h_xpzp = Math.max(baseHeight, Math.max(nearby_heights[7], nearby_heights[1], nearby_heights[3]));
          
          //vertex y position in the x- z- corner (and so on) on a scale of -0.5 to 0.5 inclusive
          var vy_xmzm = (h_xmzm / 16) - 0.5;
          var vy_xpzm = (h_xpzm / 16) - 0.5;
          var vy_xmzp = (h_xmzp / 16) - 0.5;
          var vy_xpzp = (h_xpzp / 16) - 0.5;
          
          //texture v position in the x- z- corner (and so on) on a scale of 0.0 to 1.0 inclusive
          var ty_xmzm = h_xmzm / 16;
          var ty_xpzm = h_xpzm / 16;
          var ty_xmzp = h_xmzp / 16;
          var ty_xpzp = h_xpzp / 16;
          
          fluidVerts = [
            [
              -0.5, vy_xmzm, -0.5,
              -0.5, -0.5, -0.5,
              -0.5, vy_xmzp, 0.5,
              
              -0.5, -0.5, -0.5,
              -0.5, -0.5, 0.5,
              -0.5, vy_xmzp, 0.5
            ],
            [
              0.5, vy_xpzp, 0.5,
              0.5, -0.5, 0.5,
              0.5, vy_xpzm, -0.5,
              
              0.5, -0.5, 0.5,
              0.5, -0.5, -0.5,
              0.5, vy_xpzm, -0.5
            ],
            [
              0.5, -0.5, 0.5,
              -0.5, -0.5, 0.5,
              0.5, -0.5, -0.5,
              
              -0.5, -0.5, 0.5,
              -0.5, -0.5, -0.5,
              0.5, -0.5, -0.5
            ],
            [
              0.5, vy_xpzm, -0.5,
              -0.5, vy_xmzm, -0.5,
              0.5, vy_xpzp, 0.5,
              
              -0.5, vy_xmzm, -0.5,
              -0.5, vy_xmzp, 0.5,
              0.5, vy_xpzp, 0.5
            ],
            [
              0.5, vy_xpzm, -0.5,
              0.5, -0.5, -0.5,
              -0.5, vy_xmzm, -0.5,
              
              0.5, -0.5, -0.5,
              -0.5, -0.5, -0.5,
              -0.5, vy_xmzm, -0.5
            ],
            [
              -0.5, vy_xmzp, 0.5,
              -0.5, -0.5, 0.5,
              0.5, vy_xpzp, 0.5,
              
              -0.5, -0.5, 0.5,
              0.5, -0.5, 0.5,
              0.5, vy_xpzp, 0.5
            ]
          ];
          
          fluidUVs = [
            [
              0.0, ty_xmzm,
              0.0, 0.0,
              1.0, ty_xmzp,
              
              0.0, 0.0,
              1.0, 0.0,
              1.0, ty_xmzp
            ],
            [
              0.0, ty_xpzp,
              0.0, 0.0,
              1.0, ty_xpzm,
              
              0.0, 0.0,
              1.0, 0.0,
              1.0, ty_xpzm
            ],
            [
              0.0, 1.0,
              0.0, 0.0,
              1.0, 1.0,
              
              0.0, 0.0,
              1.0, 0.0,
              1.0, 1.0
            ],
            [
              0.0, 1.0,
              0.0, 0.0,
              1.0, 1.0,
              
              0.0, 0.0,
              1.0, 0.0,
              1.0, 1.0
            ],
            [
              0.0, ty_xpzm,
              0.0, 0.0,
              1.0, ty_xmzm,
              
              0.0, 0.0,
              1.0, 0.0,
              1.0, ty_xmzm
            ],
            [
              0.0, ty_xmzp,
              0.0, 0.0,
              1.0, ty_xpzp,
              
              0.0, 0.0,
              1.0, 0.0,
              1.0, ty_xpzp
            ]
          ];
          
          function scale(n, min1, max1, min2, max2) {
            return (n - min1) * (max2 - min2) / (max1 - min1) + min2;
          }
          
          for(var i = 0; i < 6; i++) {
            for(var n = 0; n < fluidUVs[i].length; n += 2) {
              fluidUVs[i][n] = scale(fluidUVs[i][n], 0, 1, def.fluidTexLoc[0], def.fluidTexLoc[2]);
              fluidUVs[i][n + 1] = scale(fluidUVs[i][n + 1], 0, 1, def.fluidTexLoc[1], def.fluidTexLoc[3]);
            }
          }
        }
        
        for(var faceIndex = 0; faceIndex < 6; faceIndex++) {
          //physical face
          var face = stdFaces[faceIndex];
          
          if(def.rotDataType != "rot") { rot = 0; }
          
          //face to display
          var rotFaceIndex = stdRotMapping[rot].shuffle[faceIndex];
          var rotFace = stdFaces[rotFaceIndex];
          var rotAxis = stdRotMapping[rot].rot[faceIndex];
          
          var rx = x + face.x; var ry = y + face.y; var rz = z + face.z;
          if(rx < 0 || rx >= size.x + 2) { continue; }
          if(ry < 0 || ry >= size.y + 2) { continue; }
          if(rz < 0 || rz >= size.z + 2) { continue; }
          var relD = data[rx][ry][rz];
          if(relD == -1) { continue; }
          var relID = relD & 32767;
          var relLightRaw = (relD >> 23) & 255;
          var relLight = Math.max(relLightRaw & 15, Math.round(((relLightRaw >> 4) & 15) * sun));
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
          
          if(!def.transparent && !relDef.transparent)
            continue;
          //if(!def.renderAdj && !relDef.renderAdj)
          //  continue;
          
          //joined nodes (ie water)
          if(def.itemstring == relDef.itemstring && def.joined)
            continue;
          
          // workaround for water
          if(useTransparent && def.joined && !relDef.transparent)
            continue;
          
          var sunkLit = def.faceIsRecessed[rotFaceIndex];
          if(sunkLit == null) { sunkLit = false; }
          var smoothLightIgnoreSolid = false;
          
          //TODO: use transFaces?
          //if(def.transparent && def.transFaces[faceIndex] && light > relLight) { relLight = light; tLight = true; }
          if(def.isFluid) {
            sunkLit = true;
            smoothLightIgnoreSolid = true;
          } else if(def.transparent && light > relLight && def.faceIsRecessed[rotFaceIndex] == null) {
            //relLight = light; tLight = true;
            sunkLit = true;
            smoothLightIgnoreSolid = true;
          }
          
          //Only show faces that are lit by a node in the current mapblock.
          //Exclude self-lit nodes outside of the current mapblock.
          if((tLight || sunkLit) && (x < 1 || y < 1 || z < 1 || x >= size.x + 1 || y >= size.y + 1 || z >= size.z + 1)) { continue; }
          //Exclude non-self-lit nodes where the current face is lit by a node outside of the current mapblock.
          if(!tLight && !sunkLit && (rx < 1 || ry < 1 || rz < 1 || rx >= size.x + 1 || ry >= size.y + 1 || rz >= size.z + 1)) { continue; }
          
          var tint = 1;
          if(def.useTint) {
            if(faceIndex == 3) { tint = 1; } else
            if(faceIndex == 2) { tint = 0.6; } else
            if(faceIndex == 0 || faceIndex == 1) { tint = 0.75; } else
            { tint = 0.875; }
          }
          
          var colorR = Math.round(lightCurve[relLight] * tint);
          var colorG = colorR;
          var colorB = colorR;
          
          var arr;
          if(def.isFluid) {
            arr = fluidVerts[rotFaceIndex];
          } else {
            if(def.customMesh) {
              arr = def.customMeshVerts[rotFaceIndex];
            } else {
              arr = stdVerts[faceIndex];
            }
          }
          if(def.connectingMesh) {
            arr = arr.slice();
            for(var n = 0; n < 6; n++) {
              if(connectDirs[n]) {
                if(def.connectingVerts[n] != null) {
                  arr.push.apply(arr, def.connectingVerts[n][rotFaceIndex]);
                }
              }
            }
          }
          
          if(def.isFluid || def.customMesh) {
            //shuffle faces as needed
            if(faceIndex != rotFaceIndex) {
              var map = faceShuffleMap[rotFaceIndex][faceIndex];
              
              //clone vert array
              arr = [...arr];
              
              for(var i = 0; i < arr.length; i += 3) {
                var orig = [arr[i], arr[i + 1], arr[i + 2]];
                
                arr[i] = orig[map[0]] * map[3];
                arr[i + 1] = orig[map[1]] * map[4];
                arr[i + 2] = orig[map[2]] * map[5];
              }
            }
          }
          //rotate verticies as needed
          //see geometry.js for more info
          if(rotAxis > 0) {
            var map = vertexRotMap[faceIndex];
            
            //clone vert array
            arr = [...arr];
            
            for(var count = 0; count < rotAxis; count++) {
              for(var i = 0; i < arr.length; i += 3) {
                var orig = [arr[i], arr[i + 1], arr[i + 2]];
                
                arr[i] = orig[map[0]] * map[3];
                arr[i + 1] = orig[map[1]] * map[4];
                arr[i + 2] = orig[map[2]] * map[5];
              }
            }
          }
          
          //Find the bounding box for the verticies on this face, to help with smooth lighting computations.
          var vertMinX = 0;
          var vertMaxX = 0;
          var vertMinY = 0;
          var vertMaxY = 0;
          var vertMinZ = 0;
          var vertMaxZ = 0;
          if(arr.length >= 3) {
            vertMinX = arr[0];
            vertMaxX = arr[0];
            vertMinY = arr[1];
            vertMaxY = arr[1];
            vertMinZ = arr[2];
            vertMaxZ = arr[2];
          }
          for(var i = 3; i < arr.length; i += 3) {
            vertMinX = Math.min(vertMinX, arr[i]);
            vertMaxX = Math.max(vertMaxX, arr[i]);
            vertMinY = Math.min(vertMinY, arr[i + 1]);
            vertMaxY = Math.max(vertMaxY, arr[i + 1]);
            vertMinZ = Math.min(vertMinZ, arr[i + 2]);
            vertMaxZ = Math.max(vertMaxZ, arr[i + 2]);
          }
          var vertMidX = (vertMinX + vertMaxX) / 2;
          var vertMidY = (vertMinY + vertMaxY) / 2;
          var vertMidZ = (vertMinZ + vertMaxZ) / 2;
          
          for(var i = 0; i < arr.length; i += 3) {
            if(useTransparent) {
              transparentVerts.push(arr[i] + (x - 1));
              transparentVerts.push(arr[i + 1] + (y - 1));
              transparentVerts.push(arr[i + 2] + (z - 1));
            } else {
              verts.push(arr[i] + (x - 1));
              verts.push(arr[i + 1] + (y - 1));
              verts.push(arr[i + 2] + (z - 1));
            }
            
            //FIXME - see above
            if(tLight) {
              if(useTransparent) {
                transparentColors.push(colorR);
                transparentColors.push(colorG);
                transparentColors.push(colorB);
              } else {
                colors.push(colorR);
                colors.push(colorG);
                colors.push(colorB);
              }
              
              facePos.push([x - 1, y - 1, z - 1, tint, true, null]);
            } else {
              var lx = rx;
              var ly = ry;
              var lz = rz;
              var f_relLight = relLight;
              if(sunkLit) {
                lx = x;
                ly = y;
                lz = z;
                f_relLight = light;
              }
              
              var adjList = [];
              var vertX = arr[i];
              var vertY = arr[i + 1];
              var vertZ = arr[i + 2];
              //FIXME these may not be detected correctly for some mesh geometries
              var xDiff = vertX >= vertMidX ? 1 : -1;
              var yDiff = vertY >= vertMidY ? 1 : -1;
              var zDiff = vertZ >= vertMidZ ? 1 : -1;
              
              if(face.x == 1 || face.x == -1) {
                adjList.push([lx - 1, ly - 1 + yDiff, lz - 1]);
                adjList.push([lx - 1, ly - 1, lz - 1 + zDiff]);
                adjList.push([lx - 1, ly - 1 + yDiff, lz - 1 + zDiff]);
              } else if(face.y == 1 || face.y == -1) {
                adjList.push([lx - 1 + xDiff, ly - 1, lz - 1]);
                adjList.push([lx - 1, ly - 1, lz - 1 + zDiff]);
                adjList.push([lx - 1 + xDiff, ly - 1, lz - 1 + zDiff]);
              } else if(face.z == 1 || face.z == -1) {
                adjList.push([lx - 1, ly - 1 + yDiff, lz - 1]);
                adjList.push([lx - 1 + xDiff, ly - 1, lz - 1]);
                adjList.push([lx - 1 + xDiff, ly - 1 + yDiff, lz - 1]);
              }
              
              
              
              var total = lightCurve[f_relLight] * tint;
              var count = 1;
              
              for(var n = 0; n < adjList.length; n++) {
                if(adjList[n] == null) { continue; }
                if(adjList[n][0] >= -1 && adjList[n][0] < size.x + 1 &&
                   adjList[n][1] >= -1 && adjList[n][1] < size.y + 1 &&
                   adjList[n][2] >= -1 && adjList[n][2] < size.z + 1) {
                  var adjD = data[adjList[n][0] + 1][adjList[n][1] + 1][adjList[n][2] + 1];
                  if(adjD == -1) { continue; }
                  var adjLightRaw = (adjD >> 23) & 255;
                  var adjLight = Math.max(adjLightRaw & 15, Math.round(((adjLightRaw >> 4) & 15) * sun));
                  if(adjLight > 0 && adjLight < f_relLight - 2) { continue; }
                  if(adjLight > 0 && f_relLight < adjLight - 2) { continue; }
                  if(adjLight == 0 && smoothLightIgnoreSolid) { adjList[n] = null; continue; }
                  if(adjLight == 0) { adjLight = Math.floor(f_relLight / 3); }
                  adjLight = Math.max(adjLight, 1);
                  
                  total += lightCurve[adjLight] * tint;
                  count++;
                }
              }
              
              var avgLight = Math.round(total / count);
              var f_colorR = avgLight;
              var f_colorG = f_colorR;
              var f_colorB = f_colorR;
              
              if(useTransparent) {
                transparentColors.push(f_colorR);
                transparentColors.push(f_colorG);
                transparentColors.push(f_colorB);
                
                transparentFacePos.push([lx - 1, ly - 1, lz - 1, tint, false, adjList]);
              } else {
                colors.push(f_colorR);
                colors.push(f_colorG);
                colors.push(f_colorB);
                
                facePos.push([lx - 1, ly - 1, lz - 1, tint, false, adjList]);
              }
            }
          }
          
          if(useTransparent) {
            if(def.isFluid) {
              transparentUVs.push.apply(transparentUVs, fluidUVs[rotFaceIndex]);
            } else {
              transparentUVs.push.apply(transparentUVs, def.uvs[rotFaceIndex]);
              if(def.connectingMesh) {
                for(var n = 0; n < 6; n++) {
                  if(connectDirs[n]) {
                    if(def.connectingUVs[n] != null) {
                      transparentUVs.push.apply(transparentUVs, def.connectingUVs[n][rotFaceIndex]);
                    }
                  }
                }
              }
            }
          } else {
            if(def.isFluid) {
              uvs.push.apply(uvs, fluidUVs[rotFaceIndex]);
            } else {
              uvs.push.apply(uvs, def.uvs[rotFaceIndex]);
              if(def.connectingMesh) {
                for(var n = 0; n < 6; n++) {
                  if(connectDirs[n]) {
                    if(def.connectingUVs[n] != null) {
                      uvs.push.apply(uvs, def.connectingUVs[n][rotFaceIndex]);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  
  var res = {
    verts: new Float32Array(verts),
    uvs: new Float32Array(uvs),
    colors: new Uint8Array(colors),
    facePos: facePos,
    transparentVerts: new Float32Array(transparentVerts),
    transparentUVs: new Float32Array(transparentUVs),
    transparentColors: new Uint8Array(transparentColors),
    transparentFacePos: transparentFacePos,
    pos: pos
  }
  /*if(lightNeedsUpdate) {
    res.data = data;
  }*/
  
  return res;
}
