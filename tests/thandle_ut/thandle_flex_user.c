// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/thandle.h"

#include "thandle_flex_user.h"

#define LL_FLEX_FIELDS          \
    int, a,                     \
    char*, s,                   \
    size_t, theMany[]

MU_DEFINE_STRUCT(LL_FLEX, LL_FLEX_FIELDS);


#define THANDLE_MALLOC_FUNCTION gballoc_hl_malloc
#define THANDLE_MALLOC_FLEX_FUNCTION gballoc_hl_malloc_flex
#define THANDLE_FREE_FUNCTION gballoc_hl_free
THANDLE_TYPE_DEFINE(LL_FLEX);
#undef THANDLE_MALLOC_FUNCTION
#undef THANDLE_MALLOC_FLEX_FUNCTION
#undef THANDLE_FREE_FUNCTION

static void frees_the_string(LL_FLEX* ll)
{
    if (ll->s != NULL)
    {
        free(ll->s);
    }
}

THANDLE(LL_FLEX) ll_flex_create(int a, const char* s, size_t howMany)
{
    LL_FLEX* result = THANDLE_MALLOC_WITH_EXTRA_SIZE(LL_FLEX)(frees_the_string, howMany * sizeof(int));
    if (result == NULL)
    {
        LogError("failure in THANDLE_MALLOC_WITH_EXTRA_SIZE(LL_FLEX)(frees_the_string, howMany=%zu * sizeof(int)=%zu)",
            howMany, sizeof(int));
        /*return as is*/
    }
    else
    {
        if (s != NULL)
        {
            result->s = malloc(strlen(s) + 1);
            if (result->s == NULL)
            {
                LogError("failure in malloc...");
            }
            else
            {
                (void)memcpy(result->s, s, strlen(s) + 1);
                result->a = a;
                goto allOk;
            }
        }
        else
        {
            result->a = a;
            result->s = NULL;
            goto allOk;
        }

        for (size_t i = 0; i < howMany; i++)
        {
            result->theMany[i] = i;
        }

        THANDLE_FREE(LL_FLEX)(result);
        result = NULL;
allOk:;
    }
    return result;
}
