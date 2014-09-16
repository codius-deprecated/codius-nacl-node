// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "node.h"
#include "node_file.h"
#include "node_async.h"
#include "node_buffer.h"
#include "node_internals.h"

#include "env.h"
#include "env-inl.h"
#include "string_bytes.h"
#include "util.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include <fcntl.h>

#if defined(__MINGW32__) || defined(_MSC_VER)
# include <io.h>
#endif

namespace node {

using v8::Array;
using v8::Context;
using v8::EscapableHandleScope;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::Integer;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Value;

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define TYPE_ERROR(msg) env->ThrowTypeError(msg)

#define THROW_BAD_ARGS TYPE_ERROR("Bad argument")

#define ASSERT_OFFSET(a) \
  if (!(a)->IsUndefined() && !(a)->IsNull() && !IsInt64((a)->NumberValue())) { \
    return env->ThrowTypeError("Not an integer"); \
  }
#define ASSERT_TRUNCATE_LENGTH(a) \
  if (!(a)->IsUndefined() && !(a)->IsNull() && !IsInt64((a)->NumberValue())) { \
    return env->ThrowTypeError("Not an integer"); \
  }
#define GET_OFFSET(a) ((a)->IsNumber() ? (a)->IntegerValue() : -1)
#define GET_TRUNCATE_LENGTH(a) ((a)->IntegerValue())

static inline bool IsInt64(double x) {
  return x == static_cast<double>(static_cast<int64_t>(x));
}

#define SYNC_RESULT !response->Get(String::NewFromUtf8(                       \
                                     env->isolate(), "error"))->IsNull() ? -1 :\
                                     response->Get(String::NewFromUtf8(\
                                                  env->isolate(),             \
                                                  "result"))->Int32Value()

struct codius_rpc_header {
  unsigned long magic_bytes;
  unsigned long callback_id;
  unsigned long size;
};

