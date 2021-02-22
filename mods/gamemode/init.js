(function() {
  mods.gamemode = {};
  mods.gamemode.creative = false;
  
  mods.gamemode.changeHooks = [];
  mods.gamemode.registerChangeHook = function(f) {
    mods.gamemode.changeHooks.push(f);
  };
  
  mods.gamemode.registerChangeHook(function() { api.player.creativeDigPlace = mods.gamemode.creative; });
  
  mods.gamemode.set = function(vars) {
    var didChange = false;
    
    for(var key in vars) {
      var currentVal = mods.gamemode[key];
      if(currentVal == undefined) { api.modDebug("gamemode", "warn", "unable to set gamemode variable '" + key + "': does not exist"); continue; }
      var newVal = vars[key];
      
      if(currentVal != newVal) {
        mods.gamemode[key] = newVal;
        didChange = true;
      }
    }
    
    if(didChange) {
      for(var i = 0; i < mods.gamemode.changeHooks.length; i++) {
        mods.gamemode.changeHooks[i]();
      }
    }
    
    return didChange;
  };
  
  api.onModLoaded("chat", function() {
    mods.chat.registerCommand(new mods.chat.ChatCommand("/gamemode", function(args) {
      if(args.length == 1) {
        return "creative: " + (mods.gamemode.creative ? "yes" : "no");
      } else {
        var str = args[1];
        if(str == "creative") {
          var didChange = mods.gamemode.set({creative: true});
          return didChange ? "game set to creative" : "already in creative";
        } else if(str == "survival") {
          var didChange = mods.gamemode.set({creative: false});
          return didChange ? "game set to survival" : "already in survival";
        } else {
          return "not a valid game mode.";
        }
      }
    }, "/gamemode [survival|creative] : get or set the current game mode"));
  });
})();
