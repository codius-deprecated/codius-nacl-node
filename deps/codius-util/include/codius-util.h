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

#ifndef __CODIUS_UTIL_H_
#define __CODIUS_UTIL_H_

#include "jsmn.h"
#include <time.h>

// 129 KB
#define CODIUS_MAX_MESSAGE_SIZE 132096
// 256 MB
#define CODIUS_MAX_RESPONSE_SIZE 268435456

#ifdef __cplusplus
extern "C" {
#endif

int codius_sync_call(const char* request_buf, size_t request_len,
                     char **response_buf, size_t *response_len);

/**
 * Get the type of the result field.
 */
jsmntype_t codius_parse_json_type(char *js, size_t len, const char *field_name);

/**
 * Get the integer that is present from the result field.
 */
int codius_parse_json_int(char *js, size_t len, const char *field_name);

/**
 * Get the string that is present in the result field.
 */
int codius_parse_json_str(char *js, size_t len, const char *field_name, char *buf, size_t buf_size);

/**
 * Get the struct tm that is present in the result field.
 */
int codius_parse_json_tm(char *js, size_t len, const char *field_name, struct tm *t);

#ifdef __cplusplus
}
#endif

#endif /* __CODIUS_UTIL_H_ */