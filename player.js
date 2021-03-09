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

"use strict";

var GRAVITY = 20;

class Player {
  constructor() {
    this.controls = new KeyboardControls(window, camera);
    
    window.addEventListener("keydown", function(e) {
      if(!api.ingameKey()) { return; }
      
      var key = e.key;
      if(key == "PageUp" || key == "]") {
        this.pos.w++;
      } else if(key == "PageDown" || key == "[") {
        this.pos.w--;
      }
    }.bind(this));
    
    this.pos = new MapPos(0, 10, 0, 0, 0, 0);
    this.vel = new THREE.Vector3(0, 0, 0);
    this.rot = new THREE.Quaternion();
    
    this.fly = false;
    
    this.inventory = server.getInventory(this);
    this.wieldIndex = 0;
    this.creativeDigPlace = false;
    
    this.waterPhysics = false;
    this.waterView = null;
    
    this.boundingBox = new THREE.Box3(new THREE.Vector3(-0.3, -1.5, -0.3), new THREE.Vector3(0.3, 0.3, 0.3));
    
    this.updateHooks = [];
  }
  
  tick(tscale) {
    var containingNode = server.getNode(new MapPos(Math.round(this.pos.x), Math.round(this.pos.y), Math.round(this.pos.z), this.pos.w, this.pos.world, this.pos.universe));
    if(containingNode != null) {
      var def = containingNode.getDef();
      var newWaterView;
      if(def.isFluid) {
        var h = 16 - ((containingNode.rot >> 4) & 15);
        var fluidHeight = ((h + 1) / 16) - 0.5;
        var relPosY = this.pos.y - Math.round(this.pos.y);
        if(relPosY <= fluidHeight) {
          newWaterView = def.fluidOverlayColor;
        } else {
          newWaterView = null;
        }
      } else {
        newWaterView = null;
      }
      if(this.waterView != newWaterView) {
        this.waterView = newWaterView;
        if(this.waterView == null) {
          document.getElementById("fluidOverlay").style.display = "none";
        } else {
          document.getElementById("fluidOverlay").style.display = "block";
          document.getElementById("fluidOverlay").style.backgroundColor = this.waterView;
        }
      }
    }
    
    //roughly where the feet are
    var containingNodeLower = server.getNode(new MapPos(Math.round(this.pos.x), Math.round(this.pos.y - 1.5), Math.round(this.pos.z), this.pos.w, this.pos.world, this.pos.universe));
    if(containingNodeLower != null) {
      var def = containingNodeLower.getDef();
      if(def.isFluid) {
        var h = 16 - ((containingNodeLower.rot >> 4) & 15);
        var fluidHeight = ((h + 1) / 16) - 0.5;
        var relPosY = (this.pos.y - 1.5) - Math.round(this.pos.y - 1.5);
        if(relPosY <= fluidHeight) {
          this.waterPhysics = true;
        } else {
          this.waterPhysics = false;
        }
      } else {
        this.waterPhysics = false;
      }
    }
    
    var localGravity = GRAVITY;
    
    if(this.waterPhysics) {
      localGravity = 5;
    }
    
    var vy = this.vel.y;
    this.controls.fly = this.fly;
    this.controls.ladder = this.inLadder();
    this.controls.water = this.waterPhysics;
    this.controls.tick(tscale);
    this.vel.copy(this.controls.vel);
    if(!this.fly && !this.inLadder()) {
      if(this.vel.y > 0 && vy == 0) {
        
      } else {
        if(this.waterPhysics) {
          if(this.vel.y <= 0) {
            this.vel.y -= 1;
          }
        } else {
          this.vel.y = vy - localGravity * tscale;
        }
      }
    }
    this.rot.copy(this.controls.rot);
    
    var dim = ["y", "x", "z"];
    for(var i = 0; i < dim.length; i++) {
      var d = dim[i];
      
      var oldN = this.pos[d];
      this.pos[d] += this.vel[d] * tscale;
      if(this.collide()) {
        var didStepUp = 0;
        if((d == "x" || d == "z") && this.vel["y"] == 0) {
          var oldY = this.pos["y"];
          for(var offset = 1/16; offset <= 0.5; offset += 1/16) {
            this.pos["y"] = oldY + offset;
            if(!this.collide()) {
              didStepUp = offset;
              break;
            }
          }
          this.pos["y"] = oldY;
          
          if(didStepUp > 0) {
            //Magic jump equation:
            //  v0 = sqrt(2*g*n)
            //where
            //  v0: initial velocity
            //  g: gravitational downward acceleration
            //  n: target height
            this.vel["y"] = Math.sqrt(2 * localGravity * (didStepUp * 1.2));
          }
        }
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
      return collideMap(this.boundingBox.clone().translate(new THREE.Vector3(this.pos.x, this.pos.y, this.pos.z)), this.pos);
    } else {
      return collide(box, this.boundingBox.clone().translate(new THREE.Vector3(this.pos.x, this.pos.y, this.pos.z)));
    }
  }
  
  inLadder() {
    var nodeStandingIn = server.getNode(new MapPos(Math.round(this.pos.x), Math.round(this.pos.y - 1.51), Math.round(this.pos.z), this.pos.w, this.pos.world, this.pos.universe));
    if(nodeStandingIn == null) { return false; }
    var nodeStandingInDef = nodeStandingIn.getDef();
    return nodeStandingInDef.ladderlike;
  }
  
  get wield() {
    var stack = this.inventory.getStack("main", this.wieldIndex);
    if(stack == undefined) { return null; }
    return stack;
  }
}
