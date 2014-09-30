/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
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

#include "uv.h"
#include "internal.h"
#include "codius-util.h"

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>


int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* tcp) {
  uv__stream_init(loop, (uv_stream_t*)tcp, UV_TCP);
  return 0;
}


static int maybe_new_socket(uv_tcp_t* handle, int domain, int flags) {
  int sockfd;
  int err;

  if (uv__stream_fd(handle) != -1)
    return 0;

  err = uv__socket(domain, SOCK_STREAM, 0);
  if (err < 0)
    return err;
  sockfd = err;

  err = uv__stream_open((uv_stream_t*) handle, sockfd, flags);
  if (err) {
    uv__close(sockfd);
    return err;
  }

  return 0;
}


int uv__tcp_bind(uv_tcp_t* tcp,
                 const struct sockaddr* addr,
                 unsigned int addrlen,
                 unsigned int flags) {
  int err;
  int on;
  int r;

  /* Cannot set IPv6-only mode on non-IPv6 socket. */
  if ((flags & UV_TCP_IPV6ONLY) && addr->sa_family != AF_INET6)
    return -EINVAL;

  err = maybe_new_socket(tcp,
                         addr->sa_family,
                         UV_STREAM_READABLE | UV_STREAM_WRITABLE);
  if (err)
    return err;

//   on = 1;
//   if (setsockopt(tcp->io_watcher.fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
//     return -errno;

// #ifdef IPV6_V6ONLY
//   if (addr->sa_family == AF_INET6) {
//     on = (flags & UV_TCP_IPV6ONLY) != 0;
//     if (setsockopt(tcp->io_watcher.fd,
//                    IPPROTO_IPV6,
//                    IPV6_V6ONLY,
//                    &on,
//                    sizeof on) == -1) {
//       return -errno;
//     }
//   }
// #endif

  //CODIUS-MOD: Replace 'bind' syscall with call to create server outside the sandbox.
  // errno = 0;
  // if (bind(tcp->io_watcher.fd, addr, addrlen) && errno != EADDRINUSE)
  //   return -errno;
  // tcp->delayed_error = -errno;

  // TODO-CODIUS: We're about to cast addr to a sockaddr_in, which is only possible
  //              if the family is IPv4. For IPv6 that next section will have to
  //              be different.
  assert(addr->sa_family == AF_INET);

  const char* frame = "{\"type\":\"api\",\"api\":\"net\",\"method\":\"bind\",\"data\":[%d, %d, %d, %d]}";
  char message [UV_SYNC_MAX_MESSAGE_SIZE];
  int len;
  len = snprintf (message, UV_SYNC_MAX_MESSAGE_SIZE, frame, tcp->io_watcher.fd, addr->sa_family, 
                  ((struct sockaddr_in*)addr)->sin_addr.s_addr, ((struct sockaddr_in*)addr)->sin_port);
  if (len==-1) {
    printf("Error forming socket message.");
    abort();
  }
  char *resp_buf;
  size_t resp_len;
  int result = codius_sync_call(message, len, &resp_buf, &resp_len);
  assert(result != -1);
  r = codius_parse_json_int(resp_buf, resp_len, "result");
  free(resp_buf);

  if (addr->sa_family == AF_INET6)
    tcp->flags |= UV_HANDLE_IPV6;

  return 0;
}


int uv__tcp_connect(uv_connect_t* req,
                    uv_tcp_t* handle,
                    const struct sockaddr* addr,
                    unsigned int addrlen,
                    uv_connect_cb cb) {
  int err;
  int r;

  assert(handle->type == UV_TCP);

  if (handle->connect_req != NULL)
    return -EALREADY;  /* FIXME(bnoordhuis) -EINVAL or maybe -EBUSY. */

  err = maybe_new_socket(handle,
                         addr->sa_family,
                         UV_STREAM_READABLE | UV_STREAM_WRITABLE);

  if (err)
    return err;

  handle->delayed_error = 0;

  // TODO-CODIUS: We're about to cast addr to a sockaddr_in, which is only possible
  //              if the family is IPv4. For IPv6 that next section will have to
  //              be different.
  assert(addr->sa_family == AF_INET);

  const char* frame = "{\"type\":\"api\",\"api\":\"net\",\"method\":\"connect\",\"data\":[%d, %d, %d, %d]}";
  char message [UV_SYNC_MAX_MESSAGE_SIZE];
  int len;
  len = snprintf (message, UV_SYNC_MAX_MESSAGE_SIZE, frame, uv__stream_fd(handle), addr->sa_family, ((struct sockaddr_in*)addr)->sin_addr.s_addr, ((struct sockaddr_in*)addr)->sin_port);
  if (len==-1) {
    printf("Error forming socket message.");
    abort();
  }
  char *resp_buf;
  size_t resp_len;
  int result = codius_sync_call(message, len, &resp_buf, &resp_len);
  assert(result != -1);
  r = codius_parse_json_int(resp_buf, resp_len, "result");
  free(resp_buf);

  if (r == -1) {
    if (errno == EINPROGRESS)
      ; /* not an error */
    else if (errno == ECONNREFUSED)
    /* If we get a ECONNREFUSED wait until the next tick to report the
     * error. Solaris wants to report immediately--other unixes want to
     * wait.
     */
      handle->delayed_error = -errno;
    else
      return -errno;
  }

  // Successful codius_sync_call == UV_CONNECT event
  // Call the callback
  req->handle = (uv_stream_t*) handle;
  cb(req, 0);

  return 0;
}


