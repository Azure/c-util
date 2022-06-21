// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_SM_H
#define REAL_SM_H

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_SM_GLOBAL_MOCK_HOOK()                  \
    MU_FOR_EACH_1(R2,                                   \
        sm_create,                                      \
        sm_destroy,                                     \
        sm_open_begin,                                  \
        sm_open_end,                                    \
        sm_close_begin,                                 \
        sm_close_end,                                   \
        sm_exec_begin,                                  \
        sm_exec_end,                                    \
        sm_barrier_begin,                               \
        sm_barrier_end,                                 \
        sm_fault                                        \
    )

#include "c_util/sm.h"



SM_HANDLE real_sm_create(const char* name);
void real_sm_destroy(SM_HANDLE sm);

SM_RESULT real_sm_open_begin(SM_HANDLE sm);
void real_sm_open_end(SM_HANDLE sm, bool success);

SM_RESULT real_sm_close_begin(SM_HANDLE sm);
void real_sm_close_end(SM_HANDLE sm);

SM_RESULT real_sm_exec_begin(SM_HANDLE sm);
void real_sm_exec_end(SM_HANDLE sm);

SM_RESULT real_sm_barrier_begin(SM_HANDLE sm);
void real_sm_barrier_end(SM_HANDLE sm);

void real_sm_fault(SM_HANDLE sm);




#endif //REAL_SM_H