static int Sync_Call(Environment* env, const char* func, 
                     const FunctionCallbackInfo<Value>& args,
                     Handle<Object>* response) {
  // If you hit this assertion, you forgot to enter the v8::Context first.
  assert(env->context() == env->isolate()->GetCurrentContext());
  size_t bytes_read;
  const int fd = 3;
  Local<Object> message = Object::New(env->isolate());
  Handle<Array> params;
  
  // Use legacy string interface for read so buffer isn't passed in:
  // (fd, length, position, encoding)
  if (0==strcmp(func, "read")) {
    params = Array::New(env->isolate(), 4);
    params->Set(0, args[0]);
    params->Set(1, args[3]);
    params->Set(2, args[4]);
    params->Set(3, String::NewFromUtf8(env->isolate(), "utf8"));
  } else {
    params = Array::New(env->isolate(), args.Length());
    for (int i=0; i<args.Length(); ++i) {
      params->Set(i, args[i]);
    }
  }

  message->Set(String::NewFromUtf8(env->isolate(), "type"),
               String::NewFromUtf8(env->isolate(), "api"));
  message->Set(String::NewFromUtf8(env->isolate(), "api"),
               String::NewFromUtf8(env->isolate(), "fs"));
  message->Set(String::NewFromUtf8(env->isolate(), "method"),
               String::NewFromUtf8(env->isolate(), func));
  message->Set(String::NewFromUtf8(env->isolate(), "data"),
               params);
               
  // Stringify the JSON message.
  Local<Object> global = env->context()->Global();
  Handle<Object> JSON = global->Get(String::NewFromUtf8(
                                      env->isolate(), "JSON"))->ToObject();
  Handle<Function> JSON_stringify = Handle<Function>::Cast(JSON->Get(
                                      String::NewFromUtf8(env->isolate(),
                                                          "stringify")));
  Local<Value> stringify_args[] = { message };
  Local<Value> message_json = JSON_stringify->Call(JSON, 1, stringify_args);
  node::Utf8Value message_v(message_json);
  if (!message_v.length()) {
    TYPE_ERROR("error converting json to string");
    return -1;
  }

  const unsigned long codius_magic_bytes = 0xC0D105FE;
  codius_rpc_header rpc_header;
  rpc_header.magic_bytes = codius_magic_bytes;
  rpc_header.callback_id = 0;
  rpc_header.size = message_v.length();
  
  if (-1==write(fd, &rpc_header, sizeof(rpc_header)) ||
      -1==write(fd, *message_v, strlen(*message_v))) {
    perror("write()");
    TYPE_ERROR("Error writing to sync fd 4");
    return -1;
  }
  
  bytes_read = read(fd, &rpc_header, sizeof(rpc_header));
  if (rpc_header.magic_bytes!=codius_magic_bytes) {
    TYPE_ERROR("Error reading sync fd 4, invalid header");
  }
  
  char resp_buf[rpc_header.size];
  bytes_read = read (fd, &resp_buf, sizeof(resp_buf));
  Local<String> response_str = String::NewFromUtf8(env->isolate(), resp_buf, 
                                                   String::kNormalString,
                                                   sizeof(resp_buf));
  
  // Parse the response.
  Handle<Function> JSON_parse = Handle<Function>::Cast(JSON->Get(
                                  String::NewFromUtf8(env->isolate(),
                                                      "parse")));
  Local<Value> parse_args[] = { response_str };
  *response = Handle<Object> (JSON_parse->Call(JSON, 1, parse_args)->ToObject());
  Handle<Value> resp_err = (*response)->Get(String::NewFromUtf8(env->isolate(),
                                                                "error"));
  if (!resp_err->IsNull()) {
    Handle<Object> resp_obj = resp_err->ToObject();

    Local<String> estring = 
        resp_obj->Get(String::NewFromUtf8(env->isolate(), "code"))->ToString();
    
    //TODO-CODIUS Get the error message using only the error code.    
    Local<String> message = String::NewFromUtf8(env->isolate(), func);
    Local<String> path = 
        resp_obj->Get(String::NewFromUtf8(env->isolate(), "path"))->ToString();
    
    Local<String> cons1 =
        String::Concat(estring, FIXED_ONE_BYTE_STRING(env->isolate(), ", "));
    Local<String> cons2 = 
        String::Concat(cons1, message);
    Local<String> cons3 =
        String::Concat(cons2, FIXED_ONE_BYTE_STRING(env->isolate(), " '"));
    Local<String> cons4 =
        String::Concat(cons3, path);
    Local<String> cons5 =
        String::Concat(cons4, FIXED_ONE_BYTE_STRING(env->isolate(), "'"));

    env->ThrowError(env->isolate(), *Utf8Value(cons5));

    return -1;
  }
  
  return 0;
}

static int Async_Call(Environment* env, const char* func,
                      Handle<Function> callback,
                      const FunctionCallbackInfo<Value>& args) {
  // If you hit this assertion, you forgot to enter the v8::Context first.
  assert(env->context() == env->isolate()->GetCurrentContext());
  Local<Object> message = Object::New(env->isolate());
  Handle<Array> params;
  
  // Use legacy string interface for read so buffer isn't passed in:
  // (fd, length, position, encoding)
  if (0==strcmp(func, "read")) {
    params = Array::New(env->isolate(), 4);
    params->Set(0, args[0]);
    params->Set(1, args[3]);
    params->Set(2, args[4]);
    params->Set(3, String::NewFromUtf8(env->isolate(), "utf8"));
  } else {
    params = Array::New(env->isolate(), args.Length()-1);
    for (int i=0; i<args.Length()-1; ++i) {
      params->Set(i, args[i]);
    }
  }

  message->Set(String::NewFromUtf8(env->isolate(), "type"),
               String::NewFromUtf8(env->isolate(), "api"));
  message->Set(String::NewFromUtf8(env->isolate(), "api"),
               String::NewFromUtf8(env->isolate(), "fs"));
  message->Set(String::NewFromUtf8(env->isolate(), "method"),
               String::NewFromUtf8(env->isolate(), func));
  message->Set(String::NewFromUtf8(env->isolate(), "data"),
               params);

  // Stringify the JSON message.
  Local<Object> global = env->context()->Global();
  Handle<Object> JSON = global->Get(String::NewFromUtf8(
                                      env->isolate(), "JSON"))->ToObject();
  Handle<Function> JSON_stringify = Handle<Function>::Cast(JSON->Get(
                                      String::NewFromUtf8(env->isolate(),
                                                          "stringify")));
  Local<Value> stringify_args[] = { message };
  Local<Value> message_json = JSON_stringify->Call(JSON, 1, stringify_args);
  node::Utf8Value message_v(message_json);
  if (!message_v.length()) {
    TYPE_ERROR("error converting json to string");
    return -1;
  }
  
  Async::PostMessage(env, *message_v, message_v.length(), callback);
  
  return 0;
}


