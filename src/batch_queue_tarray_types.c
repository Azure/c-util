// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stddef.h>

#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/thandle.h"
#include "c_util/tarray.h"

#include "batch_queue_tarray_types.h"

THANDLE_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(BATCH_ITEM_CONTEXT_HANDLE));
THANDLE_TYPE_DEFINE(TARRAY_TYPEDEF_NAME(BATCH_ITEM_CONTEXT_HANDLE));

TARRAY_TYPE_DEFINE(BATCH_ITEM_CONTEXT_HANDLE);
