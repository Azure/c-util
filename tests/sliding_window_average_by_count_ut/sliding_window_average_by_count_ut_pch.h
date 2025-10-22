// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for sliding_window_average_by_count_ut

#ifndef SLIDING_WINDOW_AVERAGE_BY_COUNT_UT_PCH_H
#define SLIDING_WINDOW_AVERAGE_BY_COUNT_UT_PCH_H

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_struct.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/srw_lock.h"
#include "c_pal/thandle.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

// Must include umock_c_prod so mocks are not expanded in reals
#include "umock_c/umock_c_prod.h"

#include "real_gballoc_hl.h"
#include "real_interlocked.h"
#include "real_interlocked_hl.h"
#include "real_srw_lock.h"

#include "c_util/sliding_window_average_by_count.h"

#endif // SLIDING_WINDOW_AVERAGE_BY_COUNT_UT_PCH_H
