// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for constbuffer_ut

#ifndef CONSTBUFFER_UT_PCH_H
#define CONSTBUFFER_UT_PCH_H

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include <limits.h>

#include "umock_c/umocktypes_stdint.h"

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_util/buffer_.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "umock_c/umock_c_prod.h"

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"

#endif // CONSTBUFFER_UT_PCH_H
