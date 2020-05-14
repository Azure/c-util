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



MOCKABLE_FUNCTION(, int32_t, interlocked_add, volatile int32_t*, addend, int32_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_and, volatile int32_t*, destination, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_and_16, volatile int16_t*, destination, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_and_64, volatile int64_t*, destination, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_and_8, volatile int8_t*, destination, int8_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_compare_exchange, volatile int32_t*, destination, int32_t, exchange, int32_t, comperand);
MOCKABLE_FUNCTION(, bool, interlocked_compare_exchange_128, volatile int64_t*, destination, int64_t, exchange_high, int64_t, exchange_low, int64_t*, comperand_result);
MOCKABLE_FUNCTION(, int16_t, interlocked_compare_exchange_16, volatile int16_t*, destination, int16_t, exchange, int16_t, comperand);
MOCKABLE_FUNCTION(, int64_t, interlocked_compare_exchange_64, volatile int64_t*, destination, int64_t, exchange, int64_t, comperand);
MOCKABLE_FUNCTION(, void*, interlocked_compare_exchange_pointer, volatile void**, destination, void*, exchange, void*, comperand);
MOCKABLE_FUNCTION(, int32_t, interlocked_decrement, volatile int32_t*, addend);
MOCKABLE_FUNCTION(, int16_t, interlocked_decrement_16, volatile int16_t*, addend);
MOCKABLE_FUNCTION(, int64_t, interlocked_decrement_64, volatile int64_t*, addend);
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange, volatile int32_t*, target, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_exchange_16, volatile int16_t*, target, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_64, volatile int64_t*, target, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_exchange_8, volatile int8_t*, target, int8_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange_add, volatile int32_t*, addend, int32_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_add_64, volatile int64_t*, addend, int64_t, value);
MOCKABLE_FUNCTION(, void*, interlocked_exchange_pointer, volatile void**, target, void*, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_increment, volatile int32_t*, addend);
MOCKABLE_FUNCTION(, int16_t, interlocked_increment_16, volatile int16_t*, addend);
MOCKABLE_FUNCTION(, int64_t, interlocked_increment_64, volatile int64_t*, addend);
MOCKABLE_FUNCTION(, int32_t, interlocked_or, volatile int32_t*, destination, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_or_16, volatile int16_t*, destination, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_or_64, volatile int64_t*, destination, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_or_8, volatile int8_t*, destination, int8_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_xor, volatile int32_t*, destination, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_xor_16, volatile int16_t*, destination, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_xor_64, volatile int64_t*, destination, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_xor_8, volatile int8_t*, destination, int8_t, value);

#ifdef __cplusplus
}
#endif
#endif
