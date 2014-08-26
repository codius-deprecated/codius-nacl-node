var fs = require('fs');
var concat = require('concat-stream');
var split = require('split');

var PassthroughApi = function (sandbox) {
  this._sandbox = sandbox;

  this._sandbox.stdio[1].pipe(process.stdout);
  this._sandbox.stdio[2].pipe(process.stderr);
  this._sandbox.stdio[3].pipe(concat(process.stdout));
  //this._sandbox.stdio[4].pipe(concat(handleSyncFilesystemCall));
  this._sandbox.stdio[4].pipe(split()).on('data', this.handleSyncCall.bind(this));
};

PassthroughApi.prototype.handleSyncCall = function (message_string) {
	var message;
	
	if (Buffer.isBuffer(message_string)) {
		message_string = message_string.toString('utf8');
	}
	
	console.log('message_string', message_string);
	
	try {
		message = JSON.parse(message_string);
	} catch (e) {
		this.syncFilesystemCallback(null, new Error('Error parsing message JSON: ' + e));
		return;
	}
	
	if (message.type !== 'api' || message.api !== 'fs') {
		this.syncFilesystemCallback(message.callback, new Error('fd 4 reserved for synchronous filesystem calls'));
		return;
	}
	
	var method = (message.method || '').replace(/sync/i, '');
	
	var args;
	if (typeof message.data === 'string') {
		args = [message.data];
	} else if (typeof message.data === 'object') {
		args = message.data;
	}
	
	
	args.push(this.syncFilesystemCallback.bind(this, message.callback));

	if (typeof args[0]==='string' && args[0].indexOf('/') === 0) {
	  args[0] = '.' + args[0];
	}
	
	fs[method].apply(null, args);
};

PassthroughApi.prototype.syncFilesystemCallback	= function (callback_id, error, result, result2) {
  var response = {
		type: 'callback',
		callback: callback_id,
		error: error,
		result: result2 ? [ result, result2 ] : result
	};
	// console.log('response:', response);
	if (error) {
		console.log(error)
		console.log('errno', error.errno);
		console.log('path', error.path);
		console.log('code', error.code);
		console.log('message', error.message);
	}

	this._sandbox.stdio[4].write(JSON.stringify((response)) + '\n');
	// sandbox.stdio[4].end();
};

exports.PassthroughApi = PassthroughApi;