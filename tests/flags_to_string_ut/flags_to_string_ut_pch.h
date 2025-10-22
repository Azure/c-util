// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for flags_to_string_ut

#ifndef FLAGS_TO_STRING_UT_PCH_H
#define FLAGS_TO_STRING_UT_PCH_H

#include <stdlib.h>
#include <inttypes.h>
#include <stdarg.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_windows.h"
#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/string_utils.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_string_utils.h"
#include "real_gballoc_hl.h"

#include "flags_to_string_helper.h"

/*following function cannot be mocked because of variable number of arguments:( so it is copy&pasted here*/

#endif // FLAGS_TO_STRING_UT_PCH_H
