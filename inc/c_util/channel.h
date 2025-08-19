// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CHANNEL_H
#define CHANNEL_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle_log_context_handle.h"

#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"
#include "c_util/rc_string.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CHANNEL_RESULT_VALUES \
    CHANNEL_RESULT_OK, \
    CHANNEL_RESULT_INVALID_ARGS, \
    CHANNEL_RESULT_ERROR, \
    CHANNEL_RESULT_OVERFLOWED

MU_DEFINE_ENUM(CHANNEL_RESULT, CHANNEL_RESULT_VALUES);

#define CHANNEL_CALLBACK_RESULT_VALUES \
    CHANNEL_CALLBACK_RESULT_OK, \
    CHANNEL_CALLBACK_RESULT_CANCELLED, \
    CHANNEL_CALLBACK_RESULT_ABANDONED

MU_DEFINE_ENUM(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_VALUES);

typedef void(*ON_DATA_AVAILABLE_CB)(void* pull_context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id, THANDLE(RC_PTR) data);
typedef void(*ON_DATA_CONSUMED_CB)(void* push_context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id);

typedef struct CHANNEL_TAG CHANNEL;

typedef struct CHANNEL_STATS_TAG
{
    int32_t count_of_operations_pushed;
    int32_t count_of_operations_pulled;
} CHANNEL_STATS;

THANDLE_TYPE_DECLARE(CHANNEL);

    MOCKABLE_FUNCTION(, THANDLE(CHANNEL), channel_create, THANDLE(PTR(LOG_CONTEXT_HANDLE)), log_context, THANDLE(THREADPOOL), threadpool, int32_t, overflow_level);
    MOCKABLE_FUNCTION(, int, channel_open, THANDLE(CHANNEL), channel);
    MOCKABLE_FUNCTION(, void, channel_close, THANDLE(CHANNEL), channel);
    MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_pull, THANDLE(CHANNEL), channel, THANDLE(RC_STRING), correlation_id, ON_DATA_AVAILABLE_CB, on_data_available_cb, void*, pull_context, THANDLE(ASYNC_OP)*, out_op_pull);
    MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_push, THANDLE(CHANNEL), channel, THANDLE(RC_STRING), correlation_id, THANDLE(RC_PTR), data, ON_DATA_CONSUMED_CB, on_data_consumed_cb, void*, push_context, THANDLE(ASYNC_OP)*, out_op_push);
    MOCKABLE_FUNCTION(, int, channel_get_stat_snapshot, THANDLE(CHANNEL), channel, CHANNEL_STATS*, channel_stats);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CHANNEL_H */
