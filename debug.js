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

var allDebugHandlers = [];
var debugBuf = [];

function debugTimestamp() {
  return performance.now() / 1000;
}

class DebugMessage {
  constructor(source, level, str, time) {
    this.source = source;
    this.level = level;
    this.str = str;
    this.time = time;
  }
  
  fmt() {
    return "[ " + this.time.toFixed(2).padStart(8, " ") + " ] " + this.level + "[" + this.source + "]: " + this.str;
  }
}

class DebugHandler {
  constructor(levels, func) {
    this.levels = levels;
    this.func = func;
  }
  
  match(msg) {
    if(this.levels.includes(msg.level)) {
      return true;
    }
  }
  
  run(msg) {
    if(this.levels.includes(msg.level)) {
      this.func(msg);
    }
  }
}

function debug(source, level, str) {
  var msg;
  if(level instanceof DebugMessage) {
    msg = level;
  } else {
    msg = new DebugMessage(source, level, str, debugTimestamp());
  }
  
  debugBuf.push(msg);
  while(debugBuf.length > 100) { debugBuf.shift(); }
  
  for(var i = 0; i < allDebugHandlers.length; i++) {
    allDebugHandlers[i].run(msg);
  }
}

function registerDebugHandler(h) {
  allDebugHandlers.push(h);
}

api.getDebugBacklog = function(handler) {
  var out = [];
  for(var i = 0; i < debugBuf.length; i++) {
    if(handler.match(debugBuf[i])) {
      out.push(debugBuf[i]);
    }
  }
  return out;
};

api.debugTimestamp = debugTimestamp;
api.DebugMessage = DebugMessage;
api.DebugHandler = DebugHandler;
api.registerDebugHandler = registerDebugHandler;
api.modDebug = function(modName, level, str) { debug("mod " + modName, level, str); };
