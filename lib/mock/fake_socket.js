var net = require('net');

var FakeSocket = function (domain, type, protocol) {
  if (domain !== FakeSocket.AF_INET) {
    throw new Error("Unsupported socket domain: "+domain);
  }

  if (type !== FakeSocket.SOCK_STREAM) {
    throw new Error("Unsupported socket type: "+type);
  }
  
  if (protocol !== 0) {
    throw new Error("Unsupported protocol: "+protocol);
  }
  
  this._socket = null;
  this._buffer = [];
}

FakeSocket.AF_INET = 2;

FakeSocket.SOCK_STREAM = 1;

FakeSocket.prototype.connect = function (family, address, port, callback) {
  var self = this;

  if (family != FakeSocket.AF_INET) {
    throw new Error("Unsupported socket family: "+family);
  }
  
  var addressArray = [
    address       & 0xff,
    address >>  8 & 0xff,
    address >> 16 & 0xff,
    address >> 24 & 0xff
  ];
  
  // Convert endianness
  port = (port >> 8 & 0xff) + (port << 8 & 0xffff);
  
  self._socket = net.createConnection(port, addressArray.join('.'));
  self._socket.once('connect', function (e) {
    callback(null, 0);
  })

  self._socket.on('data', function(data){
    self._buffer.push(data);
  });
};

FakeSocket.prototype.read = function (callback) {
  callback(null, Buffer.concat(this._buffer).toString('hex'));
  this._buffer = [];
};

FakeSocket.prototype.write = function (stringToWrite, callback) {
  this._socket.write(stringToWrite);
  callback(null);
}

FakeSocket.prototype.close = function (callback) {
  this._socket.destroy();
  callback(null);
}

exports.FakeSocket = FakeSocket;