(function() {
  mods.chat = {};
  mods.chat.commands = {};
  
  mods.chat.print = function(str) {
    
  };
  
  class ChatCommand {
    constructor(name, callback, helptext="") {
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
    }
  }
  
  mods.chat.registerCommand(new mods.chat.ChatCommand("/giveme", function(args) {
    if(args.length >= 2) {
      var stack = api.ItemStack.fromString(args.slice(1).join(" "));
      if(stack == null) { return "Invalid item"; }
      var res = api.player.inventory.give("main", stack);
      return "'" + stack.toString() + "' added to inventory";
    }
  }));
})();
