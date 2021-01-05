// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef EXTERNAL_COMMAND_HELPER_H
#define EXTERNAL_COMMAND_HELPER_H

#include "c_util/rc_string_array.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

#define EXTERNAL_COMMAND_RESULT_VALUES \
        EXTERNAL_COMMAND_OK, \
        EXTERNAL_COMMAND_INVALID_ARGS, \
        EXTERNAL_COMMAND_ERROR

MU_DEFINE_ENUM(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_RESULT_VALUES)

MOCKABLE_FUNCTION(, EXTERNAL_COMMAND_RESULT, external_command_helper_execute, const char*, command, RC_STRING_ARRAY**, lines, int*, return_code);

#ifdef __cplusplus
}
#endif

#endif // EXTERNAL_COMMAND_HELPER_H
