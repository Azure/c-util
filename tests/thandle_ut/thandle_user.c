// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <limits.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/thandle.h"

#include "thandle_user.h"

#define LL_FIELDS           \
    char*, s,               \
    int, a                  \

MU_DEFINE_STRUCT(LL, LL_FIELDS);

THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(LL, gballoc_hl_malloc, gballoc_hl_malloc_flex, gballoc_hl_free);

static void frees_the_string(LL* ll)
{
    if (ll->s != NULL)
    {
        free(ll->s);
    }
}

THANDLE(LL) ll_create(int a, const char* s)
{
    LL* result = THANDLE_MALLOC(LL)(frees_the_string);
    if (result == NULL)
    {
        LogError("failure in THANDLE_MALLOC(LL)(frees_the_string)");
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

        THANDLE_FREE(LL)(result);
        result = NULL;
allOk:;
    }
    return result;
}

void ll_increment_a(THANDLE(LL) ll, int amount)
{
    if (ll == NULL)
    {
        LogError("invalid arguments: THANDLE(LL) ll=%p", ll);
    }
    else
    {
        LL* underlying = THANDLE_GET_T(LL)(ll);
        underlying->a += amount;
    }
}

int ll_get_a(THANDLE(LL) ll)
{
    int result;
    if (ll == NULL)
    {
        LogError("invalid arguments: THANDLE(LL) ll=%p", ll);
        result = INT_MAX;
    }
    else
    {
        LL* underlying = THANDLE_GET_T(LL)(ll);
        result = underlying->a;
    }
    return result;
}
