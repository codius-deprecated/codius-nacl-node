var fs = require('fs');
var concat = require('concat-stream');
var split = require('split');

var PassthroughApi = function (sandbox) {
  this._sandbox = sandbox;

  // fs.createReadStream('test.js').pipe(this._sandbox.stdio[0]);
  this._sandbox.stdio[1].pipe(process.stdout);
  this._sandbox.stdio[2].pipe(process.stderr);
  //this._sandbox.stdio[3].pipe(concat(process.stdout));
  //this._sandbox.stdio[4].pipe(concat(handleSyncFilesystemCall));
  this._sandbox.stdio[3].pipe(split()).on('data', this.handleAsyncCall.bind(this));
  this._sandbox.stdio[4].pipe(split()).on('data', this.handleSyncCall.bind(this));
};

PassthroughApi.prototype.handleAsyncCall = function (message_string) {
	try {
		console.log('>>> [3]', message_string);
		this.handleCall(message_string, this.asyncFilesystemCallback.bind(this));
	} catch (e) {
		console.error("handleSyncCall", e);
	}
};

PassthroughApi.prototype.handleSyncCall = function (message_string) {
	try {
		console.log('>>> [4]', message_string);
		this.handleCall(message_string, this.syncFilesystemCallback.bind(this));
	} catch (e) {
		console.error("handleSyncCall", e);
	}
};


PassthroughApi.prototype.handleCall = function (message_string, callback) {
	var message;
	
	if (Buffer.isBuffer(message_string)) {
		message_string = message_string.toString('utf8');
	}
	
	if (message_string === '') return;
	
	try {
		message = JSON.parse(message_string);
	} catch (e) {
		throw new Error('Error parsing message JSON: ' + e);
		return;
	}
	
	if (message.type !== 'api' || message.api !== 'fs') {
		callback(message.callback, new Error('fd 4 reserved for synchronous filesystem calls'));
		return;
	}
	
	var method = (message.method || '').replace(/sync/i, '');
	
	var args;
	if (typeof message.data === 'string') {
		args = [message.data];
	} else if (typeof message.data === 'object') {
		args = message.data;
	}
	
	
	args.push(callback.bind(this, message.callback));

	if (typeof args[0]==='string' && args[0].indexOf('/') === 0) {
	  args[0] = '.' + args[0];
	}
	
	fs[method].apply(null, args);
};

PassthroughApi.prototype.asyncFilesystemCallback	= function (callback_id, error, result, result2) {
  var response = {
		type: 'callback',
		callback: callback_id,
		error: error,
		result: result2 ? [ result, result2 ] : result
	};

  var responseString = JSON.stringify(response);
  console.log('<<< [3]', responseString);
	this._sandbox.stdio[3].write(responseString + '\n');
};

PassthroughApi.prototype.syncFilesystemCallback	= function (callback_id, error, result, result2) {
  var response = {
		type: 'callback',
		callback: callback_id,
		error: error,
		result: result2 ? [ result, result2 ] : result
	};

  var responseString = JSON.stringify(response);
  console.log('<<< [4]', responseString);
	this._sandbox.stdio[4].write(responseString + '\n');
};

exports.PassthroughApi = PassthroughApi;