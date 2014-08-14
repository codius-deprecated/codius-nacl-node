onmessage = function(message) {
  // var file_stuff = __readFileSync('file');

  // var response = {
  //   'api':'test_module',
  //   'method':file_stuff
  // };
  // postMessage (JSON.stringify(response));

  //postMessage (JSON.stringify(JSON.parse(message)));

  var call = {
    'api':'test_module',
    'method':'dummy'
  };
  postMessage (JSON.stringify(call));

  //var parsed_msg = JSON.parse(message);
  var parsed_msg = JSON.parse(JSON.parse(message));
  if (1 || parsed_msg.call==='foo') {

    var module_call = {
      api:'test_module',
      //method:parsed_msg
      method:'callback'
    };
    postMessage (JSON.stringify(module_call));
  }
};


var module_call = {
  api:'test_module',
  method:'first'
};

postMessage(JSON.stringify(module_call));


var file_stuff = __readFileSync('file');

var response = {
  'api':'test_module',
  'method':file_stuff
};
postMessage (JSON.stringify(response));
