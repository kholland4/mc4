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

var VERSION = "0.1.11-dev3";

var scene;
var camera;
var renderer;
var viewport = {w: 640, h: 480};
var renderMapGroup;
var raycaster;
var raycasterSel;

var destroySel = null;
var placeSel = null;
var digTimer = null;
var digTimerStart = null;
var digSel = null;
var isDigging = false;
var digOverlay;
var destroySelNullCount = 0;

var needsRaycast = false;

var controls;

var server;
var player;

var RAYCAST_DISTANCE = 10;

var DIG_PREEMPT_TIME = 0.04;
var CREATIVE_DIG_TIME = 0.15;

var menuConfig = {
  gameType: "local", // local, remote
  remoteServer: "ws://localhost:8080/"
};

function initEntryMenu() {
  if(window.location.hostname != "") {
    menuConfig.remoteServer = "ws://" + window.location.hostname + ":8080/";
  }
  
  var win = new UIWindow();
  
  win.add(uiElement("text", "Local"));
  win.add(uiElement("br"));
  var startButton = uiElement("button");
  startButton.innerText = "Start Game";
  startButton.onclick = function() { menuConfig.gameType = "local"; uiHideWindow(); init(); };
  win.add(startButton);
  
  win.add(uiElement("spacer"));
  
  win.add(uiElement("text", "Join Server"));
  win.add(uiElement("br"));
  var serverAddr = uiElement("input", menuConfig.remoteServer);
  serverAddr.onchange = function() { menuConfig.remoteServer = this.value; };
  win.add(serverAddr);
  
  var startButton = uiElement("button");
  startButton.innerText = "Connect";
  startButton.onclick = function() { menuConfig.gameType = "remote"; uiHideWindow(); init(); };
  win.add(startButton);
  
  uiShowWindow(win);
}
document.addEventListener("DOMContentLoaded", initEntryMenu);

function init() {
  debug("main", "status", "starting...");
  
  viewport.w = window.innerWidth;
  viewport.h = window.innerHeight;
  
  scene = new THREE.Scene();
  camera = new THREE.PerspectiveCamera(75, viewport.w / viewport.h, 0.1, 1000);
  renderer = new THREE.WebGLRenderer();
  renderer.setSize(viewport.w, viewport.h);
  //renderer.gammaOutput = true;
  //renderer.gammaFactor = 2.2;
  document.body.appendChild(renderer.domElement);
  
  controls = new THREE.PointerLockControls(camera, document.body);
  renderer.domElement.addEventListener("click", function() {
    if(!controls.isLocked) {
      controls.lock();
    }
  });
  
  if(!controls.isLocked) { controls.lock(); }
  
  renderMapGroup = new THREE.Group();
  scene.add(renderMapGroup);
  
  raycaster = new THREE.Raycaster();
  
  api.mouse = new THREE.Vector2(0, 0);
  document.body.addEventListener("mousemove", function(e) {
    api.mouse.x = e.clientX;
    api.mouse.y = e.clientY;
  });
  
  api.registerNode(new Node("air", {transparent: true, passSunlight: true, visible: false, walkable: true, canPlaceInside: true}));
  api.registerNode(new Node("ignore", {transparent: false, passSunlight: false, visible: false, breakable: false}));
  api.registerNode(new Node("nothing", {transparent: false, passSunlight: false, visible: true, breakable: false}));
  api.loadTexture("unknown:texAll", "textures/unknown_node.png");
  api.registerNode(new Node("unknown", {breakable: false, texAll: "unknown:texAll"}));
  
  initTextures();
  initRenderer();
  initLightWorker();
  initUI();
  initMenu();
  
  if(menuConfig.gameType == "local") {
    server = new ServerLocal(new MapLocal(new MapgenDefault(Math.random())));
  } else if(menuConfig.gameType == "remote") {
    server = new ServerRemote(menuConfig.remoteServer);
    
    renderDist = new THREE.Vector3(2, 2, 2);
  } else {
    //whoops
  }
  
  api.server = server;
  
  player = new Player();
  server.addPlayer(player);
  
  api.player = player;
  
  var geometry = new THREE.EdgesGeometry(new THREE.BoxGeometry(1, 1, 1));
  var material = new THREE.LineBasicMaterial({color: 0xffffff, linewidth: 2});
  raycasterSel = new THREE.LineSegments(geometry, material);
  scene.add(raycasterSel);
  
  var geometry = new THREE.BoxGeometry(1.001, 1.001, 1.001);
  var materials = [];
  for(var i = 0; i < 5; i++) {
    var texture = new THREE.TextureLoader().load("textures/crack_" + i + ".png");
    texture.minFilter = THREE.NearestFilter;
    texture.magFilter = THREE.NearestFilter;
    var material = new THREE.MeshBasicMaterial({map: texture, transparent: true});
    materials.push(material);
  }
  digOverlay = new THREE.Mesh(geometry, materials);
  digOverlay.geometry.faces.forEach(function(face) { face.materialIndex = 0; });
  scene.add(digOverlay);
  
  window.addEventListener("mousedown", function(e) {
    if(!controls.isLocked) { return; }
    if(e.which == 1) {
      isDigging = true;
    } else if(e.which == 3) {
      if(placeSel != null) {
        var nodeData = server.nodeToPlace(player);
        if(nodeData != null) {
          var def = getNodeDef(nodeData.itemstring);
          var ok = true;
          if(def.walkable) {
            
          } else if(def.boundingBox == null) {
            if(player.collide(new THREE.Box3().setFromCenterAndSize(placeSel, new THREE.Vector3(1, 1, 1)))) {
              ok = false;
            }
          } else {
            for(var i = 0; i < def.boundingBox.length; i++) {
              if(player.collide(def.boundingBox[i].clone().translate(placeSel))) {
                ok = false;
                break;
              }
            }
          }
          if(ok) {
            var existingNode = server.getNode(placeSel);
            if(existingNode != null) {
              if(existingNode.getDef().canPlaceInside) {
                server.placeNode(player, placeSel);
                needsRaycast = true;
              }
            }
          }
        }
      }
    }
  });
  window.addEventListener("mouseup", function(e) {
    if(!controls.isLocked) { return; }
    if(e.which == 1) {
      isDigging = false;
      digTimer = null;
      digSel = null;
    }
  });
  
  window.addEventListener("resize", function(e) {
    viewport.w = window.innerWidth;
    viewport.h = window.innerHeight;
    
    camera.aspect = viewport.w / viewport.h;
    camera.updateProjectionMatrix();
    renderer.setSize(viewport.w, viewport.h);
  });
  
  loadMod(new ModMeta("default", "mods/default"));
  loadMod(new ModMeta("stairs", "mods/stairs", ["default"])); //FIXME mod dependencies don't actually do anything
  loadMod(new ModMeta("flowers", "mods/flowers"));
  loadMod(new ModMeta("hud", "mods/hud"));
  loadMod(new ModMeta("inventory", "mods/inventory"));
  loadMod(new ModMeta("chat", "mods/chat"));
  loadMod(new ModMeta("gamemode", "mods/gamemode"));
  
  loadLoop();
}

