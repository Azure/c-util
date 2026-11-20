// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for thread_safe_paged_sparse_array_ut

#ifndef THREAD_SAFE_PAGED_SPARSE_ARRAY_UT_PCH_H
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_UT_PCH_H

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"

#include "c_pal/interlocked.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/srw_lock_ll.h"
#include "c_pal/sync.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_interlocked_hl.h"
#include "real_srw_lock_ll.h"
#include "real_sync.h"

#include "c_pal/thandle.h"
#include "c_util/thread_safe_paged_sparse_array.h"

#endif // THREAD_SAFE_PAGED_SPARSE_ARRAY_UT_PCH_H
