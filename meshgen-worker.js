importScripts("geometry.js");

onmessage = function(e) {
  var pos = e.data.pos;
  var size = e.data.size;
  var nodeDef = e.data.nodeDef;
  var nodeDefAdj = e.data.nodeDefAdj;
  var data = e.data.data;
  
  var verts = [];
  var uvs = [];
  var colors = [];
  
  /*if(nodeDef.length > 1) {
    for(var i = 0; i < 6; i++) {
      verts.push.apply(verts, stdVerts[i]);
      uvs.push.apply(uvs, nodeDef[1].uvs[i]);
    }
  }*/
  
  for(var x = 1; x < size.x + 1; x++) {
    for(var y = 1; y < size.y + 1; y++) {
      for(var z = 1; z < size.z + 1; z++) {
        var d = data[x][y][z];
        var id = d & 65535;
        var rot = (d >> 16) & 127;
        var light = (d >> 23) & 255;
        
        var def = nodeDef[id];
        if(!def.visible) { continue; }
        if(def.customMesh) { continue; } //TODO: implement
        
        for(var faceIndex = 0; faceIndex < 6; faceIndex++) {
          var face = stdFaces[faceIndex];
          var relD = data[x + face.x][y + face.y][z + face.z];
          var relID = relD & 65535;
          var relLight = (relD >> 23) & 255;
          var relDef;
          if(x < 1) { relDef = nodeDefAdj["-1,0,0"][relID]; } else
          if(y < 1) { relDef = nodeDefAdj["0,-1,0"][relID]; } else
          if(z < 1) { relDef = nodeDefAdj["0,0,-1"][relID]; } else
          if(x >= size.x + 1) { relDef = nodeDefAdj["1,0,0"][relID]; } else
          if(y >= size.y + 1) { relDef = nodeDefAdj["0,1,0"][relID]; } else
          if(z >= size.z + 1) { relDef = nodeDefAdj["0,0,1"][relID]; } else
          { relDef = nodeDef[relID]; }
          
          if(!relDef.transparent) { continue; }
          
          var tint = 1;
          if(faceIndex == 3) { tint = 1; } else
          if(faceIndex == 2) { tint = 0.3; } else
          { tint = 0.8; }
          
          var colorR = Math.round(255.0 * tint);
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
  
  this.postMessage({
    verts: verts,
    uvs: uvs,
    colors: colors
  });
};
