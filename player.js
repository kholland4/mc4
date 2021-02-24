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

class Player {
  constructor() {
    this.controls = new KeyboardControls(window, camera);
    
    this.pos = new THREE.Vector3(0, 10, 0);
    this.vel = new THREE.Vector3(0, 0, 0);
    this.rot = new THREE.Quaternion();
    
    this.fly = false;
    
    this.inventory = server.getInventory(this);
    this.wieldIndex = 0;
    this.creativeDigPlace = false;
    
    this.boundingBox = new THREE.Box3(new THREE.Vector3(-0.3, -1.5, -0.3), new THREE.Vector3(0.3, 0.3, 0.3));
    
    this.updateHooks = [];
  }
  
  tick(tscale) {
    var vy = this.vel.y;
    this.controls.fly = this.fly;
    this.controls.tick(tscale);
    this.vel.copy(this.controls.vel);
    if(!this.fly) {
      if(this.vel.y > 0 && vy == 0) {
        
      } else {
        this.vel.y = vy - 9.8 * tscale;
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
    
    this.updateHooks.forEach(function(hook) {
      hook(this.pos, this.vel, this.rot);
    }.bind(this));
  }
  
  registerUpdateHook(hook) {
    this.updateHooks.push(hook);
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
