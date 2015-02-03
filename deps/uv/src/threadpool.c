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

#include "uv-common.h"
#include "internal.h"

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_THREADPOOL_SIZE 128
#define CODIUS_ASYNC_IO_FD 3
#define CODIUS_MAGIC_BYTES 0xC0D105FE

static uv_once_t once = UV_ONCE_INIT;
static uv_cond_t cond;
static uv_mutex_t mutex;
static unsigned int nthreads;
static uv_thread_t* threads;
static uv_thread_t default_threads[4];
static QUEUE exit_message;
static QUEUE wq;
static volatile int initialized;
static unsigned long int unique_id = 1; //0 indicates no callback (synchronous)

typedef struct callback_list_s callback_list_t;

struct callback_list_s {
  RB_ENTRY(callback_list_s) entry;
  struct uv__work* work;
  unsigned long int id;
};

struct callback_root {
  callback_list_t* rbh_root;
};
#define CAST(p) ((struct callback_root*)(p))


static int compare_callbacks(const callback_list_t* a,
                            const callback_list_t* b) {
  if (a->id < b->id) return -1;
  if (a->id > b->id) return 1;
  return 0;
}

RB_GENERATE_STATIC(callback_root, callback_list_s, entry, compare_callbacks)

static callback_list_t* find_callback(uv_loop_t* loop, int callback_id) {
  callback_list_t w;
  w.id = callback_id;
  return RB_FIND(callback_root, CAST(&loop->async_callbacks), &w);
}

// static void uv__req_init(uv_loop_t* loop,
//                          uv_req_t* req,
//                          uv_req_type type) {
//   req->type = type;
//   uv__req_register(loop, req);
// }
// #define uv__req_init(loop, req, type) \
//   uv__req_init((loop), (uv_req_t*)(req), (type))

static void uv__cancelled(struct uv__work* w) {
  abort();
}

static void uv__codius_async_io(uv_loop_t* loop, uv__io_t* w, unsigned int events) {
  size_t bytes_read;
  codius_rpc_header_t rpc_header;
  const int fd = 3;

  const char* message = "{\"type\":\"request_async_response\"}";
  
  // char resp_buf[UV_SYNC_MAX_MESSAGE_SIZE];
  // int resp_len;
  // resp_len = uv_sync_call(message, len, resp_buf, sizeof(resp_buf));

  rpc_header.magic_bytes = CODIUS_MAGIC_BYTES;
  rpc_header.callback_id = 0;
  rpc_header.size = strlen(message);
  
  if (-1==write(fd, &rpc_header, sizeof(rpc_header)) ||
      -1==write(fd, message, strlen(message))) {
    perror("write()");
    //TYPE_ERROR("Error writing to sync fd 4");
    return -1;
  }
  
  bytes_read = read(fd, &rpc_header, sizeof(rpc_header));
  if (rpc_header.magic_bytes!=CODIUS_MAGIC_BYTES) {
    //TYPE_ERROR("Error reading sync fd 4, invalid header");
    return -1;
  }
  
  // No asynchronous responses found.
  if (rpc_header.size==0) {
    return;
  }

  char buf[rpc_header.size];
  bytes_read = read(fd, &buf, rpc_header.size);

  callback_list_t *cb_list;
  if (bytes_read && bytes_read!=-1) {
    cb_list = find_callback(loop, rpc_header.callback_id);
    cb_list->work->done(cb_list->work, 0, buf, rpc_header.size);
  }
}

int uv_codius_async_init(uv_loop_t* loop) {
  uv__io_init(&loop->codius_async_watcher, uv__codius_async_io, CODIUS_ASYNC_IO_FD);
  uv__io_start(loop, &loop->codius_async_watcher, UV__POLLIN);

  return 0;
}


// static void init_once(void) {
//   unsigned int i;
//   const char* val;

//   nthreads = ARRAY_SIZE(default_threads);
//   val = getenv("UV_THREADPOOL_SIZE");
//   if (val != NULL)
//     nthreads = atoi(val);
//   if (nthreads == 0)
//     nthreads = 1;
//   if (nthreads > MAX_THREADPOOL_SIZE)
//     nthreads = MAX_THREADPOOL_SIZE;

//   threads = default_threads;
//   if (nthreads > ARRAY_SIZE(default_threads)) {
//     threads = malloc(nthreads * sizeof(threads[0]));
//     if (threads == NULL) {
//       nthreads = ARRAY_SIZE(default_threads);
//       threads = default_threads;
//     }
//   }

//   if (uv_cond_init(&cond))
//     abort();

//   if (uv_mutex_init(&mutex))
//     abort();

//   QUEUE_INIT(&wq);

//   for (i = 0; i < nthreads; i++)
//     if (uv_thread_create(threads + i, worker, NULL))
//       abort();

//   initialized = 1;
// }


// void uv__work_submit(uv_loop_t* loop,
//                      struct uv__work* w,
//                      void (*work)(struct uv__work* w),
//                      void (*done)(struct uv__work* w, int status)) {
//   //uv_once(&once, init_once);
//   w->loop = loop;
//   w->work = work;
//   w->done = done;
//   post(&w->wq);
// }


