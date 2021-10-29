// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*
this file exists to prove the 2 THANDLEs can be compiled in the same .h/.c file
*/

#include "c_pal/gballoc_hl.h" /*THANDLE needs malloc/malloc_flex/free to exist*/
#include "c_pal/gballoc_hl_redirect.h" 

#include "c_util/thandle.h"

#include "malloc_mocks.h"

#include "two_thandles_same_file.h"

THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(TYPE_1, type_malloc, type_malloc_flex, type_free);
THANDLE_TYPE_DEFINE(TYPE_2);

void donothing(void)
{
    return;
}

