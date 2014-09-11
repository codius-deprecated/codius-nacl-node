var fs = require('fs');
var dns = require('dns');
var net = require('net');

var format = require('../binary/format');
var RpcParser = require('../binary/rpc_parser').RpcParser;

var PassthroughApi = function (sandbox) {
  this._sandbox = sandbox;
  
  var asyncParser = new RpcParser();
  var syncParser = new RpcParser();

  // fs.createReadStream('test.js').pipe(this._sandbox.stdio[0]);
  this._sandbox.stdio[1].pipe(process.stdout);
  this._sandbox.stdio[2].pipe(process.stderr);
  this._sandbox.stdio[3].pipe(asyncParser);
  this._sandbox.stdio[4].pipe(syncParser);
  
  asyncParser.on('message', this.handleAsyncCall.bind(this));
  syncParser.on('message', this.handleSyncCall.bind(this));
};

PassthroughApi.prototype.handleAsyncCall = function (message_string, callback_id) {
	try {
		message_string
		console.log('>>> [3]', message_string);
		this.handleCall(message_string, this.asyncFilesystemCallback.bind(this, callback_id));
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
	
	var method = (message.method || '').replace(/sync/i, '');
	
	var args;
	if (typeof message.data === 'string') {
		args = [message.data];
	} else if (typeof message.data === 'object') {
		args = message.data;
	}
	
	args.push(callback);

	if (typeof args[0]==='string' && args[0].indexOf('/') === 0) {
	  args[0] = '.' + args[0];
	}

  console.log(args);
  console.log('args.length:', args.length);
	
	//fs[method].apply(null, args);
  switch(message.api) {
    case 'fs':
      fs[method].apply(null, args);
      break;
    case 'dns':
      dns[method].apply(null, args);
      break;
    case 'net':
      //net[method].apply(null, args);
      args[1] = void(0);
      var ret = net[method].apply(null, args);
      var connectionId = connections.length;
      connections.push(ret);
      ret.on('connect', function () {
        callback(null, connectionId);
      });
      break;
    default:
      throw new Error('Unhandled api type: ' + message.api);
  }
};

PassthroughApi.prototype.asyncFilesystemCallback	= function (callback_id, error, result, result2) {
  var response = {
		type: 'callback',
		error: error,
		result: result2 ? [ result, result2 ] : result
	};
  
  var responseString = JSON.stringify(response);
  console.log('<<< [3]', responseString);
  var headerBuffer = new Buffer(format.HEADER_SIZE);
  headerBuffer.writeUInt32LE(format.MAGIC_BYTES, 0);
  headerBuffer.writeUInt32LE(callback_id, 4);
  headerBuffer.writeUInt32LE(responseString.length, 8);
	this._sandbox.stdio[3].write(headerBuffer);
	this._sandbox.stdio[3].write(responseString);
};

PassthroughApi.prototype.syncFilesystemCallback	= function (error, result, result2) {
  var response = {
		type: 'callback',
		error: error,
		result: result2 ? [ result, result2 ] : result
	};

  var responseString = JSON.stringify(response);
  console.log('<<< [4]', responseString);
  var headerBuffer = new Buffer(format.HEADER_SIZE);
  headerBuffer.writeUInt32LE(format.MAGIC_BYTES, 0);
  headerBuffer.writeUInt32LE(0, 4);
  headerBuffer.writeUInt32LE(responseString.length, 8);
	this._sandbox.stdio[4].write(headerBuffer);
	this._sandbox.stdio[4].write(responseString);
};

exports.PassthroughApi = PassthroughApi;