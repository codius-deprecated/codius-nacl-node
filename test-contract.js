// var http = require('http');
// http.get("http://www.example.com/", function(res) {
//   console.log("Got response: " + res.statusCode);
//   var buffer = '';
//   res.setEncoding('utf8');
//   res.on('data', function (chunk) {
//     buffer += chunk;
//   });
//   res.on('end', function () {
//     console.log('body', buffer);
//   });
// }).on('error', function(e) {
//   console.log("Got error: " + e.message);
// });

var https = require('https');
https.get("https://justmoon.net/", function(res) {
  console.log("Got response: " + res.statusCode);
  var buffer = '';
  res.setEncoding('utf8');
  res.on('data', function (chunk) {
    buffer += chunk;
  });
  res.on('end', function () {
    console.log('body', buffer);
  });
}).on('error', function(e) {
  console.log("Got error: " + e.message);
});

// var fs = require('fs');

// console.log('test contract running');

// function statCallback (err, data) {
//   if (err) {
//     console.log('err:', err);
//   } else {  
//     console.log("Stat for sandbox.js", data);
//   }
// };

// fs.stat('sandbox.js', statCallback);

// var codius = process.binding('async');
// var message = {
//   type:'api',
//   api:'fs',
//   method:'stat',
//   data:['sandbox.js']
// };

// codius.postMessage(message, statCallback);

// codius.postMessage(JSON.stringify(message), statCallback);

// var net = require('net');

// var HOST = '127.0.0.1';
// var PORT = 6969;

// var client = net.connect(PORT, HOST, function(error, connectionId) {
//   //console.log('CONNECTED TO: ' + HOST + ':' + PORT);
//   //client.write("Hello");
// });

// client.on('data', function(data) {
//   console.log(data);
//   console.log(data.toString('utf8'));
//   client.destroy();
// });

// client.on('close', function() {
//   console.log('Connection closed');
// });

// var crypto = require('crypto');
// var hash = crypto.createHash('sha512');
// hash.update('hello');
// var result = hash.digest('hex');
// console.log(result);

// console.log(require('crypto').randomBytes(40).toString('hex'));

// var ripple = require('ripple-lib');

// var remote = new ripple.Remote({
//   // see the API Reference for available options
//   servers: [ 'wss://s1.ripple.com:443' ]
// });

// remote.connect(function() {
//   /* remote connected */
//   remote.request('server_info', function(err, info) {
//     console.log(info);
//   });
// });