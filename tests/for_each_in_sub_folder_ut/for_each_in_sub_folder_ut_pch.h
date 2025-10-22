// Copyright (c) Microsoft. All rights reserved.

// Precompiled header for for_each_in_sub_folder_ut

#ifndef FOR_EACH_IN_SUB_FOLDER_UT_PCH_H
#define FOR_EACH_IN_SUB_FOLDER_UT_PCH_H

#include <stdlib.h>
#include <stdint.h>

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"

#include "c_pal/interlocked.h" /*included for mocking reasons - it will prohibit creation of mocks belonging to interlocked.h - at the moment verified through int tests - this is porting legacy code, temporary solution*/

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/string_utils.h"

#include "c_util/for_each_in_folder.h"

#include "umock_c/umock_c_prod.h"

#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_string_utils.h"

#include "c_util/for_each_in_sub_folder.h"

#define TEST_FOLDER_DEFINE "c:\\folder"

#endif // FOR_EACH_IN_SUB_FOLDER_UT_PCH_H