function loadLoop() {
  var ready = true;
  
  //if(!server.ready) { ready = false; } //FIXME
  
  if(!texturesLoaded()) { ready = false; }
  if(!iconsLoaded()) { ready = false; }
  if(!modsLoaded()) { ready = false; }
  
  if(ready) {
    debug("main", "status", "loaded textures, icons, and mods");
    server.connect();
    loadLoop2();
  } else {
    requestAnimationFrame(loadLoop);
  }
}

function loadLoop2() {
  var ready = true;
  
  if(!server.ready) { ready = false; }
  
  if(ready) {
    afterLoad();
  } else {
    requestAnimationFrame(loadLoop2);
  }
}

function afterLoad() {
  debug("main", "status", "ready");
  
  //---
  player.inventory.give("main", new ItemStack("default:pick_diamond"));
  player.inventory.give("main", new ItemStack("default:axe_diamond"));
  player.inventory.give("main", new ItemStack("default:dirt", 3));
  player.inventory.give("main", new ItemStack("default:shovel_diamond"));
  //player.inventory.give("main", new ItemStack("default:tree", 64));
  player.inventory.give("main", new ItemStack("default:wood", 64));
  //player.inventory.give("main", new ItemStack("default:leaves", 64));
  player.inventory.give("main", new ItemStack("default:cobble", 64));
  player.inventory.give("main", new ItemStack("stairs:slab_stone", 64));
  player.inventory.give("main", new ItemStack("default:torch", 64));
  //---
  
  //FIXME: bodge
  //var oldRenderDist = renderDist;
  //renderDist = new THREE.Vector3(1, 2, 1);
  //renderUpdateMap(new THREE.Vector3(Math.round(player.pos.x / MAPBLOCK_SIZE.x), Math.round(player.pos.y / MAPBLOCK_SIZE.y), Math.round(player.pos.z / MAPBLOCK_SIZE.z)));
  //renderDist = new THREE.Vector3(2, 2, 2);
  //renderUpdateMap(new THREE.Vector3(Math.round(player.pos.x / MAPBLOCK_SIZE.x), Math.round(player.pos.y / MAPBLOCK_SIZE.y), Math.round(player.pos.z / MAPBLOCK_SIZE.z)));
  //renderDist = oldRenderDist;
  //renderUpdateMap(new THREE.Vector3(Math.round(player.pos.x / MAPBLOCK_SIZE.x), Math.round(player.pos.y / MAPBLOCK_SIZE.y), Math.round(player.pos.z / MAPBLOCK_SIZE.z)));
  
  animateLastTime = performance.now();
  animate();
}

var animateLastTime = 0;
var frameTime = 0;

var raycastCounter = 0;

