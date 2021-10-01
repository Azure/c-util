// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*PLAY is a module that pretends to be a production module that will embed a TARRAY in its handle*/

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/tarray_ll.h"
#include "c_util/tarray.h"

#include "play_undo_op_types.h"

#include "play_tarray_undo_op_types.h"

#define THANDLE_MALLOC_FUNCTION malloc
#define THANDLE_FREE_FUNCTION free

THANDLE_TYPE_DEFINE(TARRAY_TYPEDEF_NAME(UNDO_OP));

