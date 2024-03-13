// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "real_tarray_undo_op_renames.h"

#include "c_pal/thandle_ll.h"
#include "c_util/tarray_ll.h"

#include "real_gballoc_ll.h"
#include "real_gballoc_ll_renames.h"

#define malloc_flex real_gballoc_ll_malloc_flex /*THANDLE needs malloc/malloc_flex/free to exist. In this case nobody would be providing malloc_flex (except we are now)*/

#include "real_tarray_undo_op.h"

THANDLE_LL_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(real_UNDO_OP), TARRAY_TYPEDEF_NAME(UNDO_OP));
THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(TARRAY_TYPEDEF_NAME(real_UNDO_OP), TARRAY_TYPEDEF_NAME(UNDO_OP), gballoc_ll_malloc, gballoc_ll_malloc_flex, gballoc_ll_free);


#define realloc_2 real_gballoc_ll_realloc_2
#define malloc_2 real_gballoc_ll_malloc_2
#define free real_gballoc_ll_free
TARRAY_LL_TYPE_DEFINE(real_UNDO_OP, UNDO_OP);

