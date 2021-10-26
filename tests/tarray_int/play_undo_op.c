// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/tarray.h"

#include "play_undo_op.h"

#define THANDLE_MALLOC_FUNCTION malloc
#define THANDLE_MALLOC_FLEX_FUNCTION malloc_flex
#define THANDLE_FREE_FUNCTION free

TARRAY_TYPE_DEFINE(UNDO_OP);

