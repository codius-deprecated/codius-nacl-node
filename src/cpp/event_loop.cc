
#include "event_loop.h"

#include <errno.h>
#include <unistd.h>

namespace node {

void EventLoop::Init() {
  unsigned int i;
  int err;

//  uv__signal_global_once_init();

  heap_init((struct heap*) &timer_heap_);
//  QUEUE_INIT(&loop->wq);
//  QUEUE_INIT(&loop->active_reqs);
//  QUEUE_INIT(&loop->idle_handles);
//  QUEUE_INIT(&loop->async_handles);
//  QUEUE_INIT(&loop->check_handles);
//  QUEUE_INIT(&loop->prepare_handles);
  QUEUE_INIT(&handle_queue_);
  for (int i = 0; i <= MAX_LOOP_WATCHER_HANDLE_TYPE; i++) {
    QUEUE_INIT(&loop_watcher_handles_[i]);
  }

  nfds_ = 0;
  watchers_ = NULL;
  nwatchers_ = 0;
//  QUEUE_INIT(&loop->pending_queue);
  QUEUE_INIT(&watcher_queue_);

  closing_handles_ = NULL;
  UpdateTime();
//  uv__async_init(&loop->async_watcher);
//  loop->signal_pipefd[0] = -1;
//  loop->signal_pipefd[1] = -1;
//  loop->backend_fd = -1;
//  loop->emfile_fd = -1;

  timer_counter_ = 0;
//  loop->stop_flag = 0;

//  uv_signal_init(loop, &loop->child_watcher);
//  uv__handle_unref(&loop->child_watcher);
//  loop->child_watcher.flags |= UV__HANDLE_INTERNAL;

//  for (i = 0; i < ARRAY_SIZE(loop->process_handles); i++)
//    QUEUE_INIT(loop->process_handles + i);

//  if (uv_rwlock_init(&loop->cloexec_lock))
//    abort();

//  if (uv_mutex_init(&loop->wq_mutex))
//    abort();

//  if (uv_async_init(loop, &loop->wq_async, uv__work_done))
//    abort();

//  uv__handle_unref(&loop->wq_async);
//  loop->wq_async.flags |= UV__HANDLE_INTERNAL;
}

void EventLoop::Handle::FinishClose() {
  assert(flags_ & fClosing);
  assert(!(flags_ & fClosed));
  flags_ |= fClosed;

  switch (type_) {
    case handleTypeCheck:
    case handleTypeIdle:
    case handleTypePrepare:
    case handleTypeTimer:
      break;

    case handleTypePipe:
      reinterpret_cast<PipeHandle*>(this)->Destroy();
      break;

    default:
      assert(0);
      break;
  }

  Unref();
  QUEUE_REMOVE(&handle_queue_);

  if (close_cb_) {
    close_cb_(this);
  }
}

int EventLoop::TimerHandle::LessThan(const struct heap_node* ha,
                                     const struct heap_node* hb) {
  const EventLoop::TimerHandle* a;
  const EventLoop::TimerHandle* b;

  a = ContainerOf(&TimerHandle::heap_node_, ha);
  b = ContainerOf(&TimerHandle::heap_node_, hb);

  if (a->timeout_ < b->timeout_)
    return 1;
  if (b->timeout_ < a->timeout_)
    return 0;

  /* Compare start_id when both have the same timeout. start_id is
   * allocated with loop->timer_counter in uv_timer_start().
   */
  if (a->start_id_ < b->start_id_)
    return 1;
  if (b->start_id_ < a->start_id_)
    return 0;

  return 0;
}

void EventLoop::TimerHandle::Start(Callback cb, uint64_t timeout, uint64_t repeat) {
  uint64_t clamped_timeout;

  if (IsActive())
    Stop();

  clamped_timeout = loop_->time_ + timeout;
  if (clamped_timeout < timeout)
    clamped_timeout = (uint64_t) -1;

  cb_ = cb;
  timeout_ = clamped_timeout;
  repeat_ = repeat;
  /* start_id is the second index to be compared in uv__timer_cmp() */
  start_id_ = loop_->timer_counter_++;

  heap_insert((struct heap*) &loop_->timer_heap_,
              (struct heap_node*) &heap_node_,
              EventLoop::TimerHandle::LessThan);
  Handle::Start();
}

void EventLoop::TimerHandle::Stop() {
  if (!IsActive())
    return;

  heap_remove((struct heap*) &loop_->timer_heap_,
              (struct heap_node*) &heap_node_,
              EventLoop::TimerHandle::LessThan);

  Handle::Stop();
}

void EventLoop::TimerHandle::Again() {
  assert(cb_ != NULL);

  if (repeat_) {
    Stop();
    Start(cb_, repeat_, repeat_);
  }
}

void EventLoop::IoWatcher::Init(IoCallback cb, int fd) {
  assert(cb != NULL);
  assert(fd >= -1);

  //QUEUE_INIT(&pending_queue_);
  QUEUE_INIT(&watcher_queue_);

  cb_ = cb;
  fd_ = fd;
  pevents_ = 0;
}

void EventLoop::IoWatcher::Start(EventLoop* loop, unsigned int events) {
  assert(0 == (events & ~(ioEventIn | ioEventOut)));
  assert(0 != events);
  assert(fd_ >= 0);
  assert(fd_ < INT_MAX);

  pevents_ |= events;
  loop->MaybeResize(fd_ + 1);

  if (QUEUE_EMPTY(&watcher_queue_))
    QUEUE_INSERT_TAIL(&loop->watcher_queue_, &watcher_queue_);

  if (loop->watchers_[fd_] == NULL) {
    loop->watchers_[fd_] = this;
    loop->nfds_++;
  }
}

void EventLoop::IoWatcher::Stop(EventLoop* loop, unsigned int events) {
  assert(0 == (events & ~(ioEventIn | ioEventOut)));
  assert(0 != events);

  if (fd_ == -1)
    return;

  assert(fd_ >= 0);

  /* Happens when Stop() is called on a handle that was never started. */
  if ((unsigned) fd_ >= loop->nwatchers_)
    return;

  pevents_ &= ~events;

  if (pevents_ == 0) {
    QUEUE_REMOVE(&watcher_queue_);
    QUEUE_INIT(&watcher_queue_);

    if (loop->watchers_[fd_] != NULL) {
      assert(loop->watchers_[fd_] == this);
      assert(loop->nfds_ > 0);
      loop->watchers_[fd_] = NULL;
      loop->nfds_--;
//      events_ = 0;
    }
  }
  else if (QUEUE_EMPTY(&watcher_queue_))
    QUEUE_INSERT_TAIL(&loop->watcher_queue_, &watcher_queue_);
}

void EventLoop::PipeHandle::OnIo(EventLoop* loop,
                                 IoWatcher* watcher,
                                 unsigned int events) {
  PipeHandle* pipe = ContainerOf(&PipeHandle::watcher_, watcher);

  assert(pipe->type_ = handleTypePipe);
  assert(!(pipe->flags_ & fClosing));

  assert(watcher->fd_ >= 0);

  if (events & (ioEventIn | ioEventErr)) {
    pipe->Read();
  }

  // Read callback may close pipe
  if (watcher->fd_ == -1) {
    return;
  }

  /* Short-circuit iff POLLHUP is set, the user is still interested in read
   * events and uv__read() reported a partial read but not EOF. If the EOF
   * flag is set, uv__read() called read_cb with err=UV_EOF and we don't
   * have to do anything. If the partial read flag is not set, we can't
   * report the EOF yet because there is still data to read.
   */
  if ((events & ioEventHup) &&
      (pipe->flags_ & fStreamReading) &&
      (pipe->flags_ & fStreamReadPartial) &&
      !(pipe->flags_ & fStreamReadEof)) {
    Buffer buf = { NULL, 0 };
    pipe->Eof(&buf);
  }

  // EOF callback may close pipe
  if (watcher->fd_ == -1) {
    return;
  }

// TODO-CODIUS: Enable async writes?
//  if (events & (UV__POLLOUT | UV__POLLERR | UV__POLLHUP)) {
//    uv__write(stream);
//    uv__write_callbacks(stream);
//
//    /* Write queue drained. */
//    if (QUEUE_EMPTY(&stream->write_queue))
//      uv__drain(stream);
//  }
}

int EventLoop::PipeHandle::ReadStart(AllocCallback alloc_cb,
                                     ReadCallback read_cb) {
  flags_ |= fStreamReading;

  assert(watcher_.fd_ >= 0);
  assert(alloc_cb);

  alloc_cb_ = alloc_cb;
  read_cb_ = read_cb;

  watcher_.Start(loop_, ioEventIn);
  Handle::Start();

  return 0;
}

int EventLoop::PipeHandle::ReadStop() {
  flags_ &= ~fStreamReading;
  watcher_.Stop(loop_, ioEventIn);
  if (!watcher_.IsActive(ioEventOut))
    Handle::Stop();

  alloc_cb_ = NULL;
  read_cb_ = NULL;

  return 0;
}

void EventLoop::PipeHandle::Read() {
  Buffer buf;
  ssize_t nread;
//  struct msghdr msg;
//  char cmsg_space[CMSG_SPACE(UV__CMSG_FD_SIZE)];
  int count;
//  int err;
//  int is_ipc;

  flags_ &= ~fStreamReadPartial;
//
//  /* Prevent loop starvation when the data comes in as fast as (or faster than)
//   * we can read it. XXX Need to rearm fd if we switch to edge-triggered I/O.
//   */
  count = 32;

//  is_ipc = stream->type == UV_NAMED_PIPE && ((uv_pipe_t*) stream)->ipc;

  /* XXX: Maybe instead of having UV_STREAM_READING we just test if
   * tcp->read_cb is NULL or not?
   */
  while (read_cb_
         && (flags_ & fStreamReading)
         && (count-- > 0)) {
    assert(alloc_cb_ != NULL);

    alloc_cb_(this, 64 * 1024, &buf);
//    if (buf.len == 0) {
//      /* User indicates it can't or won't handle the read. */
//      stream->read_cb(stream, UV_ENOBUFS, &buf);
//      return;
//    }
//
    assert(buf.base != NULL);
    assert(watcher_.fd_ >= 0);
//
//    if (!is_ipc) {
      do {
        nread = read(watcher_.fd_, buf.base, buf.len);
      }
      while (nread < 0 && errno == EINTR);
//    } else {
//      /* ipc uses recvmsg */
//      msg.msg_flags = 0;
//      msg.msg_iov = (struct iovec*) &buf;
//      msg.msg_iovlen = 1;
//      msg.msg_name = NULL;
//      msg.msg_namelen = 0;
//      /* Set up to receive a descriptor even if one isn't in the message */
//      msg.msg_controllen = sizeof(cmsg_space);
//      msg.msg_control = cmsg_space;
//
//      do {
//        nread = uv__recvmsg(uv__stream_fd(stream), &msg, 0);
//      }
//      while (nread < 0 && errno == EINTR);
//    }
//
    if (nread < 0) {
//      /* Error */
//      if (errno == EAGAIN || errno == EWOULDBLOCK) {
//        /* Wait for the next one. */
//        if (stream->flags & UV_STREAM_READING) {
//          uv__io_start(stream->loop, &stream->io_watcher, UV__POLLIN);
//          uv__stream_osx_interrupt_select(stream);
//        }
//        stream->read_cb(stream, 0, &buf);
//      } else {
//        /* Error. User should call uv_close(). */
//        stream->read_cb(stream, -errno, &buf);
//        assert(!uv__io_active(&stream->io_watcher, UV__POLLIN) &&
//               "stream->read_cb(status=-1) did not call uv_close()");
//      }
      printf("TODO-CODIUS: Unhandled read error!\n");
      abort();
      return;
    } else if (nread == 0) {
      // TODO-CODIUS: Handle EOF
      printf("TODO-CODIUS: Unhandled EOF!\n");
      abort();
//      uv__stream_eof(stream, &buf);
      return;
    } else {
      /* Successful read */
      ssize_t buflen = buf.len;

//      if (is_ipc) {
//        err = uv__stream_recv_cmsg(stream, &msg);
//        if (err != 0) {
//          stream->read_cb(stream, err, &buf);
//          return;
//        }
//      }
      read_cb_(this, nread, &buf);

      /* Return if we didn't fill the buffer, there is no more data to read. */
      if (nread < buflen) {
        flags_ |= fStreamReadPartial;
        return;
      }
    }
  }
}

void EventLoop::PipeHandle::Eof(Buffer* buf) {
  flags_ |= fStreamReadEof;
  watcher_.Stop(loop_, ioEventIn);
  if (!watcher_.IsActive(ioEventOut))
    Handle::Stop();
  read_cb_(this, EOF, buf);
}

void EventLoop::PollIo(int timeout) {
  struct pollfd *pfds;
  QUEUE* q;
  IoWatcher* watcher;
  uint64_t base;
  uint64_t diff;
  int nevents;
  int count;
  int nfds;
  int fd;
  int i;

  if (nfds_ == 0) {
    assert(QUEUE_EMPTY(&watcher_queue_));
    return;
  }

  pfds = static_cast<struct pollfd*>(calloc(sizeof(pollfd), nwatchers_));

  i = 0;
  while (!QUEUE_EMPTY(&watcher_queue_)) {
    q = QUEUE_HEAD(&watcher_queue_);
    QUEUE_REMOVE(q);
    QUEUE_INIT(q);

    watcher = ContainerOf(&IoWatcher::watcher_queue_, q);
    assert(watcher->fd_ >= 0);
    assert(watcher->fd_ < (int) nwatchers_);

    pfds[i].fd = watcher->fd_;
    pfds[i].events = watcher->pevents_;

    i++;
  }

  assert(timeout >= -1);
  base = time_;

  for (;;) {
    nfds = poll(pfds, i, timeout);

    UpdateTime();

    if (nfds == 0) {
      assert(timeout != -1);
      return;
    }

    if (nfds == -1) {
      if (errno != EINTR)
        abort();

      if (timeout == -1)
        continue;

      if (timeout == 0)
        return;

      /* Interrupted by a signal. Update timeout and poll again. */
      goto update_timeout;
    }

    nevents = 0;

    if (nfds == -1) {
      // TODO-CODIUS: Handle error better
      perror("select()");
      assert(0);
    } else if (nfds) {
      for (i = 0; i < nfds; i++) {
        fd = pfds[i].fd;

        if (fd == -1) {
          continue;
        }

        assert(fd >= 0);
        assert((unsigned) fd < nwatchers_);

        watcher = watchers_[fd];

        pfds[i].revents &= watcher->pevents_ | ioEventErr | ioEventHup;

        if (pfds[i].revents == ioEventErr || pfds[i].events == ioEventHup)
          pfds[i].revents |= watcher->pevents_ & (ioEventIn | ioEventOut);

        if (pfds[i].revents != 0) {
          watcher->cb_(this, watcher, ioEventIn);
          nevents++;
        }
      }

      if (nevents != 0) {
        return;
      }

      if (timeout == 0)
        return;

      if (timeout == -1)
        continue;

update_timeout:
      assert(timeout > 0);

      diff = time_ - base;
      if (diff >= static_cast<uint64_t>(timeout))
        return;

      timeout -= diff;
    }

    return;
  }
}

}  // namespace node
