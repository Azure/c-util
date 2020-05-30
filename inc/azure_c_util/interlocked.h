// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef INTERLOCKED_H
#define INTERLOCKED_H

#ifdef __cplusplus
#include <cstdint>

#else
#include <stdint.h>
#include <stdbool.h>
#endif

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define volatile_atomic volatile
#else
#define volatile_atomic volatile _Atomic
#endif

MOCKABLE_FUNCTION(, int32_t, interlocked_add, volatile_atomic int32_t*, addend, int32_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_add_64, volatile_atomic int64_t*, addend, int64_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_and, volatile_atomic int32_t*, destination, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_and_16, volatile_atomic int16_t*, destination, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_and_64, volatile_atomic int64_t*, destination, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_and_8, volatile_atomic int8_t*, destination, int8_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_compare_exchange, volatile_atomic int32_t*, destination, int32_t, exchange, int32_t, comperand);
#ifdef _WIN64
MOCKABLE_FUNCTION(, bool, interlocked_compare_exchange_128, volatile_atomic int64_t*, destination, int64_t, exchange_high, int64_t, exchange_low, int64_t*, comperand_result);
#endif
MOCKABLE_FUNCTION(, int16_t, interlocked_compare_exchange_16, volatile_atomic int16_t*, destination, int16_t, exchange, int16_t, comperand);
MOCKABLE_FUNCTION(, int64_t, interlocked_compare_exchange_64, volatile_atomic int64_t*, destination, int64_t, exchange, int64_t, comperand);
MOCKABLE_FUNCTION(, void*, interlocked_compare_exchange_pointer, void* volatile_atomic*, destination, void*, exchange, void*, comperand);
MOCKABLE_FUNCTION(, int32_t, interlocked_decrement, volatile_atomic int32_t*, addend);
MOCKABLE_FUNCTION(, int16_t, interlocked_decrement_16, volatile_atomic int16_t*, addend);
MOCKABLE_FUNCTION(, int64_t, interlocked_decrement_64, volatile_atomic int64_t*, addend);
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange, volatile_atomic int32_t*, target, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_exchange_16, volatile_atomic int16_t*, target, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_64, volatile_atomic int64_t*, target, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_exchange_8, volatile_atomic int8_t*, target, int8_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange_add, volatile_atomic int32_t*, addend, int32_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_add_64, volatile_atomic int64_t*, addend, int64_t, value);
MOCKABLE_FUNCTION(, void*, interlocked_exchange_pointer, void* volatile_atomic*, target, void*, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_increment, volatile_atomic int32_t*, addend);
MOCKABLE_FUNCTION(, int16_t, interlocked_increment_16, volatile_atomic int16_t*, addend);
MOCKABLE_FUNCTION(, int64_t, interlocked_increment_64, volatile_atomic int64_t*, addend);
MOCKABLE_FUNCTION(, int32_t, interlocked_or, volatile_atomic int32_t*, destination, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_or_16, volatile_atomic int16_t*, destination, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_or_64, volatile_atomic int64_t*, destination, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_or_8, volatile_atomic int8_t*, destination, int8_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_xor, volatile_atomic int32_t*, destination, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_xor_16, volatile_atomic int16_t*, destination, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_xor_64, volatile_atomic int64_t*, destination, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_xor_8, volatile_atomic int8_t*, destination, int8_t, value);

#ifdef __cplusplus
}
#endif
#endif
