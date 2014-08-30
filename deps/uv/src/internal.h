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

#ifndef UV_UNIX_INTERNAL_H_
#define UV_UNIX_INTERNAL_H_

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

void uv__io_init(uv__io_t* w, uv__io_cb cb, int fd);
void uv__io_start(uv_loop_t* loop, uv__io_t* w, unsigned int events);
void uv__io_stop(uv_loop_t* loop, uv__io_t* w, unsigned int events);
void uv__io_close(uv_loop_t* loop, uv__io_t* w);
void uv__io_feed(uv_loop_t* loop, uv__io_t* w);
int uv__io_active(const uv__io_t* w, unsigned int events);
void uv__io_poll(uv_loop_t* loop, int timeout); /* in milliseconds or -1 */

#endif /* UV_UNIX_INTERNAL_H_ */