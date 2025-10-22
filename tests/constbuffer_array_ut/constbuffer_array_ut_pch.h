// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for constbuffer_array_ut

#ifndef CONSTBUFFER_ARRAY_UT_PCH_H
#define CONSTBUFFER_ARRAY_UT_PCH_H

#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>

#include "real_gballoc_ll.h"

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_util/constbuffer.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_interlocked.h"
#include "real_constbuffer.h"
#include "real_gballoc_hl.h"

#include "c_util/constbuffer_array.h"

#endif // CONSTBUFFER_ARRAY_UT_PCH_H
