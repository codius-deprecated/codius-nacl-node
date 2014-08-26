var Sandbox = require('./sandbox');
var PassthroughApi = require('./lib/api/passthrough').PassthroughApi;

var sandbox = new Sandbox({
  api: PassthroughApi
});
sandbox.run('', 'test-contract.js');

