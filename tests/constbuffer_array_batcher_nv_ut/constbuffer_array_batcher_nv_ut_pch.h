// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for constbuffer_array_batcher_nv_ut

#ifndef CONSTBUFFER_ARRAY_BATCHER_NV_UT_PCH_H
#define CONSTBUFFER_ARRAY_BATCHER_NV_UT_PCH_H

#include <stdlib.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "real_gballoc_ll.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS

#include "c_util/constbuffer.h"
#include "c_util/constbuffer_array.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_util/memory_data.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "c_util/constbuffer_array_batcher_nv.h"

#include "real_gballoc_hl.h"

#include "../reals/real_constbuffer.h"
#include "../reals/real_constbuffer_array.h"
#include "../reals/real_memory_data.h"

#endif // CONSTBUFFER_ARRAY_BATCHER_NV_UT_PCH_H
