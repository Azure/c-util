// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_SRW_LOCK_H
#define REAL_SRW_LOCK_H

#ifdef __cplusplus
#include <cstdbool>
#else
#include <stdbool.h>
#endif

#include "azure_macro_utils/macro_utils.h"

#include "azure_c_util/srw_lock.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_SRW_LOCK_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        srw_lock_create, \
        srw_lock_destroy, \
        srw_lock_acquire_exclusive, \
        srw_lock_release_exclusive, \
        srw_lock_acquire_shared, \
        srw_lock_release_shared \
)

#ifdef __cplusplus
extern "C" {
#endif


SRW_LOCK_HANDLE real_srw_lock_create(bool do_statistics, const char* lock_name);

void real_srw_lock_destroy(SRW_LOCK_HANDLE handle);

void real_srw_lock_acquire_exclusive(SRW_LOCK_HANDLE handle);

void real_srw_lock_release_exclusive(SRW_LOCK_HANDLE handle);

void real_srw_lock_acquire_shared(SRW_LOCK_HANDLE handle);

void real_srw_lock_release_shared(SRW_LOCK_HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif // REAL_SRW_LOCK_H
