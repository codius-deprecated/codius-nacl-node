/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 * Copyright (c) 2014 Ripple Labs Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#ifndef UV_UNIX_INTERNAL_H_
#define UV_UNIX_INTERNAL_H_

#include "uv-common.h"
#include <sys/time.h>

#define STATIC_ASSERT(expr)                                                   \
  void uv__static_assert(int static_assert_failed[1 - 2 * !(expr)])

/* The __clang__ and __INTEL_COMPILER checks are superfluous because they
 * define __GNUC__. They are here to convey to you, dear reader, that these
 * macros are enabled when compiling with clang or icc.
 */
#if defined(__clang__) ||                                                     \
    defined(__GNUC__) ||                                                      \
    defined(__INTEL_COMPILER) ||                                              \
    defined(__SUNPRO_C)
# define UV_DESTRUCTOR(declaration) __attribute__((destructor)) declaration
# define UV_UNUSED(declaration)     __attribute__((unused)) declaration
#else
# define UV_DESTRUCTOR(declaration) declaration
# define UV_UNUSED(declaration)     declaration
#endif
    
#ifndef UV__POLLIN
# define UV__POLLIN   1
#endif

#ifndef UV__POLLOUT
# define UV__POLLOUT  2
#endif

#ifndef UV__POLLERR
# define UV__POLLERR  4
#endif

#ifndef UV__POLLHUP
# define UV__POLLHUP  8
#endif

typedef struct uv__stream_queued_fds_s uv__stream_queued_fds_t;

/* handle flags */
enum {
  UV_CLOSING              = 0x01,   /* uv_close() called but not finished. */
  UV_CLOSED               = 0x02,   /* close(2) finished. */
  UV_STREAM_READING       = 0x04,   /* uv_read_start() called. */
  UV_STREAM_SHUTTING      = 0x08,   /* uv_shutdown() called but not complete. */
  UV_STREAM_SHUT          = 0x10,   /* Write side closed. */
  UV_STREAM_READABLE      = 0x20,   /* The stream is readable */
  UV_STREAM_WRITABLE      = 0x40,   /* The stream is writable */
  UV_STREAM_BLOCKING      = 0x80,   /* Synchronous writes. */
  UV_STREAM_READ_PARTIAL  = 0x100,  /* read(2) read less than requested. */
  UV_STREAM_READ_EOF      = 0x200,  /* read(2) read EOF. */
  UV_TCP_NODELAY          = 0x400,  /* Disable Nagle. */
  UV_TCP_KEEPALIVE        = 0x800,  /* Turn on keep-alive. */
  UV_TCP_SINGLE_ACCEPT    = 0x1000, /* Only accept() when idle. */
  UV_HANDLE_IPV6          = 0x2000  /* Handle is bound to a IPv6 socket. */
};

typedef enum {
  UV_CLOCK_PRECISE = 0,  /* Use the highest resolution clock available. */
  UV_CLOCK_FAST = 1      /* Use the fastest clock with <= 1ms granularity. */
} uv_clocktype_t;

struct uv__stream_queued_fds_s {
  unsigned int size;
  unsigned int offset;
  int fds[1];
};

/* core */
int uv__nonblock(int fd, int set);
int uv__close(int fd);
int uv__cloexec(int fd, int set);
int uv__socket(int domain, int type, int protocol);
int uv__dup(int fd);
ssize_t uv__recvmsg(int fd, struct msghdr *msg, int flags);
void uv__make_close_pending(uv_handle_t* handle);

void uv__io_init(uv__io_t* w, uv__io_cb cb, int fd);
void uv__io_start(uv_loop_t* loop, uv__io_t* w, unsigned int events);
void uv__io_stop(uv_loop_t* loop, uv__io_t* w, unsigned int events);
void uv__io_close(uv_loop_t* loop, uv__io_t* w);
void uv__io_feed(uv_loop_t* loop, uv__io_t* w);
int uv__io_active(const uv__io_t* w, unsigned int events);
void uv__io_poll(uv_loop_t* loop, int timeout); /* in milliseconds or -1 */

/* loop */
void uv__run_idle(uv_loop_t* loop);
void uv__run_check(uv_loop_t* loop);
void uv__run_prepare(uv_loop_t* loop);

/* stream */
void uv__stream_init(uv_loop_t* loop, uv_stream_t* stream,
    uv_handle_type type);
int uv__stream_open(uv_stream_t*, int fd, int flags);
void uv__stream_destroy(uv_stream_t* stream);
#if defined(__APPLE__)
int uv__stream_try_select(uv_stream_t* stream, int* fd);
#endif /* defined(__APPLE__) */
void uv__server_io(uv_loop_t* loop, uv__io_t* w, unsigned int events);
int uv__accept(int sockfd);
int uv__dup2_cloexec(int oldfd, int newfd);
int uv__open_cloexec(const char* path, int flags);

/* timer */
void uv__run_timers(uv_loop_t* loop);
int uv__next_timeout(const uv_loop_t* loop);

/* platform specific */
uint64_t uv__hrtime(uv_clocktype_t type);
int uv__kqueue_init(uv_loop_t* loop);
int uv__platform_loop_init(uv_loop_t* loop, int default_loop);
void uv__platform_loop_delete(uv_loop_t* loop);
void uv__platform_invalidate_fd(uv_loop_t* loop, int fd);

/* various */
void uv__async_close(uv_async_t* handle);
void uv__check_close(uv_check_t* handle);
void uv__fs_event_close(uv_fs_event_t* handle);
void uv__idle_close(uv_idle_t* handle);
void uv__pipe_close(uv_pipe_t* handle);
void uv__poll_close(uv_poll_t* handle);
void uv__prepare_close(uv_prepare_t* handle);
void uv__process_close(uv_process_t* handle);
void uv__stream_close(uv_stream_t* handle);
void uv__tcp_close(uv_tcp_t* handle);
void uv__timer_close(uv_timer_t* handle);
void uv__udp_close(uv_udp_t* handle);
void uv__udp_finish_close(uv_udp_t* handle);
uv_handle_type uv__handle_type(int fd);

#define uv__stream_fd(handle) ((handle)->io_watcher.fd)

UV_UNUSED(static void uv__req_init(uv_loop_t* loop,
                                   uv_req_t* req,
                                   uv_req_type type)) {
  req->type = type;
  uv__req_register(loop, req);
}
#define uv__req_init(loop, req, type) \
  uv__req_init((loop), (uv_req_t*)(req), (type))

UV_UNUSED(static void uv__update_time(uv_loop_t* loop)) {
  /* Use a fast time source if available.  We only need millisecond precision.
   */
  //loop->time = uv__hrtime(UV_CLOCK_FAST) / 1000000;
  struct timeval now;
  gettimeofday(&now, NULL);
  // Convert to milliseconds
  loop->time = now.tv_sec * 1000 + now.tv_usec / 1000;
}

#ifdef HAVE_DTRACE
#include "uv-dtrace.h"
#else
#define UV_TICK_START(arg0, arg1)
#define UV_TICK_STOP(arg0, arg1)
#endif

#endif /* UV_UNIX_INTERNAL_H_ */