// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/tarray_ll.h"
#include "c_util/tarray.h"

#include "real_tarray_undo_op.h"

#define THANDLE_MALLOC_FUNCTION malloc
#define THANDLE_FREE_FUNCTION free

THANDLE_TYPE_DEFINE(TARRAY_TYPEDEF_NAME(UNDO_OP));

TARRAY_LL_TYPE_DEFINE(real_UNDO_OP, UNDO_OP);


