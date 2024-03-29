// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef UUID_STRING_H
#define UUID_STRING_H

#include "macro_utils/macro_utils.h"

#include "c_pal/uuid.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

#define UUID_FROM_STRING_RESULT_VALUES \
    UUID_FROM_STRING_RESULT_OK, \
    UUID_FROM_STRING_RESULT_INVALID_DATA, \
    UUID_FROM_STRING_RESULT_INVALID_ARG

MU_DEFINE_ENUM(UUID_FROM_STRING_RESULT, UUID_FROM_STRING_RESULT_VALUES)

#define UUID_T_STRING_LENGTH 36 /*all UUID_T have 36 characters when stringified (not counting a '\0' terminator)*/

extern const UUID_T NIL_UUID;

MOCKABLE_FUNCTION(, UUID_FROM_STRING_RESULT, uuid_from_string, const char*, uuid_string, UUID_T, uuid);

MOCKABLE_FUNCTION(, char*, uuid_to_string, const UUID_T, uuid);

#ifdef __cplusplus
}
#endif

#endif /* UUID_STRING_H */
