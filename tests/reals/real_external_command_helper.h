// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_EXTERNAL_COMMAND_HELPER_H
#define REAL_EXTERNAL_COMMAND_HELPER_H

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_EXTERNAL_COMMAND_HELPER_GLOBAL_MOCK_HOOKS()     \
    MU_FOR_EACH_1(R2,                                           \
        external_command_helper_execute                         \
    )

#include "c_util/external_command_helper.h"

#ifdef __cplusplus
extern "C" {
#endif

    EXTERNAL_COMMAND_RESULT real_external_command_helper_execute(const char* command, RC_STRING_ARRAY** lines, int* return_code);

#ifdef __cplusplus
}
#endif

#endif //REAL_EXTERNAL_COMMAND_HELPER_H