static void Close(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());

  if (args.Length() < 1 || !args[0]->IsInt32()) {
    return THROW_BAD_ARGS;
  }

  Handle<Object> response;
  Sync_Call(env, "close", args, &response);
}


Local<Value> BuildStatsObject(Environment* env, const Handle<Object> src) {
  // If you hit this assertion, you forgot to enter the v8::Context first.
  assert(env->context() == env->isolate()->GetCurrentContext());

  EscapableHandleScope handle_scope(env->isolate());
  
  // Pass stats as the first argument, this is the object we are modifying.
  Local<Value> argv[] = {
    src->Get(String::NewFromUtf8(env->isolate(), "dev")),
    src->Get(String::NewFromUtf8(env->isolate(), "mode")),
    src->Get(String::NewFromUtf8(env->isolate(), "nlink")),
    src->Get(String::NewFromUtf8(env->isolate(), "uid")),
    src->Get(String::NewFromUtf8(env->isolate(), "gid")),
    src->Get(String::NewFromUtf8(env->isolate(), "rdev")),
    src->Get(String::NewFromUtf8(env->isolate(), "blksize")),
    src->Get(String::NewFromUtf8(env->isolate(), "ino")),
    src->Get(String::NewFromUtf8(env->isolate(), "size")),
    src->Get(String::NewFromUtf8(env->isolate(), "blocks")),
    src->Get(String::NewFromUtf8(env->isolate(), "atim_msec")),
    src->Get(String::NewFromUtf8(env->isolate(), "mtim_msec")),
    src->Get(String::NewFromUtf8(env->isolate(), "ctim_msec")),
    src->Get(String::NewFromUtf8(env->isolate(), "birthtim_msec"))
  };

  // Call out to JavaScript to create the stats object.
  Local<Value> stats =
    env->fs_stats_constructor_function()->NewInstance(ARRAY_SIZE(argv), argv);

  if (stats.IsEmpty())
    return handle_scope.Escape(Local<Object>());

  return handle_scope.Escape(stats);
}


static void Stat(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());
  
  if (args.Length() < 1)
    return TYPE_ERROR("path required");
  if (!args[0]->IsString())
    return TYPE_ERROR("path must be a string");
  
  node::Utf8Value path(args[0]);
  
  if (args[1]->IsFunction()) {
    Async_Call(env, "stat", Handle<Function>::Cast(args[1]), args);
  } else {
    Handle<Object> response;
    if (-1==Sync_Call(env, "stat", args, &response))
      return;
    
    //What if response.result is empty?
    args.GetReturnValue().Set(
      BuildStatsObject(env, 
        response->Get(String::NewFromUtf8(env->isolate(), "result"))->ToObject()));
  }
}

static void LStat(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());

  if (args.Length() < 1)
    return TYPE_ERROR("path required");
  if (!args[0]->IsString())
    return TYPE_ERROR("path must be a string");

  if (args[1]->IsFunction()) {
    Async_Call(env, "lstat", Handle<Function>::Cast(args[1]), args);
  } else {
    Handle<Object> response;
    if (-1==Sync_Call(env, "lstat", args, &response))
      return;
    args.GetReturnValue().Set(
      BuildStatsObject(env, 
        response->Get(String::NewFromUtf8(env->isolate(), "result"))->ToObject()));
  }
}

