var fs = require('fs');
var Sandbox = require('./sandbox');

var sandbox = new Sandbox({
	sandbox_filesystem_path: '/home/evan/codius-lang-js/contract_filesystem/'
});

var code = fs.readFileSync('test-contract.js', 'utf8');

sandbox.run('', code);
sandbox.on('message', function(message){
	console.log('Message received from inside sandbox: ', message);
});

sandbox.postMessage(JSON.stringify({
	"type": "callback",
	"callback": "foo",
	"error": null,
	"result": "hello"
}));
