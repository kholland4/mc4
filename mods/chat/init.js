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

(function() {
  mods.chat = {};
  mods.chat.commands = {};
  
  mods.chat.displayBuf = [];
  
  mods.chat.dom = document.createElement("div");
  mods.chat.dom.style.position = "fixed";
  mods.chat.dom.style.left = "0px";
  mods.chat.dom.style.top = "0px";
  mods.chat.dom.style.width = "100%";
  mods.chat.dom.style.zIndex = "7";
  api.registerHUD(mods.chat.dom);
  
  mods.chat.textbox = document.createElement("div");
  mods.chat.textbox.style.padding = "10px";
  mods.chat.textbox.style.paddingBottom = "0px";
  mods.chat.textbox.style.fontSize = "14px";
  mods.chat.textbox.style.fontFamily = "monospace";
  mods.chat.textbox.style.color = "white";
  mods.chat.textbox.style.whiteSpace = "pre-wrap";
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
      if(this.value.startsWith("/")) {
        mods.chat.execCommand(this.value);
      } else {
        api.server.sendMessage({
          type: "send_chat",
          channel: "main",
          message: this.value
        });
      }
      mods.chat.hideInput();
    }
  };
  mods.chat.dom.appendChild(mods.chat.inputbox);
  
  mods.chat.updateDom = function() {
    //TODO: config
    var res = [];
    var cutoff = api.debugTimestamp() - 25;
    for(var i = mods.chat.displayBuf.length - 1; i >= 0 && res.length < 7; i--) {
      if(mods.chat.displayBuf[i].time > cutoff) {
        res.unshift(mods.chat.displayBuf[i].str);
      }
    }
    var newText = res.join("\n");
    if(mods.chat.textbox.innerText != newText)
      mods.chat.textbox.innerText = newText;
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
  
  registerConfig("keybind_open_chat_command", "Slash");
  registerConfig("keybind_open_chat", "KeyT");
  registerConfig("keybind_close_chat", "Escape");
  
  api.registerKey(function(key) {
    var keybind = mapKey(key);
    
    if(!mods.chat.inputOpen && !api.uiWindowOpen()) {
      if(keybind.includes("keybind_open_chat_command")) {
        mods.chat.showInput("/");
        return false;
      } else if(keybind.includes("keybind_open_chat")) {
        mods.chat.showInput();
        return false;
      }
    } else if(mods.chat.inputOpen && keybind.includes("keybind_close_chat")) {
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
      if(out != undefined) {
        mods.chat.print(out);
      }
    } else {
      //mods.chat.print("unknown command '" + name + "'");
      api.server.sendMessage({
        type: "chat_command",
        command: str
      });
    }
  }
  
  mods.chat.registerCommand(new mods.chat.ChatCommand("/help", function(args) {
    if(args.length == 1) {
      var commands = [];
      for(var key in mods.chat.commands) {
        commands.push(key);
      }
      commands.sort();
      
      if(api.server.isRemote()) {
        api.server.sendMessage({
          type: "chat_command",
          command: "/help"
        });
        
        return "available commands (client): " + commands.join(" ");
      } else {
        return "available commands: " + commands.join(" ");
      }
    } else if(args.length >= 2) {
      var name = args[1];
      if(!(name in mods.chat.commands) && !name.startsWith("/")) { name = "/" + name; }
      if(!(name in mods.chat.commands)) {
        if(api.server.isRemote()) {
          api.server.sendMessage({
            type: "chat_command",
            command: args.join(" ")
          });
          return;
        }
        return "unknown command '" + name + "'";
      }
      
      var helptext = mods.chat.commands[name].helptext;
      if(helptext != null)
        return helptext;
      
      return "no help available for '" + name + "'";
    }
  }, "/help [<command>] : list available commands or give help for a command"));
  
  if(!api.server.isRemote()) {
    mods.chat.registerCommand(new mods.chat.ChatCommand("/giveme", function(args) {
      if(args.length >= 2) {
        var itemstring = args.slice(1).join(" ");
        var stack = api.ItemStack.fromString(itemstring);
        if(stack == null || stack.getDef().isUnknown) { return "item '" + itemstring + "' not found"; }
        var displayIS = stack.toString();
        var res = server.invGive(new InvRef("player", null, "main", null), stack);
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
    
    mods.chat.registerCommand(new mods.chat.ChatCommand("/clearinv", function(args) {
      for(var i = 0; i < server.invGetList(new InvRef("player", null, "main", null)).length; i++) {
        server.invSetStack(new InvRef("player", null, "main", i), null);
      }
    }, "/clearinv : remove all items from your main inventory"));
  }
  
  mods.chat.registerCommand(new mods.chat.ChatCommand("/pulverize", function(args) {
    if(api.server.isRemote()) {
      api.server.sendMessage({
        type: "inv_pulverize",
        wield: api.player.wieldIndex
      });
    } else {
      var current = server.invGetStack(new api.InvRef("player", null, "main", api.player.wieldIndex));
      
      if(current == null)
        return "nothing to pulverize";
      
      server.invSetStack(new api.InvRef("player", null, "main", api.player.wieldIndex), null);
      
      return "pulverized '" + current.toString() + "'"
    }
  }, "/pulverize : destroy the currently held item stack"));
  
  mods.chat.registerCommand(new mods.chat.ChatCommand("/whatisthis", function(args) {
    var current = server.invGetStack(new api.InvRef("player", null, "main", api.player.wieldIndex));
    
    if(current == null)
      return "(nothing)";
    return current.toString();
  }, "/whatisthis : info about the currently held item stack"));
  
  
  
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
  
  
  
  api.server.registerMessageHook("send_chat", function(data) {
    var prefix = "[#" + data["channel"] + "] ";
    if("private" in data) {
      if(data["private"]) {
        prefix = "[PM on #" + data["channel"] + "] ";
      }
    }
    if("from" in data) {
      prefix += "<" + data["from"] + "> ";
    }
    var justify = " ".repeat(prefix.length);
    var res = prefix;
    var lines = data["message"].split("\n");
    for(var i = 0; i < lines.length; i++) {
      if(i != 0) { res += "\n" + justify; }
      res += lines[i];
    }
    mods.chat.print(res);
  });
})();
