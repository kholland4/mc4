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
