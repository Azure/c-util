// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdatomic.h>
#include "mock_interlocked.h"

#undef atomic_fetch_add
#define atomic_fetch_add mock_atomic_fetch_add
#define mock_atomic_fetch_add(X, Y) _Generic((X), \
    volatile_atomic int16_t*: mock_atomic_fetch_add_16, \
    volatile_atomic int32_t*: mock_atomic_fetch_add_32, \
    volatile_atomic int64_t*: mock_atomic_fetch_add_64 \
)(X, Y)

#undef atomic_fetch_and
#define atomic_fetch_and mock_atomic_fetch_and
#define mock_atomic_fetch_and(X, Y) _Generic((X), \
    volatile_atomic int8_t*: mock_atomic_fetch_and_8, \
    volatile_atomic int16_t*: mock_atomic_fetch_and_16, \
    volatile_atomic int32_t*: mock_atomic_fetch_and_32, \
    volatile_atomic int64_t*: mock_atomic_fetch_and_64 \
)(X, Y)

#undef atomic_compare_exchange_strong
#define atomic_compare_exchange_strong mock_atomic_compare_exchange_strong
#define mock_atomic_compare_exchange_strong(X, Y, Z) _Generic((X), \
    volatile_atomic int16_t*: mock_atomic_compare_exchange_16, \
    volatile_atomic int32_t*: mock_atomic_compare_exchange_32, \
    volatile_atomic int64_t*: mock_atomic_compare_exchange_64, \
    void* volatile_atomic*: mock_atomic_compare_exchange_pointer \
)(X, Y, Z)

#undef atomic_fetch_sub
#define atomic_fetch_sub mock_atomic_fetch_sub
#define mock_atomic_fetch_sub(X, Y) _Generic((X), \
    volatile_atomic int16_t*: mock_atomic_fetch_sub_16, \
    volatile_atomic int32_t*: mock_atomic_fetch_sub_32, \
    volatile_atomic int64_t*: mock_atomic_fetch_sub_64 \
)(X, Y)

#undef atomic_exchange
#define atomic_exchange mock_atomic_exchange
#define mock_atomic_exchange(X, Y) _Generic((X), \
    volatile_atomic int8_t*: mock_atomic_exchange_8, \
    volatile_atomic int16_t*: mock_atomic_exchange_16, \
    volatile_atomic int32_t*: mock_atomic_exchange_32, \
    volatile_atomic int64_t*: mock_atomic_exchange_64, \
    void* volatile_atomic*: mock_atomic_exchange_pointer \
)(X, Y)

#undef atomic_fetch_or
#define atomic_fetch_or mock_atomic_fetch_or
#define mock_atomic_fetch_or(X, Y) _Generic((X), \
    volatile_atomic int8_t*: mock_atomic_fetch_or_8, \
    volatile_atomic int16_t*: mock_atomic_fetch_or_16, \
    volatile_atomic int32_t*: mock_atomic_fetch_or_32, \
    volatile_atomic int64_t*: mock_atomic_fetch_or_64 \
)(X, Y)


#undef atomic_fetch_xor
#define atomic_fetch_xor mock_atomic_fetch_xor
#define mock_atomic_fetch_xor(X, Y) _Generic((X), \
    volatile_atomic int8_t*: mock_atomic_fetch_xor_8, \
    volatile_atomic int16_t*: mock_atomic_fetch_xor_16, \
    volatile_atomic int32_t*: mock_atomic_fetch_xor_32, \
    volatile_atomic int64_t*: mock_atomic_fetch_xor_64 \
)(X, Y)

#undef atomic_load
#define atomic_load mock_atomic_load
#define mock_atomic_load(X) _Generic((X), \
    volatile_atomic int16_t*: mock_atomic_load_16, \
    volatile_atomic int32_t*: mock_atomic_load_32, \
    volatile_atomic int64_t*: mock_atomic_load_64, \
    void* volatile_atomic*: mock_atomic_load_pointer \
)(X)

#include "../../adapters/interlocked_linux.c"
