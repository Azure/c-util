// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for uuid_string_ut

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/uuid.h"
#undef ENABLE_MOCKS

#include "c_pal/umocktypes_uuid_t.h"

#include "real_uuid.h" /*the one from c_pal*/
#include "real_gballoc_hl.h"

#include "c_util/uuid_string.h"