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
      if(currentVal == undefined) { api.modDebug("gamemode", "warning", "unable to set gamemode variable '" + key + "': does not exist"); continue; }
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
  
  if(!api.server.isRemote()) {
    api.onModLoaded("chat", function() {
      mods.chat.registerCommand(new mods.chat.ChatCommand("/creative", function(args) {
        if(args.length == 1) {
          return "creative: " + (mods.gamemode.creative ? "yes" : "no");
        } else {
          var str = args[1];
          if(str == "on") {
            if(!mods.gamemode.creative && !api.player.privs.includes("creative")) {
              return "no creative priv";
            }
            
            var didChange = mods.gamemode.set({creative: true});
            return didChange ? "game set to creative" : "already in creative";
          } else if(str == "off") {
            var didChange = mods.gamemode.set({creative: false});
            return didChange ? "game set to survival" : "already in survival";
          } else {
            return "expected 'on' or 'off'";
          }
        }
      }, "/creative [on|off] : get or set creative mode"));
    });
  } else {
    api.server.registerMessageHook("set_player_opts", function(data) {
      mods.gamemode.set({creative: data.creative_mode});
    });
  }
})();
