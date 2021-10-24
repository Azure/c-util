// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "malloc_mocks.h"

#include "g_on_t_off_i_off.h"

#define THANDLE_MALLOC_FUNCTION global_malloc
#define THANDLE_MALLOC_FLEX_FUNCTION global_malloc_flex
#define THANDLE_FREE_FUNCTION global_free
THANDLE_TYPE_DEFINE(G_ON_T_OFF_I_OFF_DUMMY);
#undef THANDLE_MALLOC_FUNCTION 
#undef THANDLE_MALLOC_FLEX_FUNCTION 
#undef THANDLE_FREE_FUNCTION

THANDLE(G_ON_T_OFF_I_OFF_DUMMY) G_ON_T_OFF_I_OFF_create(int x)
{
    G_ON_T_OFF_I_OFF_DUMMY* d = THANDLE_MALLOC(G_ON_T_OFF_I_OFF_DUMMY)(NULL);
    if (d != NULL)
    {
        d->x = x;
    }
    return d;
}