static void FStat(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());

  if (args.Length() < 1 || !args[0]->IsInt32()) {
    return THROW_BAD_ARGS;
  }
  
  if (args[1]->IsFunction()) {
    Async_Call(env, "fstat", Handle<Function>::Cast(args[1]), args);
  } else {
    Handle<Object> response;
    Sync_Call(env, "fstat", args, &response);
    args.GetReturnValue().Set(
      BuildStatsObject(env, 
        response->Get(String::NewFromUtf8(env->isolate(), "result"))->ToObject()));
  }
}

//static void Symlink(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  int len = args.Length();
//  if (len < 1)
//    return TYPE_ERROR("dest path required");
//  if (len < 2)
//    return TYPE_ERROR("src path required");
//  if (!args[0]->IsString())
//    return TYPE_ERROR("dest path must be a string");
//  if (!args[1]->IsString())
//    return TYPE_ERROR("src path must be a string");
//
//  node::Utf8Value dest(args[0]);
//  node::Utf8Value path(args[1]);
//  int flags = 0;
//
//  if (args[2]->IsString()) {
//    node::Utf8Value mode(args[2]);
//    if (strcmp(*mode, "dir") == 0) {
//      flags |= UV_FS_SYMLINK_DIR;
//    } else if (strcmp(*mode, "junction") == 0) {
//      flags |= UV_FS_SYMLINK_JUNCTION;
//    } else if (strcmp(*mode, "file") != 0) {
//      return env->ThrowError("Unknown symlink type");
//    }
//  }
//
//  SYNC_DEST_CALL(symlink, *path, *dest, *dest, *path, flags)
//}
//
//static void Link(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  int len = args.Length();
//  if (len < 1)
//    return TYPE_ERROR("dest path required");
//  if (len < 2)
//    return TYPE_ERROR("src path required");
//  if (!args[0]->IsString())
//    return TYPE_ERROR("dest path must be a string");
//  if (!args[1]->IsString())
//    return TYPE_ERROR("src path must be a string");
//
//  node::Utf8Value orig_path(args[0]);
//  node::Utf8Value new_path(args[1]);
//
//  SYNC_DEST_CALL(link, *orig_path, *new_path, *orig_path, *new_path)
//}
//
//static void ReadLink(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  if (args.Length() < 1)
//    return TYPE_ERROR("path required");
//  if (!args[0]->IsString())
//    return TYPE_ERROR("path must be a string");
//
//  node::Utf8Value path(args[0]);
//
//  SYNC_CALL(readlink, *path, *path)
//  const char* link_path = static_cast<const char*>(SYNC_REQ.ptr);
//  Local<String> rc = String::NewFromUtf8(env->isolate(), link_path);
//  args.GetReturnValue().Set(rc);
//}
//
//static void Rename(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  int len = args.Length();
//  if (len < 1)
//    return TYPE_ERROR("old path required");
//  if (len < 2)
//    return TYPE_ERROR("new path required");
//  if (!args[0]->IsString())
//    return TYPE_ERROR("old path must be a string");
//  if (!args[1]->IsString())
//    return TYPE_ERROR("new path must be a string");
//
//  node::Utf8Value old_path(args[0]);
//  node::Utf8Value new_path(args[1]);
//
//  SYNC_DEST_CALL(rename, *old_path, *new_path, *old_path, *new_path)
//}
//
//static void FTruncate(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  if (args.Length() < 2 || !args[0]->IsInt32()) {
//    return THROW_BAD_ARGS;
//  }
//
//  int fd = args[0]->Int32Value();
//
//  ASSERT_TRUNCATE_LENGTH(args[1]);
//  int64_t len = GET_TRUNCATE_LENGTH(args[1]);
//
//  SYNC_CALL(ftruncate, 0, fd, len)
//}
//
//static void Fdatasync(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  if (args.Length() < 1 || !args[0]->IsInt32()) {
//    return THROW_BAD_ARGS;
//  }
//
//  int fd = args[0]->Int32Value();
//
//  SYNC_CALL(fdatasync, 0, fd)
//}
//
//static void Fsync(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  if (args.Length() < 1 || !args[0]->IsInt32()) {
//    return THROW_BAD_ARGS;
//  }
//
//  int fd = args[0]->Int32Value();
//
//  SYNC_CALL(fsync, 0, fd)
//}

