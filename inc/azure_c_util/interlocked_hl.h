// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef INTERLOCKED_HL_H
#define INTERLOCKED_HL_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

#include "azure_macro_utils/macro_utils.h"
#include "azure_c_pal/interlocked.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

#define INTERLOCKED_HL_RESULT_VALUES \
    INTERLOCKED_HL_OK, \
    INTERLOCKED_HL_ERROR, \
    INTERLOCKED_HL_CHANGED

MU_DEFINE_ENUM(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

typedef bool (*INTERLOCKED_COMPARE_EXCHANGE_64_IF)(int64_t target, int64_t exchange);

MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_Add64WithCeiling, int64_t volatile_atomic*, Addend, int64_t, Ceiling, int64_t, Value, int64_t*, originalAddend)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWake, int32_t volatile_atomic*, address, int32_t, value)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWakeAll, int32_t volatile_atomic*, address, int32_t, value)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForValue, int32_t volatile_atomic*, address, int32_t, value, uint32_t, milliseconds)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForNotValue, int32_t volatile_atomic*, address, int32_t, value, uint32_t, milliseconds)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_CompareExchange64If, int64_t volatile_atomic*, target, int64_t, exchange, INTERLOCKED_COMPARE_EXCHANGE_64_IF, compare, int64_t*, original_target)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);

#ifdef __cplusplus
}
#endif

#endif // INTERLOCKED_HL_H
