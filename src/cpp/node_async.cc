#include "node.h"
#include "node_async.h"

#include "env.h"
#include "env-inl.h"
#include "v8.h"

#include <unistd.h>
#include <string.h>

namespace node {
namespace Async {

using v8::Context;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Value;
using v8::TryCatch;

struct Async_req
{
  const char *data;
  size_t data_length;
  Isolate* isolate;
    
  Persistent<Function> callback;
};

void AsyncAfter(uv_work_t* req, int something, const char *buf, size_t buf_len)
{
  Handle<Object> response;
  
  Async_req *data = ((struct Async_req*)req->data);
  
  // Parse the response.
  Local<String> response_str = String::NewFromUtf8(data->isolate, buf, 
                                                   String::kNormalString, buf_len);
  Local<Object> global = data->isolate->GetCurrentContext()->Global();
  Handle<Object> JSON = global->Get(String::NewFromUtf8(
                                      data->isolate, "JSON"))->ToObject();
  Handle<Function> JSON_parse = Handle<Function>::Cast(JSON->Get(
                                  String::NewFromUtf8(data->isolate,
                                                      "parse")));
  Local<Value> parse_args[] = { response_str };
  response = Handle<Object> (JSON_parse->Call(JSON, 1, parse_args)->ToObject());
  Handle<Value> resp_err = response->Get(String::NewFromUtf8(data->isolate,
                                                                "error"));
  
  Local<Value> args[] = {
    resp_err,
    response->Get(String::NewFromUtf8(data->isolate, "result"))
  };
  
  TryCatch try_catch;

  //defined in async-wrap-inl.h
  // MakeCallback(data->isolate,
  //             data->callback,
  //             2, args);
               
               
  Local<Function> callback_fn = Local<Function>::New(data->isolate, data->callback);
  callback_fn->Call(data->isolate->GetCurrentContext()->Global(), 2, args);
  
  if (try_catch.HasCaught()) 
  {
      FatalException(try_catch);
  }

  //data->callback.Dispose();

  //delete request;
  delete req;
  
                                                                
  // HandleScope scope;

  // Async_req* request = (Async_req*)req->data;
  // delete req;

  // Handle<Value> argv[2];

  // // XXX: Error handling
  // argv[0] = Undefined();
  // argv[1] = Integer::New(request->result);

  // TryCatch try_catch;

  // request->callback->Call(Context::GetCurrent()->Global(), 2, argv);

  // if (try_catch.HasCaught()) 
  // {
  //     FatalException(try_catch);
  // }

  // request->callback.Dispose();

  // delete request;
}
                          
void PostMessage(Environment* env, const char *data, size_t data_length, Handle<Function> callback) {
  //TODO-CODIUS uv__work_submit
  Async_req* request = new Async_req;
  
  request->data = data;
  request->data_length = data_length;
  request->isolate = env->isolate();
  //request->callback = Persistent<Function>::New(env->isolate(), callback);
  request->callback.Reset(env->isolate(), callback);
  
  uv_work_t* req = new uv_work_t();
  req->data = request;

  uv_queue_work(env->event_loop(), req, data, data_length, AsyncAfter);
}

static void PostMessage(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());

  if (args.Length() < 2)
    return ThrowError(env->isolate(), "needs argument object and callback");

  if (!args[1]->IsFunction()) {
    return ThrowError(env->isolate(), "second argument should be a callback");
  }

  if (args[0]->IsString()) {
    v8::String::Utf8Value message(args[0]);
    PostMessage(env, *message, message.length(), Handle<Function>::Cast(args[1]));
  } else if (args[0]->IsObject()) {

    // Stringify the JSON
    Local<Object> global = env->context()->Global();
    Handle<Object> JSON = global->Get(String::NewFromUtf8(
                                        env->isolate(), "JSON"))->ToObject();
    Handle<Function> JSON_stringify = Handle<Function>::Cast(JSON->Get(
                                        String::NewFromUtf8(env->isolate(),
                                                            "stringify")));
    Local<Value> stringify_args[] = { args[0] };
    Local<String> str = JSON_stringify->Call(JSON, 1, stringify_args)->ToString();
    v8::String::Utf8Value message(str);
    PostMessage(env, *message, message.length(), Handle<Function>::Cast(args[1]));
  } else {
    return ThrowError(env->isolate(), "first argument should be a message (string or object)");
  }
}

void Initialize(Handle<Object> target,
                Handle<Value> unused,
                Handle<Context> context) {
  Environment* env = Environment::GetCurrent(context);

  NODE_SET_METHOD(target, "postMessage", PostMessage);
}


}  // namespace Async
}  // namespace node

NODE_MODULE_CONTEXT_AWARE_BUILTIN(async, node::Async::Initialize)
