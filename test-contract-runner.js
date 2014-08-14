// Run this test with `node ipc-test.js <command to run>`
// This will log messages received from the child process
// and 1 second after it spawns the child will send a message to it

var ChildProcess = require('child_process');
var fs = require('fs');
var split = require('split');

var command_to_run = process.argv[3];

var args = process.argv.slice(4);

// Append JavaScript contract code to args.
var contract_file_name = process.argv[2];
console.log('contract_file_name: ', contract_file_name);

var file = fs.readFileSync(contract_file_name, "utf8");

args.push(file);

var child = ChildProcess.spawn(command_to_run, args, { stdio: [process.stdin, process.stdout, process.stderr, 'ipc', 'pipe'] });

// Post message
child.on('message', function(message){
  console.log(message);
  if (message.api==='file_system') {
    message.data = 'file contents';
    child.send(JSON.stringify(message));
  } /*else if (message.api==='test_module') {
    console.log('test_module call')
    message.data = 'test_module contents';
    child.send(JSON.stringify(message));
  }*/

  if (message.hasOwnProperty('resp') && message.resp.foo==='bar') {
    console.log ('parsed the message')
  }
});

// Filesystem access
child.stdio[4].pipe(split()).on('data', function(data) {
  console.log('data: ' + String(data));
  //child.stdio[4].write(String(data));
});

child.on('error', function(error){
	console.log('child had error: ', error)
});
child.on('exit', function(data){
	console.log('child exited with data: ', data)
});

setTimeout(function(){
  var foo = {
    'resp': {
      'foo':'bar\nstuff'
    }
  };
  child.send(JSON.stringify(foo));
  var bar = {
    'resp': {
      'bar':5
    }
  };
  child.send(JSON.stringify(bar));
}, 1000);