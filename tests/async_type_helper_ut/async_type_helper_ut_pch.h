// Copyright (c) Microsoft. All rights reserved.



// Precompiled header for async_type_helper_ut

#include <stdlib.h>
#include <stddef.h>


#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "c_pal/interlocked.h" /*included for mocking reasons - it will prohibit creation of mocks belonging to interlocked.h - at the moment verified through int tests - this is porting legacy code, temporary solution*/

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_util/constbuffer.h"
#include "c_util/constbuffer_array.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_constbuffer.h"
#include "real_constbuffer_array.h"


#include "c_util/async_type_helper.h"

#define TEST_PAYLOAD_SIZE 128