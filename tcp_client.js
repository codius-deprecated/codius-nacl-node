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