// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef INTERLOCKED_HL_H
#define INTERLOCKED_HL_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include "windows.h"

#include "azure_macro_utils/macro_utils.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

#define INTERLOCKED_HL_RESULT_VALUES \
    INTERLOCKED_HL_OK, \
    INTERLOCKED_HL_ERROR, \
    INTERLOCKED_HL_CHANGED

MU_DEFINE_ENUM(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

typedef bool (*INTERLOCKED_COMPARE_EXCHANGE_64_IF)(LONG64 target, LONG64 exchange);

MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_Add64WithCeiling, LONG64 volatile*, Addend, LONG64, Ceiling, LONG64, Value, LONG64*, originalAddend)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWake, LONG volatile*, address, LONG, value)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForValue, LONG volatile*, address, LONG, value, DWORD, milliseconds)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForValue64, LONG64 volatile*, address, LONG64, value, DWORD, milliseconds)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForNotValue, LONG volatile*, address, LONG, value, DWORD, milliseconds)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);
MOCKABLE_FUNCTION_WITH_RETURNS(, INTERLOCKED_HL_RESULT, InterlockedHL_CompareExchange64If, LONG64 volatile*, target, LONG64, exchange, INTERLOCKED_COMPARE_EXCHANGE_64_IF, compare, LONG64 *, original_target)(INTERLOCKED_HL_OK, INTERLOCKED_HL_ERROR);

#ifdef __cplusplus
}
#endif

#endif // INTERLOCKED_HL_H
