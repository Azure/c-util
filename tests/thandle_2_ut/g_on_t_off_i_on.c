// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "malloc_mocks.h"

#include "g_on_t_off_i_on.h"

#include "c_util/thandle_ll.h"

#define THANDLE_MALLOC_FUNCTION global_malloc
#define THANDLE_MALLOC_FLEX_FUNCTION global_malloc_flex
#define THANDLE_FREE_FUNCTION global_free
THANDLE_TYPE_DEFINE(G_OFF_T_OFF_I_ON_DUMMY);
#undef THANDLE_MALLOC_FUNCTION 
#undef THANDLE_MALLOC_FLEX_FUNCTION 
#undef THANDLE_FREE_FUNCTION

THANDLE(G_OFF_T_OFF_I_ON_DUMMY) G_OFF_T_OFF_I_ON_create(int x)
{
    G_OFF_T_OFF_I_ON_DUMMY* d = THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS(G_OFF_T_OFF_I_ON_DUMMY)(NULL, var_malloc, var_free);
    if (d != NULL)
    {
        d->x = x;
    }
    return d;
}
