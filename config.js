/*
    mc4, a web voxel building game
    Copyright (C) 2021 kholland4

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

var defaultConfig = {
  keybind_forward: "KeyW",
  keybind_left: "KeyA",
  keybind_backward: "KeyS",
  keybind_right: "KeyD",
  keybind_jump: "Space",
  keybind_sneak: "ShiftLeft",
  keybind_w_backward: "BracketLeft",
  keybind_w_forward: "BracketRight",
  keybind_w_peek_backward: "KeyR",
  keybind_w_peek_forward: "KeyF",
  keybind_close_window1: "KeyE",
  keybind_close_window2: "Escape",
  keybind_open_menu: "Escape",
  keybind_show_debug: "F4"
};

var userConfig = {};

function initConfig() {
  try {
    var storedConfig = localStorage.getItem("mc4_user_config");
    if(!storedConfig)
      return;
  } catch(e) {
    console.log("Failed to load localStorage mc4_user_config", e);
    return;
  }
  
  try {
    userConfig = JSON.parse(storedConfig);
  } catch(e) {
    console.log("Failed to parse localStorage mc4_user_config", e);
    return;
  }
}

function saveConfig() {
  try {
    localStorage.setItem("mc4_user_config", JSON.stringify(userConfig));
    return true;
  } catch(e) {
    console.log("Failed to save localStorage mc4_user_config", e);
    return false;
  }
}

function resetConfig() {
  userConfig = {};
  try {
    localStorage.removeItem("mc4_user_config");
    return true;
  } catch(e) {
    console.log("Failed to remove localStorage mc4_user_config", e);
    return false;
  }
}

function registerConfig(key, val) {
  if(key in defaultConfig)
    throw new Error("cannot register config key '" + key + "', already exists");
  
  defaultConfig[key] = val;
  buildKeymap();
}

function getConfig(key) {
  if(!(key in defaultConfig))
    throw new Error("cannot get nonexistent config key '" + key + "'");
  
  if(key in userConfig)
    return userConfig[key];
  return defaultConfig[key];
}

function setConfig(key, val) {
  if(!(key in defaultConfig))
    throw new Error("cannot set nonexistent config key '" + key + "'");
  if(typeof val != typeof defaultConfig[key])
    throw new Error("cannot assign type " + (typeof val) + " to config key '" + key + "' (type " + (typeof defaultConfig[key]) + ")");
  
  userConfig[key] = val;
  saveConfig();
  
  buildKeymap();
}

function buildKeymap() {
  keymap = {};
  for(var keybind of Object.keys(defaultConfig)) {
    if(!keybind.startsWith("keybind_"))
      continue;
    
    var val = keybind in userConfig ? userConfig[keybind] : defaultConfig[keybind];
    if(!(val in keymap))
      keymap[val] = [];
    keymap[val].push(keybind);
  }
}

function mapKey(k) {
  return keymap[k] || [];
}

var keymap = {};
buildKeymap();
