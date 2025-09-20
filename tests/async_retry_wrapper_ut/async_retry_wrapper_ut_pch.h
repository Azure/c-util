// Copyright (c) Microsoft. All rights reserved.


// Precompiled header for async_retry_wrapper_ut

#include <stdlib.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "c_pal/interlocked.h" /*included for mocking reasons - it will prohibit creation of mocks belonging to interlocked.h - at the moment verified through int tests - this is porting legacy code, temporary solution*/

#define ENABLE_MOCKS
#define GBALLOC_HL_REDIRECT_H
#undef GBALLOC_HL_REDIRECT_H

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/threadapi.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle.h"
#include "c_pal/timer.h"

#include "c_pal/interlocked_hl.h"
#include "c_pal/log_critical_and_terminate.h"

#include "test_async.h"
#include "test_ref_counted.h"
#include "c_util/async_type_helper.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "real_test_ref_counted.h"

#include "test_async_retry_wrappers.h"

#include "c_util/async_retry_wrapper.h"