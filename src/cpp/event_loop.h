#ifndef SRC_EVENT_LOOP_H_
#define SRC_EVENT_LOOP_H_

#include "queue.h"
#include "util.h"
#include "util-inl.h"
#include "heap-inl.h"

#include <sys/time.h>

namespace node {

class EventLoop {
public:
  static EventLoop& GetLoop() {
    // Guaranteed to be destroyed.
    // Instantiated on first use.
    static EventLoop instance;
    return instance;
  }

  typedef struct {
    char* base;
    size_t len;
  } Buffer;

  enum RunModes {
    modeRunOnce,
    modeRunNoWait
  };

  enum LoopWatcherHandleType {
    handleLoopWatcherIdle,
    handleLoopWatcherCheck,
    handleLoopWatcherPrepare,
    MAX_LOOP_WATCHER_HANDLE_TYPE = handleLoopWatcherPrepare
  };

  enum HandleType {
    handleTypeIdle = handleLoopWatcherIdle,
    handleTypeCheck = handleLoopWatcherCheck,
    handleTypePrepare = handleLoopWatcherPrepare,
    handleTypeTimer,
    handleTypePipe
  };

  class Handle {
  public:

    inline void Init(EventLoop* loop, HandleType type) {
      loop_ = loop;
      type_ = type;
      flags_ = fHandleRef;
      QUEUE_INSERT_TAIL(&loop_->handle_queue_, &handle_queue_);
      next_closing_ = NULL;
    }

    inline bool IsActive() const {
      return (flags_ & fHandleActive);
    }

    inline void Start() {
      if (IsActive()) return;
      flags_ |= fHandleActive;
      if (HasRef()) loop_->active_handles_++;
    }

    inline void Stop() {
      if (!IsActive()) return;
      flags_ &= ~fHandleActive;
      if (HasRef()) loop_->active_handles_--;
    }

    inline bool HasRef() const {
      return (flags_ & fHandleRef);
    }

    inline void Ref() {
      if (HasRef()) return;
      flags_ |= fHandleRef;
      if (IsActive()) loop_->active_handles_++;
    }

    inline void Unref() {
      if (!HasRef()) return;
      flags_ &= ~fHandleRef;
      if (IsActive()) loop_->active_handles_--;
    }

    inline void MakeClosePending() {
      assert(flags_ & fClosing);
      assert(!(flags_ & fClosed));
      next_closing_ = loop_->closing_handles_;
      loop_->closing_handles_ = this;
    }

    typedef void (*CloseCallback)(Handle* handle);
    inline void Close(CloseCallback close_cb) {
      assert(!(flags_ & (fClosing | fClosed)));

      flags_ |= fClosing;
      close_cb_ = close_cb;

      // CODIUS-TODO: Type-specific closing

      MakeClosePending();
    }

    void *data;

  private:

    enum {
      fClosing        = 0x0001,
      fClosed         = 0x0002,
      fHandleRef      = 0x2000,
      fHandleActive   = 0x4000,
      fHandleInternal = 0x8000
    };
    EventLoop* loop_;
    HandleType type_;
    unsigned int flags_;
    QUEUE handle_queue_;
    CloseCallback close_cb_;
    Handle* next_closing_;

    friend class EventLoop;
  };

  template <LoopWatcherHandleType type>
  class LoopWatcherHandle : public Handle {
  public:
    typedef void (*Callback)(LoopWatcherHandle* handle);

    inline void Init(EventLoop* loop) {
      Handle::Init(loop, static_cast<HandleType>(type));
      cb_ = NULL;
    }

    inline int Start(Callback cb) {
      if (IsActive()) return 0;
      assert(cb != NULL);

      Handle::Start();

      QUEUE_INSERT_HEAD(&loop_->loop_watcher_handles_[type], &queue_);
      cb_ = cb;
      return 0;
    }
  private:

    Callback cb_;
    QUEUE queue_;

    friend class EventLoop;
  };

  typedef LoopWatcherHandle<handleLoopWatcherIdle> IdleHandle;
  typedef LoopWatcherHandle<handleLoopWatcherCheck> CheckHandle;
  typedef LoopWatcherHandle<handleLoopWatcherPrepare> PrepareHandle;

  class TimerHandle : public Handle {
  public:
    typedef void (*Callback)(TimerHandle* handle);

    inline void Init(EventLoop* loop) {
      Handle::Init(loop, handleTypeTimer);
      cb_ = NULL;
      repeat_ = 0;
    }

