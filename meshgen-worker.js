importScripts("geometry.js");

onmessage = function(e) {
  var pos = e.data.pos;
  var size = e.data.size;
  var nodeDef = e.data.nodeDef;
  var nodeDefAdj = e.data.nodeDefAdj;
  var data = e.data.data;
  var lightNeedsUpdate = e.data.lightNeedsUpdate;
  var sunAmount = e.data.sunAmount;
  //var ldata = e.data.ldata;
  
  var verts = [];
  var uvs = [];
  var colors = [];
  var facePos = [];
  
  for(var x = 1; x < size.x + 1; x++) {
    for(var y = 1; y < size.y + 1; y++) {
      for(var z = 1; z < size.z + 1; z++) {
        var d = data[x][y][z];
        var id = d & 65535;
        var rot = (d >> 16) & 127;
        var lightRaw = (d >> 23) & 255;
        var light = Math.max(lightRaw & 15, ((lightRaw >> 4) & 15) * sunAmount);
        light = Math.max(light, 2);
        
        var def = nodeDef[id];
        if(!def.visible) { continue; }
        
        for(var faceIndex = 0; faceIndex < 6; faceIndex++) {
          var face = stdFaces[faceIndex];
          var rx = x + face.x; var ry = y + face.y; var rz = z + face.z;
          var relD = data[rx][ry][rz];
          var relID = relD & 65535;
          var relLightRaw = (relD >> 23) & 255;
          var relLight = Math.max(relLightRaw & 15, ((relLightRaw >> 4) & 15) * sunAmount);
          relLight = Math.max(relLight, 2);
          //relLight = Math.max(relLight, def.lightLevel);
          //FIXME - not desired behavior but needed to accomodate renderUpdateLighting
          if(def.lightLevel > 0) { relLight = light; }
          
          var relDef;
          if(rx < 1) { relDef = nodeDefAdj["-1,0,0"][relID]; } else
          if(ry < 1) { relDef = nodeDefAdj["0,-1,0"][relID]; } else
          if(rz < 1) { relDef = nodeDefAdj["0,0,-1"][relID]; } else
          if(rx >= size.x + 1) { relDef = nodeDefAdj["1,0,0"][relID]; } else
          if(ry >= size.y + 1) { relDef = nodeDefAdj["0,1,0"][relID]; } else
          if(rz >= size.z + 1) { relDef = nodeDefAdj["0,0,1"][relID]; } else
          { relDef = nodeDef[relID]; }
          
          if(!def.transparent && !relDef.transparent) { continue; }
          
          var tint = 1;
          if(faceIndex == 3) { tint = 1; } else
          if(faceIndex == 2) { tint = 0.5; } else
          { tint = 0.8; }
          
          var colorR = Math.round((relLight * 17) * tint);
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
            
            colors.push(colorR);
            colors.push(colorG);
            colors.push(colorB);
            
            //FIXME - see above
            if(def.lightLevel > 0) {
              facePos.push([x - 1, y - 1, z - 1, tint]);
            } else {
              facePos.push([rx - 1, ry - 1, rz - 1, tint]);
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
