var amqp = require('amqplib'),
    exec = require('child_process').exec,
    spawn = require('child_process').spawn,
    request = require('request'),
    fs = require('fs'),
    unzip = require('unzip');

/* Change these */
var host = "amqp://updater:updater@192.168.4.235";
var destination_path = '../taz';
var window_num = 1;
var ship_num = 1;

function GetURL(version) {
  return "https://github.com/plushmonkey/monkeybot/releases/download/v" + version + "/screenbot-release-" + version + ".zip"
}

function GetPID(image_name, callback) {
  exec('tasklist /nh /fo csv /fi "imagename eq ' + image_name + '"', function(err, stdout, stderr) {
    var lines = stdout.toString().split('\r\n');
    
    if (lines.length > 0) {
      var line = '';

      lines.forEach(function(current) {
        if (current.length > 0)
          line = current;
      });

      var parts = line.split(',');
      
      if (parts.length > 1) {
        var pid = parts[1];
        
        pid = parseInt(pid.substr(1, pid.length - 2));
        callback(null, pid);
        return;
      }
    }
    
    callback(new Error('No ' + image_name + ' processes running.'));
  });
}

function Download(url, callback) {
  console.log("Fetching " + url);

  var match = url.match(/^http.+\/(.+)$/);

  if (!match) {
    callback(new Error('Not a valid url to download.'));
    return;
  }

  var dest = match[1];
  var file = fs.createWriteStream(dest).on('error', function(err) {
    callback(err);
    return;
  });

  request.get(url).on('error', function(err) {
    callback(err);
  }).on('end', function() {
    callback(null, dest);
  }).pipe(file);
}

function Copy(source, dest, callback) {
  var rd = fs.createReadStream(source).on('error', function(err) {
    callback(err);
    return;
  });

  var wr = fs.createWriteStream(dest).on('error', function(err) {
    callback(err);
    return;
  }).on('close', function() {
    callback(null);
  });

  rd.pipe(wr);
}

function Run(file) {
  var child = spawn(file, [], { 'cwd': destination_path });

  child.stdout.on('data', function(data) {
    var mesg = data.toString().trim();

    console.log(mesg);

    if (mesg[mesg.length - 1] == '>')
      child.stdin.write(window_num.toString() + '\n');

    if (mesg.indexOf('Ship number:') != -1)
      child.stdin.write(ship_num.toString() + '\n');
  });

  child.stderr.on('data', function(data) {
    console.log(data.toString());
  });
}

amqp.connect(host).then(function(conn) {
  return conn.createChannel().then(function(ch) {
    var ok = ch.assertExchange('update', 'fanout', { durable: false });
    ok = ok.then(function() { 
      return ch.assertQueue('', { exclusive: true });
    });
    ok = ok.then(function(qok) {
      return ch.bindQueue(qok.queue, 'update', '').then(function() {
        return qok.queue;
      });
    });

    ok = ok.then(function(queue) {
      return ch.consume(queue, do_work, { noAck: false });
    });
    return ok.then(function() {
      console.log('Waiting for update.');
    });

    function do_work(message) {
      var mesg = JSON.parse(message.content.toString());

      ch.ack(message);

      if (typeof mesg.version == 'undefined') {
        console.log('Bad version received.');
        return;
      }

      var url;

      if (typeof mesg.url != 'undefined')
        url = mesg.url;
      else
        url = GetURL(mesg.version);
          
      GetPID("screenbot.exe", function(err, pid) {
        if (!err) {
          // Kill process if found
          console.log("Killing screenbot.exe [" + pid + "]");
          process.kill(pid);
        } else { console.log(err.message); }

        Download(url, function(err, filename) {
          if (err) {
            console.log(err.message);
            return;
          }

          console.log('Downloaded ' + filename);

          /* Extract downloaded file into current directory */
          var extract = unzip.Extract({ path: '.' }).on('close', function() {
            console.log('Extracted ' + filename);

            var source = 'screenbot/screenbot.exe';
            var dest = destination_path;

            if (dest.length == 0) {
              console.log('Destination path was not set correctly.');
              process.exit(1);
            }

            if (dest[dest.length - 1] == '/')
              dest += 'screenbot.exe';
            else
              dest += '/screenbot.exe';

            /* Copy the extracted screenbot.exe to the destination path */
            Copy(source, dest, function(err) {
              if (err) {
                console.log(err.message);
                return;
              }
              console.log('File copied to ' + dest);

              /* Spawn a screenbot process and write to stdin */
              Run(dest);
            });
          });

          fs.createReadStream(filename).pipe(extract);
        });
      });
    }

  });
}).then(null, console.warn);
