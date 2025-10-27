// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for external_command_helper_ut

#ifndef EXTERNAL_COMMAND_HELPER_UT_PCH_H
#define EXTERNAL_COMMAND_HELPER_UT_PCH_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "real_gballoc_ll.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "c_logging/logger.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/pipe.h"
#include "c_util/rc_string.h"
#include "c_util/rc_string_array.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

// Must include umock_c_prod so mocks are not expanded in real_rc_string
#include "umock_c/umock_c_prod.h"

#include "real_gballoc_hl.h"
#include "real_rc_string.h"
#include "real_rc_string_array.h"

#include "c_util/external_command_helper.h"

#endif // EXTERNAL_COMMAND_HELPER_UT_PCH_H
