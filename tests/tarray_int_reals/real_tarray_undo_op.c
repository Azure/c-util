// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "real_tarray_undo_op_renames.h"

#include "c_util/thandle_ll.h"
#include "c_util/tarray_ll.h"

#include "real_tarray_undo_op.h"

#define THANDLE_MALLOC_FUNCTION malloc
#define THANDLE_FREE_FUNCTION free

THANDLE_LL_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(real_UNDO_OP), TARRAY_TYPEDEF_NAME(UNDO_OP));
THANDLE_LL_TYPE_DEFINE(TARRAY_TYPEDEF_NAME(real_UNDO_OP), TARRAY_TYPEDEF_NAME(UNDO_OP));

TARRAY_LL_TYPE_DEFINE(real_UNDO_OP, UNDO_OP);



