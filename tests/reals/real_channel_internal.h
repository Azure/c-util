// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_CHANNEL_INTERNAL_H
#define REAL_CHANNEL_INTERNAL_H

#include "macro_utils/macro_utils.h"

#include "../../inc/c_util/channel_internal.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_CHANNEL_INTERNAL_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        channel_internal_create_and_open, \
        channel_internal_pull, \
        channel_internal_push, \
        channel_internal_close \
    ) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_MOVE(CHANNEL_INTERNAL), THANDLE_MOVE(real_CHANNEL_INTERNAL)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(CHANNEL_INTERNAL), THANDLE_INITIALIZE(real_CHANNEL_INTERNAL)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE_MOVE(CHANNEL_INTERNAL), THANDLE_INITIALIZE_MOVE(real_CHANNEL_INTERNAL)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(CHANNEL_INTERNAL), THANDLE_ASSIGN(real_CHANNEL_INTERNAL)) \


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    THANDLE_TYPE_DECLARE(CHANNEL_INTERNAL);
    typedef struct CHANNEL_INTERNAL_TAG real_CHANNEL_INTERNAL;
    THANDLE_TYPE_DECLARE(real_CHANNEL_INTERNAL);

    THANDLE(CHANNEL_INTERNAL) real_channel_internal_create_and_open(THANDLE(PTR(LOG_CONTEXT_HANDLE)) log_context, THANDLE(THREADPOOL) threadpool);
    void real_channel_internal_close(THANDLE(CHANNEL_INTERNAL) channel_internal);
    CHANNEL_RESULT real_channel_internal_pull(THANDLE(CHANNEL_INTERNAL) channel_internal, THANDLE(RC_STRING) correlation_id, PULL_CALLBACK pull_callback, void* pull_context, THANDLE(ASYNC_OP)* out_op_pull);
    CHANNEL_RESULT real_channel_internal_push(THANDLE(CHANNEL_INTERNAL) channel_internal, THANDLE(RC_STRING) correlation_id, THANDLE(RC_PTR) data, PUSH_CALLBACK push_callback, void* push_context, THANDLE(ASYNC_OP)* out_op_push);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // REAL_CHANNEL_INTERNAL_H
