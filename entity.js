class Entity {
  //#object
  
  constructor(data) {
    if(!data) { data = {}; }
    
    this.pos = new THREE.Vector3();
    this.vel = new THREE.Vector3();
    this.rot = new THREE.Quaternion();
    
    this.oldPos = this.pos.clone();
    this.oldVel = this.vel.clone();
    this.oldRot = this.rot.clone();
    
    this.oldPosTimestamp = 0;
    this.posTimestamp = 0;
    this.interpProgress = 0;
    this.doInterpolate = true;
    
    if("id" in data) {
      this.id = data.id;
    } else {
      this.id = hopefullyPassableUUIDv4Generator();
    }
    
    var geometry = new THREE.SphereGeometry(0.4, 12, 12);
    this.object = new THREE.Mesh(geometry, new THREE.MeshBasicMaterial({color: Math.random() * 0xffffff}));
    scene.add(this.object);
    
    this.update(data);
  }
  
  updatePosVelRot(pos, vel, rot) {
    if(this.doInterpolate) {
      if(this.posTimestamp == 0) {
        //If this is the first update, set both pos and oldPos and move immediately
        this.pos.copy(pos);
        this.vel.copy(vel);
        this.rot.copy(rot);
        this.posTimestamp = performance.now() / 1000.0;
        
        this.oldPos.copy(this.pos);
        this.oldVel.copy(this.vel);
        this.oldRot.copy(this.rot);
        this.oldPosTimestamp = this.posTimestamp;
        
        this.interpProgress = 0;
        this.object.position.copy(this.oldPos);
      } else {
        //On subsequent updates, demote pos to oldPos and update pos
        this.oldPos.copy(this.pos);
        this.oldVel.copy(this.vel);
        this.oldRot.copy(this.rot);
        this.oldPosTimestamp = this.posTimestamp;
        
        this.pos.copy(pos);
        this.vel.copy(vel);
        this.rot.copy(rot);
        this.posTimestamp = performance.now() / 1000.0;
        
        this.interpProgress = 0;
        this.object.position.copy(this.oldPos);
      }
    } else {
      this.pos.copy(pos);
      this.vel.copy(vel);
      this.rot.copy(rot);
      
      this.updateObject();
    }
  }
  
  update(data) {
    if(!data) { data = {}; }
    
    var nPos = this.pos.clone();
    var nVel = this.pos.clone();
    var nRot = this.pos.clone();
    
    if("pos" in data) {
      nPos.x = data.pos.x;
      nPos.y = data.pos.y;
      nPos.z = data.pos.z;
    }
    if("vel" in data) {
      nVel.x = data.vel.x;
      nVel.y = data.vel.y;
      nVel.z = data.vel.z;
    }
    if("rot" in data) {
      nRot.x = data.rot.x;
      nRot.y = data.rot.y;
      nRot.z = data.rot.z;
      nRot.w = data.rot.w;
    }
    
    this.updatePosVelRot(nPos, nVel, nRot);
  }
  
  destroy() {
    scene.remove(this.object);
  }
  
  updateObject() {
    this.object.position.copy(this.pos);
    this.object.quaternion.copy(this.rot);
  }
  
  tick(tscale) {
    if(!this.doInterpolate) { return; }
    
    var len = this.posTimestamp - this.oldPosTimestamp;
    if(len == 0) {
      this.object.position.copy(this.oldPos);
      return;
    }
    
    var velX = (this.pos.x - this.oldPos.x) / len;
    var velY = (this.pos.y - this.oldPos.y) / len;
    var velZ = (this.pos.z - this.oldPos.z) / len;
    
    this.object.position.x += velX * tscale;
    this.object.position.y += velY * tscale;
    this.object.position.z += velZ * tscale;
  }
}
