//------------------------------------------------------------------------------
/*
    This file is part of Codius: https://github.com/codius
    Copyright (c) 2014 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include "uv.h"
#include "internal.h"
#include <stdio.h>
#include <string.h>

void uv__io_poll(uv_loop_t* loop, int timeout) {
  uv__io_t* watcher;

  if (loop->nfds == 0) {
    assert(QUEUE_EMPTY(&loop->watcher_queue));
    return;
  }

  // QUEUE_FOREACH is unsafe if the the callback removes the watcher from the
  // queue. So instead we do this.
  assert(!QUEUE_EMPTY(&loop->watcher_queue));
  QUEUE queue;
  QUEUE* q = QUEUE_HEAD(&loop->watcher_queue);
  QUEUE_SPLIT(&loop->watcher_queue, q, &queue);
  while (!QUEUE_EMPTY(&queue)) {
    q = QUEUE_HEAD(&queue);
    QUEUE_REMOVE(q);
    QUEUE_INSERT_TAIL(&loop->watcher_queue, q);

    watcher = QUEUE_DATA(q, uv__io_t, watcher_queue);

    assert(watcher->fd >= 0);
    assert(watcher->fd < (int) loop->nwatchers);

    if (watcher->pevents & (UV__POLLIN | UV__POLLOUT)) {
      watcher->cb(loop, watcher, watcher->pevents & (UV__POLLIN | UV__POLLOUT));
    }
  }
}
