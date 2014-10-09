var fs = require('fs');
var spawn = require('child_process').spawn;
var util = require('util');
var path = require('path');
var EventEmitter = require('events').EventEmitter;

// TODO: Make these configurable
var NACL_SDK_ROOT = process.env.NACL_SDK_ROOT;
var RUN_CONTRACT_COMMAND = NACL_SDK_ROOT + '/tools/sel_ldr_x86_32';
var RUN_CONTRACT_LIBS = [
	__dirname,
	path.resolve(__dirname, 'deps/v8/out/nacl_ia32.release/lib.target'),
	NACL_SDK_ROOT + '/ports/lib/glibc_x86_32/Release',
	NACL_SDK_ROOT + '/toolchain/linux_x86_glibc/x86_64-nacl/lib32',
	NACL_SDK_ROOT + '/lib/glibc_x86_32/Debug'
]
var RUN_CONTRACT_ARGS = [
  '-h', 
  '3:3',
  '-a', 
  '--', 
  NACL_SDK_ROOT + '/toolchain/linux_x86_glibc/x86_64-nacl/lib32/runnable-ld.so', 
  '--library-path', 
  RUN_CONTRACT_LIBS.join(':'),
  path.resolve(__dirname, 'codius_node.nexe')
];
var RUN_CONTRACT_COMMAND_NONACL = path.resolve(__dirname, 'codius_node');
var RUN_CONTRACT_ARGS_NONACL = []

/**
 * Sandbox class wrapper around Native Client
 */
function Sandbox(opts) {
	var self = this;
	
	if (!opts) {
		opts = {};
	}

	self.stdio = null;
	self._timeout = opts.timeout || 1000;
	self._apiClass = opts.api || null;
	self._disableNaCl = opts.disableNaCl || false;
	self._enableGdb = opts.enableGdb || false;
	self._enableValgrind = opts.enableValgrind || false;

	self._native_client_child = null;

}
util.inherits(Sandbox, EventEmitter);

/**
 * Run the given file inside the sandbox
 *
 * @param {String} manifest_hash
 * @param {String} file_path
 */
Sandbox.prototype.run = function(manifest_hash, file_path) {
	var self = this;

	// Create new sandbox
	self._native_client_child = self.spawnChildToRunCode(file_path, self._disableNaCl);
	self._native_client_child.on('exit', function(code){
		self.emit('exit', code);
	});
	self.stdio = self._native_client_child.stdio;

	if (this._apiClass) {
		this._api = new this._apiClass(this);
	}
};

Sandbox.prototype.spawnChildToRunCode = function (code, disableNaCl) {
  if (typeof disableNaCl==='string') {
    disableNaCl = parseInt(disableNaCl);
  }

	var cmd = disableNaCl ? RUN_CONTRACT_COMMAND_NONACL : RUN_CONTRACT_COMMAND;
	var args = disableNaCl ? RUN_CONTRACT_ARGS_NONACL.slice() : RUN_CONTRACT_ARGS.slice();

	args.push(code);

	if (this._enableGdb) {
		args.unshift(cmd);
		args.unshift('localhost:4483');
		cmd = 'gdbserver';
	} else if (this._enableValgrind) {
		args.unshift('--');
		args.unshift(cmd);
		args.unshift('--leak-check=full');
		cmd = 'valgrind';
	}
  var env = {
    TEST:'/this/test/file'
  }
	var child = spawn(cmd, args, {
    env: env,
	  stdio: [
	    'pipe',
	    'pipe',
	    'pipe',
	    'pipe'
	    ] 
	  });

	return child;
}

/**
 * Kill the sandbox process.
 *
 * @param {String} ['SIGKILL'] message
 */
Sandbox.prototype.kill = function(message){
	var self = this;

	if (!message) {
		message = 'SIGKILL';
	}
	self._native_client_child.kill(message);
};

module.exports = Sandbox;
