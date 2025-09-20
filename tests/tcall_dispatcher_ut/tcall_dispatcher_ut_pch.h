// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.See LICENSE file in the project root for full license information.

// Precompiled header for tcall_dispatcher_ut

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_charptr.h"

#include "c_pal/thandle.h"

#define ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/srw_lock_ll.h"
#include "c_util/doublylinkedlist.h"

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "real_srw_lock_ll.h"
#include "real_doublylinkedlist.h"

#include "c_util/tcall_dispatcher.h"