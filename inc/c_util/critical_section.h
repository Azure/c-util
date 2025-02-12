// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef CRITICAL_SECTION_H
#define CRITICAL_SECTION_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/interlocked.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MOCKABLE_FUNCTION(, void, enter_crit_section, volatile_atomic int32_t*, access_value);
    MOCKABLE_FUNCTION(, void, leave_crit_section, volatile_atomic int32_t*, access_value);

#ifdef __cplusplus
}
#endif

#endif // CRITICAL_SECTION_H

