// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for constbuffer_ut

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include <limits.h>

#include "umock_c/umocktypes_stdint.h"

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#define ENABLE_MOCKS
#include "c_util/buffer_.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "umock_c/umock_c_prod.h"

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"