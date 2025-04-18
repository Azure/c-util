// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef RC_PTR_H
#define RC_PTR_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "c_pal/thandle.h"

typedef void (*RC_PTR_FREE_FUNC)(void* context, void* ptr);

typedef struct RC_PTR_TAG
{
    void* ptr;
    RC_PTR_FREE_FUNC free_func;
    void* free_func_context;
} RC_PTR;

THANDLE_TYPE_DECLARE(RC_PTR);

#define PRI_RC_PTR "p"

#define RC_PTR_VALUE(rc) ((rc)->ptr)

#define RC_PTR_OR_NULL(rc) (((rc) == NULL) ? NULL : (rc)->ptr)

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C"
{
#endif

    MOCKABLE_FUNCTION(, THANDLE(RC_PTR), rc_ptr_create_with_move_pointer, void*, ptr, RC_PTR_FREE_FUNC, free_func, void*, free_func_context);

#ifdef __cplusplus
}
#endif

#endif  /* RC_PTR_H */