    static int LessThan(const struct heap_node* ha,
                        const struct heap_node* hb);

    void Start(Callback cb, uint64_t timeout, uint64_t repeat);
    void Stop();

    void Again();

    inline void SetRepeat(uint64_t repeat) {
      repeat_ = repeat;
    }

    inline uint64_t GetRepeat() const {
      return repeat_;
    }

  private:

    Callback cb_;
    uint64_t timeout_;
    uint64_t repeat_;
    uint64_t start_id_;
    struct heap_node heap_node_;

    friend class EventLoop;
  };

  class PipeHandle : public Handle {
  public:
    typedef void (*Callback)(TimerHandle* handle);

    inline void Init(EventLoop* loop, bool ipc) {
      Handle::Init(loop, handleTypePipe);

      fd_ = -1;
    }

    inline void Open(int fd) {
      fd_ = fd;

      // TODO-CODIUS: Determine readable/writable flags
    }

    inline int GetFileDescriptor() const {
      return fd_;
    }
  private:
    int fd_;

    friend class EventLoop;
  };

  inline bool HasActiveHandles() const {
    return active_handles_ > 0;
  }

  inline bool HasActiveRequests() const {
    return false;
  }

  inline bool IsAlive() const {
    return HasActiveHandles() ||
      HasActiveRequests();
  }

  inline void UpdateTime() {
    timeval now;
    gettimeofday(&now, NULL);
    // Convert to milliseconds
    time_ = now.tv_sec * 1000 + now.tv_usec / 1000;
  }

  inline uint64_t Now() const {
    return time_;
  }

  inline void RunTimers() {
    struct heap_node* heap_node;
    TimerHandle* handle;

    for (;;) {
      heap_node = heap_min((struct heap*) &timer_heap_);
      if (heap_node == NULL)
        break;

      handle = ContainerOf(&TimerHandle::heap_node_, heap_node);
      if (handle->timeout_ > time_)
        break;

      handle->Stop();
      handle->Again();
      handle->cb_(handle);
    }
  }

  template<LoopWatcherHandleType type>
  inline void RunLoopWatcherHandles() {
    LoopWatcherHandle<type>* h;
    QUEUE* q;
    QUEUE_FOREACH(q, &loop_watcher_handles_[type]) {
      h = ContainerOf(&LoopWatcherHandle<type>::queue_, q);
      h->cb_(h);
    }
  }

  inline bool Run(RunModes mode) {
    bool r;

    r = IsAlive();
    if (!r)
      UpdateTime();

    while (r != 0 && stop_flag_ == 0) {
      UpdateTime();

      RunTimers();
//      RunPending();
      RunLoopWatcherHandles<handleLoopWatcherIdle>();
      RunLoopWatcherHandles<handleLoopWatcherPrepare>();

//      timeout = 0;
//      if ((mode & UV_RUN_NOWAIT) == 0)
//        timeout = uv_backend_timeout(loop);

//      uv__io_poll(loop, timeout);
      RunLoopWatcherHandles<handleLoopWatcherCheck>();
//
      if (mode == modeRunOnce) {
        /* modeRunOnce implies forward progess: at least one callback must have
         * been invoked when it returns. uv__io_poll() can return without doing
         * I/O (meaning: no callbacks) when its timeout expires - which means we
         * have pending timers that satisfy the forward progress constraint.
         *
         * modeRunNoWait makes no guarantees about progress so it's omitted from
         * the check.
         */
        UpdateTime();
        RunTimers();
      }

      r = IsAlive();

      if (mode & (modeRunOnce | modeRunNoWait))
        break;
    }

    // The if statement lets gcc compile it to a conditional store. Avoids
    // dirtying a cache line.
    if (stop_flag_ != 0)
      stop_flag_ = 0;
  }

private:
  EventLoop() {
    Init();
  };

  void Init();

  unsigned int active_handles_;
  QUEUE loop_watcher_handles_[MAX_LOOP_WATCHER_HANDLE_TYPE + 1];
  QUEUE handle_queue_;
  uint64_t time_;
  uint64_t timer_counter_;
  struct heap timer_heap_;
  unsigned int stop_flag_;
  Handle* closing_handles_;

  // Singleton. Don't allow copies.
  EventLoop(EventLoop const&);      // Do not implement
  void operator=(EventLoop const&); // Do not implement
};

}  // namespace node

#endif  // SRC_EVENT_LOOP_H_
