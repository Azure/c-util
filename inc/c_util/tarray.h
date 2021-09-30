// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TARRAY_H
#define TARRAY_H

#ifdef __cplusplus
#include <cinttypes>
#else
#include <inttypes.h>
#endif

#include "c_util/thandle.h"

#include "c_util/tarray_ll.h"

#include "umock_c/umock_c_prod.h"

#define TARRAY_CREATE_DECLARE(T) TARRAY_LL_CREATE_DECLARE(T, T)
#define TARRAY_CREATE_DEFINE(T) TARRAY_LL_CREATE_DEFINE(T, T)

#define TARRAY_ENSURE_CAPACITY_DECLARE(T) TARRAY_LL_ENSURE_CAPACITY_DECLARE(T, T)
#define TARRAY_ENSURE_CAPACITY_DEFINE(T) TARRAY_LL_ENSURE_CAPACITY_DEFINE(T, T)

#define TARRAY_FREE_DEFINE(T) TARRAY_LL_FREE_DEFINE(T, T)

#define TARRAY_CREATE(C) TARRAY_LL_CREATE(C)
#define TARRAY_ENSURE_CAPACITY(C) TARRAY_LL_ENSURE_CAPACITY(C)

/*macro to be used in headers*/                                                                                     \
#define TARRAY_TYPE_DECLARE(T)                                                                                      \
    TARRAY_DEFINE_STRUCT_TYPE(T)                                                                                    \
    THANDLE_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(T))                                                                    \
    TARRAY_CREATE_DECLARE(T)                                                                                        \
    TARRAY_ENSURE_CAPACITY_DECLARE(T)                                                                               \

#define TARRAY_TYPE_DEFINE(T)                                                                                       \
    THANDLE_TYPE_DEFINE(TARRAY_TYPEDEF_NAME(T))                                                                     \
    TARRAY_FREE_DEFINE(T)                                                                                           \
    TARRAY_CREATE_DEFINE(T)                                                                                         \
    TARRAY_ENSURE_CAPACITY_DEFINE(T)                                                                                \

#endif  /*TARRAY_H*/
