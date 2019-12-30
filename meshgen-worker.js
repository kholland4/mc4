importScripts("geometry.js");

onmessage = function(e) {
  var pos = e.data.pos;
  var size = e.data.size;
  var nodeDef = e.data.nodeDef;
  var nodeDefAdj = e.data.nodeDefAdj;
  var data = e.data.data;
  var lightNeedsUpdate = e.data.lightNeedsUpdate;
  //var ldata = e.data.ldata;
  
  var verts = [];
  var uvs = [];
  var colors = [];
  
  /*if(nodeDef.length > 1) {
    for(var i = 0; i < 6; i++) {
      verts.push.apply(verts, stdVerts[i]);
      uvs.push.apply(uvs, nodeDef[1].uvs[i]);
    }
  }*/
  
  /*if(lightNeedsUpdate) {
    for(var rep = 0; rep < 4; rep++) {
      for(var x = 1; x < size.x * 3 - 1; x++) {
        var cx = Math.floor((x - size.x) / size.x);
        for(var y = 1; y < size.y * 3 - 1; y++) {
          var cy = Math.floor((y - size.y) / size.y);
          for(var z = 1; z < size.z * 3 - 1; z++) {
            var cz = Math.floor((z - size.z) / size.z);
            
            var d = ldata[x][y][z];
            var id = d & 65535;
            
            var def = nodeDefAdj[cx + "," + cy + "," + cz];
            var lightLevel = def.lightLevel;
            
            var v = (d >> 23) & 255;
            var val1 = (v >> 4) & 15;
            var val2 = v & 15;
            
            if(lightLevel > val2) {
              val2 = lightLevel;
            }
            
            v = ((val1 << 4) | val2) & 255;
            ldata[x][y][z] = (ldata[x][y][z] & 0x7fffff) | (v << 23);
            if(x >= size.x - 1 && x < size.x * 2 + 1 && y >= size.y - 1 && y < size.y * 2 + 1 && z >= size.z - 1 && z < size.z * 2 + 1) {
              data[x - (size.x - 1)][y - (size.y - 1)][z - (size.z - 1)] = (data[x - (size.x - 1)][y - (size.y - 1)][z - (size.z - 1)] & 0x7fffff) | (v << 23);
            }
            
            if(def.transparent) {
              var amt = val2 - 1;
              for(var faceIndex = 0; faceIndex < 6; faceIndex++) {
                var face = stdFaces[faceIndex];
                var d = ldata[x + face.x][y + face.y][z + face.z];var v = (d >> 23) & 255;
                
                var v = (d >> 23) & 255;
                var val1 = (v >> 4) & 15;
                var val2 = v & 15;
                
                if(amt > val2) {
                  val2 = amt;
                }
                
                v = ((val1 << 4) | val2) & 255;
                var rx = x + face.x; var ry = y + face.y; var rz = z + face.z;
                ldata[rx][ry][rz] = (ldata[rx][ry][rz] & 0x7fffff) | (v << 23);
                
                if(rx >= size.x - 1 && rx < size.x * 2 + 1 && ry >= size.y - 1 && ry < size.y * 2 + 1 && rz >= size.z - 1 && rz < size.z * 2 + 1) {
                  data[rx - (size.x - 1)][ry - (size.y - 1)][rz - (size.z - 1)] = (data[rx - (size.x - 1)][ry - (size.y - 1)][rz - (size.z - 1)] & 0x7fffff) | (v << 23);
                }
              }
            }
          }
        }
      }
    }
  }*/
  
  for(var x = 1; x < size.x + 1; x++) {
    for(var y = 1; y < size.y + 1; y++) {
      for(var z = 1; z < size.z + 1; z++) {
        var d = data[x][y][z];
        var id = d & 65535;
        var rot = (d >> 16) & 127;
        //var light = (d >> 23) & 255;
        
        var def = nodeDef[id];
        if(!def.visible) { continue; }
        if(def.customMesh) { continue; } //TODO: implement
        
        for(var faceIndex = 0; faceIndex < 6; faceIndex++) {
          var face = stdFaces[faceIndex];
          var rx = x + face.x; var ry = y + face.y; var rz = z + face.z;
          var relD = data[rx][ry][rz];
          var relID = relD & 65535;
          var relLightRaw = (relD >> 23) & 255;
          var relLight = Math.max(relLightRaw & 15, (relLightRaw >> 4) & 15);
          relLight = Math.max(relLight, 15);
          
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
          if(faceIndex == 2) { tint = 0.3; } else
          { tint = 0.8; }
          
          var colorR = Math.round((relLight * 17) * tint);
          var colorG = colorR;
          var colorB = colorR;
          
          var arr = stdVerts[faceIndex];
          for(var i = 0; i < arr.length; i += 3) {
            verts.push(arr[i] + (x - 1));
            verts.push(arr[i + 1] + (y - 1));
            verts.push(arr[i + 2] + (z - 1));
            
            colors.push(colorR);
            colors.push(colorG);
            colors.push(colorB);
          }
          uvs.push.apply(uvs, def.uvs[faceIndex]);
        }
      }
    }
  }
  
  var res = {
    verts: verts,
    uvs: uvs,
    colors: colors
  }
  /*if(lightNeedsUpdate) {
    res.data = data;
  }*/
  this.postMessage(res);
};
