var Sandbox = require('./sandbox');
var PassthroughApi = require('./lib/api/passthrough').PassthroughApi;

var sandbox = new Sandbox({
  api: PassthroughApi,
  disableNaCl: process.env.NONACL || false
});
sandbox.run('', 'test-contract.js');

