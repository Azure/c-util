// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef RC_STRING_ARRAY_H
#define RC_STRING_ARRAY_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_util/thandle.h"
#include "c_util/rc_string.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RC_STRING_ARRAY_TAG
{
    const uint32_t count;
    THANDLE(RC_STRING)* string_array;
} RC_STRING_ARRAY;

MOCKABLE_FUNCTION(, RC_STRING_ARRAY*, rc_string_array_create, uint32_t, count);
MOCKABLE_FUNCTION(, void, rc_string_array_destroy, RC_STRING_ARRAY*, rc_string_array);

#ifdef __cplusplus
}
#endif

#endif // RC_STRING_ARRAY_H
