// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_CHANNEL_H
#define REAL_CHANNEL_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_CHANNEL_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        channel_create, \
        channel_open, \
        channel_close, \
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

#include "c_util/channel.h"

    typedef struct CHANNEL_TAG real_CHANNEL;
    THANDLE_TYPE_DECLARE(real_CHANNEL);

    THANDLE(CHANNEL) real_channel_create(THANDLE(PTR(LOG_CONTEXT_HANDLE)) log_context, THANDLE(THREADPOOL) threadpool);
    int real_channel_open(THANDLE(CHANNEL) channel);
    void real_channel_close(THANDLE(CHANNEL) channel);
    CHANNEL_RESULT real_channel_pull(THANDLE(CHANNEL) channel, THANDLE(RC_STRING) correlation_id, ON_DATA_AVAILABLE_CB on_data_available_cb, void* pull_context, THANDLE(ASYNC_OP)* out_op_pull);
    CHANNEL_RESULT real_channel_push(THANDLE(CHANNEL) channel, THANDLE(RC_STRING) correlation_id, THANDLE(RC_PTR) data, ON_DATA_CONSUMED_CB on_data_consumed_cb, void* push_context, THANDLE(ASYNC_OP)* out_op_push);
    int real_channel_get_stat_snapshot(THANDLE(CHANNEL) channel, CHANNEL_STATS* channel_stats);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // REAL_CHANNEL_H
