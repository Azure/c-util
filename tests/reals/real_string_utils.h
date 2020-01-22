// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_STRING_UTILS_H
#define REAL_STRING_UTILS_H

#ifdef __cplusplus
#include <cstdbool>
#else
#include <stdbool.h>
#endif

#include "azure_macro_utils/macro_utils.h"

#include "azure_c_util/srw_lock.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_STRING_UTILS_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        sprintf_char_function, \
        sprintf_wchar_function, \
        vsprintf_char, \
        vsprintf_wchar, \
        FILETIME_toAsciiArray, \
        mbs_to_wcs, \
        wcs_to_mbs \
)

#ifdef __cplusplus
extern "C" {
#endif


char* real_sprintf_char_function(const char* format, ...);

wchar_t* real_sprintf_wchar_function(const wchar_t* format, ...);

char* real_vsprintf_char(const char* format, va_list va);

wchar_t* real_vsprintf_wchar(const wchar_t* format, va_list va);

char* real_FILETIME_toAsciiArray(const FILETIME* fileTime);

wchar_t* real_mbs_to_wcs(const char* source);

char* real_wcs_to_mbs(const wchar_t* source);

#ifdef __cplusplus
}
#endif

#endif // REAL_STRING_UTILS_H
