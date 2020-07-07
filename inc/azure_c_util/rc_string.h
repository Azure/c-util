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
#include "azure_c_util/thandle.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct RC_STRING_TAG
    {
        const char* string;
    } RC_STRING;

    THANDLE_TYPE_DECLARE(RC_STRING);

    MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create, const char*, string);

#ifdef __cplusplus
}
#endif

#endif  /* RC_STRING_H */
