// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for worker_thread_ut

#ifndef WORKER_THREAD_UT_PCH_H
#define WORKER_THREAD_UT_PCH_H

#include <stdlib.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_stdint.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/sm.h"
#include "c_pal/sync.h"
#include "c_pal/threadapi.h"

#include "c_util/singlylinkedlist.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_sm.h"

#include "c_util/worker_thread.h"

#endif // WORKER_THREAD_UT_PCH_H
