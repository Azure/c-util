// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for uuid_string_ut

#ifndef UUID_STRING_UT_PCH_H
#define UUID_STRING_UT_PCH_H

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/uuid.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "c_pal/umocktypes_uuid_t.h"

#include "real_uuid.h" /*the one from c_pal*/
#include "real_gballoc_hl.h"

#include "c_util/uuid_string.h"

#endif // UUID_STRING_UT_PCH_H