void uv__work_submit(uv_loop_t* loop,
                     struct uv__work* w,
                     const char *buf,
                     size_t buf_len,
                     void (*done)(struct uv__work* w, int status, const char *buf, size_t buf_len)) {
  // uv_once(&once, init_once);
  w->loop = loop;
  w->done = done;

  callback_list_t* c;
  
  if (unique_id == ULONG_MAX) {
    w->done(w, UV_ENOSPC, NULL, 0);
    return;
  }
  
  c = (callback_list_t*)malloc(sizeof(*c));
  c->id = unique_id++;
  c->work = w;
  RB_INSERT(callback_root, CAST(&loop->async_callbacks), c);

  unsigned long magic_bytes = CODIUS_MAGIC_BYTES;
  if (-1==write(CODIUS_ASYNC_IO_FD, &magic_bytes, sizeof(magic_bytes)) ||
      -1==write(CODIUS_ASYNC_IO_FD, &c->id, sizeof(c->id)) ||
      -1==write(CODIUS_ASYNC_IO_FD, &buf_len, sizeof(buf_len)) ||
      -1==write(CODIUS_ASYNC_IO_FD, buf, buf_len)) {
    //TODO-CODIUS Throw some error
  }
}


static int uv__work_cancel(uv_loop_t* loop, uv_req_t* req, struct uv__work* w) {
  // int cancelled;

  // uv_mutex_lock(&mutex);
  // uv_mutex_lock(&w->loop->wq_mutex);

  // cancelled = !QUEUE_EMPTY(&w->wq) && w->work != NULL;
  // if (cancelled)
  //   QUEUE_REMOVE(&w->wq);

  // uv_mutex_unlock(&w->loop->wq_mutex);
  // uv_mutex_unlock(&mutex);

  // if (!cancelled)
    return UV_EBUSY;

  // w->work = uv__cancelled;
  // uv_mutex_lock(&loop->wq_mutex);
  // QUEUE_INSERT_TAIL(&loop->wq, &w->wq);
  // uv_async_send(&loop->wq_async);
  // uv_mutex_unlock(&loop->wq_mutex);

  // return 0;
}


void uv__work_done(uv_async_t* handle) {
  struct uv__work* w;
  uv_loop_t* loop;
  QUEUE* q;
  QUEUE wq;
  int err;

  loop = container_of(handle, uv_loop_t, wq_async);
  QUEUE_INIT(&wq);

  // uv_mutex_lock(&loop->wq_mutex);
  if (!QUEUE_EMPTY(&loop->wq)) {
    q = QUEUE_HEAD(&loop->wq);
    QUEUE_SPLIT(&loop->wq, q, &wq);
  }
  // uv_mutex_unlock(&loop->wq_mutex);

  while (!QUEUE_EMPTY(&wq)) {
    q = QUEUE_HEAD(&wq);
    QUEUE_REMOVE(q);

    w = container_of(q, struct uv__work, wq);
    w->done(w, 0, NULL, 0);
  }
}


static void uv__queue_work(struct uv__work* w) {
  uv_work_t* req = container_of(w, uv_work_t, work_req);

  req->work_cb(req);
}


static void uv__queue_done(struct uv__work* w, int err, const char *buf, size_t buf_len) {
  uv_work_t* req;

  req = container_of(w, uv_work_t, work_req);
  uv__req_unregister(req->loop, req);

  if (req->after_work_cb == NULL)
    return;

  req->after_work_cb(req, err, buf, buf_len);
}


// int uv_queue_work(uv_loop_t* loop,
//                   uv_work_t* req,
//                   uv_work_cb work_cb,
//                   uv_after_work_cb after_work_cb) {
//   if (work_cb == NULL)
//     return -EINVAL;

//   uv__req_init(loop, req, UV_WORK);
//   req->loop = loop;
//   req->work_cb = work_cb;
//   req->after_work_cb = after_work_cb;
//   uv__work_submit(loop, &req->work_req, uv__queue_work, uv__queue_done);
//   return 0;
// }


int uv_queue_work(uv_loop_t* loop,
                  uv_work_t* req,
                  const char *buf,
                  size_t buf_len,
                  uv_after_work_cb after_work_cb) {
  uv__req_init(loop, req, UV_WORK);
  req->loop = loop;
  req->after_work_cb = after_work_cb;
  uv__work_submit(loop, &req->work_req, buf, buf_len, uv__queue_done);
  return 0;
}


int uv_cancel(uv_req_t* req) {
  struct uv__work* wreq;
  uv_loop_t* loop;

  switch (req->type) {
  case UV_FS:
    loop =  ((uv_fs_t*) req)->loop;
    wreq = &((uv_fs_t*) req)->work_req;
    break;
  case UV_GETADDRINFO:
    loop =  ((uv_getaddrinfo_t*) req)->loop;
    wreq = &((uv_getaddrinfo_t*) req)->work_req;
    break;
  case UV_GETNAMEINFO:
    loop = ((uv_getnameinfo_t*) req)->loop;
    wreq = &((uv_getnameinfo_t*) req)->work_req;
    break;
  case UV_WORK:
    loop =  ((uv_work_t*) req)->loop;
    wreq = &((uv_work_t*) req)->work_req;
    break;
  default:
    return UV_EINVAL;
  }

  return uv__work_cancel(loop, req, wreq);
}