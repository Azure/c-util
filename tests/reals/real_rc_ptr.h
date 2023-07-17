// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_RC_PTR_H
#define REAL_RC_PTR_H

#include "macro_utils/macro_utils.h"
#include "c_pal/thandle.h"

#include "c_util/rc_ptr.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_RC_PTR_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        rc_ptr_create_with_move_pointer \
    ) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_MOVE(RC_PTR), THANDLE_MOVE(real_RC_PTR)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(RC_PTR), THANDLE_INITIALIZE(real_RC_PTR)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE_MOVE(RC_PTR), THANDLE_INITIALIZE_MOVE(real_RC_PTR)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(RC_PTR), THANDLE_ASSIGN(real_RC_PTR)) \

#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct RC_PTR_TAG real_RC_PTR;
    THANDLE_TYPE_DECLARE(real_RC_PTR);

    MOCKABLE_FUNCTION(, THANDLE(RC_PTR), real_rc_ptr_create_with_move_pointer, void*, ptr, RC_PTR_FREE_FUNC, free_func);

#ifdef __cplusplus
}
#endif

#endif // REAL_RC_PTR_H
