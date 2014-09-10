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