//static void Unlink(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  if (args.Length() < 1)
//    return TYPE_ERROR("path required");
//  if (!args[0]->IsString())
//    return TYPE_ERROR("path must be a string");
//
//  node::Utf8Value path(args[0]);
//
//  SYNC_CALL(unlink, *path, *path)
//}
//
//static void RMDir(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  if (args.Length() < 1)
//    return TYPE_ERROR("path required");
//  if (!args[0]->IsString())
//    return TYPE_ERROR("path must be a string");
//
//  node::Utf8Value path(args[0]);
//
//  SYNC_CALL(rmdir, *path, *path)
//}
//
//static void MKDir(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsInt32()) {
//    return THROW_BAD_ARGS;
//  }
//
//  node::Utf8Value path(args[0]);
//  int mode = static_cast<int>(args[1]->Int32Value());
//
//  SYNC_CALL(mkdir, *path, *path, mode)
//}

static void ReadDir(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());

  if (args.Length() < 1)
    return TYPE_ERROR("path required");
  if (!args[0]->IsString())
    return TYPE_ERROR("path must be a string");

  if (args[1]->IsFunction()) {
    Async_Call(env, "readdir", Handle<Function>::Cast(args[1]), args);
  } else {
    Handle<Object> response;
    if (-1==Sync_Call(env, "readdir", args, &response))
      return;
    
    Local<Array> names = Handle<Array>::Cast(response->Get(String::NewFromUtf8(env->isolate(), "result")));
  
    args.GetReturnValue().Set(names);
  }
}

static void Open(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());
  
  int len = args.Length();
  if (len < 1)
    return TYPE_ERROR("path required");
  if (len < 2)
    return TYPE_ERROR("flags required");
  if (len < 3)
    return TYPE_ERROR("mode required");
  if (!args[0]->IsString())
    return TYPE_ERROR("path must be a string");
  if (!args[1]->IsInt32())
    return TYPE_ERROR("flags must be an int");
  if (!args[2]->IsInt32())
    return TYPE_ERROR("mode must be an int");

  if (args[3]->IsFunction()) {
    Async_Call(env, "open", Handle<Function>::Cast(args[3]), args);
  } else {
    Handle<Object> response;
    if (-1==Sync_Call(env, "open", args, &response)) {
      args.GetReturnValue().Set(-1);
    } else {
      args.GetReturnValue().Set(response->Get(String::NewFromUtf8(
                                                env->isolate(),
                                                "result"))->Int32Value());
    }
  }
}


