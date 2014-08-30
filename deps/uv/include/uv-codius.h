#ifndef UV_LINUX_H
#define UV_LINUX_H

#define UV_PLATFORM_LOOP_FIELDS \
  void *async_callbacks; \
  uv__io_t codius_async_watcher;

#endif /* UV_LINUX_H */