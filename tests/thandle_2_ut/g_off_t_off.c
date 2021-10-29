// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "c_pal/gballoc_hl.h" /*THANDLE needs malloc/malloc_flex/free to exist*/
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/thandle.h"

#include "malloc_mocks.h"

#include "g_off_t_off.h"

THANDLE_TYPE_DEFINE(G_OFF_T_OFF_DUMMY); /*this is a type that can only be instantiated with by global malloc or at every creation by THANDLE_MALLOC*/

/*uses global*/
THANDLE(G_OFF_T_OFF_DUMMY) G_OFF_T_OFF_create(int x)
{
    G_OFF_T_OFF_DUMMY* d = THANDLE_MALLOC(G_OFF_T_OFF_DUMMY)(NULL); /*uses global malloc functions*/
    if (d != NULL)
    {
        d->x = x;
        d->n = 0;
    }
    return d;
}

/*uses instance defined functions*/
THANDLE(G_OFF_T_OFF_DUMMY) G_OFF_T_OFF_create_with_malloc_functions(int x)
{
    G_OFF_T_OFF_DUMMY* d = THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS(G_OFF_T_OFF_DUMMY)(NULL, var_malloc, var_free);
    if (d != NULL)
    {
        d->x = x;
        d->n = 0;
    }
    return d;
}

/*uses global*/
THANDLE(G_OFF_T_OFF_DUMMY) G_OFF_T_OFF_create_with_extra_size(int x, const char* s)
{
    G_OFF_T_OFF_DUMMY* d = THANDLE_MALLOC_WITH_EXTRA_SIZE(G_OFF_T_OFF_DUMMY)(NULL, strlen(s)+1); /*uses global malloc functions*/
    if (d != NULL)
    {
        d->x = x;
        d->n = (uint32_t)strlen(s);
        (void)memcpy(d->s, s, d->n+1);
    }
    return d;
}

/*uses instance*/
THANDLE(G_OFF_T_OFF_DUMMY) G_OFF_T_OFF_create_with_extra_size_with_malloc_functions(int x, const char* s)
{
    G_OFF_T_OFF_DUMMY* d = THANDLE_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS(G_OFF_T_OFF_DUMMY)(NULL, strlen(s) + 1, var_malloc_flex, var_free);
    if (d != NULL)
    {
        d->x = x;
        d->n = (uint32_t)strlen(s);
        (void)memcpy(d->s, s, d->n+1);
    }
    return d;
}

static int copies_dummy(G_OFF_T_OFF_DUMMY* destination, const G_OFF_T_OFF_DUMMY* source)
{
    destination->x = source->x;
    destination->n = source->n;
    if (source->n > 0)
    {
        (void)memcpy(destination->s, source->s, source->n);
    }
    return 0;
}

static size_t sizeof_dummy(const G_OFF_T_OFF_DUMMY* source)
{
    return sizeof(G_OFF_T_OFF_DUMMY) + ((source->n==0)?0:(source->n+1));
}


THANDLE(G_OFF_T_OFF_DUMMY) G_OFF_T_OFF_create_from_content_flex(const G_OFF_T_OFF_DUMMY* origin)
{
    G_OFF_T_OFF_DUMMY* d = THANDLE_CREATE_FROM_CONTENT_FLEX(G_OFF_T_OFF_DUMMY)(origin, NULL, copies_dummy, sizeof_dummy); /*uses global malloc functions*/
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

THANDLE(G_OFF_T_OFF_DUMMY) G_OFF_T_OFF_create_from_content_flex_with_getsizeof_NULL(const G_OFF_T_OFF_DUMMY* origin)
{
    G_OFF_T_OFF_DUMMY* d = THANDLE_CREATE_FROM_CONTENT_FLEX(G_OFF_T_OFF_DUMMY)(origin, NULL, copies_dummy, NULL);
    /*d is NULL here because get_sizeof is NULL*/
    return d;
}

THANDLE(G_OFF_T_OFF_DUMMY) G_OFF_T_OFF_create_from_content_flex_with_malloc_functions(const G_OFF_T_OFF_DUMMY* origin)
{
    G_OFF_T_OFF_DUMMY* d = THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS(G_OFF_T_OFF_DUMMY)(origin, NULL, copies_dummy, sizeof_dummy, var_malloc_flex, var_free); /*uses global malloc functions*/
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

