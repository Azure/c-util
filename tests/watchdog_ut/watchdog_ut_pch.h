// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for watchdog_ut

#ifndef WATCHDOG_UT_PCH_H
#define WATCHDOG_UT_PCH_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_bool.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/threadpool.h"
#include "c_pal/ps_util.h"
#include "c_util/rc_string.h"
#include "c_pal/thandle.h"
#include "c_pal/sm.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

// Must include umock_c_prod so mocks are not expanded in real_rc_string
#include "umock_c/umock_c_prod.h"

#include "real_gballoc_hl.h"
#include "real_interlocked.h"
#include "real_interlocked_hl.h"
#include "real_rc_string.h"
#include "real_sm.h"

#include "c_util/watchdog.h"

#endif // WATCHDOG_UT_PCH_H
