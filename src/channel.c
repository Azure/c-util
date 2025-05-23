// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/thandle.h"
#include "c_pal/srw_lock.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle_log_context_handle.h"

#include "c_util/doublylinkedlist.h"
#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"
#include "c_util/rc_string.h"

#include "c_util/channel_internal.h"

#include "c_util/channel.h"

typedef struct CHANNEL_TAG
{
    THANDLE(CHANNEL_INTERNAL) channel_internal;
} CHANNEL;

THANDLE_TYPE_DEFINE(CHANNEL);

static void channel_dispose(CHANNEL* channel)
{
    /*Codes_SRS_CHANNEL_43_092: [ channel_dispose shall release the reference to THANDLE(CHANNEL_INTERNAL). ]*/
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel->channel_internal, NULL);
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CHANNEL), channel_create, THANDLE(PTR(LOG_CONTEXT_HANDLE)), log_context, THANDLE(THREADPOOL), threadpool)
{
    THANDLE(CHANNEL) result = NULL;
    /* Codes_SRS_CHANNEL_43_077: [ If threadpool is NULL, channel_create shall fail and return NULL. ] */
    if (threadpool == NULL)
    {
        LogError("Invalid arguments: THANDLE(PTR(LOG_CONTEXT_HANDLE)) log_context=%p, THANDLE(THREADPOOL) threadpool=%p", log_context, threadpool);
    }
    else
    {
        /*Codes_SRS_CHANNEL_43_078: [ channel_create shall create a CHANNEL_INTERNAL object by calling THANDLE_MALLOC with channel_internal_dispose as dispose.]*/
        THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create(log_context, threadpool);
        if (channel_internal == NULL)
        {
            /*Codes_SRS_CHANNEL_43_002: [ If there are any failures, channel_create shall fail and return NULL. ]*/
            LogError("Failure in channel_internal_create(log_context=%p, threadpool=%p)", log_context, threadpool);
        }
        else
        {
            /*Codes_SRS_CHANNEL_43_001: [ channel_create shall create a CHANNEL object by calling THANDLE_MALLOC with channel_dispose as dispose. ]*/
            THANDLE(CHANNEL) channel = THANDLE_MALLOC(CHANNEL)(channel_dispose);
            if (channel == NULL)
            {
                /*Codes_SRS_CHANNEL_43_002: [ If there are any failures, channel_create shall fail and return NULL. ]*/
                LogError("Failure in THANDLE_MALLOC(CHANNEL)(channel_dispose=%p)", channel_dispose);
            }
            else
            {
                CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);
                /*Codes_SRS_CHANNEL_43_079: [ channel_create shall store the created THANDLE(CHANNEL_INTERNAL) in the THANDLE(CHANNEL). ]*/
                THANDLE_INITIALIZE_MOVE(CHANNEL_INTERNAL)(&channel_ptr->channel_internal, &channel_internal);

                /*Codes_SRS_CHANNEL_43_086: [ channel_create shall succeed and return the created THANDLE(CHANNEL). ]*/
                THANDLE_INITIALIZE_MOVE(CHANNEL)(&result, &channel);
                goto all_ok;
            }
            THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
        }
        THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
    }
all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, channel_open, THANDLE(CHANNEL), channel)
{
    int result;
    /*Codes_SRS_CHANNEL_43_095: [If channel is NULL, channel_open shall fail and return a non - zero value.]*/
    if (channel == NULL)
    {
        LogError("Invalid arguments: THANDLE(CHANNEL) channel=%p", channel);
        result = MU_FAILURE;
    }
    else
    {
        CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);
        /*Codes_SRS_CHANNEL_43_096 : [channel_open shall call channel_internal_open.]*/
        if (channel_internal_open(channel_ptr->channel_internal) != 0)
        {
            /*Codes_SRS_CHANNEL_43_099: [ If there are any failures, channel_open shall fail and return a non-zero value. ]*/
            LogError("Failure in channel_internal_open(channel_internal=%p)", channel_ptr->channel_internal);
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, channel_close, THANDLE(CHANNEL), channel)
{
    /*Codes_SRS_CHANNEL_43_097: [If channel is NULL, channel_close shall return immediately.]*/
    if (channel == NULL)
    {
        LogError("Invalid arguments: THANDLE(CHANNEL) channel=%p", channel);
    }
    else
    {
        CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);
        /*Codes_SRS_CHANNEL_43_098: [ channel_close shall call channel_internal_close. ]*/
        channel_internal_close(channel_ptr->channel_internal);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_pull, THANDLE(CHANNEL), channel, THANDLE(RC_STRING), correlation_id,ON_DATA_AVAILABLE_CB, on_data_available_cb, void*, pull_context, THANDLE(ASYNC_OP)*, out_op_pull)
{
    CHANNEL_RESULT result;

    /*Codes_SRS_CHANNEL_43_007: [ If channel is NULL, channel_pull shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
    /*Codes_SRS_CHANNEL_43_008: [ If on_data_available_cb is NULL, channel_pull shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
    /*Codes_SRS_CHANNEL_43_009: [ If out_op_pull is NULL, channel_pull shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
    if (channel == NULL ||
        on_data_available_cb == NULL ||
        out_op_pull == NULL
        )
    {
        LogError("Invalid arguments: THANDLE(CHANNEL) channel=%p, THANDLE(RC_STRING) correlation_id=%" PRI_RC_STRING ", ON_DATA_AVAILABLE_CB on_data_available_cb=%p, void* pull_context=%p, THANDLE(ASYNC_OP)* out_op_pull=%p", channel, RC_STRING_VALUE_OR_NULL(correlation_id),  on_data_available_cb, pull_context, out_op_pull);
        result = CHANNEL_RESULT_INVALID_ARGS;
    }
    else
    {
        CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);

        /*Codes_SRS_CHANNEL_43_011: [ channel_pull shall call channel_internal_pull and return as it returns. ]*/
        result = channel_internal_pull(channel_ptr->channel_internal, correlation_id, on_data_available_cb, pull_context, out_op_pull);
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_push, THANDLE(CHANNEL), channel, THANDLE(RC_STRING), correlation_id, THANDLE(RC_PTR), data, ON_DATA_CONSUMED_CB, on_data_consumed_cb, void*, push_context, THANDLE(ASYNC_OP)*, out_op_push)
{
    CHANNEL_RESULT result;

    /*Codes_SRS_CHANNEL_43_024: [ If channel is NULL, channel_push shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
    /*Codes_SRS_CHANNEL_43_025: [ If on_data_consumed_cb is NULL, channel_push shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
    /*Codes_SRS_CHANNEL_43_026: [ If out_op_push is NULL, channel_push shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
    if (channel == NULL ||
        on_data_consumed_cb == NULL ||
        out_op_push == NULL
        )
    {
        LogError("Invalid arguments: THANDLE(CHANNEL) channel=%p, THANDLE(RC_STRING) correlation_id=%" PRI_RC_STRING ", THANDLE(RC_PTR) data=%p, ON_DATA_CONSUMED_CB on_data_consumed_cb=%p, void* push_context=%p, THANDLE(ASYNC_OP)* out_op_push=%p", channel, RC_STRING_VALUE_OR_NULL(correlation_id), data, on_data_consumed_cb, push_context, out_op_push);
        result = CHANNEL_RESULT_INVALID_ARGS;
    }
    else
    {
        CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);

        /*Codes_SRS_CHANNEL_43_041: [ channel_push shall call channel_internal_push and return as it returns. ]*/
        result = channel_internal_push(channel_ptr->channel_internal, correlation_id, data, on_data_consumed_cb, push_context, out_op_push);
    }
    return result;
}
