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

#include <stdio.h>
#include <unistd.h>

#include "codius-util.h"

typedef struct codius_rpc_header_s codius_rpc_header_t;

struct codius_rpc_header_s {
  unsigned long magic_bytes;
  unsigned long callback_id;
  unsigned long size;
};


const unsigned long CODIUS_MAGIC_BYTES = 0xC0D105FE;


/* Make synchronous function call outside the sandbox.
   Return response_len or -1 for error. */
int codius_sync_call(const char* request_buf, size_t request_len,
                     char **response_buf, size_t *response_len) {
  size_t bytes_read;
  const int sync_fd = 3;
  int resp_len;

  codius_rpc_header_t rpc_header;
  rpc_header.magic_bytes = CODIUS_MAGIC_BYTES;
  rpc_header.callback_id = 0;
  rpc_header.size = request_len;
  
  if (-1==write(sync_fd, &rpc_header, sizeof(rpc_header)) ||
      -1==write(sync_fd, request_buf, request_len)) {
    perror("write()");
    printf("Error writing to fd %d\n", sync_fd);
    return -1;
  }
  
  bytes_read = read(sync_fd, &rpc_header, sizeof(rpc_header));
  if (bytes_read==-1 || rpc_header.magic_bytes!=CODIUS_MAGIC_BYTES) {
    printf("Error reading from fd %d\n", sync_fd);
    return -1;
  }
  
  if (rpc_header.size > CODIUS_MAX_RESPONSE_SIZE) {
    printf("Message too large from fd %d\n", sync_fd);
    abort();
  }
  *response_len = rpc_header.size;  
  
  *response_buf = (char*) malloc(*response_len);
  
  bytes_read = read(sync_fd, *response_buf, *response_len);
  if (bytes_read==-1) {
    perror("read()");
    printf("Error reading from fd %d\n", sync_fd);
    fflush(stdout);

    return -1;
  }

  return *response_len;
}