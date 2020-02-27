// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "azure_macro_utils/macro_utils.h"

#include "azure_c_util/gballoc.h"

#include "azure_c_util/thandle.h"

#define LL_FIELDS           \
    int, a,                 \
    char*, s                \
    
MU_DEFINE_STRUCT(LL, LL_FIELDS);

THANDLE_TYPE_DEFINE(LL);

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
