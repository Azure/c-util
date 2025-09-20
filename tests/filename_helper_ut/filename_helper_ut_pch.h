// Copyright (c) Microsoft. All rights reserved.



// Precompiled header for filename_helper_ut

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>


#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"

#include "c_pal/interlocked.h" /*included for mocking reasons - it will prohibit creation of mocks belonging to interlocked.h - at the moment verified through int tests - this is porting legacy code, temporary solution*/

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/string_utils.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_string_utils.h"

#include "c_util/filename_helper.h"

/*following function cannot be mocked because of variable number of arguments:( so it is copy&pasted here*/