function animate() {
  requestAnimationFrame(animate);
  
  //TODO: server.ready?
  
  var currentTime = performance.now();
  frameTime = (currentTime - animateLastTime) / 1000.0;
  animateLastTime = currentTime;
  
  apiOnFrame(frameTime);
  
  if(raycastCounter >= 3 || needsRaycast) {
    raycaster.setFromCamera(new THREE.Vector2(0, 0), camera);
    var intersects = raycaster.intersectObjects(renderMapGroup.children);
    if(intersects.length > 0) {
      var intersect = intersects[0];
      if(intersect.distance < RAYCAST_DISTANCE) {
        var vertices = intersect.object.geometry.getAttribute("position");
        var a = new THREE.Vector3().fromBufferAttribute(vertices, intersect.face.a).add(intersect.object.position);
        var b = new THREE.Vector3().fromBufferAttribute(vertices, intersect.face.b).add(intersect.object.position);
        var c = new THREE.Vector3().fromBufferAttribute(vertices, intersect.face.c).add(intersect.object.position);
        var box = new THREE.Box3().setFromPoints([a, b, c]);
        var center = box.getCenter(new THREE.Vector3());
        
        var isAxisAligned = true;
        
        var plane = 0;
        if(box.min.x == box.max.x) { plane = 0; } else
        if(box.min.y == box.max.y) { plane = 1; } else
        if(box.min.z == box.max.z) { plane = 2; } else
        { isAxisAligned = false; }
        
        if(isAxisAligned) {
          var side = 1;
          if(player.pos.getComponent(plane) > center.getComponent(plane)) { side = -1; }
          
          destroySel = center.clone().add(new THREE.Vector3(0, 0, 0).setComponent(plane, side * 0.01));
          destroySel = new THREE.Vector3(Math.round(destroySel.x), Math.round(destroySel.y), Math.round(destroySel.z));
          placeSel = center.clone().add(new THREE.Vector3(0, 0, 0).setComponent(plane, side * -0.99));
          placeSel = new THREE.Vector3(Math.round(placeSel.x), Math.round(placeSel.y), Math.round(placeSel.z));
        } else {
          //usually flowers or whatever
          destroySel = center.clone();
          destroySel = new THREE.Vector3(Math.round(destroySel.x), Math.round(destroySel.y), Math.round(destroySel.z));
          placeSel = destroySel.clone();
        }
        
        raycasterSel.position.copy(destroySel);
        raycasterSel.visible = true;
      } else {
        destroySel = null;
        placeSel = null;
        raycasterSel.visible = false;
      }
    } else {
      destroySel = null;
      placeSel = null;
      raycasterSel.visible = false;
    }
    
    raycastCounter = 0;
    needsRaycast = false;
  } else {
    raycastCounter++;
  }
  
  if(digTimer > DIG_PREEMPT_TIME) {
    if(destroySel == null) {
      //Accounts for very slight delay in updating meshes where the mesh disappears.
      destroySelNullCount++;
      if(destroySelNullCount >= 6) {
        digTimer = null;
        digSel = null;
        destroySelNullCount = 0;
      }
    } else if(digSel != null) {
      if(!destroySel.equals(digSel)) {
        digTimer = null;
        digSel = null;
      } else if(server.getNode(digSel).itemstring == "air") {
        digTimer = null;
        digSel = null;
      }
      
      destroySelNullCount = 0;
    }
  } else {
    destroySelNullCount = 0;
  }
  if(isDigging && destroySel != null && digSel == null) {
    var nodeData = server.getNode(destroySel);
    if(nodeData.itemstring != "air") {
      if(player.creativeDigPlace) {
        var def = nodeData.getDef();
        if(def.breakable) {
          digTimer = CREATIVE_DIG_TIME;
        } else {
          digTimer = null;
        }
      } else {
        digTimer = calcDigTime(nodeData, player.wield);
      }
      if(digTimer != null) {
        digTimerStart = digTimer;
        digSel = destroySel.clone();
      }
    }
  }
  if(digTimer != null) {
    digTimer -= frameTime;
    var nodeData = server.getNode(digSel);
    if(digTimer <= DIG_PREEMPT_TIME && nodeData.itemstring != "air") {
      if(!player.creativeDigPlace) {
        useTool(nodeData, player.inventory, "main", player.wieldIndex);
      }
      server.digNode(player, digSel);
    } else if(digTimer <= 0) {
      digTimer = null;
      digSel = null;
      needRaycast = true;
    }
  }
  if(digTimer != null && digSel != null) {
    digOverlay.position.copy(digSel);
    //digOverlay.material.color.setRGB((1 - (digTimer / digTimerStart)), 0, 0);
    var n = Math.floor((1 - (Math.min(digTimer, digTimerStart - 0.0001) / digTimerStart)) * 5);
    digOverlay.geometry.faces.forEach(function(face) { face.materialIndex = n; });
    digOverlay.geometry.elementsNeedUpdate = true;
    digOverlay.visible = true;
  } else {
    digOverlay.visible = false;
  }
  
  player.tick(frameTime);
  camera.position.copy(player.pos);
  
  server.onFrame(frameTime);
  
  lightUpdate();
  renderUpdateMap(new THREE.Vector3(Math.round(player.pos.x / MAPBLOCK_SIZE.x), Math.round(player.pos.y / MAPBLOCK_SIZE.y), Math.round(player.pos.z / MAPBLOCK_SIZE.z)));
  
  renderer.render(scene, camera);
}
