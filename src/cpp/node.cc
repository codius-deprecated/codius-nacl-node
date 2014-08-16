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
#include "node_buffer.h"
#include "node_constants.h"
#include "node_javascript.h"
#include "node_version.h"

#include "codius_version.h"
#include "env.h"
#include "env-inl.h"
#include "util.h"
#include "zlib.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>  // PATH_MAX
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

namespace node {

using v8::Array;
using v8::ArrayBuffer;
using v8::Boolean;
using v8::Context;
using v8::EscapableHandleScope;
using v8::Exception;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::HeapStatistics;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::Locker;
using v8::Message;
using v8::Number;
using v8::Object;
using v8::ObjectTemplate;
using v8::PropertyCallbackInfo;
using v8::String;
using v8::TryCatch;
using v8::Uint32;
using v8::V8;
using v8::Value;
using v8::kExternalUnsignedIntArray;

static node_module* modpending;
static node_module* modlist_builtin;
static node_module* modlist_addon;

// process-relative uptime base, initialized at start-up
static uint64_t prog_start_time;

static Isolate* node_isolate = NULL;

int WRITE_UTF8_FLAGS = v8::String::HINT_MANY_WRITES_EXPECTED |
                       v8::String::NO_NULL_TERMINATION;

class ArrayBufferAllocator : public ArrayBuffer::Allocator {
 public:
  // Impose an upper limit to avoid out of memory errors that bring down
  // the process.
  static const size_t kMaxLength = 0x3fffffff;
  static ArrayBufferAllocator the_singleton;
  virtual ~ArrayBufferAllocator() {}
  virtual void* Allocate(size_t length);
  virtual void* AllocateUninitialized(size_t length);
  virtual void Free(void* data, size_t length);
 private:
  ArrayBufferAllocator() {}
  ArrayBufferAllocator(const ArrayBufferAllocator&);
  void operator=(const ArrayBufferAllocator&);
};

ArrayBufferAllocator ArrayBufferAllocator::the_singleton;


void* ArrayBufferAllocator::Allocate(size_t length) {
  if (length > kMaxLength)
    return NULL;
  char* data = new char[length];
  memset(data, 0, length);
  return data;
}


void* ArrayBufferAllocator::AllocateUninitialized(size_t length) {
  if (length > kMaxLength)
    return NULL;
  return new char[length];
}


void ArrayBufferAllocator::Free(void* data, size_t length) {
  delete[] static_cast<char*>(data);
}



static inline const char *errno_string(int errorno) {
#define ERRNO_CASE(e)  case e: return #e;
  switch (errorno) {
#ifdef EACCES
  ERRNO_CASE(EACCES);
#endif

#ifdef EADDRINUSE
  ERRNO_CASE(EADDRINUSE);
#endif

#ifdef EADDRNOTAVAIL
  ERRNO_CASE(EADDRNOTAVAIL);
#endif

#ifdef EAFNOSUPPORT
  ERRNO_CASE(EAFNOSUPPORT);
#endif

#ifdef EAGAIN
  ERRNO_CASE(EAGAIN);
#endif

#ifdef EWOULDBLOCK
# if EAGAIN != EWOULDBLOCK
  ERRNO_CASE(EWOULDBLOCK);
# endif
#endif

#ifdef EALREADY
  ERRNO_CASE(EALREADY);
#endif

#ifdef EBADF
  ERRNO_CASE(EBADF);
#endif

#ifdef EBADMSG
  ERRNO_CASE(EBADMSG);
#endif

#ifdef EBUSY
  ERRNO_CASE(EBUSY);
#endif

#ifdef ECANCELED
  ERRNO_CASE(ECANCELED);
#endif

#ifdef ECHILD
  ERRNO_CASE(ECHILD);
#endif

#ifdef ECONNABORTED
  ERRNO_CASE(ECONNABORTED);
#endif

#ifdef ECONNREFUSED
  ERRNO_CASE(ECONNREFUSED);
#endif

#ifdef ECONNRESET
  ERRNO_CASE(ECONNRESET);
#endif

#ifdef EDEADLK
  ERRNO_CASE(EDEADLK);
#endif

#ifdef EDESTADDRREQ
  ERRNO_CASE(EDESTADDRREQ);
#endif

#ifdef EDOM
  ERRNO_CASE(EDOM);
#endif

#ifdef EDQUOT
  ERRNO_CASE(EDQUOT);
#endif

#ifdef EEXIST
  ERRNO_CASE(EEXIST);
#endif

#ifdef EFAULT
  ERRNO_CASE(EFAULT);
#endif

#ifdef EFBIG
  ERRNO_CASE(EFBIG);
#endif

#ifdef EHOSTUNREACH
  ERRNO_CASE(EHOSTUNREACH);
#endif

#ifdef EIDRM
  ERRNO_CASE(EIDRM);
#endif

#ifdef EILSEQ
  ERRNO_CASE(EILSEQ);
#endif

#ifdef EINPROGRESS
  ERRNO_CASE(EINPROGRESS);
#endif

#ifdef EINTR
  ERRNO_CASE(EINTR);
#endif

#ifdef EINVAL
  ERRNO_CASE(EINVAL);
#endif

#ifdef EIO
  ERRNO_CASE(EIO);
#endif

#ifdef EISCONN
  ERRNO_CASE(EISCONN);
#endif

#ifdef EISDIR
  ERRNO_CASE(EISDIR);
#endif

#ifdef ELOOP
  ERRNO_CASE(ELOOP);
#endif

#ifdef EMFILE
  ERRNO_CASE(EMFILE);
#endif

#ifdef EMLINK
  ERRNO_CASE(EMLINK);
#endif

#ifdef EMSGSIZE
  ERRNO_CASE(EMSGSIZE);
#endif

#ifdef EMULTIHOP
  ERRNO_CASE(EMULTIHOP);
#endif

#ifdef ENAMETOOLONG
  ERRNO_CASE(ENAMETOOLONG);
#endif

#ifdef ENETDOWN
  ERRNO_CASE(ENETDOWN);
#endif

#ifdef ENETRESET
  ERRNO_CASE(ENETRESET);
#endif

#ifdef ENETUNREACH
  ERRNO_CASE(ENETUNREACH);
#endif

#ifdef ENFILE
  ERRNO_CASE(ENFILE);
#endif

#ifdef ENOBUFS
  ERRNO_CASE(ENOBUFS);
#endif

#ifdef ENODATA
  ERRNO_CASE(ENODATA);
#endif

#ifdef ENODEV
  ERRNO_CASE(ENODEV);
#endif

#ifdef ENOENT
  ERRNO_CASE(ENOENT);
#endif

#ifdef ENOEXEC
  ERRNO_CASE(ENOEXEC);
#endif

#ifdef ENOLINK
  ERRNO_CASE(ENOLINK);
#endif

#ifdef ENOLCK
# if ENOLINK != ENOLCK
  ERRNO_CASE(ENOLCK);
# endif
#endif

#ifdef ENOMEM
  ERRNO_CASE(ENOMEM);
#endif

#ifdef ENOMSG
  ERRNO_CASE(ENOMSG);
#endif

#ifdef ENOPROTOOPT
  ERRNO_CASE(ENOPROTOOPT);
#endif

#ifdef ENOSPC
  ERRNO_CASE(ENOSPC);
#endif

#ifdef ENOSR
  ERRNO_CASE(ENOSR);
#endif

#ifdef ENOSTR
  ERRNO_CASE(ENOSTR);
#endif

#ifdef ENOSYS
  ERRNO_CASE(ENOSYS);
#endif

#ifdef ENOTCONN
  ERRNO_CASE(ENOTCONN);
#endif

#ifdef ENOTDIR
  ERRNO_CASE(ENOTDIR);
#endif

#ifdef ENOTEMPTY
  ERRNO_CASE(ENOTEMPTY);
#endif

#ifdef ENOTSOCK
  ERRNO_CASE(ENOTSOCK);
#endif

#ifdef ENOTSUP
  ERRNO_CASE(ENOTSUP);
#else
# ifdef EOPNOTSUPP
  ERRNO_CASE(EOPNOTSUPP);
# endif
#endif

#ifdef ENOTTY
  ERRNO_CASE(ENOTTY);
#endif

#ifdef ENXIO
  ERRNO_CASE(ENXIO);
#endif


#ifdef EOVERFLOW
  ERRNO_CASE(EOVERFLOW);
#endif

#ifdef EPERM
  ERRNO_CASE(EPERM);
#endif

#ifdef EPIPE
  ERRNO_CASE(EPIPE);
#endif

#ifdef EPROTO
  ERRNO_CASE(EPROTO);
#endif

#ifdef EPROTONOSUPPORT
  ERRNO_CASE(EPROTONOSUPPORT);
#endif

#ifdef EPROTOTYPE
  ERRNO_CASE(EPROTOTYPE);
#endif

#ifdef ERANGE
  ERRNO_CASE(ERANGE);
#endif

#ifdef EROFS
  ERRNO_CASE(EROFS);
#endif

#ifdef ESPIPE
  ERRNO_CASE(ESPIPE);
#endif

#ifdef ESRCH
  ERRNO_CASE(ESRCH);
#endif

#ifdef ESTALE
  ERRNO_CASE(ESTALE);
#endif

#ifdef ETIME
  ERRNO_CASE(ETIME);
#endif

#ifdef ETIMEDOUT
  ERRNO_CASE(ETIMEDOUT);
#endif

#ifdef ETXTBSY
  ERRNO_CASE(ETXTBSY);
#endif

#ifdef EXDEV
  ERRNO_CASE(EXDEV);
#endif

  default: return "";
  }
}

const char *signo_string(int signo) {
#define SIGNO_CASE(e)  case e: return #e;
  switch (signo) {
#ifdef SIGHUP
  SIGNO_CASE(SIGHUP);
#endif

#ifdef SIGINT
  SIGNO_CASE(SIGINT);
#endif

#ifdef SIGQUIT
  SIGNO_CASE(SIGQUIT);
#endif

#ifdef SIGILL
  SIGNO_CASE(SIGILL);
#endif

#ifdef SIGTRAP
  SIGNO_CASE(SIGTRAP);
#endif

#ifdef SIGABRT
  SIGNO_CASE(SIGABRT);
#endif

#ifdef SIGIOT
# if SIGABRT != SIGIOT
  SIGNO_CASE(SIGIOT);
# endif
#endif

#ifdef SIGBUS
  SIGNO_CASE(SIGBUS);
#endif

#ifdef SIGFPE
  SIGNO_CASE(SIGFPE);
#endif

#ifdef SIGKILL
  SIGNO_CASE(SIGKILL);
#endif

#ifdef SIGUSR1
  SIGNO_CASE(SIGUSR1);
#endif

#ifdef SIGSEGV
  SIGNO_CASE(SIGSEGV);
#endif

#ifdef SIGUSR2
  SIGNO_CASE(SIGUSR2);
#endif

#ifdef SIGPIPE
  SIGNO_CASE(SIGPIPE);
#endif

#ifdef SIGALRM
  SIGNO_CASE(SIGALRM);
#endif

  SIGNO_CASE(SIGTERM);

#ifdef SIGCHLD
  SIGNO_CASE(SIGCHLD);
#endif

#ifdef SIGSTKFLT
  SIGNO_CASE(SIGSTKFLT);
#endif


#ifdef SIGCONT
  SIGNO_CASE(SIGCONT);
#endif

#ifdef SIGSTOP
  SIGNO_CASE(SIGSTOP);
#endif

#ifdef SIGTSTP
  SIGNO_CASE(SIGTSTP);
#endif

#ifdef SIGBREAK
  SIGNO_CASE(SIGBREAK);
#endif

#ifdef SIGTTIN
  SIGNO_CASE(SIGTTIN);
#endif

#ifdef SIGTTOU
  SIGNO_CASE(SIGTTOU);
#endif

#ifdef SIGURG
  SIGNO_CASE(SIGURG);
#endif

#ifdef SIGXCPU
  SIGNO_CASE(SIGXCPU);
#endif

#ifdef SIGXFSZ
  SIGNO_CASE(SIGXFSZ);
#endif

#ifdef SIGVTALRM
  SIGNO_CASE(SIGVTALRM);
#endif

#ifdef SIGPROF
  SIGNO_CASE(SIGPROF);
#endif

#ifdef SIGWINCH
  SIGNO_CASE(SIGWINCH);
#endif

#ifdef SIGIO
  SIGNO_CASE(SIGIO);
#endif

#ifdef SIGPOLL
# if SIGPOLL != SIGIO
  SIGNO_CASE(SIGPOLL);
# endif
#endif

#ifdef SIGLOST
  SIGNO_CASE(SIGLOST);
#endif

#ifdef SIGPWR
# if SIGPWR != SIGLOST
  SIGNO_CASE(SIGPWR);
# endif
#endif

#ifdef SIGSYS
  SIGNO_CASE(SIGSYS);
#endif

  default: return "";
  }
}

void ThrowError(v8::Isolate* isolate, const char* errmsg) {
  Environment::GetCurrent(isolate)->ThrowError(errmsg);
}


void ThrowTypeError(v8::Isolate* isolate, const char* errmsg) {
  Environment::GetCurrent(isolate)->ThrowTypeError(errmsg);
}


void ThrowRangeError(v8::Isolate* isolate, const char* errmsg) {
  Environment::GetCurrent(isolate)->ThrowRangeError(errmsg);
}


void ThrowErrnoException(v8::Isolate* isolate,
                         int errorno,
                         const char* syscall,
                         const char* message,
                         const char* path) {
  Environment::GetCurrent(isolate)->ThrowErrnoException(errorno,
                                                        syscall,
                                                        message,
                                                        path);
}


Local<Value> ErrnoException(Isolate* isolate,
                            int errorno,
                            const char *syscall,
                            const char *msg,
                            const char *path) {
  Environment* env = Environment::GetCurrent(isolate);

  Local<Value> e;
  Local<String> estring = OneByteString(env->isolate(), errno_string(errorno));
  if (msg == NULL || msg[0] == '\0') {
    msg = strerror(errorno);
  }
  Local<String> message = OneByteString(env->isolate(), msg);

  Local<String> cons1 =
      String::Concat(estring, FIXED_ONE_BYTE_STRING(env->isolate(), ", "));
  Local<String> cons2 = String::Concat(cons1, message);

  if (path) {
    Local<String> cons3 =
        String::Concat(cons2, FIXED_ONE_BYTE_STRING(env->isolate(), " '"));
    Local<String> cons4 =
        String::Concat(cons3, String::NewFromUtf8(env->isolate(), path));
    Local<String> cons5 =
        String::Concat(cons4, FIXED_ONE_BYTE_STRING(env->isolate(), "'"));
    e = Exception::Error(cons5);
  } else {
    e = Exception::Error(cons2);
  }

  Local<Object> obj = e->ToObject();
  obj->Set(env->errno_string(), Integer::New(env->isolate(), errorno));
  obj->Set(env->code_string(), estring);

  if (path != NULL) {
    obj->Set(env->path_string(), String::NewFromUtf8(env->isolate(), path));
  }

  if (syscall != NULL) {
    obj->Set(env->syscall_string(), OneByteString(env->isolate(), syscall));
  }

  return e;
}

void SetupNextTick(const FunctionCallbackInfo<Value>& args) {
  HandleScope handle_scope(args.GetIsolate());
  Environment* env = Environment::GetCurrent(args.GetIsolate());

  assert(args[0]->IsObject());
  assert(args[1]->IsFunction());

  // Values use to cross communicate with processNextTick.
  Local<Object> tick_info_obj = args[0].As<Object>();
  tick_info_obj->SetIndexedPropertiesToExternalArrayData(
      env->tick_info()->fields(),
      kExternalUnsignedIntArray,
      env->tick_info()->fields_count());

  env->set_tick_callback_function(args[1].As<Function>());

  // Do a little housekeeping.
  env->process_object()->Delete(
      FIXED_ONE_BYTE_STRING(args.GetIsolate(), "_setupNextTick"));
}

enum encoding ParseEncoding(Isolate* isolate,
                            Handle<Value> encoding_v,
                            enum encoding _default) {
  HandleScope scope(isolate);

