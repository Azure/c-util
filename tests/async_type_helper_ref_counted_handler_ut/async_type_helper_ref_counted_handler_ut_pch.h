// Copyright (c) Microsoft. All rights reserved.



// Precompiled header for async_type_helper_ref_counted_handler_ut

#include <stdlib.h>
#include <stddef.h>


#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_stdint.h"

#include "c_pal/interlocked.h" /*included for mocking reasons - it will prohibit creation of mocks belonging to this interlocked.h, temporary solution*/

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "test_ref_counted.h"
#undef ENABLE_MOCKS
#include "real_gballoc_hl.h"


#include "c_util/async_type_helper.h"
#include "test_ref_counted_async_type_helper_handler.h"

#include "c_util/async_type_helper_ref_counted_handler.h"