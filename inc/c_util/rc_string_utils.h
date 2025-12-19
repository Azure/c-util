// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef RC_STRING_UTILS_H
#define RC_STRING_UTILS_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"
#include "c_util/rc_string.h"
#include "c_util/rc_string_array.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, RC_STRING_ARRAY*, rc_string_utils_split_by_char, THANDLE(RC_STRING), str, char, delimiter);

#ifdef __cplusplus
}
#endif

#endif // RC_STRING_UTILS_H
