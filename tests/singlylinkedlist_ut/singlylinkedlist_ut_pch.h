// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for singlylinkedlist_ut

#include <stdbool.h>
#include <stdlib.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_bool.h"
#include "c_util/singlylinkedlist.h"

#define ENABLE_MOCKS
/* test match function mock */

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#define TEST_CONTEXT ((const void*)0x4242)