
var fs = require('fs');

console.log('test contract running');

function statCallback (err, data) {
  if (err) {
    console.log('err:', err);
  } else {  
    console.log("Stat for sandbox.js", data);
  }
};

fs.stat('sandbox.js', statCallback);

var codius = process.binding('async');
var message = {
  type:'api',
  api:'fs',
  method:'stat',
  data:['sandbox.js']
};

codius.postMessage(message, statCallback);

codius.postMessage(JSON.stringify(message), statCallback);

var net = require('net');

var HOST = '127.0.0.1';
var PORT = 6969;

var client = net.connect(PORT, HOST, function(error, connectionId) {
  console.log('CONNECTED TO: ' + HOST + ':' + PORT);
  client.write("Hello");
});

client.on('data', function(data) {
  console.log(data);
  console.log(data.toString('utf8'));
  client.destroy();
});

client.on('close', function() {
  console.log('Connection closed');
});



//console.log(require('crypto').randomBytes(40).toString('hex'));

//console.log(fs.readdirSync('/'));