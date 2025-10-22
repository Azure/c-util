// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for object_lifetime_tracker_ut

#ifndef OBJECT_LIFETIME_TRACKER_UT_PCH_H
#define OBJECT_LIFETIME_TRACKER_UT_PCH_H

#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/srw_lock.h"

#include "c_util/doublylinkedlist.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_doublylinkedlist.h"
#include "real_srw_lock.h"

#include "c_util/object_lifetime_tracker.h"

#endif // OBJECT_LIFETIME_TRACKER_UT_PCH_H
