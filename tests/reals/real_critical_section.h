// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_CRITICAL_SECTION_H
#define REAL_CRITICAL_SECTION_H

#include <stdint.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/sync.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_CRITICAL_SECTION_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        critical_section_enter, \
        critical_section_leave \
    )


#include "c_util/critical_section.h"

int real_critical_section_enter(volatile_atomic int32_t* access_value);
int real_critical_section_leave(volatile_atomic int32_t* access_value);

#endif //REAL_CRITICAL_SECTION_H
