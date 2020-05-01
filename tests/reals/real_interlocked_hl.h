// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_INTERLOCKED_HL_H
#define REAL_INTERLOCKED_HL_H

#include "azure_macro_utils/macro_utils.h"

#include "windows.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_INTERLOCKED_HL_GLOBAL_MOCK_HOOK()                    \
    MU_FOR_EACH_1(R2,                      \
        InterlockedHL_Add64WithCeiling, \
        InterlockedHL_WaitForValue, \
        InterlockedHL_WaitForNotValue, \
        InterlockedHL_SetAndWake, \
        InterlockedHL_CompareExchange64If \
    )

#include "azure_c_util/interlocked_hl.h"

#ifdef __cplusplus
extern "C" {
#endif

    INTERLOCKED_HL_RESULT real_InterlockedHL_Add64WithCeiling(LONGLONG volatile * Addend, LONGLONG Ceiling, LONGLONG Value, LONGLONG* originalAddend);
    INTERLOCKED_HL_RESULT real_InterlockedHL_WaitForValue(LONG volatile* address, LONG value, DWORD milliseconds);
    INTERLOCKED_HL_RESULT real_InterlockedHL_WaitForValue64(LONG64 volatile* address, LONG64 value, DWORD milliseconds);
    INTERLOCKED_HL_RESULT real_InterlockedHL_WaitForNotValue(LONG volatile* address, LONG value, DWORD milliseconds);
    INTERLOCKED_HL_RESULT real_InterlockedHL_SetAndWake(LONG volatile* address, LONG value);
    INTERLOCKED_HL_RESULT real_InterlockedHL_CompareExchange64If(LONG64 volatile* target, LONG64 exchange, INTERLOCKED_COMPARE_EXCHANGE_64_IF compare, LONG64* original_target);

#ifdef __cplusplus
}
#endif

#endif //REAL_INTERLOCKED_HL_H
