importScripts("geometry.js");

onmessage = function(e) {
  var pos = e.data.pos;
  var size = e.data.size;
  var nodeDefAdj = e.data.nodeDefAdj;
  var data = e.data.data;
  var lightNeedsUpdate = e.data.lightNeedsUpdate;
  
  for(var x = 0; x < size.x; x++) {
    for(var y = 0; y < size.y; y++) {
      for(var z = 0; z < size.z; z++) {
        var rx = x + size.x;
        var ry = y + size.y;
        var rz = z + size.z;
        
        var d = data[rx][ry][rz];
        var id = d & 65535;
        var rot = (d >> 16) & 127;
        var light = (d >> 23) & 255;
        
        light = 0;
        if(x < 4 && z < 4) {
          light = 10;
        }
        
        data[rx][ry][rz] = id | (rot << 16) | (light << 23);
      }
    }
  }
  
  var res = {
    data: data
  }
  this.postMessage(res);
};
