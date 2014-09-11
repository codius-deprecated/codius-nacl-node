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

// #define CARES_STATICLIB
// #include "ares.h"
#include "async-wrap.h"
#include "async-wrap-inl.h"
#include "env.h"
#include "env-inl.h"
#include "node.h"
#include "req_wrap.h"
#include "tree.h"
#include "util.h"
#include "uv.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if defined(__ANDROID__) || \
    defined(__MINGW32__) || \
    defined(__OpenBSD__) || \
    defined(_MSC_VER)

# include <nameser.h>
#else
# include <arpa/nameser.h>
#endif


namespace node {
namespace cares_wrap {

using v8::Array;
using v8::Context;
using v8::EscapableHandleScope;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::Handle;
using v8::HandleScope;
using v8::Integer;
using v8::Local;
using v8::Null;
using v8::Object;
using v8::String;
using v8::Value;

typedef class ReqWrap<uv_getaddrinfo_t> GetAddrInfoReqWrap;
typedef class ReqWrap<uv_getnameinfo_t> GetNameInfoReqWrap;

// static void IsIP(const FunctionCallbackInfo<Value>& args) {
//   Environment* env = Environment::GetCurrent(args.GetIsolate());
//   HandleScope scope(env->isolate());

//   node::Utf8Value ip(args[0]);
//   char address_buffer[sizeof(struct in6_addr)];

//   int rc = 0;
//   if (uv_inet_pton(AF_INET, *ip, &address_buffer) == 0)
//     rc = 4;
//   else if (uv_inet_pton(AF_INET6, *ip, &address_buffer) == 0)
//     rc = 6;

//   args.GetReturnValue().Set(rc);
// }

// void AfterGetAddrInfo(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
//   GetAddrInfoReqWrap* req_wrap = static_cast<GetAddrInfoReqWrap*>(req->data);
//   Environment* env = req_wrap->env();

//   HandleScope handle_scope(env->isolate());
//   Context::Scope context_scope(env->context());

//   Local<Value> argv[] = {
//     Integer::New(env->isolate(), status),
//     Null(env->isolate())
//   };

//   if (status == 0) {
//     // Success
//     struct addrinfo *address;
//     int n = 0;

//     // Count the number of responses.
//     for (address = res; address; address = address->ai_next) {
//       n++;
//     }

//     // Create the response array.
//     Local<Array> results = Array::New(env->isolate(), n);

//     char ip[INET6_ADDRSTRLEN];
//     const char *addr;

//     n = 0;

//     // Iterate over the IPv4 responses again this time creating javascript
//     // strings for each IP and filling the results array.
//     address = res;
//     while (address) {
//       assert(address->ai_socktype == SOCK_STREAM);

//       // Ignore random ai_family types.
//       if (address->ai_family == AF_INET) {
//         // Juggle pointers
//         addr = reinterpret_cast<char*>(&(reinterpret_cast<struct sockaddr_in*>(
//             address->ai_addr)->sin_addr));
//         int err = uv_inet_ntop(address->ai_family,
//                                addr,
//                                ip,
//                                INET6_ADDRSTRLEN);
//         if (err)
//           continue;

//         // Create JavaScript string
//         Local<String> s = OneByteString(env->isolate(), ip);
//         results->Set(n, s);
//         n++;
//       }

//       // Increment
//       address = address->ai_next;
//     }

//     // Iterate over the IPv6 responses putting them in the array.
//     address = res;
//     while (address) {
//       assert(address->ai_socktype == SOCK_STREAM);

//       // Ignore random ai_family types.
//       if (address->ai_family == AF_INET6) {
//         // Juggle pointers
//         addr = reinterpret_cast<char*>(&(reinterpret_cast<struct sockaddr_in6*>(
//             address->ai_addr)->sin6_addr));
//         int err = uv_inet_ntop(address->ai_family,
//                                addr,
//                                ip,
//                                INET6_ADDRSTRLEN);
//         if (err)
//           continue;

//         // Create JavaScript string
//         Local<String> s = OneByteString(env->isolate(), ip);
//         results->Set(n, s);
//         n++;
//       }

//       // Increment
//       address = address->ai_next;
//     }


//     argv[1] = results;
//   }

//   uv_freeaddrinfo(res);

//   // Make the callback into JavaScript
//   req_wrap->MakeCallback(env->oncomplete_string(), ARRAY_SIZE(argv), argv);

//   delete req_wrap;
// }

// static void GetAddrInfo(const FunctionCallbackInfo<Value>& args) {
//   HandleScope handle_scope(args.GetIsolate());
//   Environment* env = Environment::GetCurrent(args.GetIsolate());

//   assert(args[0]->IsObject());
//   assert(args[1]->IsString());
//   assert(args[2]->IsInt32());
//   Local<Object> req_wrap_obj = args[0].As<Object>();
//   node::Utf8Value hostname(args[1]);

//   int32_t flags = (args[3]->IsInt32()) ? args[3]->Int32Value() : 0;
//   int family;

//   switch (args[2]->Int32Value()) {
//   case 0:
//     family = AF_UNSPEC;
//     break;
//   case 4:
//     family = AF_INET;
//     break;
//   case 6:
//     family = AF_INET6;
//     break;
//   default:
//     assert(0 && "bad address family");
//     abort();
//   }

//   GetAddrInfoReqWrap* req_wrap =
//     new GetAddrInfoReqWrap(env,
//                            req_wrap_obj,
//                            AsyncWrap::PROVIDER_GETADDRINFOREQWRAP);

//   struct addrinfo hints;
//   memset(&hints, 0, sizeof(struct addrinfo));
//   hints.ai_family = family;
//   hints.ai_socktype = SOCK_STREAM;
//   hints.ai_flags = flags;

//   int err = uv_getaddrinfo(env->event_loop(),
//                            &req_wrap->req_,
//                            AfterGetAddrInfo,
//                            *hostname,
//                            NULL,
//                            &hints);
//   req_wrap->Dispatched();
//   if (err)
//     delete req_wrap;

//   args.GetReturnValue().Set(err);
// }

static void Initialize(Handle<Object> target,
                       Handle<Value> unused,
                       Handle<Context> context) {
  Environment* env = Environment::GetCurrent(context);

  // int r = ares_library_init(ARES_LIB_INIT_ALL);
  // assert(r == ARES_SUCCESS);

  // struct ares_options options;
  // memset(&options, 0, sizeof(options));
  // options.flags = ARES_FLAG_NOCHECKRESP;
  // options.sock_state_cb = ares_sockstate_cb;
  // options.sock_state_cb_data = env;

  // /* We do the call to ares_init_option for caller. */
  // r = ares_init_options(env->cares_channel_ptr(),
  //                       &options,
  //                       ARES_OPT_FLAGS | ARES_OPT_SOCK_STATE_CB);
  // assert(r == ARES_SUCCESS);

  /* Initialize the timeout timer. The timer won't be started until the */
  /* first socket is opened. */
  // uv_timer_init(env->event_loop(), env->cares_timer_handle());

  // NODE_SET_METHOD(target, "queryA", Query<QueryAWrap>);
  // NODE_SET_METHOD(target, "queryAaaa", Query<QueryAaaaWrap>);
  // NODE_SET_METHOD(target, "queryCname", Query<QueryCnameWrap>);
  // NODE_SET_METHOD(target, "queryMx", Query<QueryMxWrap>);
  // NODE_SET_METHOD(target, "queryNs", Query<QueryNsWrap>);
  // NODE_SET_METHOD(target, "queryTxt", Query<QueryTxtWrap>);
  // NODE_SET_METHOD(target, "querySrv", Query<QuerySrvWrap>);
  // NODE_SET_METHOD(target, "queryNaptr", Query<QueryNaptrWrap>);
  // NODE_SET_METHOD(target, "querySoa", Query<QuerySoaWrap>);
  // NODE_SET_METHOD(target, "getHostByAddr", Query<GetHostByAddrWrap>);

  // NODE_SET_METHOD(target, "getaddrinfo", GetAddrInfo);
  // NODE_SET_METHOD(target, "getnameinfo", GetNameInfo);
  // NODE_SET_METHOD(target, "isIP", IsIP);

  // NODE_SET_METHOD(target, "strerror", StrError);
  // NODE_SET_METHOD(target, "getServers", GetServers);
  // NODE_SET_METHOD(target, "setServers", SetServers);

  target->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "AF_INET"),
              Integer::New(env->isolate(), AF_INET));
  target->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "AF_INET6"),
              Integer::New(env->isolate(), AF_INET6));
  target->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "AF_UNSPEC"),
              Integer::New(env->isolate(), AF_UNSPEC));
  target->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "AI_ADDRCONFIG"),
              Integer::New(env->isolate(), AI_ADDRCONFIG));
  target->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "AI_V4MAPPED"),
              Integer::New(env->isolate(), AI_V4MAPPED));
}

}  // namespace cares_wrap
}  // namespace node

NODE_MODULE_CONTEXT_AWARE_BUILTIN(cares_wrap, node::cares_wrap::Initialize)