  if (!encoding_v->IsString())
    return _default;

  node::Utf8Value encoding(encoding_v);

  if (strcasecmp(*encoding, "utf8") == 0) {
    return UTF8;
  } else if (strcasecmp(*encoding, "utf-8") == 0) {
    return UTF8;
  } else if (strcasecmp(*encoding, "ascii") == 0) {
    return ASCII;
  } else if (strcasecmp(*encoding, "base64") == 0) {
    return BASE64;
  } else if (strcasecmp(*encoding, "ucs2") == 0) {
    return UCS2;
  } else if (strcasecmp(*encoding, "ucs-2") == 0) {
    return UCS2;
  } else if (strcasecmp(*encoding, "utf16le") == 0) {
    return UCS2;
  } else if (strcasecmp(*encoding, "utf-16le") == 0) {
    return UCS2;
  } else if (strcasecmp(*encoding, "binary") == 0) {
    return BINARY;
  } else if (strcasecmp(*encoding, "buffer") == 0) {
    return BUFFER;
  } else if (strcasecmp(*encoding, "hex") == 0) {
    return HEX;
  } else {
    return _default;
  }
}

void AppendExceptionLine(Environment* env,
                         Handle<Value> er,
                         Handle<Message> message) {
  if (message.IsEmpty())
    return;

  HandleScope scope(env->isolate());
  Local<Object> err_obj;
  if (!er.IsEmpty() && er->IsObject()) {
    err_obj = er.As<Object>();

    // Do it only once per message
    if (!err_obj->GetHiddenValue(env->processed_string()).IsEmpty())
      return;
    err_obj->SetHiddenValue(env->processed_string(), True(env->isolate()));
  }

  static char arrow[1024];

  // Print (filename):(line number): (message).
  node::Utf8Value filename(message->GetScriptResourceName());
  const char* filename_string = *filename;
  int linenum = message->GetLineNumber();
  // Print line of source code.
  node::Utf8Value sourceline(message->GetSourceLine());
  const char* sourceline_string = *sourceline;

  // Because of how node modules work, all scripts are wrapped with a
  // "function (module, exports, __filename, ...) {"
  // to provide script local variables.
  //
  // When reporting errors on the first line of a script, this wrapper
  // function is leaked to the user. There used to be a hack here to
  // truncate off the first 62 characters, but it caused numerous other
  // problems when vm.runIn*Context() methods were used for non-module
  // code.
  //
  // If we ever decide to re-instate such a hack, the following steps
  // must be taken:
  //
  // 1. Pass a flag around to say "this code was wrapped"
  // 2. Update the stack frame output so that it is also correct.
  //
  // It would probably be simpler to add a line rather than add some
  // number of characters to the first line, since V8 truncates the
  // sourceline to 78 characters, and we end up not providing very much
  // useful debugging info to the user if we remove 62 characters.

  int start = message->GetStartColumn();
  int end = message->GetEndColumn();

  int off = snprintf(arrow,
                     sizeof(arrow),
                     "%s:%i\n%s\n",
                     filename_string,
                     linenum,
                     sourceline_string);
  assert(off >= 0);

  // Print wavy underline (GetUnderline is deprecated).
  for (int i = 0; i < start; i++) {
    if (sourceline_string[i] == '\0' ||
        static_cast<size_t>(off) >= sizeof(arrow)) {
      break;
    }
    assert(static_cast<size_t>(off) < sizeof(arrow));
    arrow[off++] = (sourceline_string[i] == '\t') ? '\t' : ' ';
  }
  for (int i = start; i < end; i++) {
    if (sourceline_string[i] == '\0' ||
        static_cast<size_t>(off) >= sizeof(arrow)) {
      break;
    }
    assert(static_cast<size_t>(off) < sizeof(arrow));
    arrow[off++] = '^';
  }
  assert(static_cast<size_t>(off - 1) <= sizeof(arrow) - 1);
  arrow[off++] = '\n';
  arrow[off] = '\0';

  Local<String> arrow_str = String::NewFromUtf8(env->isolate(), arrow);
  Local<Value> msg;
  Local<Value> stack;

  // Allocation failed, just print it out
  if (arrow_str.IsEmpty() || err_obj.IsEmpty() || !err_obj->IsNativeError())
    goto print;

  msg = err_obj->Get(env->message_string());
  stack = err_obj->Get(env->stack_string());

  if (msg.IsEmpty() || stack.IsEmpty())
    goto print;

  err_obj->Set(env->message_string(),
               String::Concat(arrow_str, msg->ToString()));
  err_obj->Set(env->stack_string(),
               String::Concat(arrow_str, stack->ToString()));
  return;

 print:
  if (env->printed_error())
    return;
  env->set_printed_error(true);

  fprintf(stderr, "\n%s", arrow);
}

static void ReportException(Environment* env,
                            Handle<Value> er,
                            Handle<Message> message) {
  HandleScope scope(env->isolate());

  AppendExceptionLine(env, er, message);

  Local<Value> trace_value;

  if (er->IsUndefined() || er->IsNull())
    trace_value = Undefined(env->isolate());
  else
    trace_value = er->ToObject()->Get(env->stack_string());

  node::Utf8Value trace(trace_value);

  // range errors have a trace member set to undefined
  if (trace.length() > 0 && !trace_value->IsUndefined()) {
    fprintf(stderr, "%s\n", *trace);
  } else {
    // this really only happens for RangeErrors, since they're the only
    // kind that won't have all this info in the trace, or when non-Error
    // objects are thrown manually.
    Local<Value> message;
    Local<Value> name;

    if (er->IsObject()) {
      Local<Object> err_obj = er.As<Object>();
      message = err_obj->Get(env->message_string());
      name = err_obj->Get(FIXED_ONE_BYTE_STRING(env->isolate(), "name"));
    }

    if (message.IsEmpty() ||
        message->IsUndefined() ||
        name.IsEmpty() ||
        name->IsUndefined()) {
      // Not an error object. Just print as-is.
      node::Utf8Value message(er);
      fprintf(stderr, "%s\n", *message);
    } else {
      node::Utf8Value name_string(name);
      node::Utf8Value message_string(message);
      fprintf(stderr, "%s: %s\n", *name_string, *message_string);
    }
  }

  fflush(stderr);
}


static void ReportException(Environment* env, const TryCatch& try_catch) {
  ReportException(env, try_catch.Exception(), try_catch.Message());
}


// Executes a str within the current v8 context.
static Local<Value> ExecuteString(Environment* env,
                                  Handle<String> source,
                                  Handle<String> filename) {
  EscapableHandleScope scope(env->isolate());
  TryCatch try_catch;

  // try_catch must be nonverbose to disable FatalException() handler,
  // we will handle exceptions ourself.
  try_catch.SetVerbose(false);

  Local<v8::Script> script = v8::Script::Compile(source, filename);
  if (script.IsEmpty()) {
    ReportException(env, try_catch);
    exit(3);
  }

  Local<Value> result = script->Run();
  if (result.IsEmpty()) {
    ReportException(env, try_catch);
    exit(4);
  }

  return scope.Escape(result);
}

static void Abort(const FunctionCallbackInfo<Value>& args) {
  abort();
}

static void Chdir(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());

