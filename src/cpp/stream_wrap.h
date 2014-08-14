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

#ifndef SRC_STREAM_WRAP_H_
#define SRC_STREAM_WRAP_H_

#include "base-object.h"
#include "string_bytes.h"
#include "v8.h"

#include <iostream>

namespace node {

// Forward declaration
class StreamWrap;

class StreamWrap : public BaseObject {
 public:
  static void GetFD(v8::Local<v8::String>,
                    const v8::PropertyCallbackInfo<v8::Value>&);

  // JavaScript functions
  static void ReadStart(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void ReadStop(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Shutdown(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void Writev(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void WriteBuffer(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void WriteAsciiString(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void WriteUtf8String(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void WriteUcs2String(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void SetBlocking(const v8::FunctionCallbackInfo<v8::Value>& args);

  inline std::iostream* stream() const {
    return stream_;
  }

 protected:
  static size_t WriteBuffer(v8::Handle<v8::Value> val, std::string* buf);

  StreamWrap(v8::Isolate isolate,
             v8::Local<v8::Object> object,
             std::iostream* stream);

  ~StreamWrap() { }

  void StateChange() { }
  void UpdateWriteQueueSize();

 private:

  template <enum encoding encoding>
  static void WriteStringImpl(const v8::FunctionCallbackInfo<v8::Value>& args);

  std::iostream* const stream_;
};


}  // namespace node


#endif  // SRC_STREAM_WRAP_H_
