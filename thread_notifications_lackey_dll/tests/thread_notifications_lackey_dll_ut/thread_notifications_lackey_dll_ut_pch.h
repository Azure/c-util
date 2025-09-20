// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.See LICENSE file in the project root for full license information.

// Precompiled header for thread_notifications_lackey_dll_ut

#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>

#include "windows.h"

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_logging/logger.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_windows.h"
#include "umock_c/umock_c_negative_tests.h"

#include "c_pal/interlocked.h"

#define ENABLE_MOCKS

#include "c_pal/ps_util.h"

#undef ENABLE_MOCKS

#include "thread_notifications_lackey_dll/thread_notifications_lackey_dll.h"


BOOL WINAPI DllMain_under_test(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);