
#include "event_loop.h"

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

//  loop->nfds = 0;
//  loop->watchers = NULL;
//  loop->nwatchers = 0;
//  QUEUE_INIT(&loop->pending_queue);
//  QUEUE_INIT(&loop->watcher_queue);

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

}  // namespace node