  if (args.Length() != 1 || !args[0]->IsString()) {
    // FIXME(bnoordhuis) ThrowTypeError?
    return env->ThrowError("Bad argument.");
  }

  node::Utf8Value path(args[0]);
  if (chdir(*path)) {
    return env->ThrowErrnoException(errno, "chdir");
  }
}


static void Cwd(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());
#ifdef _WIN32
  /* MAX_PATH is in characters, not bytes. Make sure we have enough headroom. */
  char buf[MAX_PATH * 4];
#else
  char buf[PATH_MAX];
#endif

  size_t cwd_len = sizeof(buf);
  if (!getcwd(buf, cwd_len)) {
    return env->ThrowErrnoException(errno, "getcwd");
  }

  Local<String> cwd = String::NewFromUtf8(env->isolate(),
                                          buf,
                                          String::kNormalString,
                                          cwd_len - 1);
  args.GetReturnValue().Set(cwd);
}

static void Umask(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());
  uint32_t old = 2;

  // TODO-CODIUS: Enable writable filesystem with umask()

  args.GetReturnValue().Set(old);
}


void Exit(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());
  exit(args[0]->IntegerValue());
}


static void Uptime(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());
  timeval now;
  uint64_t uptime;

  gettimeofday(&now, NULL);
  uptime = (now.tv_sec * 1000000 + now.tv_usec) - prog_start_time;

  args.GetReturnValue().Set(Number::New(env->isolate(), uptime / 1000000));
}


