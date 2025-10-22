// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for tp_worker_thread_ut

#ifndef TP_WORKER_THREAD_UT_PCH_H
#define TP_WORKER_THREAD_UT_PCH_H

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"
#include "c_pal/log_critical_and_terminate.h"
#include "c_pal/sm.h"
#include "c_pal/threadpool.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "c_util/tp_worker_thread.h"

// Must include umock_c_prod so mocks are not expanded in real_rc_string
#include "umock_c/umock_c_prod.h"

#include "real_gballoc_hl.h"
#include "real_interlocked.h"
#include "real_sm.h"
#include "real_threadpool_thandle.h"
#include "real_threadpool_work_item_thandle.h"

#endif // TP_WORKER_THREAD_UT_PCH_H
