// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for channel_ut

#ifndef CHANNEL_UT_PCH_H
#define CHANNEL_UT_PCH_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/srw_lock.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle_log_context_handle.h"
#include "c_pal/sm.h"
#include "c_pal/log_critical_and_terminate.h"

#include "c_util/doublylinkedlist.h"
#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"
#include "c_util/rc_string.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_interlocked.h"
#include "real_srw_lock.h"
#include "real_thandle_helper.h"
#include "real_thandle_log_context_handle.h"
#include "real_sm.h"

#include "real_doublylinkedlist.h"
#include "real_async_op.h"
#include "real_rc_ptr.h"
#include "real_rc_string.h"

#include "c_pal/thandle.h"

#include "c_util/channel.h"

#define DISABLE_TEST_FUNCTION(x) static void x(void)

#endif // CHANNEL_UT_PCH_H
