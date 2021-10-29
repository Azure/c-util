// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "c_pal/gballoc_hl.h" 
#include "c_pal/gballoc_hl_redirect.h" /*THANDLE needs malloc/malloc_flex/free to exist*/

#include "c_util/thandle.h"

#include "malloc_mocks.h"

#include "t_off.h"

THANDLE_TYPE_DEFINE(T_OFF_DUMMY); /*this is a type that allocates its memory by global malloc or by the function specified at THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS*/

/*uses global*/
THANDLE(T_OFF_DUMMY) T_OFF_create(int x)
{
    T_OFF_DUMMY* d = THANDLE_MALLOC(T_OFF_DUMMY)(NULL); /*uses global malloc functions*/
    if (d != NULL)
    {
        d->x = x;
        d->n = 0;
    }
    return d;
}

/*uses instance defined functions*/
THANDLE(T_OFF_DUMMY) T_OFF_create_with_malloc_functions(int x)
{
    T_OFF_DUMMY* d = THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS(T_OFF_DUMMY)(NULL, var_malloc, var_free);
    if (d != NULL)
    {
        d->x = x;
        d->n = 0;
    }
    return d;
}

/*uses global*/
THANDLE(T_OFF_DUMMY) T_OFF_create_with_extra_size(int x, const char* s)
{
    T_OFF_DUMMY* d = THANDLE_MALLOC_WITH_EXTRA_SIZE(T_OFF_DUMMY)(NULL, strlen(s)+1); /*uses global malloc functions*/
    if (d != NULL)
    {
        d->x = x;
        d->n = (uint32_t)strlen(s);
        (void)memcpy(d->s, s, d->n+1);
    }
    return d;
}

/*uses instance*/
THANDLE(T_OFF_DUMMY) T_OFF_create_with_extra_size_with_malloc_functions(int x, const char* s)
{
    T_OFF_DUMMY* d = THANDLE_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS(T_OFF_DUMMY)(NULL, strlen(s) + 1, var_malloc_flex, var_free);
    if (d != NULL)
    {
        d->x = x;
        d->n = (uint32_t)strlen(s);
        (void)memcpy(d->s, s, d->n+1);
    }
    return d;
}

static int copies_dummy(T_OFF_DUMMY* destination, const T_OFF_DUMMY* source)
{
    destination->x = source->x;
    destination->n = source->n;
    if (source->n > 0)
    {
        (void)memcpy(destination->s, source->s, source->n);
    }
    return 0;
}

static size_t sizeof_dummy(const T_OFF_DUMMY* source)
{
    return sizeof(T_OFF_DUMMY) + ((source->n==0)?0:(source->n+1));
}


THANDLE(T_OFF_DUMMY) T_OFF_create_from_content_flex(const T_OFF_DUMMY* origin)
{
    T_OFF_DUMMY* d = THANDLE_CREATE_FROM_CONTENT_FLEX(T_OFF_DUMMY)(origin, NULL, copies_dummy, sizeof_dummy); /*uses global malloc functions*/
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

THANDLE(T_OFF_DUMMY) T_OFF_create_from_content_flex_with_getsizeof_NULL(const T_OFF_DUMMY* origin)
{
    T_OFF_DUMMY* d = THANDLE_CREATE_FROM_CONTENT_FLEX(T_OFF_DUMMY)(origin, NULL, copies_dummy, NULL);
    /*d is NULL here because get_sizeof is NULL*/
    return d;
}

THANDLE(T_OFF_DUMMY) T_OFF_create_from_content_flex_with_malloc_functions(const T_OFF_DUMMY* origin)
{
    T_OFF_DUMMY* d = THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS(T_OFF_DUMMY)(origin, NULL, copies_dummy, sizeof_dummy, var_malloc_flex, var_free); /*uses var_malloc_flex/var_free functions*/
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