void MemoryUsage(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());

  // V8 memory usage
  HeapStatistics v8_heap_stats;
  env->isolate()->GetHeapStatistics(&v8_heap_stats);

  Local<Integer> heap_total =
      Integer::NewFromUnsigned(env->isolate(), v8_heap_stats.total_heap_size());
  Local<Integer> heap_used =
      Integer::NewFromUnsigned(env->isolate(), v8_heap_stats.used_heap_size());

  Local<Object> info = Object::New(env->isolate());
  info->Set(env->heap_total_string(), heap_total);
  info->Set(env->heap_used_string(), heap_used);

  args.GetReturnValue().Set(info);
}

void Kill(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());

  // TODO-CODIUS: Re-enable if we ever add child processes

  args.GetReturnValue().Set(0);
}

extern "C" void node_module_register(void* m) {
  struct node_module* mp = reinterpret_cast<struct node_module*>(m);

  if (mp->nm_flags & NM_F_BUILTIN) {
    mp->nm_link = modlist_builtin;
    modlist_builtin = mp;
  } else {
    assert(modpending == NULL);
    modpending = mp;
  }
}

struct node_module* get_builtin_module(const char* name) {
  struct node_module* mp;

  for (mp = modlist_builtin; mp != NULL; mp = mp->nm_link) {
    if (strcmp(mp->nm_modname, name) == 0)
      break;
  }

