var api = {};
var mods = {};

var apiRunOnFrame = [];
function apiOnFrame(tscale) {
  for(var i = 0; i < apiRunOnFrame.length; i++) {
    apiRunOnFrame[i](tscale);
  }
}

api.registerOnFrame = function(f) {
  apiRunOnFrame.push(f);
};

api.registerKey = function(f) {
  document.body.addEventListener("keydown", function(e) {
    var res = this(e.key);
    if(res === false) {
      e.preventDefault();
      return false;
    }
  }.bind(f));
};
