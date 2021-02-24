class Entity {
  #object
  
  constructor(data) {
    if(!data) { data = {}; }
    
    this.pos = new THREE.Vector3();
    this.vel = new THREE.Vector3();
    this.rot = new THREE.Quaternion();
    
    if("id" in data) {
      this.id = data.id;
    } else {
      this.id = hopefullyPassableUUIDv4Generator();
    }
    
    var geometry = new THREE.SphereGeometry(0.4, 12, 12);
    this.#object = new THREE.Mesh(geometry, new THREE.MeshBasicMaterial({color: Math.random() * 0xffffff}));
    scene.add(this.#object);
    
    this.update(data);
  }
  
  updatePosVelRot(pos, vel, rot) {
    this.pos.copy(pos);
    this.vel.copy(vel);
    this.rot.copy(rot);
    
    this.#updateObject();
  }
  
  update(data) {
    if(!data) { data = {}; }
    
    if("pos" in data) {
      this.pos.x = data.pos.x;
      this.pos.y = data.pos.y;
      this.pos.z = data.pos.z;
    }
    if("vel" in data) {
      this.vel.x = data.vel.x;
      this.vel.y = data.vel.y;
      this.vel.z = data.vel.z;
    }
    if("rot" in data) {
      this.rot.x = data.rot.x;
      this.rot.y = data.rot.y;
      this.rot.z = data.rot.z;
      this.rot.w = data.rot.w;
    }
    
    this.#updateObject();
  }
  
  destroy() {
    scene.remove(this.#object);
  }
  
  #updateObject() {
    this.#object.position.copy(this.pos);
    this.#object.quaternion.copy(this.rot);
  }
}
