var fs = require('fs');
var dns = require('dns');
var net = require('net');

var format = require('../binary/format');
var RpcParser = require('../binary/rpc_parser').RpcParser;

var FakeSocket = require('../mock/fake_socket').FakeSocket;

function AsyncResponse(callback_id, resp_message) {
  this.callback_id = callback_id;
  this.message = resp_message;
}

var PassthroughApi = function (sandbox) {
  this._sandbox = sandbox;
  
  var messageParser = new RpcParser();

  // fs.createReadStream('test.js').pipe(this._sandbox.stdio[0]);
  this._sandbox.stdio[1].pipe(process.stdout);
  this._sandbox.stdio[2].pipe(process.stderr);
  this._sandbox.stdio[3].pipe(messageParser);
  
  // We want the first TCP file descriptor to be 5, so that there is no ambiguity
  // with actually existing file descriptors (stdin, out, err, async, sync)
  this._connections = [null, null, null, null, null];

  this._async_responses = [];
  
  messageParser.on('message', this.handleCall.bind(this));
};

PassthroughApi.prototype.handleCall = function (message_string, callback_id) {
	var message;
  var callback;

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
	
  if (message.type==='request_async_response') {
    console.log('>>> [async] {\'type\':\'request_async_response\'}');

    // Pass in the first async response message.
    var headerBuffer = new Buffer(format.HEADER_SIZE);
    var asyncResponse = this._async_responses.shift();

    headerBuffer.writeUInt32LE(format.MAGIC_BYTES, 0);
    headerBuffer.writeUInt32LE(asyncResponse ? asyncResponse.callback_id: 0, 4);
    headerBuffer.writeUInt32LE(asyncResponse ? asyncResponse.message.length : 0, 8);
    this._sandbox.stdio[3].write(headerBuffer);
    if (asyncResponse) {
      this._sandbox.stdio[3].write(asyncResponse.message);
    } else {
      console.log('<<< [async] no async responses');
    }
    return;
  }

  if (callback_id===0) {
    console.log('>>> [sync]', message_string);
    callback = this.syncCallback.bind(this);
  } else if (callback_id>0) {
    console.log('>>> [async]', message_string);
    callback = this.asyncCallback.bind(this, callback_id);
  } else {
    throw new Error('Invalid callback_id: ' + callback_id);
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

	//fs[method].apply(null, args);
  switch(message.api) {
    case 'fs':
      fs[method].apply(null, args);
      break;
    case 'dns':
      dns[method].apply(null, args);
      break;
    case 'net':
    	var sock;
      switch (method) {
    		case 'socket':
    			sock = new FakeSocket(args[0], args[1], args[2]);
      		var connectionId = this._connections.length;
      		this._connections.push(sock);
    			args[3](null, connectionId);
    			break;
    		case 'connect':
    		case 'read':
    		case 'write':
    			sock = this._connections[args[0]];
    			sock[method].apply(sock, args.slice(1));
    			break;
    		default:
    			throw new Error('Unhandled net method: ' + message.method);
    	}
      break;
    default:
      throw new Error('Unhandled api type: ' + message.api);
  }
};

PassthroughApi.prototype.asyncCallback	= function (callback_id, error, result, result2) {
  var response = {
		type: 'callback',
		error: error,
		result: result2 ? [ result, result2 ] : result
	};

  var responseString = JSON.stringify(response);
  console.log('<<< [async]', responseString);

  // Store the asynchronous response message to be retrieved by a synchronous request.
  this._async_responses.push(new AsyncResponse(callback_id, responseString));
};


PassthroughApi.prototype.syncCallback	= function (error, result, result2) {
  var response = {
		type: 'callback',
		error: error,
		result: result2 ? [ result, result2 ] : result
	};

  // Send back the synchronous response message.
  var responseString = JSON.stringify(response);
  console.log('<<< [sync]', responseString);
  var headerBuffer = new Buffer(format.HEADER_SIZE);
  headerBuffer.writeUInt32LE(format.MAGIC_BYTES, 0);
  headerBuffer.writeUInt32LE(0, 4);
  headerBuffer.writeUInt32LE(responseString.length, 8);
  this._sandbox.stdio[3].write(headerBuffer);
  this._sandbox.stdio[3].write(responseString);
};

exports.PassthroughApi = PassthroughApi;