// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_CRITICAL_SECTION_H
#define REAL_CRITICAL_SECTION_H

#include <stdint.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"


#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_CRITICAL_SECTION_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        enter_crit_section, \
        leave_crit_section \
    )


#include "c_util/critical_section.h"

void real_enter_crit_section(volatile_atomic int32_t* access_value);
void real_leave_crit_section(volatile_atomic int32_t* access_value);

#endif //REAL_CRITICAL_SECTION_H
