// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_HASH_H
#define REAL_HASH_H

#include "macro_utils/macro_utils.h"
#include "MurmurHash2.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_HASH_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        MurmurHash2 \
        MurmurHash64A \
        MurmurHash64B \
        MurmurHash2A \
        MurmurHashNeutral2 \
        MurmurHashAligned2 \
    )

#ifdef __cplusplus
extern "C" {
#endif

uint32_t real_MurmurHash2        ( const void * key, int len, uint32_t seed );
uint64_t real_MurmurHash64A      ( const void * key, int len, uint64_t seed );
uint64_t real_MurmurHash64B      ( const void * key, int len, uint64_t seed );
uint32_t real_MurmurHash2A       ( const void * key, int len, uint32_t seed );
uint32_t real_MurmurHashNeutral2 ( const void * key, int len, uint32_t seed );
uint32_t real_MurmurHashAligned2 ( const void * key, int len, uint32_t seed );

#ifdef __cplusplus
}
#endif

#endif // REAL_HASH_H
