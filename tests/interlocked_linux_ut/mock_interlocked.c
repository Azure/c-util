// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "mock_interlocked.h"

#undef atomic_exchange
#define atomic_exchange mock_atomic_exchange


#define atomic_compare_exchange_strong mock_atomic_compare_exchange_strong
#define atomic_fetch_add mock_atomic_fetch_add
#define atomic_fetch_sub mock_atomic_fetch_sub
#define atomic_fetch_or mock_atomic_fetch_or
#define atomic_fetch_xor mock_atomic_fetch_xor
#define atomic_fetch_and mock_atomic_fetch_and

#define mock_atomic_exchange(X, Y) _Generic((Y), \
    int64_t: mock_atomic_exchange_64, \
    int32_t: mock_atomic_exchange_32, \ 
    int16_t: mock_atomic_exchange_16 \
)(X, Y)

#include "../../adapters/interlocked_linux.c"
