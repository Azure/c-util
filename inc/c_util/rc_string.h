// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef RC_STRING_H
#define RC_STRING_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "umock_c/umock_c_prod.h"
#include "c_util/thandle.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct RC_STRING_TAG
    {
        const char* string;
    } RC_STRING;

    THANDLE_TYPE_DECLARE(RC_STRING);

    typedef void (*RC_STRING_FREE_FUNC)(void* context);

    #define PRI_RC_STRING "s"
    #define RC_STRING_VALUE(rc) (((rc) == NULL) ? "NULL" : MU_P_OR_NULL(rc->string))

    MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create, const char*, string);
    MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create_with_move_memory, const char*, string);
    MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create_with_custom_free, const char*, string, RC_STRING_FREE_FUNC, free_func, void*, free_func_context);
    MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_recreate, THANDLE(RC_STRING), self);

#ifdef __cplusplus
}
#endif

#endif  /* RC_STRING_H */
