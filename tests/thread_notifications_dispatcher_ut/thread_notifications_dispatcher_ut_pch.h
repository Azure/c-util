// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.See LICENSE file in the project root for full license information.

// Precompiled header for thread_notifications_dispatcher_ut

#ifndef THREAD_NOTIFICATIONS_DISPATCHER_UT_PCH_H
#define THREAD_NOTIFICATIONS_DISPATCHER_UT_PCH_H

#include <stdlib.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"

#include "c_pal/thandle.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/tcall_dispatcher_thread_notification_call.h"

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_tcall_dispatcher_thread_notification_call.h"

#include "c_util/thread_notifications_dispatcher.h"

#endif // THREAD_NOTIFICATIONS_DISPATCHER_UT_PCH_H
