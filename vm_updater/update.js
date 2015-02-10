var amqp = require('amqplib'),
    when = require('when'),
    version = process.argv[2];
	
if (typeof version == 'undefined') {
  console.log('usage: node update 3.3.2');

  process.exit(1);
}

amqp.connect('amqp://localhost').then(function(conn) {
  return when(conn.createChannel().then(function(ch) {
    var exchange = 'update';
    var msg = JSON.stringify({
      version: version
    });

    var ok = ch.assertExchange(exchange, 'fanout', { durable: false });

    return ok.then(function() {
      ch.publish(exchange, '', new Buffer(msg));
      console.log('Sent update');
      return ch.close();
    });
  })).ensure(function() { conn.close(); });
}).then(null, console.warn);
