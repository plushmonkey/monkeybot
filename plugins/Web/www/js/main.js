var canvas = null;
var radar = null;
var FPS = 30;
var UpdateFrequency = 1000/FPS;
var connection_info = {
  host: "71.253.125.6",
  port: 9002
};
var paused = true;

var PlayerManager = {
  players: [],

  addPlayer: function(player) {
    this.players.push(player);
  },

  removePlayer: function(player) {
    var index = this.players.indexOf(player);
    if (index > -1)
      this.players.splice(index, 1);
  },

  getPlayerByName: function(name) {
    for (var i = 0; i < this.players.length; ++i) {
      if (this.players[i].name == name)
        return this.players[i];
    }

    return null;
  },

  getPlayerByPid: function(pid) {
    for (var i = 0; i < this.players.length; ++i) {
      if (this.players[i].pid == pid)
        return this.players[i];
    }

    return null;
  },

  updatePlayer: function(player, newPlayer) {
    var index = this.players.indexOf(player);
    this.players[index] = newPlayer;

    var d = new Date();

    this.players[index].lastUpdated = d.getTime();
  }
};

var pathPlan = [];

function mapPixelsToCanvasCoords(x, y) {
  var mapdim = 1024*16;
  var x = (x / mapdim) * canvas.width;
  var y = (y / mapdim) * canvas.height;
  return { x: x, y: y };
}

function mapTileToCanvasCoords(x, y) {
  return mapPixelsToCanvasCoords(x * 16, y * 16);
}

function getColor(player) {
  var stealth = (player.status & 1) != 0;
  var cloak = (player.status & 2) != 0;
  var xradar = (player.status & 4) != 0;

  if (stealth && !cloak) return "#FF6464";
  if (cloak && !stealth) return "#FFC8C8";
  if (stealth && cloak) return "#FF0000";
  if (xradar) return "#00FF00";
  if (player.freq == 90) return "#FFA040";
  if (player.freq == 91) return "#A0A0FF";

  return "white";
}

// Draws centered text on the canvas around x, y.
function drawText(text, x, y, color) {
  var ctx = canvas.getContext("2d");
  var width = ctx.measureText(text).width;

  x -= width / 2;

  ctx.font = "12px arial";
  ctx.fillStyle = color || "white";
  ctx.textBaseline = "middle";
  ctx.fillText(text, x, y);
}

function update() {
  if (paused) return;

  var players = PlayerManager.players;

  var d = new Date();
  var ms = d.getTime();

  var remove = [];

  for (var i = 0; i < players.length; ++i) {
    if (players[i].pos.x != 0 && players[i].pos.y != 0) {
      players[i].pos.x += players[i].velocity.x * (UpdateFrequency / 1000);
      players[i].pos.y += players[i].velocity.y * (UpdateFrequency / 1000);
    }

    if (ms - players[i].lastUpdated >= 1000)
      remove.push(players[i]);
  }

  for (var i = 0; i < remove.length; ++i)
    PlayerManager.removePlayer(remove[i]);
}

function draw() {
  var ctx = canvas.getContext("2d");

  ctx.clearRect(0, 0, canvas.width, canvas.height);

  if (canvas.width >= radar.width && canvas.height >= radar.height)
    ctx.drawImage(radar, 0, 0);
  else
    ctx.drawImage(radar, 0, 0, canvas.width, canvas.height);

  var bot = PlayerManager.getPlayerByName("taz");

  if (pathPlan.length > 0 && bot.ship != 9) {
    ctx.strokeStyle = "red";
    ctx.beginPath();

    var pos = mapPixelsToCanvasCoords(bot.pos.x, bot.pos.y);

    ctx.moveTo(pos.x, pos.y);
    
    for (var i = 0; i < pathPlan.length; ++i) {
      var node = pathPlan[i];

      pos = mapTileToCanvasCoords(node.x, node.y);
      ctx.lineTo(pos.x, pos.y);
    }
    ctx.stroke();
  }

  var players = PlayerManager.players;
  for (var i = 0; i < players.length; ++i) {
    var player = players[i];

    if (player.ship == 9 || player.name[0] == "<") continue;

    var pos = mapPixelsToCanvasCoords(player.pos.x, player.pos.y);
    var color = getColor(player);
    
    drawText(player.name, pos.x, pos.y, color);
  }
}

function init() {
  canvas = document.getElementById("canvas");

  radar = new Image();
  radar.src = "images/radar.png";

  radar.onload = function() {
    setInterval(function() {
      update();
      draw();
    }, UpdateFrequency);

    window.onresize();
  }
}

var handlers = {
  players: function(obj) {
    var players = obj.players;

    for (var i = 0; i < players.length; ++i) {
      var player = PlayerManager.getPlayerByName(players[i].name);

      if (player) {
        PlayerManager.updatePlayer(player, players[i]);
      } else {
        PlayerManager.addPlayer(players[i]);
      }
    }
  },

  chat: function(obj) {
    var chat = obj.chat;
    var type = chat.type;

    if (type == "public")
      console.log(chat.player + "> " + chat.message);
    else if (type == "team")
      console.log("T " + chat.player + "> " + chat.message);
  },

  ai: function(obj) {
    var ai = obj.ai;

    pathPlan = ai.plan;
  }
};

window.onresize = function() {
  if (window.innerWidth > radar.width)
    canvas.width = radar.width;
  else
    canvas.width = window.innerWidth;

  if (window.innerHeight > radar.height)
    canvas.height = radar.height;
  else
    canvas.height = window.innerHeight;
};


function connect() {
  var connection = new WebSocket('ws://' + connection_info.host + ':' + connection_info.port);

  connection.binaryType = 'arraybuffer';

  connection.onopen = function() {
    console.log("WebSocket opened.");
    paused = false;
  };

  connection.onerror = function(error) {
    console.log('WebSocket error ' + error);
  };

  var reconnect = function() {
    connection = connect();

    if (!connection || connection.readyState == 3)
      setTimeout(reconnect, 2000);
  };

  connection.onclose = function(error) {
    console.log('WebSocket closed');

    paused = true;
    setTimeout(reconnect, 2000);
  };

  connection.onmessage = function(e) {
    var data = String.fromCharCode.apply(null, new Uint8Array(e.data));
    var obj = JSON.parse(data);

    if (!obj) {
      console.log("Failed to parse JSON object");
      return;
    }

    var handler = handlers[obj.type];

    if (!handler) {
      console.log("Received unknown object", obj);
      return;
    }

    handler(obj);
  };

  return connection;
}

window.onload = function() {
  init();

  connect();
};
