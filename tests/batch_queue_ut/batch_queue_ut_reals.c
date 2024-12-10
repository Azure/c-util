// Copyright (c) Microsoft. All rights reserved.

#include <stddef.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#define ENABLE_MOCKS
#include "c_pal/interlocked.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/thandle_ll.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_interlocked.h"

#include "real_interlocked_renames.h"

#include "umock_c/umock_c_prod.h"
#pragma warning(disable: 4505) // C4505 is the warning for unreferenced static functions

typedef BATCH_ITEM_CONTEXT_HANDLE REAL_BATCH_ITEM_CONTEXT_HANDLE;
TARRAY_DEFINE_STRUCT_TYPE(REAL_BATCH_ITEM_CONTEXT_HANDLE);

THANDLE_LL_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(REAL_BATCH_ITEM_CONTEXT_HANDLE), TARRAY_TYPEDEF_NAME(BATCH_ITEM_CONTEXT_HANDLE));
THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(TARRAY_TYPEDEF_NAME(REAL_BATCH_ITEM_CONTEXT_HANDLE), TARRAY_TYPEDEF_NAME(BATCH_ITEM_CONTEXT_HANDLE), real_gballoc_hl_malloc, real_gballoc_hl_malloc_flex, real_gballoc_hl_free);
TARRAY_LL_TYPE_DECLARE(REAL_BATCH_ITEM_CONTEXT_HANDLE, BATCH_ITEM_CONTEXT_HANDLE);
#include "real_gballoc_hl_renames.h"
TARRAY_LL_TYPE_DEFINE(REAL_BATCH_ITEM_CONTEXT_HANDLE, BATCH_ITEM_CONTEXT_HANDLE);
