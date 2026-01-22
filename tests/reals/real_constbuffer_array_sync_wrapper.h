// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_CONSTBUFFER_ARRAY_SYNC_WRAPPER_H
#define REAL_CONSTBUFFER_ARRAY_SYNC_WRAPPER_H

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_CONSTBUFFER_ARRAY_SYNC_WRAPPER_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        async_type_helper_CONSTBUFFER_ARRAY_HANDLE_copy, \
        async_type_helper_CONSTBUFFER_ARRAY_HANDLE_free \
)

#include "c_util/constbuffer_array.h"

int real_async_type_helper_CONSTBUFFER_ARRAY_HANDLE_copy(CONSTBUFFER_ARRAY_HANDLE* dst, CONSTBUFFER_ARRAY_HANDLE src);
void real_async_type_helper_CONSTBUFFER_ARRAY_HANDLE_free(CONSTBUFFER_ARRAY_HANDLE arg);

#endif // REAL_CONSTBUFFER_ARRAY_SYNC_WRAPPER_H
