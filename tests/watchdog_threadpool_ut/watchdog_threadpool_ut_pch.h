// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for watchdog_threadpool_ut

#ifndef WATCHDOG_THREADPOOL_UT_PCH_H
#define WATCHDOG_THREADPOOL_UT_PCH_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle.h"
#include "c_pal/execution_engine.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_interlocked.h"

#include "c_util/watchdog_threadpool.h"

#endif // WATCHDOG_THREADPOOL_UT_PCH_H
