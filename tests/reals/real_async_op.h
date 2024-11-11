// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_ASYNC_OP_H
#define REAL_ASYNC_OP_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"

#include "c_util/async_op.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_ASYNC_OP_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        async_op_create, \
        async_op_cancel \
    ) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_MOVE(ASYNC_OP), THANDLE_MOVE(real_ASYNC_OP)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(ASYNC_OP), THANDLE_INITIALIZE(real_ASYNC_OP)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE_MOVE(ASYNC_OP), THANDLE_INITIALIZE_MOVE(real_ASYNC_OP)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(ASYNC_OP), THANDLE_ASSIGN(real_ASYNC_OP)) \

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct ASYNC_OP_TAG real_ASYNC_OP;
    THANDLE_TYPE_DECLARE(real_ASYNC_OP);

    THANDLE(real_ASYNC_OP) real_async_op_create(ASYNC_OP_CANCEL_IMPL cancel, uint32_t context_size, uint32_t context_align, ASYNC_OP_DISPOSE dispose);
    ASYNC_OP_STATE real_async_op_cancel(THANDLE(real_ASYNC_OP) async_op);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // REAL_ASYNC_OP_H
