var fs = require('fs');
console.log('test contract running');
fs.stat('sandbox.js', function(err, data){
    console.log("Stat for sandbox.js", data);
});

//console.log(fs.readdirSync('/'));


// // console.log('test-contract');

// onmessage = function(message) {
//   // var file_stuff = __readFileSync('file');

//   // var response = {
//   //   'api':'test_module',
//   //   'method':file_stuff
//   // };
//   // postMessage (JSON.stringify(response));

//   //postMessage (JSON.stringify(JSON.parse(message)));

//   var call = {
//     'type': 'api',
//     'api':'test_module',
//     'method':'dummy'
//   };
//   postMessage (JSON.stringify(call));

//   //var parsed_msg = JSON.parse(message);
//   var parsed_msg = JSON.parse(JSON.parse(message));
//   if (parsed_msg.callback === 'foo') {
//     // console.log('Sandbox got callback foo!');
//   }
// };


// var module_call = {
//   type: 'api',
//   api:'test_module',
//   method:'first'
// };
// postMessage(JSON.stringify(module_call));


// var file_stuff = __readFileSync('file');

// // var response = {
// //   'type': 'api',
// //   'api':'test_module',
// //   'method':file_stuff
// // };
// // postMessage (JSON.stringify(response));