//// Wrapper for write(2).
////
//// bytesWritten = write(fd, buffer, offset, length, position, callback)
//// 0 fd        integer. file descriptor
//// 1 buffer    the data to write
//// 2 offset    where in the buffer to start from
//// 3 length    how much to write
//// 4 position  if integer, position to write at in the file.
////             if null, write from the current position
//static void WriteBuffer(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  assert(args[0]->IsInt32());
//  assert(Buffer::HasInstance(args[1]));
//
//  int fd = args[0]->Int32Value();
//  Local<Object> obj = args[1].As<Object>();
//  const char* buf = Buffer::Data(obj);
//  size_t buffer_length = Buffer::Length(obj);
//  size_t off = args[2]->Uint32Value();
//  size_t len = args[3]->Uint32Value();
//  int64_t pos = GET_OFFSET(args[4]);
//  Local<Value> cb = args[5];
//
//  if (off > buffer_length)
//    return env->ThrowRangeError("offset out of bounds");
//  if (len > buffer_length)
//    return env->ThrowRangeError("length out of bounds");
//  if (off + len < off)
//    return env->ThrowRangeError("off + len overflow");
//  if (!Buffer::IsWithinBounds(off, len, buffer_length))
//    return env->ThrowRangeError("off + len > buffer.length");
//
//  buf += off;
//
//  uv_buf_t uvbuf = uv_buf_init(const_cast<char*>(buf), len);
//
//  if (cb->IsFunction()) {
//    ASYNC_CALL(write, cb, fd, &uvbuf, 1, pos)
//    return;
//  }
//
//  SYNC_CALL(write, NULL, fd, &uvbuf, 1, pos)
//  args.GetReturnValue().Set(SYNC_RESULT);
//}
//
//
//// Wrapper for write(2).
////
//// bytesWritten = write(fd, string, position, enc, callback)
//// 0 fd        integer. file descriptor
//// 1 string    non-buffer values are converted to strings
//// 2 position  if integer, position to write at in the file.
////             if null, write from the current position
//// 3 enc       encoding of string
//static void WriteString(const FunctionCallbackInfo<Value>& args) {
//  HandleScope handle_scope(args.GetIsolate());
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//
//  if (!args[0]->IsInt32())
//    return env->ThrowTypeError("First argument must be file descriptor");
//
//  Local<Value> cb;
//  Local<Value> string = args[1];
//  int fd = args[0]->Int32Value();
//  char* buf = NULL;
//  int64_t pos;
//  size_t len;
//  bool must_free = false;
//
//  // will assign buf and len if string was external
//  if (!StringBytes::GetExternalParts(env->isolate(),
//                                     string,
//                                     const_cast<const char**>(&buf),
//                                     &len)) {
//    enum encoding enc = ParseEncoding(args[3], UTF8);
//    len = StringBytes::StorageSize(env->isolate(), string, enc);
//    buf = new char[len];
//    // StorageSize may return too large a char, so correct the actual length
//    // by the write size
//    len = StringBytes::Write(env->isolate(), buf, len, args[1], enc);
//    must_free = true;
//  }
//  pos = GET_OFFSET(args[2]);
//  cb = args[4];
//
//  uv_buf_t uvbuf = uv_buf_init(const_cast<char*>(buf), len);
//
//  if (!cb->IsFunction()) {
//    SYNC_CALL(write, NULL, fd, &uvbuf, 1, pos)
//    if (must_free)
//      delete[] buf;
//    return args.GetReturnValue().Set(SYNC_RESULT);
//  }
//
//  FSReqWrap* req_wrap = new FSReqWrap(env, "write", must_free ? buf : NULL);
//  int err = uv_fs_write(env->event_loop(),
//                        &req_wrap->req_,
//                        fd,
//                        &uvbuf,
//                        1,
//                        pos,
//                        After);
//  req_wrap->object()->Set(env->oncomplete_string(), cb);
//  req_wrap->Dispatched();
//  if (err < 0) {
//    uv_fs_t* req = &req_wrap->req_;
//    req->result = err;
//    req->path = NULL;
//    After(req);
//  }
//
//  return args.GetReturnValue().Set(req_wrap->persistent());
//}


/*
 * Wrapper for read(2).
 *
 * bytesRead = fs.read(fd, buffer, offset, length, position)
 *
 * 0 fd        integer. file descriptor
 * 1 buffer    instance of Buffer
 * 2 offset    integer. offset to start reading into inside buffer
 * 3 length    integer. length to read
 * 4 position  file position - null for current position
 *
 */
static void Read(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());
  
  if (args.Length() < 2 || !args[0]->IsInt32()) {
    return THROW_BAD_ARGS;
  }
  
  size_t len;
  int64_t pos;

  char * buf = NULL;

  if (!Buffer::HasInstance(args[1])) {
    return env->ThrowError("Second argument needs to be a buffer");
  }

  Local<Object> buffer_obj = args[1]->ToObject();
  char *buffer_data = Buffer::Data(buffer_obj);
  size_t buffer_length = Buffer::Length(buffer_obj);

  size_t off = args[2]->Int32Value();
  if (off >= buffer_length) {
    return env->ThrowError("Offset is out of bounds");
  }

  len = args[3]->Int32Value();
  if (!Buffer::IsWithinBounds(off, len, buffer_length))
    return env->ThrowRangeError("Length extends beyond buffer");

  buf = buffer_data + off;

  if (args[5]->IsFunction()) {
    Async_Call(env, "read", Handle<Function>::Cast(args[5]), args);
  } else {
    Handle<Object> response;
    Sync_Call(env, "read", args, &response);
    
    /* Legacy read returns [str, bytesRead] */
    Handle<Array> results = Handle<Array>::Cast(response->Get(String::NewFromUtf8(env->isolate(), "result")));
    
    /* Copy the read str to buffer. */
    String::Utf8Value data(results->Get(0)->ToString());
    strcpy (buffer_data, *data);
    
    /* Return bytesRead */
    args.GetReturnValue().Set(results->Get(1));
  }
}


