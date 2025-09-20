// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for critical_section_ut

#include <stdlib.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_stdint.h"

#define ENABLE_MOCKS

#include "c_pal/sync.h"
#include "c_pal/interlocked_hl.h"

#undef ENABLE_MOCKS

// Must include umock_c_prod so mocks are not expanded in reals
#include "umock_c/umock_c_prod.h"
#include "real_interlocked.h"

#include "c_util/critical_section.h"