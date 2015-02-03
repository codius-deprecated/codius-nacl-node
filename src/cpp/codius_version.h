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

#ifndef SRC_CODIUS_VERSION_H_
#define SRC_CODIUS_VERSION_H_

#define CODIUS_MAJOR_VERSION 0
#define CODIUS_MINOR_VERSION 1
#define CODIUS_PATCH_VERSION 0

#ifndef CODIUS_TAG
# define CODIUS_TAG ""
#endif

#ifndef CODIUS_STRINGIFY
#define CODIUS_STRINGIFY(n) CODIUS_STRINGIFY_HELPER(n)
#define CODIUS_STRINGIFY_HELPER(n) #n
#endif

#if CODIUS_VERSION_IS_RELEASE
# define CODIUS_VERSION_STRING  CODIUS_STRINGIFY(CODIUS_MAJOR_VERSION) "." \
                                CODIUS_STRINGIFY(CODIUS_MINOR_VERSION) "." \
                                CODIUS_STRINGIFY(CODIUS_PATCH_VERSION)     \
                                CODIUS_TAG
#else
# define CODIUS_VERSION_STRING  CODIUS_STRINGIFY(CODIUS_MAJOR_VERSION) "." \
                                CODIUS_STRINGIFY(CODIUS_MINOR_VERSION) "." \
                                CODIUS_STRINGIFY(CODIUS_PATCH_VERSION)     \
                                CODIUS_TAG "-pre"
#endif

#define CODIUS_VERSION "v" CODIUS_VERSION_STRING


#define CODIUS_VERSION_AT_LEAST(major, minor, patch) \
  (( (major) < CODIUS_MAJOR_VERSION) \
  || ((major) == CODIUS_MAJOR_VERSION && (minor) < CODIUS_MINOR_VERSION) \
  || ((major) == CODIUS_MAJOR_VERSION && \
      (minor) == CODIUS_MINOR_VERSION && (patch) <= CODIUS_PATCH_VERSION))

#endif  /* SRC_CODIUS_VERSION_H_ */
