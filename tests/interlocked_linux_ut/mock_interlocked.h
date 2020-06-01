// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef MOCK_INTERLOCKED_H
#define MOCK_INTERLOCKED_H

#ifdef __cplusplus
#include <cstdint>

#else
#include <stdint.h>
#include <stdbool.h>
#endif

#include "umock_c/umock_c_prod.h"
#include "../../inc/azure_c_util/interlocked.h"

#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, int16_t, mock_atomic_fetch_add_16, volatile_atomic int16_t*, object, int16_t, operand);
MOCKABLE_FUNCTION(, int32_t, mock_atomic_fetch_add_32, volatile_atomic int32_t*, object, int32_t, operand);
MOCKABLE_FUNCTION(, int64_t, mock_atomic_fetch_add_64, volatile_atomic int64_t*, object, int64_t, operand);
MOCKABLE_FUNCTION(, int8_t, mock_atomic_fetch_and_8, volatile_atomic int8_t*, object, int8_t, operand);
MOCKABLE_FUNCTION(, int16_t, mock_atomic_fetch_and_16, volatile_atomic int16_t*, object, int16_t, operand);
MOCKABLE_FUNCTION(, int32_t, mock_atomic_fetch_and_32, volatile_atomic int32_t*, object, int32_t, operand);
MOCKABLE_FUNCTION(, int64_t, mock_atomic_fetch_and_64, volatile_atomic int64_t*, object, int64_t, operand);
MOCKABLE_FUNCTION(, bool, mock_atomic_compare_exchange_16, volatile_atomic int16_t*, object, int16_t*, expected, int16_t, desired);
MOCKABLE_FUNCTION(, bool, mock_atomic_compare_exchange_32, volatile_atomic int32_t*, object, int32_t*, expected, int32_t, desired);
MOCKABLE_FUNCTION(, bool, mock_atomic_compare_exchange_64, volatile_atomic int64_t*, object, int64_t*, expected, int64_t, desired);
MOCKABLE_FUNCTION(, bool, mock_atomic_compare_exchange_pointer, void* volatile_atomic*, object, void**, expected, void*, desired);
MOCKABLE_FUNCTION(, int16_t, mock_atomic_fetch_sub_16, volatile_atomic int16_t*, object, int16_t, operand);
MOCKABLE_FUNCTION(, int32_t, mock_atomic_fetch_sub_32, volatile_atomic int32_t*, object, int32_t, operand);
MOCKABLE_FUNCTION(, int64_t, mock_atomic_fetch_sub_64, volatile_atomic int64_t*, object, int64_t, operand);
MOCKABLE_FUNCTION(, int16_t, mock_atomic_exchange_16, volatile_atomic int16_t*, object, int16_t, desired);
MOCKABLE_FUNCTION(, int8_t, mock_atomic_exchange_8, volatile_atomic int8_t*, object, int8_t, desired);
MOCKABLE_FUNCTION(, int32_t, mock_atomic_exchange_32, volatile_atomic int32_t*, object, int32_t, desired);
MOCKABLE_FUNCTION(, int64_t, mock_atomic_exchange_64, volatile_atomic int64_t*, object, int64_t, desired);
MOCKABLE_FUNCTION(, void*, mock_atomic_exchange_pointer, void* volatile_atomic*, object, void*, desired);
MOCKABLE_FUNCTION(, int8_t, mock_atomic_fetch_or_8, volatile_atomic int8_t*, object, int8_t, operand);
MOCKABLE_FUNCTION(, int16_t, mock_atomic_fetch_or_16, volatile_atomic int16_t*, object, int16_t, operand);
MOCKABLE_FUNCTION(, int32_t, mock_atomic_fetch_or_32, volatile_atomic int32_t*, object, int32_t, operand);
MOCKABLE_FUNCTION(, int64_t, mock_atomic_fetch_or_64, volatile_atomic int64_t*, object, int64_t, operand);
MOCKABLE_FUNCTION(, int8_t, mock_atomic_fetch_xor_8, volatile_atomic int8_t*, object, int8_t, operand);
MOCKABLE_FUNCTION(, int16_t, mock_atomic_fetch_xor_16, volatile_atomic int16_t*, object, int16_t, operand);
MOCKABLE_FUNCTION(, int32_t, mock_atomic_fetch_xor_32, volatile_atomic int32_t*, object, int32_t, operand);
MOCKABLE_FUNCTION(, int64_t, mock_atomic_fetch_xor_64, volatile_atomic int64_t*, object, int64_t, operand);
MOCKABLE_FUNCTION(, int16_t, mock_atomic_load_16, volatile_atomic int16_t*, object);
MOCKABLE_FUNCTION(, int32_t, mock_atomic_load_32, volatile_atomic int32_t*, object);
MOCKABLE_FUNCTION(, int64_t, mock_atomic_load_64, volatile_atomic int64_t*, object);
MOCKABLE_FUNCTION(, void*, mock_atomic_load_pointer, void* volatile_atomic*, object);
#ifdef __cplusplus
}
#endif
#endif