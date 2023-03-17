// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_HASH_H
#define REAL_HASH_H

#include "macro_utils/macro_utils.h"
#include "c_util/hash.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_HASH_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        hash_compute_hash \
    )

#ifdef __cplusplus
extern "C" {
#endif

int real_hash_compute_hash(const void* buffer, size_t length, uint32_t* hash);

#ifdef __cplusplus
}
#endif

#endif // REAL_HASH_H
