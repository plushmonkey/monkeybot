var connection_info = {
  host: "71.253.125.6",
  port: 9002
};

function get_text_width(text, font) {
  var canvas = get_text_width.canvas || (get_text_width.canvas = document.createElement("canvas"));
  var context = canvas.getContext("2d");
  context.font = font;
  var metrics = context.measureText(text);
  
  return metrics.width;
}

function get_text_height() {
  var font = document.getElementById("Font");
  var style = window.getComputedStyle(font, null).getPropertyValue('font-size');

  return parseFloat(style) + 1;
}

function get_status(player) {
  var stealth = (player.status & 1) != 0;
  var cloak = (player.status & 2) != 0;
  var x = (player.status & 4) != 0;

  return { stealth: stealth, cloak: cloak, x: x };
}

var handlers = {
  players: function(obj) {
    var players = obj.players;

    $("#players").empty();

    var width = window.innerWidth;
    var height = window.innerHeight;

    var mapwidth = 1024 * 16;
    var mapheight = 1024 * 16;

    var fontHeight = get_text_height();

    for (var i = 0; i < players.length; i++) {
      var player = players[i];

      var fontWidth = get_text_width(player.name, $("body").css('font'));

      var x = player.pos.x;
      var y = player.pos.y;
      var left = (x / mapwidth) * width;
      var top = (y / mapheight) * height;

      left -= fontWidth / 2;
      top -= fontHeight / 2;

      if (player.ship == 9) continue;
      if (player.name[0] == '<') continue;

      var status = get_status(player);
      
      var span = $('<span></span>').attr("id", player.name);
      span.text(player.name);

      span.css("position", "absolute");
      span.css("left", left);
      span.css("top", top);

      if (player.freq == 90)
        span.addClass("team");
      else if (player.freq == 91)
        span.addClass("enemy");
      else
        span.css("color", "white");

      if (status.x)
        span.css("color", "green");

      if (status.stealth || status.cloak)
        span.css("color", "red");

      $("#players").append(span);
    }
  },

  chat: function(obj) {
    var chat = obj.chat;

    var type = chat.type;

    if (type == "public")
      console.log(chat.player + "> " + chat.message);
    else if (type == "team")
      console.log("T " + chat.player + "> " + chat.message);
  }
};

function connect() {
  var connection = new WebSocket('ws://' + connection_info.host + ':' + connection_info.port);

  connection.onopen = function() {
    console.log("WebSocket opened.");
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
  connect();
};
