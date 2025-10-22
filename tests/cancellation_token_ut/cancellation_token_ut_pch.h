// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for cancellation_token_ut

#ifndef CANCELLATION_TOKEN_UT_PCH_H
#define CANCELLATION_TOKEN_UT_PCH_H

#include <stdlib.h>
#include <inttypes.h>

#include "real_gballoc_ll.h"

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_util/tcall_dispatcher_cancellation_token_cancel_call.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_interlocked.h"
#include "../reals/real_tcall_dispatcher_cancellation_token_cancel_call.h"

#include "c_util/cancellation_token.h"

#endif // CANCELLATION_TOKEN_UT_PCH_H
