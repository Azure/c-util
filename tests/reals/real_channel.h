// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_CHANNEL_H
#define REAL_CHANNEL_H

#include "macro_utils/macro_utils.h"
#include "c_util/channel.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_CHANNEL_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        channel_create, \
        channel_pull, \
        channel_push \
    ) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_MOVE(CHANNEL), THANDLE_MOVE(real_CHANNEL)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(CHANNEL), THANDLE_INITIALIZE(real_CHANNEL)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE_MOVE(CHANNEL), THANDLE_INITIALIZE_MOVE(real_CHANNEL)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(CHANNEL), THANDLE_ASSIGN(real_CHANNEL)) \


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

THANDLE_TYPE_DECLARE(CHANNEL);
typedef struct CHANNEL_TAG real_CHANNEL;
THANDLE_TYPE_DECLARE(real_CHANNEL);

    MOCKABLE_FUNCTION(, THANDLE(CHANNEL), real_channel_create, THANDLE(THREADPOOL), threadpool);
    MOCKABLE_FUNCTION(, CHANNEL_RESULT, real_channel_pull, THANDLE(CHANNEL), channel, PULL_CALLBACK, pull_callback, void*, pull_context, THANDLE(ASYNC_OP)*, out_op_pull);
    MOCKABLE_FUNCTION(, CHANNEL_RESULT, real_channel_push, THANDLE(CHANNEL), channel, THANDLE(RC_PTR), data, PUSH_CALLBACK, push_callback, void*, push_context, THANDLE(ASYNC_OP)*, out_op_push);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // REAL_CHANNEL_H
