// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_THREADAPI_H
#define REAL_THREADAPI_H

#include "azure_macro_utils/macro_utils.h"

#include "azure_c_util/threadapi.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_THREADAPI_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        ThreadAPI_Create, \
        ThreadAPI_Join, \
        ThreadAPI_Exit, \
        ThreadAPI_Sleep \
)

#ifdef __cplusplus
extern "C" {
#endif

THREADAPI_RESULT real_ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg);

THREADAPI_RESULT real_ThreadAPI_Join(THREAD_HANDLE threadHandle, int* res);

void real_ThreadAPI_Exit(int res);

void real_ThreadAPI_Sleep(unsigned int milliseconds);

#ifdef __cplusplus
}
#endif

#endif // REAL_THREADAPI_H
