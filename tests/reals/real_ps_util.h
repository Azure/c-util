// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_PS_UTIL_H
#define REAL_PS_UTIL_H

#include "macro_utils/macro_utils.h"
#include "c_util/ps_util.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_PS_UTIL_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        ps_util_terminate_process, \
        ps_util_exit_process \
    )

#include <stddef.h>

void ps_util_terminate_process(void);
void ps_util_exit_process(int exit_code);

#endif // REAL_PS_UTIL_H
