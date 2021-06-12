/*
    mc4, a web voxel building game
    Copyright (C) 2019 kholland4

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

var KBDCTL_PITCH_MOVEMENT = false;

class BaseControls {
  constructor() {
    this.fly = false;
    this.ladder = false;
  }
  
  get vel() {
    return new THREE.Vector3(0, 0, 0);
  }
  
  get rot() {
    return new THREE.Quaternion();
  }
}

//keycodes: 'up', 'down', 'left', 'right', 'jump', 'sneak'

class KeyboardControls extends BaseControls {
  constructor(context, camera) {
    super();
    
    this.keymap = {
      keybind_forward: "up",
      keybind_left: "left",
      keybind_backward: "down",
      keybind_right: "right",
      keybind_jump: "jump",
      keybind_sneak: "sneak"
    };
    
    this.keysPressed = {"up": false, "down": false, "left": false, "right": false, "jump": false, "sneak": false};
    
    this.camera = camera;
    
    this.speed = 4;
    
    //Magic jump equation:
    //  v0 = sqrt(2*g*n)
    //where
    //  v0: initial velocity
    //  g: gravitational downward acceleration
    //  n: target height
    this.jumpSpeed = Math.sqrt(2 * GRAVITY * 1.2);
    this.fly = false;
    this.water = false;
    
    this.kvel = new THREE.Vector3(0, 0, 0);
    
    context.addEventListener("keydown", this.keyDown.bind(this));
    context.addEventListener("keyup", this.keyUp.bind(this));
  }
  
  keyDown(e) {
    if(!api.ingameKey()) { return; }
    
    mapKey(e.key).forEach((keybind) => {
      if(!(keybind in this.keymap))
        return;
      if(!(this.keymap[keybind] in this.keysPressed))
        return;
      this.keysPressed[this.keymap[keybind]] = true;
    });
  }
  keyUp(e) {
    if(!api.ingameKey()) { return; }
    
    mapKey(e.key).forEach((keybind) => {
      if(!(keybind in this.keymap))
        return;
      if(!(this.keymap[keybind] in this.keysPressed))
        return;
      this.keysPressed[this.keymap[keybind]] = false;
    });
  }
  
  tick(tscale) {
    var amt = 7 * tscale;
    if(this.keysPressed.left) {
      this.kvel.x -= amt;
    } else if(this.keysPressed.right) {
      this.kvel.x += amt;
    } else {
      if(this.kvel.x < 0) { this.kvel.x = Math.min(0, this.kvel.x + amt); }
      if(this.kvel.x > 0) { this.kvel.x = Math.max(0, this.kvel.x - amt); }
    }
    if(this.keysPressed.up) {
      this.kvel.z -= amt;
    } else if(this.keysPressed.down) {
      this.kvel.z += amt;
    } else {
      if(this.kvel.z < 0) { this.kvel.z = Math.min(0, this.kvel.z + amt); }
      if(this.kvel.z > 0) { this.kvel.z = Math.max(0, this.kvel.z - amt); }
    }
    this.kvel.clamp(new THREE.Vector3(-1, -1, -1), new THREE.Vector3(1, 1, 1));
    
    if(this.fly || this.ladder || this.water) {
      var yamt = amt;
      if(this.ladder && !this.fly) { yamt = amt * 0.7; }
      
      if(this.keysPressed.sneak) {
        this.kvel.y -= yamt;
      } else if(this.keysPressed.jump) {
        this.kvel.y += yamt;
      } else {
        if(this.kvel.y < 0) { this.kvel.y = Math.min(0, this.kvel.y + yamt); }
        if(this.kvel.y > 0) { this.kvel.y = Math.max(0, this.kvel.y - yamt); }
      }
      this.kvel.clamp(new THREE.Vector3(-1, -1, -1), new THREE.Vector3(1, 1, 1));
    } else {
      if(this.keysPressed.jump) {
        this.kvel.y = this.jumpSpeed;
      } else {
        this.kvel.y = 0;
      }
    }
  }
  
  get vel() {
    var res = this.kvel.clone();
    res.y = 0;
    //res.normalize();
    var len = res.length();
    
    var adjSpeed = this.speed;
    if(this.water) { adjSpeed *= 0.5; }
    
    var mat = new THREE.Matrix4().compose(new THREE.Vector3(0, 0, 0), this.rot, new THREE.Vector3(1, 1, 1));
    res.transformDirection(mat);
    res.y = 0;
    if(!KBDCTL_PITCH_MOVEMENT) { res.normalize(); }
    res.multiplyScalar(len * adjSpeed);
    
    res.y = this.kvel.y;
    if(this.fly) { res.y *= adjSpeed; }
    else if(this.ladder) { res.y *= adjSpeed * 0.75; }
    else if(this.water) { res.y *= adjSpeed; }
    
    return res;
  }
  
  get rot() {
    return this.camera.quaternion;
  }
}