  assert(mp == NULL || (mp->nm_flags & NM_F_BUILTIN) != 0);
  return (mp);
}

static void OnFatalError(const char* location, const char* message) {
  if (location) {
    fprintf(stderr, "FATAL ERROR: %s %s\n", location, message);
  } else {
    fprintf(stderr, "FATAL ERROR: %s\n", message);
  }
  fflush(stderr);
  abort();
}


NO_RETURN void FatalError(const char* location, const char* message) {
  OnFatalError(location, message);
  // to supress compiler warning
  abort();
}


void FatalException(Isolate* isolate,
                    Handle<Value> error,
                    Handle<Message> message) {
  HandleScope scope(isolate);

  Environment* env = Environment::GetCurrent(isolate);
  Local<Object> process_object = env->process_object();
  Local<String> fatal_exception_string = env->fatal_exception_string();
  Local<Function> fatal_exception_function =
      process_object->Get(fatal_exception_string).As<Function>();

  if (!fatal_exception_function->IsFunction()) {
    // failed before the process._fatalException function was added!
    // this is probably pretty bad.  Nothing to do but report and exit.
    ReportException(env, error, message);
    exit(6);
  }

  TryCatch fatal_try_catch;

  // Do not call FatalException when _fatalException handler throws
  fatal_try_catch.SetVerbose(false);

  // this will return true if the JS layer handled it, false otherwise
  Local<Value> caught =
      fatal_exception_function->Call(process_object, 1, &error);

  if (fatal_try_catch.HasCaught()) {
    // the fatal exception function threw, so we must exit
    ReportException(env, fatal_try_catch);
    exit(7);
  }

  if (false == caught->BooleanValue()) {
    ReportException(env, error, message);
    exit(1);
  }
}


void FatalException(Isolate* isolate, const TryCatch& try_catch) {
  HandleScope scope(isolate);
  // TODO(bajtos) do not call FatalException if try_catch is verbose
  // (requires V8 API to expose getter for try_catch.is_verbose_)
  FatalException(isolate, try_catch.Exception(), try_catch.Message());
}


void OnMessage(Handle<Message> message, Handle<Value> error) {
  // The current version of V8 sends messages for errors only
  // (thus `error` is always set).
  FatalException(Isolate::GetCurrent(), error, message);
}

static void Binding(const FunctionCallbackInfo<Value>& args) {
  HandleScope handle_scope(args.GetIsolate());
  Environment* env = Environment::GetCurrent(args.GetIsolate());

  Local<String> module = args[0]->ToString();
  node::Utf8Value module_v(module);

  Local<Object> cache = env->binding_cache_object();
  Local<Object> exports;

  if (cache->Has(module)) {
    exports = cache->Get(module)->ToObject();
    args.GetReturnValue().Set(exports);
    return;
  }

  // Append a string to process.moduleLoadList
  char buf[1024];
  snprintf(buf, sizeof(buf), "Binding %s", *module_v);

  Local<Array> modules = env->module_load_list_array();
  uint32_t l = modules->Length();
  modules->Set(l, OneByteString(env->isolate(), buf));

  node_module* mod = get_builtin_module(*module_v);
  if (mod != NULL) {
    exports = Object::New(env->isolate());
    // Internal bindings don't have a "module" object, only exports.
    assert(mod->nm_register_func == NULL);
    assert(mod->nm_context_register_func != NULL);
    Local<Value> unused = Undefined(env->isolate());
    mod->nm_context_register_func(exports, unused,
      env->context(), mod->nm_priv);
    cache->Set(module, exports);
  } else if (!strcmp(*module_v, "constants")) {
    exports = Object::New(env->isolate());
    DefineConstants(exports);
    cache->Set(module, exports);
  } else if (!strcmp(*module_v, "natives")) {
    exports = Object::New(env->isolate());
    DefineJavaScript(env, exports);
    cache->Set(module, exports);
  } else {
    char errmsg[1024];
    snprintf(errmsg,
             sizeof(errmsg),
             "No such module: %s",
             *module_v);
    return env->ThrowError(errmsg);
  }

  args.GetReturnValue().Set(exports);
}

