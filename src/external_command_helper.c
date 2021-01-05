// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/external_command_helper.h"

MU_DEFINE_ENUM_STRINGS(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_RESULT_VALUES);

IMPLEMENT_MOCKABLE_FUNCTION(, EXTERNAL_COMMAND_RESULT, external_command_helper_execute, const char*, command, RC_STRING_ARRAY**, lines, int*, return_code)
{
    EXTERNAL_COMMAND_RESULT result;

    (void)command;
    (void)lines;
    (void)return_code;
    result = EXTERNAL_COMMAND_ERROR;

    return result;
}
