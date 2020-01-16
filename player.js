class Player {
  constructor() {
    this.controls = new KeyboardControls(window, camera);
    
    this.pos = new THREE.Vector3(0, 10, 0);
    this.vel = new THREE.Vector3(0, 0, 0);
    this.rot = new THREE.Quaternion();
    
    this.fly = false;
    
    this.inventory = server.getInventory(this);
    this.wieldIndex = 0;
    
    this.boundingBox = new THREE.Box3(new THREE.Vector3(-0.3, -1.5, -0.3), new THREE.Vector3(0.3, 0.3, 0.3));
  }
  
  tick(tscale) {
    var vy = this.vel.y;
    this.controls.fly = this.fly;
    this.controls.tick(tscale);
    this.vel.copy(this.controls.vel);
    if(!this.fly) {
      if(this.vel.y > 0 && vy == 0) {
        
      } else {
        this.vel.y = vy - 10 * tscale;
      }
    }
    this.rot.copy(this.controls.rot);
    
    var dim = ["x", "y", "z"];
    for(var i = 0; i < dim.length; i++) {
      var d = dim[i];
      
      var oldN = this.pos[d];
      this.pos[d] += this.vel[d] * tscale;
      if(this.collide()) {
        this.pos[d] = oldN;
        if(d == "y" && this.vel[d] > 0) { //FIXME bodge
          this.vel[d] = -0.0001;
        } else {
          this.vel[d] = 0;
        }
      }
    }
    /*var dim = ["x", "y", "z"];
    var yd = false;
    for(var i = 0; i < dim.length; i++) {
      var d = dim[i];
      
      var oldN = this.pos[d];
      this.pos[d] += this.vel[d] * tscale;
      if(this.collide()) {
        if((d == "x" || d == "z") && !yd) {
          this.pos.y += 0.5;
          if(this.collide()) {
            this.pos.y -= 0.5;
            this.pos[d] = oldN;
            this.vel[d] = 0;
          } else {
            yd = true;
          }
        } else {
          this.pos[d] = oldN;
          this.vel[d] = 0;
        }
      }
    }*/
  }
  
  collide(box) {
    if(box == undefined) {
      return collideMap(this.boundingBox.clone().translate(this.pos));
    } else {
      return collide(box, this.boundingBox.clone().translate(this.pos));
    }
  }
  
  get wield() {
    var stack = this.inventory.getStack("main", this.wieldIndex);
    if(stack == undefined) { return null; }
    return stack;
  }
}
