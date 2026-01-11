// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for paged_sparse_array_ut

#ifndef PAGED_SPARSE_ARRAY_UT_PCH_H
#define PAGED_SPARSE_ARRAY_UT_PCH_H

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/thandle.h"
#include "c_util/paged_sparse_array.h"

#endif // PAGED_SPARSE_ARRAY_UT_PCH_H
