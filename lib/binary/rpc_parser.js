var Parser = require('stream-parser');
var inherits = require('util').inherits;
var Transform = require('stream').Transform;

var format = require('./format');


function RpcParser() {
  Transform.call(this);

  this._bytes(format.HEADER_SIZE, this.onheader); 
}
inherits(RpcParser, Transform);

// mixin stream-parser into MyParser's `prototype`
Parser(RpcParser.prototype);

// invoked when the first 8 bytes have been received
RpcParser.prototype.onheader = function (buffer, output) {
  // parse the "buffer" into a useful "header" object
  var header = {};
  header.magic = buffer.readUInt32LE(0);
  header.callback_id = buffer.readUInt32LE(4);
  header.size = buffer.readUInt32LE(8);
  
  if (header.magic !== format.MAGIC_BYTES) {
    throw new Error("Magic bytes don't match (received: "+buffer.slice(0, 4).toString('hex')+')');
  }
  this.emit('header', header);

  this._bytes(header.size, this.onbody.bind(this, header.callback_id));
};

RpcParser.prototype.onbody = function (callback_id, buffer) {
  this.emit('message', buffer.toString('utf-8'), callback_id);

  this._bytes(format.HEADER_SIZE, this.onheader); 
};

exports.RpcParser = RpcParser;