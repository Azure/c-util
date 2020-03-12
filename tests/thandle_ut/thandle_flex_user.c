// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "azure_macro_utils/macro_utils.h"

#include "azure_c_util/gballoc.h"

#include "azure_c_util/thandle.h"

#include "thandle_flex_user.h"

#define LL_FLEX_FIELDS          \
    int, a,                     \
    char*, s,                   \
    int, theMany[]

#ifdef _MSC_VER
    /*warning C4200: nonstandard extension used: zero-sized array in struct/union : looks very standard in C99 and it is called flexible array. Documentation-wise is a flexible array, but called "unsized" in Microsoft's docs*/ /*https://msdn.microsoft.com/en-us/library/b6fae073.aspx*/
#pragma warning(disable:4200)
#endif
MU_DEFINE_STRUCT(LL_FLEX, LL_FLEX_FIELDS);

THANDLE_TYPE_DEFINE(LL_FLEX);

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
        LogError("failure in THANDLE_MALLOC_WITH_EXTRA_SIZE(LL)(frees_the_string, howMany=%zu * sizeof(int)=%zu)",
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

        for (int i = 0; i < howMany; i++)
        {
            result->theMany[i] = i;
        }

        THANDLE_FREE(LL_FLEX)(result);
        result = NULL;
allOk:;
    }
    return result;
}
