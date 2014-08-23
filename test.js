var fs = require('fs');
var Sandbox = require('./sandbox');
var concat = require('concat-stream');
var split = require('split');

var sandbox = new Sandbox();
sandbox.run('', 'test-contract.js');
sandbox.stdio[1].pipe(process.stdout);
sandbox.stdio[2].pipe(process.stderr);
sandbox.stdio[3].pipe(concat(process.stdout));
//sandbox.stdio[4].pipe(concat(handleSyncFilesystemCall));
sandbox.stdio[4].pipe(split()).on('data', handleSyncFilesystemCall);

function handleSyncFilesystemCall(message_string) {
	var message;
	
	if (Buffer.isBuffer(message_string)) {
		message_string = message_string.toString('utf8');
	}
	
	console.log('message_string', message_string);
	
	try {
		message = JSON.parse(message_string);
	} catch (e) {
		syncFilesystemCallback(null, new Error('Error parsing message JSON: ' + e));
		return;
	}
	
	if (message.type !== 'api' || message.api !== 'fs') {
		syncFilesystemCallback(message.callback, new Error('fd 4 reserved for synchronous filesystem calls'));
		return;
	}
	
	var method = (message.method || '').replace(/sync/i, '');
	
	var args;
	if (typeof message.data === 'string') {
		args = [message.data];
	} else if (typeof message.data === 'object') {
		args = message.data;
	}
	
	
	args.push(syncFilesystemCallback.bind(null, message.callback));

	if (args[0]!==0 && typeof args[0]==='string' && args[0].indexOf('/') === 0) {
	  args[0] = '.' + args[0];
	}
	
	fs[method].apply(null, args);
	
}

function syncFilesystemCallback(callback_id, error, result) {
	var response = {
		type: 'callback',
		callback: callback_id,
		error: error,
		result: result
	};
	// console.log('sending response: ', response);
	sandbox.stdio[4].write(JSON.stringify((response)) + '\n');
	// sandbox.stdio[4].end();
}




// sandbox.on('message', function(message){
// 	console.log('Message received from inside sandbox: ', message);
// });

// sandbox.postMessage(JSON.stringify({
// 	"type": "callback",
// 	"callback": "foo",
// 	"error": null,
// 	"result": "hello"
// }));
