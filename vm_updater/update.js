var amqp = require('amqplib'),
    when = require('when'),
    version = process.argv[2];
	
var production = true;
var debug_url = 'http://192.168.4.235/screenbot.zip';

if (typeof version == 'undefined') {
  console.log('usage: node update 3.3.2');

  process.exit(1);
}


if (version == 'local')
  production = false;

amqp.connect('amqp://localhost').then(function(conn) {
  return when(conn.createChannel().then(function(ch) {
    var exchange = 'update';
    var msg;

    if (production) {
      msg = JSON.stringify({
        version: version
      });
    } else {
      msg = JSON.stringify({
        version: version,
        url: debug_url
      });
    }

    var ok = ch.assertExchange(exchange, 'fanout', { durable: false });

    return ok.then(function() {
      ch.publish(exchange, '', new Buffer(msg));
      console.log('Updating to version ' + version);
      return ch.close();
    });
  })).ensure(function() { conn.close(); });
}).then(null, console.warn);