static Handle<Object> GetFeatures(Environment* env) {
  EscapableHandleScope scope(env->isolate());

  Local<Object> obj = Object::New(env->isolate());
#if defined(DEBUG) && DEBUG
  Local<Value> debug = True(env->isolate());
#else
  Local<Value> debug = False(env->isolate());
#endif  // defined(DEBUG) && DEBUG

  obj->Set(env->debug_string(), debug);

  obj->Set(env->uv_string(), False(env->isolate()));

  obj->Set(env->ipv6_lc_string(), True(env->isolate()));

#ifdef OPENSSL_NPN_NEGOTIATED
  Local<Boolean> tls_npn = True(env->isolate());
#else
  Local<Boolean> tls_npn = False(env->isolate());
#endif
  obj->Set(env->tls_npn_string(), tls_npn);

#ifdef SSL_CTRL_SET_TLSEXT_SERVERNAME_CB
  Local<Boolean> tls_sni = True(env->isolate());
#else
  Local<Boolean> tls_sni = False(env->isolate());
#endif
  obj->Set(env->tls_sni_string(), tls_sni);

#if !defined(OPENSSL_NO_TLSEXT) && defined(SSL_CTX_set_tlsext_status_cb)
  Local<Boolean> tls_ocsp = True(env->isolate());
#else
  Local<Boolean> tls_ocsp = False(env->isolate());
#endif  // !defined(OPENSSL_NO_TLSEXT) && defined(SSL_CTX_set_tlsext_status_cb)
  obj->Set(env->tls_ocsp_string(), tls_ocsp);

  obj->Set(env->tls_string(),
           Boolean::New(env->isolate(), get_builtin_module("crypto") != NULL));

  return scope.Escape(obj);
}

#define READONLY_PROPERTY(obj, str, var)                                      \
  do {                                                                        \
    obj->Set(OneByteString(env->isolate(), str), var, v8::ReadOnly);          \
  } while (0)

void SetupProcessObject(Environment* env,
                        int argc,
                        const char* const* argv,
                        int exec_argc,
                        const char* const* exec_argv) {
  HandleScope scope(env->isolate());

  Local<Object> process = env->process_object();

  process->Set(env->title_string(), FIXED_ONE_BYTE_STRING(env->isolate(), "node-nacl"));

  // process.version
  READONLY_PROPERTY(process,
                    "version",
                    FIXED_ONE_BYTE_STRING(env->isolate(), NODE_VERSION));

  // process.moduleLoadList
  READONLY_PROPERTY(process,
                    "moduleLoadList",
                    env->module_load_list_array());

  // process.versions
  Local<Object> versions = Object::New(env->isolate());
  READONLY_PROPERTY(process, "versions", versions);

  const char http_parser_version[] = NODE_STRINGIFY(HTTP_PARSER_VERSION_MAJOR)
                                     "."
                                     NODE_STRINGIFY(HTTP_PARSER_VERSION_MINOR);
  READONLY_PROPERTY(versions,
                    "http_parser",
                    FIXED_ONE_BYTE_STRING(env->isolate(), http_parser_version));

  // +1 to get rid of the leading 'v'
  READONLY_PROPERTY(versions,
                    "codius",
                    OneByteString(env->isolate(), CODIUS_VERSION + 1));
  // +1 to get rid of the leading 'v'
  READONLY_PROPERTY(versions,
                    "node",
                    OneByteString(env->isolate(), NODE_VERSION + 1));
  READONLY_PROPERTY(versions,
                    "v8",
                    OneByteString(env->isolate(), V8::GetVersion()));
  READONLY_PROPERTY(versions,
                    "zlib",
                    FIXED_ONE_BYTE_STRING(env->isolate(), ZLIB_VERSION));

  const char node_modules_version[] = NODE_STRINGIFY(NODE_MODULE_VERSION);
  READONLY_PROPERTY(
      versions,
      "modules",
      FIXED_ONE_BYTE_STRING(env->isolate(), node_modules_version));

#if HAVE_OPENSSL
  // Stupid code to slice out the version string.
  {  // NOLINT(whitespace/braces)
    size_t i, j, k;
    int c;
    for (i = j = 0, k = sizeof(OPENSSL_VERSION_TEXT) - 1; i < k; ++i) {
      c = OPENSSL_VERSION_TEXT[i];
      if ('0' <= c && c <= '9') {
        for (j = i + 1; j < k; ++j) {
          c = OPENSSL_VERSION_TEXT[j];
          if (c == ' ')
            break;
        }
        break;
      }
    }
    READONLY_PROPERTY(
        versions,
        "openssl",
        OneByteString(env->isolate(), &OPENSSL_VERSION_TEXT[i], j - i));
  }
#endif

  // process.arch
  READONLY_PROPERTY(process, "arch", OneByteString(env->isolate(), NODE_NACL_ARCH));

  // process.platform
  READONLY_PROPERTY(process,
                    "platform",
                    OneByteString(env->isolate(), "linux"));

  // process.argv
  Local<Array> arguments = Array::New(env->isolate(), argc);
  for (int i = 0; i < argc; ++i) {
    arguments->Set(i, String::NewFromUtf8(env->isolate(), argv[i]));
  }
  process->Set(env->argv_string(), arguments);

  // process.execArgv
  Local<Array> exec_arguments = Array::New(env->isolate(), exec_argc);
  for (int i = 0; i < exec_argc; ++i) {
    exec_arguments->Set(i, String::NewFromUtf8(env->isolate(), exec_argv[i]));
  }
  process->Set(env->exec_argv_string(), exec_arguments);

  // create process.env
  // TODO-CODIUS: Enable
  /*Local<ObjectTemplate> process_env_template =
      ObjectTemplate::New(env->isolate());
  process_env_template->SetNamedPropertyHandler(EnvGetter,
                                                EnvSetter,
                                                EnvQuery,
                                                EnvDeleter,
                                                EnvEnumerator,
                                                Object::New(env->isolate()));
  Local<Object> process_env = process_env_template->NewInstance();
  process->Set(env->env_string(), process_env);*/

  process->Set(env->env_string(), Object::New(env->isolate()));

  READONLY_PROPERTY(process, "pid", Integer::New(env->isolate(), getpid()));
  READONLY_PROPERTY(process, "features", GetFeatures(env));

  // TODO-CODIUS: Enable _needImmediateCallback
  //process->SetAccessor(env->need_imm_cb_string(),
  //    NeedImmediateCallbackGetter,
  //    NeedImmediateCallbackSetter);

  process->Set(env->exec_path_string(),
               FIXED_ONE_BYTE_STRING(env->isolate(), "/usr/bin/node-nacl"));

  // TODO-CODIUS: Allow attaching a JavaScript debugger
  //process->SetAccessor(env->debug_port_string(),
  //                     DebugPortGetter,
  //                     DebugPortSetter);

  // define various internal methods
  // TODO-CODIUS: Allow profiler
  //NODE_SET_METHOD(process,
  //                "_startProfilerIdleNotifier",
  //                StartProfilerIdleNotifier);
  //NODE_SET_METHOD(process,
  //                "_stopProfilerIdleNotifier",
  //                StopProfilerIdleNotifier);
  //NODE_SET_METHOD(process, "_getActiveRequests", GetActiveRequests);
  //NODE_SET_METHOD(process, "_getActiveHandles", GetActiveHandles);
  NODE_SET_METHOD(process, "reallyExit", Exit);
  NODE_SET_METHOD(process, "abort", Abort);
  NODE_SET_METHOD(process, "chdir", Chdir);
  NODE_SET_METHOD(process, "cwd", Cwd);

  NODE_SET_METHOD(process, "umask", Umask);

#if defined(__POSIX__) && !defined(__ANDROID__)
  NODE_SET_METHOD(process, "getuid", GetUid);
  NODE_SET_METHOD(process, "setuid", SetUid);

  NODE_SET_METHOD(process, "setgid", SetGid);
  NODE_SET_METHOD(process, "getgid", GetGid);

  NODE_SET_METHOD(process, "getgroups", GetGroups);
  NODE_SET_METHOD(process, "setgroups", SetGroups);
  NODE_SET_METHOD(process, "initgroups", InitGroups);
#endif  // __POSIX__ && !defined(__ANDROID__)

  NODE_SET_METHOD(process, "_kill", Kill);

  // TODO-CODIUS: Re-enable debugger
  //NODE_SET_METHOD(process, "_debugProcess", DebugProcess);
  //NODE_SET_METHOD(process, "_debugPause", DebugPause);
  //NODE_SET_METHOD(process, "_debugEnd", DebugEnd);

  // TODO-CODIUS: Re-enable high-resolution timer
  //NODE_SET_METHOD(process, "hrtime", Hrtime);

  // TODO-CODIUS: Allow native extensions
  //NODE_SET_METHOD(process, "dlopen", DLOpen);

  NODE_SET_METHOD(process, "uptime", Uptime);
  NODE_SET_METHOD(process, "memoryUsage", MemoryUsage);

  NODE_SET_METHOD(process, "binding", Binding);

  // TODO-CODIUS: Disabled these...
  //NODE_SET_METHOD(process, "_setupAsyncListener", SetupAsyncListener);
  NODE_SET_METHOD(process, "_setupNextTick", SetupNextTick);
  //NODE_SET_METHOD(process, "_setupDomainUse", SetupDomainUse);

  // pre-set _events object for faster emit checks
  process->Set(env->events_string(), Object::New(env->isolate()));
}


