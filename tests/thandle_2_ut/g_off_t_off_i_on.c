// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "malloc_mocks.h"

#include "g_off_t_off_i_on.h"

THANDLE_TYPE_DEFINE(G_OFF_T_OFF_I_ON_DUMMY);

THANDLE(G_OFF_T_OFF_I_ON_DUMMY) G_OFF_T_OFF_I_ON_create(int x)
{
    G_OFF_T_OFF_I_ON_DUMMY* d = THANDLE_MALLOC_FUNCTION_WITH_MALLOC_FUNCTIONS_NAME(G_OFF_T_OFF_I_ON_DUMMY, G_OFF_T_OFF_I_ON_DUMMY)(NULL, var_malloc, var_free);
    if (d != NULL)
    {
        d->x = x;
    }
    return d;
}
