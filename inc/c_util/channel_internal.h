// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CHANNEL_INTERNAL_H
#define CHANNEL_INTERNAL_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle_log_context_handle.h"

#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"
#include "c_util/rc_string.h"

#include "channel_common.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct CHANNEL_INTERNAL_TAG CHANNEL_INTERNAL;

THANDLE_TYPE_DECLARE(CHANNEL_INTERNAL);

    MOCKABLE_FUNCTION(, THANDLE(CHANNEL_INTERNAL), channel_internal_create, THANDLE(PTR(LOG_CONTEXT_HANDLE)), log_context, THANDLE(THREADPOOL), threadpool);
    MOCKABLE_FUNCTION(, int, channel_internal_open, THANDLE(CHANNEL_INTERNAL), channel_internal);
    MOCKABLE_FUNCTION(, void, channel_internal_close, THANDLE(CHANNEL_INTERNAL), channel_internal);
    MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_internal_pull, THANDLE(CHANNEL_INTERNAL), channel_internal, THANDLE(RC_STRING), correlation_id, ON_DATA_AVAILABLE_CB, on_data_available_cb, void*, pull_context, THANDLE(ASYNC_OP)*, out_op_pull);
    MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_internal_push, THANDLE(CHANNEL_INTERNAL), channel_internal, THANDLE(RC_STRING), correlation_id, THANDLE(RC_PTR), data, ON_DATA_CONSUMED_CB, on_data_consumed_cb, void*, push_context, THANDLE(ASYNC_OP)*, out_op_push);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CHANNEL_INTERNAL_H