// Most of the time, it's best to use `console.error` to write
// to the process.stderr stream.  However, in some cases, such as
// when debugging the stream.Writable class or the process.nextTick
// function, it is useful to bypass JavaScript entirely.
static void RawDebug(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  HandleScope scope(env->isolate());

  assert(args.Length() == 1 && args[0]->IsString() &&
         "must be called with a single string");

  node::Utf8Value message(args[0]);
  fprintf(stderr, "%s\n", *message);
  fflush(stderr);
}

void Load(Environment* env) {
  HandleScope handle_scope(env->isolate());

  // Compile, execute the src/node.js file. (Which was included as static C
  // string in node_natives.h. 'natve_node' is the string containing that
  // source code.)

  TryCatch try_catch;

  // Disable verbose mode to stop FatalException() handler from trying
  // to handle the exception. Errors this early in the start-up phase
  // are not safe to ignore.
  try_catch.SetVerbose(false);

  Local<String> script_name = FIXED_ONE_BYTE_STRING(env->isolate(), "node.js");
  Local<Value> f_value = ExecuteString(env, MainSource(env), script_name);
  if (try_catch.HasCaught())  {
    ReportException(env, try_catch);
    exit(10);
  }
  assert(f_value->IsFunction());
  Local<Function> f = Local<Function>::Cast(f_value);

  // Now we call 'f' with the 'process' variable that we've built up with
  // all our bindings. Inside node.js we'll take care of assigning things to
  // their places.

  // We start the process this way in order to be more modular. Developers
  // who do not like how 'src/node.js' setups the module system but do like
  // Node's I/O bindings may want to replace 'f' with their own function.

  // Add a reference to the global object
  Local<Object> global = env->context()->Global();

#if defined HAVE_DTRACE || defined HAVE_ETW
  InitDTrace(env, global);
#endif

#if defined HAVE_PERFCTR
  InitPerfCounters(env, global);
#endif

  // Enable handling of uncaught exceptions
  // (FatalException(), break on uncaught exception in debugger)
  //
  // This is not strictly necessary since it's almost impossible
  // to attach the debugger fast enought to break on exception
  // thrown during process startup.
  try_catch.SetVerbose(true);

  NODE_SET_METHOD(env->process_object(), "_rawDebug", RawDebug);

  Local<Value> arg = env->process_object();
  f->Call(global, 1, &arg);
}

static void PrintHelp() {
  printf("Usage: node [options] [ -e script | script.js ] [arguments] \n"
         "       node debug script.js [arguments] \n"
         "\n"
         "Options:\n"
         "  -v, --version        print node's version\n"
         "  -e, --eval script    evaluate script\n"
         "  -p, --print          evaluate script and print result\n"
         "  -i, --interactive    always enter the REPL even if stdin\n"
         "                       does not appear to be a terminal\n"
         "  --no-deprecation     silence deprecation warnings\n"
         "  --throw-deprecation  throw an exception anytime a deprecated "
         "function is used\n"
         "  --trace-deprecation  show stack traces on deprecations\n"
         "  --v8-options         print v8 command line options\n"
         "  --max-stack-size=val set max v8 stack size (bytes)\n"
         "\n"
         "Environment variables:\n"
#ifdef _WIN32
         "NODE_PATH              ';'-separated list of directories\n"
#else
         "NODE_PATH              ':'-separated list of directories\n"
#endif
         "                       prefixed to the module search path.\n"
         "NODE_MODULE_CONTEXTS   Set to 1 to load modules in their own\n"
         "                       global contexts.\n"
         "NODE_DISABLE_COLORS    Set to 1 to disable colors in the REPL\n"
         "\n"
         "Documentation can be found at http://nodejs.org/\n");
}

