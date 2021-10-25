// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "c_util/thandle.h"

#include "malloc_mocks.h"

#include "g_on_t_off.h"

#define THANDLE_MALLOC_FUNCTION global_malloc
#define THANDLE_MALLOC_FLEX_FUNCTION global_malloc_flex
#define THANDLE_FREE_FUNCTION global_free
THANDLE_TYPE_DEFINE(G_ON_T_OFF_DUMMY);
#undef THANDLE_MALLOC_FUNCTION
#undef THANDLE_MALLOC_FLEX_FUNCTION
#undef THANDLE_FREE_FUNCTION

/*uses global*/
THANDLE(G_ON_T_OFF_DUMMY) G_ON_T_OFF_create(int x)
{
    G_ON_T_OFF_DUMMY* d = THANDLE_MALLOC(G_ON_T_OFF_DUMMY)(NULL);
    if (d != NULL)
    {
        d->x = x;
        d->n = 0;
    }
    return d;
}

/*uses instance defined functions*/
THANDLE(G_ON_T_OFF_DUMMY) G_ON_T_OFF_create_with_malloc_functions(int x)
{
    G_ON_T_OFF_DUMMY* d = THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS(G_ON_T_OFF_DUMMY)(NULL, var_malloc, var_free);
    if (d != NULL)
    {
        d->x = x;
        d->n = 0;
    }
    return d;
}

/*uses global*/
THANDLE(G_ON_T_OFF_DUMMY) G_ON_T_OFF_create_with_extra_size(int x, const char* s)
{
    G_ON_T_OFF_DUMMY* d = THANDLE_MALLOC_WITH_EXTRA_SIZE(G_ON_T_OFF_DUMMY)(NULL, strlen(s)+1);
    if (d != NULL)
    {
        d->x = x;
        d->n = (uint32_t)strlen(s);
        (void)memcpy(d->s, s, d->n+1);
    }
    return d;
}

/*uses instance*/
THANDLE(G_ON_T_OFF_DUMMY) G_ON_T_OFF_create_with_extra_size_with_malloc_functions(int x, const char* s)
{
    G_ON_T_OFF_DUMMY* d = THANDLE_LL_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS(G_ON_T_OFF_DUMMY)(NULL, strlen(s) + 1, var_malloc_flex, var_free);
    if (d != NULL)
    {
        d->x = x;
        d->n = (uint32_t)strlen(s);
        (void)memcpy(d->s, s, d->n+1);
    }
    return d;
}

static int copies_dummy(G_ON_T_OFF_DUMMY* destination, const G_ON_T_OFF_DUMMY* source)
{
    destination->x = source->x;
    destination->n = source->n;
    if (source->n > 0)
    {
        (void)memcpy(destination->s, source->s, source->n);
    }
    return 0;
}

static size_t sizeof_dummy(const G_ON_T_OFF_DUMMY* source)
{
    return sizeof(G_ON_T_OFF_DUMMY) + ((source->n==0)?0:(source->n+1));
}


THANDLE(G_ON_T_OFF_DUMMY) G_ON_T_OFF_create_from_content_flex(const G_ON_T_OFF_DUMMY* origin)
{
    G_ON_T_OFF_DUMMY* d = THANDLE_CREATE_FROM_CONTENT_FLEX(G_ON_T_OFF_DUMMY)(origin, NULL, copies_dummy, sizeof_dummy);
    if (d != NULL)
    {
        d->x = origin->x;
        d->n = origin->n;
        if (origin->n > 0)
        {
            (void)memcpy(d->s, origin->s, d->n + 1);
        }
    }
    return d;
}

THANDLE(G_ON_T_OFF_DUMMY) G_ON_T_OFF_create_from_content_flex_with_malloc_functions(const G_ON_T_OFF_DUMMY* origin)
{
    G_ON_T_OFF_DUMMY* d = THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS(G_ON_T_OFF_DUMMY)(origin, NULL, copies_dummy, sizeof_dummy, var_malloc_flex, var_free);
    if (d != NULL)
    {
        d->x = origin->x;
        d->n = origin->n;
        if (origin->n > 0)
        {
            (void)memcpy(d->s, origin->s, d->n + 1);
        }
    }
    return d;
}