///* fs.chmod(path, mode);
// * Wrapper for chmod(1) / EIO_CHMOD
// */
//static void Chmod(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsInt32()) {
//    return THROW_BAD_ARGS;
//  }
//  node::Utf8Value path(args[0]);
//  int mode = static_cast<int>(args[1]->Int32Value());
//
//  SYNC_CALL(chmod, *path, *path, mode);
//}
//
//
///* fs.fchmod(fd, mode);
// * Wrapper for fchmod(1) / EIO_FCHMOD
// */
//static void FChmod(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  if (args.Length() < 2 || !args[0]->IsInt32() || !args[1]->IsInt32()) {
//    return THROW_BAD_ARGS;
//  }
//  int fd = args[0]->Int32Value();
//  int mode = static_cast<int>(args[1]->Int32Value());
//
//  SYNC_CALL(fchmod, 0, fd, mode);
//}


/* fs.chown(path, uid, gid);
 * Wrapper for chown(1) / EIO_CHOWN
 */
//static void Chown(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  int len = args.Length();
//  if (len < 1)
//    return TYPE_ERROR("path required");
//  if (len < 2)
//    return TYPE_ERROR("uid required");
//  if (len < 3)
//    return TYPE_ERROR("gid required");
//  if (!args[0]->IsString())
//    return TYPE_ERROR("path must be a string");
//  if (!args[1]->IsUint32())
//    return TYPE_ERROR("uid must be an unsigned int");
//  if (!args[2]->IsUint32())
//    return TYPE_ERROR("gid must be an unsigned int");
//
//  node::Utf8Value path(args[0]);
//  uv_uid_t uid = static_cast<uv_uid_t>(args[1]->Uint32Value());
//  uv_gid_t gid = static_cast<uv_gid_t>(args[2]->Uint32Value());
//
//  SYNC_CALL(chown, *path, *path, uid, gid);
//}
//
//
///* fs.fchown(fd, uid, gid);
// * Wrapper for fchown(1) / EIO_FCHOWN
// */
//static void FChown(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  int len = args.Length();
//  if (len < 1)
//    return TYPE_ERROR("fd required");
//  if (len < 2)
//    return TYPE_ERROR("uid required");
//  if (len < 3)
//    return TYPE_ERROR("gid required");
//  if (!args[0]->IsInt32())
//    return TYPE_ERROR("fd must be an int");
//  if (!args[1]->IsUint32())
//    return TYPE_ERROR("uid must be an unsigned int");
//  if (!args[2]->IsUint32())
//    return TYPE_ERROR("gid must be an unsigned int");
//
//  int fd = args[0]->Int32Value();
//  uv_uid_t uid = static_cast<uv_uid_t>(args[1]->Uint32Value());
//  uv_gid_t gid = static_cast<uv_gid_t>(args[2]->Uint32Value());
//
//  SYNC_CALL(fchown, 0, fd, uid, gid);
//}


