// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <limits.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/thandle.h"

#include "thandle_user_33_characters.h"

#define LL3456789012345678901234567890123_FIELDS           \
    int, a                  \

MU_DEFINE_STRUCT(LL3456789012345678901234567890123, LL3456789012345678901234567890123_FIELDS);

THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(LL3456789012345678901234567890123, gballoc_hl_malloc, gballoc_hl_malloc_flex, gballoc_hl_free);

THANDLE(LL3456789012345678901234567890123) ll33_create(int a)
{
    LL3456789012345678901234567890123* result = THANDLE_MALLOC(LL3456789012345678901234567890123)(NULL);
    if (result == NULL)
    {
        LogError("failure in THANDLE_MALLOC(LL3456789012345678901234567890123)(NULL)");
        /*return as is*/
    }
    else
    {
        result->a = a;
        goto allOk;
allOk:;
    }
    return result;
}

#if defined(DEBUG) || defined(_DEBUG)
const char* ll33_get_name(THANDLE(LL3456789012345678901234567890123) ll)
{
    if (ll == NULL)
    {
        LogError("invalid arguments: THANDLE(LL3456789012345678901234567890123) ll=%p", ll);
        return NULL;
    }
    else
    {

        return (THANDLE_INSPECT(LL3456789012345678901234567890123)(ll)->name);
    }
}
#endif
