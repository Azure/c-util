// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_UUID_H
#define REAL_UUID_H

#include "macro_utils/macro_utils.h"
#include "c_util/uuid.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_UUID_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        UUID_generate, \
        UUID_from_string, \
        UUID_to_string \
    )

#ifdef __cplusplus
extern "C" {
#endif

int real_UUID_generate(UUID_T* uuid);
int real_UUID_from_string(const char* uuid_string, UUID_T* uuid);
char* real_UUID_to_string(const UUID_T* uuid);

#ifdef __cplusplus
}
#endif

#endif // REAL_UUID_H
