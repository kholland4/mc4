function fmtXYZ(vec) {
  return "(" + vec.x + ", " + vec.y + ", " + vec.z + ")";
}
//parses XYZ coordinates in the form "x<delim>y<delim>z" where <delim> is not 0-9, -, (, or )
function parseXYZ(str) {
  if(str.startsWith("(")) { str = str.substring(1); }
  if(str.endsWith(")")) { str = str.substring(0, str.length - 1); }
  var delim = "";
  for(var i = 0; i < str.length; i++) {
    var ch = str.charAt(i);
    if(!((ch >= '0' && ch <= '9') || ch == '-')) {
      delim += ch;
    } else {
      if(delim != "") { break; }
    }
  }
  if(delim == "") { return null; }
  
  var s = str.split(delim);
  if(s.length < 3) { return null; }
  var res = new THREE.Vector3();
  for(var i = 0; i < 3; i++) {
    var n = parseInt(s[i]);
    if(isNaN(n)) { return null; }
    res.setComponent(i, n);
  }
  
  return res;
}

api.util = {};
api.util.fmtXYZ = fmtXYZ;
api.util.parseXYZ = parseXYZ;