// Parse command line arguments.
//
// argv is modified in place. exec_argv and v8_argv are out arguments that
// ParseArgs() allocates memory for and stores a pointer to the output
// vector in.  The caller should free them with delete[].
//
// On exit:
//
//  * argv contains the arguments with node and V8 options filtered out.
//  * exec_argv contains both node and V8 options and nothing else.
//  * v8_argv contains argv[0] plus any V8 options
static void ParseArgs(int* argc,
                      const char** argv,
                      int* exec_argc,
                      const char*** exec_argv,
                      int* v8_argc,
                      const char*** v8_argv) {
  const unsigned int nargs = static_cast<unsigned int>(*argc);
  const char** new_exec_argv = new const char*[nargs];
  const char** new_v8_argv = new const char*[nargs];
  const char** new_argv = new const char*[nargs];

  for (unsigned int i = 0; i < nargs; ++i) {
    new_exec_argv[i] = NULL;
    new_v8_argv[i] = NULL;
    new_argv[i] = NULL;
  }

  // exec_argv starts with the first option, the other two start with argv[0].
  unsigned int new_exec_argc = 0;
  unsigned int new_v8_argc = 1;
  unsigned int new_argc = 1;
  new_v8_argv[0] = argv[0];
  new_argv[0] = argv[0];

  unsigned int index = 1;
  while (index < nargs && argv[index][0] == '-') {
    const char* const arg = argv[index];
    unsigned int args_consumed = 1;

    if (strcmp(arg, "--version") == 0 || strcmp(arg, "-v") == 0) {
      printf("%s\n", NODE_VERSION);
      exit(0);
    } else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
      PrintHelp();
      exit(0);
    } else if (strcmp(arg, "--v8-options") == 0) {
      new_v8_argv[new_v8_argc] = "--help";
      new_v8_argc += 1;
    } else {
      // V8 option.  Pass through as-is.
      new_v8_argv[new_v8_argc] = arg;
      new_v8_argc += 1;
    }

    memcpy(new_exec_argv + new_exec_argc,
           argv + index,
           args_consumed * sizeof(*argv));

    new_exec_argc += args_consumed;
    index += args_consumed;
  }

  // Copy remaining arguments.
  const unsigned int args_left = nargs - index;
  memcpy(new_argv + new_argc, argv + index, args_left * sizeof(*argv));
  new_argc += args_left;

  *exec_argc = new_exec_argc;
  *exec_argv = new_exec_argv;
  *v8_argc = new_v8_argc;
  *v8_argv = new_v8_argv;

  // Copy new_argv over argv and update argc.
  memcpy(argv, new_argv, new_argc * sizeof(*argv));
  delete[] new_argv;
  *argc = static_cast<int>(new_argc);
}

void Init(int* argc,
          const char** argv,
          int* exec_argc,
          const char*** exec_argv) {
  timeval now;

  // Initialize prog_start_time to get relative uptime.
  gettimeofday(&now, NULL);
  prog_start_time = now.tv_sec * 1000000 + now.tv_usec;

#if defined(NODE_V8_OPTIONS)
  // Should come before the call to V8::SetFlagsFromCommandLine()
  // so the user can disable a flag --foo at run-time by passing
  // --no_foo from the command line.
  V8::SetFlagsFromString(NODE_V8_OPTIONS, sizeof(NODE_V8_OPTIONS) - 1);
#endif

  // Parse a few arguments which are specific to Node.
  int v8_argc;
  const char** v8_argv;
  ParseArgs(argc, argv, exec_argc, exec_argv, &v8_argc, &v8_argv);

  // The const_cast doesn't violate conceptual const-ness.  V8 doesn't modify
  // the argv array or the elements it points to.
  V8::SetFlagsFromCommandLine(&v8_argc, const_cast<char**>(v8_argv), true);

  // Anything that's still in v8_argv is not a V8 or a node option.
  for (int i = 1; i < v8_argc; i++) {
    fprintf(stderr, "%s: bad option: %s\n", argv[0], v8_argv[i]);
  }
  delete[] v8_argv;
  v8_argv = NULL;

  if (v8_argc > 1) {
    exit(9);
  }

  V8::SetArrayBufferAllocator(&ArrayBufferAllocator::the_singleton);

  // Fetch a reference to the main isolate, so we have a reference to it
  // even when we need it to access it from another (debugger) thread.
  node_isolate = Isolate::GetCurrent();

  V8::SetFatalErrorHandler(node::OnFatalError);
  V8::AddMessageListener(OnMessage);
}

Environment* CreateEnvironment(Isolate* isolate,
                               Handle<Context> context,
                               int argc,
                               const char* const* argv,
                               int exec_argc,
                               const char* const* exec_argv) {
  HandleScope handle_scope(isolate);

  Context::Scope context_scope(context);
  Environment* env = Environment::New(context);

  Local<FunctionTemplate> process_template = FunctionTemplate::New(isolate);
  process_template->SetClassName(FIXED_ONE_BYTE_STRING(isolate, "process"));

  Local<Object> process_object = process_template->GetFunction()->NewInstance();
  env->set_process_object(process_object);

  SetupProcessObject(env, argc, argv, exec_argc, exec_argv);
  Load(env);

  return env;
}

int Start(int argc, char** argv) {
  int exec_argc;
  const char** exec_argv;
  Init(&argc, const_cast<const char**>(argv), &exec_argc, &exec_argv);

  int code;
  V8::Initialize();
  {
    Locker locker(node_isolate);
    HandleScope handle_scope(node_isolate);
    Local<Context> context = Context::New(node_isolate);
    Environment* env = CreateEnvironment(
        node_isolate, context, argc, argv, exec_argc, exec_argv);

    env->Dispose();
    env = NULL;
  }

  CHECK_NE(node_isolate, NULL);
  node_isolate->Dispose();
  node_isolate = NULL;
  V8::Dispose();

  delete[] exec_argv;
  exec_argv = NULL;

  return code;
}

}  // namespace node
