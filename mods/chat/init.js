(function() {
  mods.chat = {};
  mods.chat.commands = {};
  
  mods.chat.displayBuf = [];
  
  mods.chat.dom = document.createElement("div");
  mods.chat.dom.style.position = "fixed";
  mods.chat.dom.style.left = "0px";
  mods.chat.dom.style.top = "0px";
  mods.chat.dom.style.width = "100%";
  api.registerHUD(mods.chat.dom);
  
  mods.chat.textbox = document.createElement("div");
  mods.chat.textbox.style.padding = "10px";
  mods.chat.textbox.style.paddingBottom = "0px";
  mods.chat.textbox.style.fontSize = "14px";
  mods.chat.textbox.style.fontFamily = "monospace";
  mods.chat.textbox.style.color = "white";
  mods.chat.textbox.style.whiteSpace = "pre";
  mods.chat.dom.appendChild(mods.chat.textbox);
  
  mods.chat.inputbox = document.createElement("input");
  mods.chat.inputbox.style.display = "none";
  mods.chat.inputbox.style.margin = "0px";
  mods.chat.inputbox.style.border = "0px";
  mods.chat.inputbox.style.paddingLeft = "10px";
  mods.chat.inputbox.style.paddingRight = "10px";
  mods.chat.inputbox.style.outline = "none";
  mods.chat.inputbox.style.width = "100%";
  mods.chat.inputbox.style.backgroundColor = "#00000055";
  mods.chat.inputbox.style.fontSize = "14px";
  mods.chat.inputbox.style.fontFamily = "monospace";
  mods.chat.inputbox.style.color = "white";
  mods.chat.inputbox.style.whiteSpace = "pre";
  mods.chat.inputbox.spellcheck = false;
  mods.chat.inputbox.onkeydown = function(e) {
    if(e.key == "Enter") {
      mods.chat.execCommand(this.value);
      mods.chat.hideInput();
    }
  };
  mods.chat.dom.appendChild(mods.chat.inputbox);
  
  mods.chat.updateDom = function() {
    //TODO: config
    var res = [];
    var cutoff = api.debugTimestamp() - 10;
    for(var i = mods.chat.displayBuf.length - 1; i >= 0 && res.length < 7; i--) {
      if(mods.chat.displayBuf[i].time > cutoff) {
        res.unshift(mods.chat.displayBuf[i].str);
      }
    }
    mods.chat.textbox.innerText = res.join("\n");
  };
  
  mods.chat.updateDom();
  var updateTimer = 0;
  api.registerOnFrame(function(tscale) {
    updateTimer += tscale;
    if(updateTimer >= 1) {
      updateTimer -= Math.floor(updateTimer);
      mods.chat.updateDom();
    }
  });
  
  mods.chat.inputOpen = false;
  mods.chat.showInput = function(str="") {
    if(mods.chat.inputOpen || api.uiWindowOpen()) { return; }
    api.uiVWOpen();
    mods.chat.inputOpen = true;
    
    mods.chat.inputbox.value = str;
    
    mods.chat.inputbox.style.display = "block";
    mods.chat.inputbox.focus();
  };
  mods.chat.hideInput = function(clear=true) {
    if(!mods.chat.inputOpen) { return; }
    
    mods.chat.inputbox.style.display = "none";
    
    if(clear) { mods.chat.inputbox.value = ""; }
    
    api.uiVWClose();
    mods.chat.inputOpen = false;
  };
  
  api.registerVWCloseHandler(mods.chat.hideInput);
  
  api.registerKey(function(key) {
    if(!mods.chat.inputOpen && !api.uiWindowOpen()) {
      if(key == "/") {
        mods.chat.showInput("/");
        return false;
      } else if(key.toLowerCase() == "t") {
        mods.chat.showInput();
        return false;
      }
    } else if(mods.chat.inputOpen && key == "Escape") {
      mods.chat.hideInput();
    }
  });
  
  mods.chat.print = function(str) {
    mods.chat.displayBuf.push({time: api.debugTimestamp(), str: str});
    mods.chat.updateDom();
  };
  
  class ChatCommand {
    constructor(name, callback, helptext=null) {
      this.name = name;
      this.callback = callback;
      this.helptext = helptext;
    }
    exec(args) {
      return this.callback(args);
    }
  }
  mods.chat.ChatCommand = ChatCommand;
  mods.chat.registerCommand = function(cmd) {
    mods.chat.commands[cmd.name] = cmd;
  };
  
  mods.chat.execCommand = function(str) {
    if(!str.startsWith("/")) { return; }
    
    var s = str.split(" ");
    var name = s[0];
    if(name in mods.chat.commands) {
      var out = mods.chat.commands[name].exec(s);
      mods.chat.print(out);
    } else {
      mods.chat.print("unknown command '" + name + "'");
    }
  }
  
  mods.chat.registerCommand(new mods.chat.ChatCommand("/help", function(args) {
    if(args.length == 1) {
      var commands = [];
      for(var key in mods.chat.commands) {
        commands.push(key);
      }
      commands.sort();
      return "available commands: " + commands.join(" ");
    } else if(args.length >= 2) {
      var name = args[1];
      if(!(name in mods.chat.commands)) { return "unknown command '" + name + "'"; }
      
      var helptext = mods.chat.commands[name].helptext;
      if(helptext == null) { return "no help available for '" + name + "'"; }
      return helptext;
    }
  }, "/help [<command>] : list available commands or give help for a command"));
  
  mods.chat.registerCommand(new mods.chat.ChatCommand("/giveme", function(args) {
    if(args.length >= 2) {
      var itemstring = args.slice(1).join(" ");
      var stack = api.ItemStack.fromString(itemstring);
      if(stack == null) { return "item '" + itemstring + "' not found"; }
      var displayIS = stack.toString();
      var res = api.player.inventory.give("main", stack);
      if(res) {
        return "'" + displayIS + "' added to inventory";
      } else {
        return "unable to add '" + displayIS + "' to inventory";
      }
    } else {
      return "no item specified";
    }
  }, "/giveme <itemstring> [<count>] : add an item stack to player's inventory"));
  
  mods.chat.registerCommand(new mods.chat.ChatCommand("/tp", function(args) {
    if(args.length >= 2) {
      var whereRaw = args.slice(1).join(" ");
      var where = api.util.parseXYZ(whereRaw);
      if(where == null) { return "invalid coordinates '" + whereRaw + "'"; }
      
      api.player.pos.copy(where);
      
      return "teleported to " + api.util.fmtXYZ(api.player.pos);
    }
  }, "/tp <x>,<y>,<z> : teleport to a given position"));
  
  
  
  mods.chat.registerCommand(new mods.chat.ChatCommand("/about", function(args) {
    return "mc4 v" + api.version() + " -- https://github.com/kholland4/mc4/";
  }, "/about : show version and author information"));
  
  var handler = new api.DebugHandler(["status", "warning", "error"], function(msg) {
    mods.chat.print(msg.fmt());
  });
  
  api.registerDebugHandler(handler);
  
  var backlog = api.getDebugBacklog(handler);
  for(var i = 0; i < backlog.length; i++) {
    //TODO: awareness of time?
    mods.chat.print(backlog[i].fmt());
  }
})();
