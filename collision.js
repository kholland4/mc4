function collide(box1, box2) {
  return box1.intersectsBox(box2);
}

function collideMap(box) {
  for(var x = Math.round(box.min.x); x <= Math.round(box.max.x); x++) {
    for(var y = Math.round(box.min.y); y <= Math.round(box.max.y); y++) {
      for(var z = Math.round(box.min.z); z <= Math.round(box.max.z); z++) {
        var pos = new THREE.Vector3(x, y, z);
        var def = getNodeDef(server.getNode(pos).itemstring);
        
        if(def.boundingBox != null) {
          for(var i = 0; i < def.boundingBox.length; i++) {
            var newBox = def.boundingBox[i].clone().translate(pos);
            if(collide(newBox, box)) {
              return true;
            }
          }
        } else if(!def.walkable) {
          return true;
        }
      }
    }
  }
  return false;
}