//static void UTimes(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  int len = args.Length();
//  if (len < 1)
//    return TYPE_ERROR("path required");
//  if (len < 2)
//    return TYPE_ERROR("atime required");
//  if (len < 3)
//    return TYPE_ERROR("mtime required");
//  if (!args[0]->IsString())
//    return TYPE_ERROR("path must be a string");
//  if (!args[1]->IsNumber())
//    return TYPE_ERROR("atime must be a number");
//  if (!args[2]->IsNumber())
//    return TYPE_ERROR("mtime must be a number");
//
//  const node::Utf8Value path(args[0]);
//  const double atime = static_cast<double>(args[1]->NumberValue());
//  const double mtime = static_cast<double>(args[2]->NumberValue());
//
//  SYNC_CALL(utime, *path, *path, atime, mtime);
//}
//
//static void FUTimes(const FunctionCallbackInfo<Value>& args) {
//  Environment* env = Environment::GetCurrent(args.GetIsolate());
//  HandleScope scope(env->isolate());
//
//  int len = args.Length();
//  if (len < 1)
//    return TYPE_ERROR("fd required");
//  if (len < 2)
//    return TYPE_ERROR("atime required");
//  if (len < 3)
//    return TYPE_ERROR("mtime required");
//  if (!args[0]->IsInt32())
//    return TYPE_ERROR("fd must be an int");
//  if (!args[1]->IsNumber())
//    return TYPE_ERROR("atime must be a number");
//  if (!args[2]->IsNumber())
//    return TYPE_ERROR("mtime must be a number");
//
//  const int fd = args[0]->Int32Value();
//  const double atime = static_cast<double>(args[1]->NumberValue());
//  const double mtime = static_cast<double>(args[2]->NumberValue());
//
//  SYNC_CALL(futime, 0, fd, atime, mtime);
//}

void FSInitialize(const FunctionCallbackInfo<Value>& args) {
  Local<Function> stats_constructor = args[0].As<Function>();
  assert(stats_constructor->IsFunction());

  Environment* env = Environment::GetCurrent(args.GetIsolate());
  env->set_fs_stats_constructor_function(stats_constructor);
}

void InitFs(Handle<Object> target,
            Handle<Value> unused,
            Handle<Context> context,
            void* priv) {
  Environment* env = Environment::GetCurrent(context);

  // Function which creates a new Stats object.
  target->Set(
      FIXED_ONE_BYTE_STRING(env->isolate(), "FSInitialize"),
      FunctionTemplate::New(env->isolate(), FSInitialize)->GetFunction());

  NODE_SET_METHOD(target, "close", Close);
  NODE_SET_METHOD(target, "open", Open);
  NODE_SET_METHOD(target, "read", Read);
//  NODE_SET_METHOD(target, "fdatasync", Fdatasync);
//  NODE_SET_METHOD(target, "fsync", Fsync);
//  NODE_SET_METHOD(target, "rename", Rename);
//  NODE_SET_METHOD(target, "ftruncate", FTruncate);
//  NODE_SET_METHOD(target, "rmdir", RMDir);
//  NODE_SET_METHOD(target, "mkdir", MKDir);
  NODE_SET_METHOD(target, "readdir", ReadDir);
  NODE_SET_METHOD(target, "stat", Stat);
  NODE_SET_METHOD(target, "lstat", LStat);
  NODE_SET_METHOD(target, "fstat", FStat);
//  NODE_SET_METHOD(target, "link", Link);
//  NODE_SET_METHOD(target, "symlink", Symlink);
//  NODE_SET_METHOD(target, "readlink", ReadLink);
//  NODE_SET_METHOD(target, "unlink", Unlink);
//  NODE_SET_METHOD(target, "writeBuffer", WriteBuffer);
//  NODE_SET_METHOD(target, "writeString", WriteString);

//  NODE_SET_METHOD(target, "chmod", Chmod);
//  NODE_SET_METHOD(target, "fchmod", FChmod);
//  // NODE_SET_METHOD(target, "lchmod", LChmod);
//
//  NODE_SET_METHOD(target, "chown", Chown);
//  NODE_SET_METHOD(target, "fchown", FChown);
//  // NODE_SET_METHOD(target, "lchown", LChown);
//
//  NODE_SET_METHOD(target, "utimes", UTimes);
//  NODE_SET_METHOD(target, "futimes", FUTimes);

//  StatWatcher::Initialize(env, target);
}

}  // end namespace node

NODE_MODULE_CONTEXT_AWARE_BUILTIN(fs, node::InitFs)
