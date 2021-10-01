// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//#include "real_interlocked_renames.h" // IWYU pragma: keep
//#include "real_gballoc_hl_renames.h" // IWYU pragma: keep
//#include "real_memory_data_renames.h" // IWYU pragma: keep
//
//#include "real_constbuffer_renames.h" // IWYU pragma: keep
//
//#include "../src/constbuffer.c"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/tarray_ll.h"
#include "c_util/tarray.h"

#define THANDLE_MALLOC_FUNCTION malloc
#define THANDLE_FREE_FUNCTION free

#include "real_tarray_undo_op.h"

#include "../play_tarray_undo_op_types.h"

TARRAY_LL_TYPE_DEFINE(real_UNDO_OP, UNDO_OP);