int uv_tcp_open(uv_tcp_t* handle, uv_os_sock_t sock) {
  return uv__stream_open((uv_stream_t*)handle,
                         sock,
                         UV_STREAM_READABLE | UV_STREAM_WRITABLE);
}


int uv_tcp_getsockname(const uv_tcp_t* handle,
                       struct sockaddr* name,
                       int* namelen) {
  socklen_t socklen;

  if (handle->delayed_error)
    return handle->delayed_error;

  if (uv__stream_fd(handle) < 0)
    return -EINVAL;  /* FIXME(bnoordhuis) -EBADF */

  /* sizeof(socklen_t) != sizeof(int) on some systems. */
  socklen = (socklen_t) *namelen;

  if (getsockname(uv__stream_fd(handle), name, &socklen))
    return -errno;

  *namelen = (int) socklen;
  return 0;
}


int uv_tcp_getpeername(const uv_tcp_t* handle,
                       struct sockaddr* name,
                       int* namelen) {
  socklen_t socklen;

  if (handle->delayed_error)
    return handle->delayed_error;

  if (uv__stream_fd(handle) < 0)
    return -EINVAL;  /* FIXME(bnoordhuis) -EBADF */

  /* sizeof(socklen_t) != sizeof(int) on some systems. */
  socklen = (socklen_t) *namelen;

  if (getpeername(uv__stream_fd(handle), name, &socklen))
    return -errno;

  *namelen = (int) socklen;
  return 0;
}


int uv_tcp_listen(uv_tcp_t* tcp, int backlog, uv_connection_cb cb) {
  static int single_accept = -1;
  int err;

  if (tcp->delayed_error)
    return tcp->delayed_error;

  if (single_accept == -1) {
    const char* val = getenv("UV_TCP_SINGLE_ACCEPT");
    single_accept = (val != NULL && atoi(val) != 0);  /* Off by default. */
  }

  if (single_accept)
    tcp->flags |= UV_TCP_SINGLE_ACCEPT;

  err = maybe_new_socket(tcp, AF_INET, UV_STREAM_READABLE);
  if (err)
    return err;

  //CODIUS-MOD: Fake socket starts listening from uv__tcp_bind.
  // if (listen(tcp->io_watcher.fd, backlog))
  //   return -errno;

  tcp->connection_cb = cb;

  /* Start listening for connections. */
  tcp->io_watcher.cb = uv__server_io;
  uv__io_start(tcp->loop, &tcp->io_watcher, UV__POLLIN);

  return 0;
}


int uv__tcp_nodelay(int fd, int on) {
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)))
    return -errno;
  return 0;
}


int uv__tcp_keepalive(int fd, int on, unsigned int delay) {
  if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on)))
    return -errno;

#ifdef TCP_KEEPIDLE
  if (on && setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &delay, sizeof(delay)))
    return -errno;
#endif

  /* Solaris/SmartOS, if you don't support keep-alive,
   * then don't advertise it in your system headers...
   */
  /* FIXME(bnoordhuis) That's possibly because sizeof(delay) should be 1. */
#if defined(TCP_KEEPALIVE) && !defined(__sun)
  if (on && setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &delay, sizeof(delay)))
    return -errno;
#endif

  return 0;
}


int uv_tcp_nodelay(uv_tcp_t* handle, int on) {
  int err;

  if (uv__stream_fd(handle) != -1) {
    err = uv__tcp_nodelay(uv__stream_fd(handle), on);
    if (err)
      return err;
  }

  if (on)
    handle->flags |= UV_TCP_NODELAY;
  else
    handle->flags &= ~UV_TCP_NODELAY;

  return 0;
}


int uv_tcp_keepalive(uv_tcp_t* handle, int on, unsigned int delay) {
  int err;

  if (uv__stream_fd(handle) != -1) {
    err =uv__tcp_keepalive(uv__stream_fd(handle), on, delay);
    if (err)
      return err;
  }

  if (on)
    handle->flags |= UV_TCP_KEEPALIVE;
  else
    handle->flags &= ~UV_TCP_KEEPALIVE;

  /* TODO Store delay if uv__stream_fd(handle) == -1 but don't want to enlarge
   *      uv_tcp_t with an int that's almost never used...
   */

  return 0;
}


int uv_tcp_simultaneous_accepts(uv_tcp_t* handle, int enable) {
  if (enable)
    handle->flags &= ~UV_TCP_SINGLE_ACCEPT;
  else
    handle->flags |= UV_TCP_SINGLE_ACCEPT;
  return 0;
}


void uv__tcp_close(uv_tcp_t* handle) {
  uv__stream_close((uv_stream_t*)handle);
}
