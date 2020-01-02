// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file lock.h
*    @brief        A minimalistic platform agnostic lock abstraction for thread
*                synchronization.
*    @details    The Lock component is implemented in order to achieve thread
*                synchronization, as we may have a requirement to consume locks
*                across different platforms. This component exposes some generic
*                APIs so that it can be extended for platform specific
*                implementations.
*/

#ifndef LOCK_H
#define LOCK_H

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* LOCK_HANDLE;

#define LOCK_RESULT_VALUES \
    LOCK_OK, \
    LOCK_ERROR \

MU_DEFINE_ENUM(LOCK_RESULT, LOCK_RESULT_VALUES);

MOCKABLE_FUNCTION(, LOCK_HANDLE, Lock_Init);
MOCKABLE_FUNCTION(, LOCK_RESULT, Lock, LOCK_HANDLE, handle);
MOCKABLE_FUNCTION(, LOCK_RESULT, Unlock, LOCK_HANDLE, handle);
MOCKABLE_FUNCTION(, LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, handle);

#ifdef __cplusplus
}
#endif

#endif /* LOCK_H */
