// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*
this file exists to prove the 2 THANDLEs can be compiled in the same .h/.c file
*/

#include "c_util/thandle.h"

#include "malloc_mocks.h"

#include "two_thandles_same_file.h"

#define THANDLE_MALLOC_FUNCTION global_malloc
#define THANDLE_MALLOC_FLEX_FUNCTION global_malloc_flex
#define THANDLE_FREE_FUNCTION global_free
THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(TYPE_1, type_malloc, type_malloc_flex, type_free);
THANDLE_TYPE_DEFINE(TYPE_2);
#undef THANDLE_MALLOC_FUNCTION
#undef THANDLE_MALLOC_FLEX_FUNCTION
#undef THANDLE_FREE_FUNCTION

void donothing(void)
{
    return;
}

