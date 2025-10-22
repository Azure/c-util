// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for sync_wrapper_ut

#ifndef SYNC_WRAPPER_UT_PCH_H
#define SYNC_WRAPPER_UT_PCH_H

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "c_pal/interlocked.h" /*included for mocking reasons - it will prohibit creation of mocks belonging to interlocked.h - at the moment verified through int tests - this is porting legacy code, temporary solution*/
#include "c_pal/sync.h" /*included for mocking reasons - it will prohibit creation of mocks belonging to sync.h - at the moment verified through int tests - this is porting legacy code, temporary solution*/

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/interlocked_hl.h"
#include "c_pal/log_critical_and_terminate.h"

#include "test_async.h"
#include "test_ref_counted.h"

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"

#include "test_sync_wrappers.h"

#include "c_util/sync_wrapper.h"

#endif // SYNC_WRAPPER_UT_PCH_H
