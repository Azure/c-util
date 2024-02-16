// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/thandle.h"
#include "c_pal/srw_lock.h"
#include "c_pal/threadpool.h"

#include "c_util/doublylinkedlist.h"
#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"

#include "c_util/channel_internal.h"

#include "c_util/channel.h"

typedef struct CHANNEL_TAG
{
    THANDLE(CHANNEL_INTERNAL) channel_internal;
} CHANNEL;

THANDLE_TYPE_DEFINE(CHANNEL);

static void channel_dispose(CHANNEL* channel)
{
    /*Codes_SRS_CHANNEL_43_094: [ channel_dispose shall call channel_internal_close. ]*/
    channel_internal_close(channel->channel_internal);
    /*Codes_SRS_CHANNEL_43_092: [ channel_dispose shall release the reference to THANDLE(CHANNEL_INTERNAL). ]*/
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel->channel_internal, NULL);
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CHANNEL), channel_create, THANDLE(THREADPOOL), threadpool)
{
    THANDLE(CHANNEL) result = NULL;
    /* Codes_SRS_CHANNEL_43_077: [ If threadpool is NULL, channel_create shall fail and return NULL. ] */
    if (threadpool == NULL)
    {
        LogError("Invalid arguments: THANDLE(THREADPOOL) threadpool=%p", threadpool);
    }
    else
    {
        /*Codes_SRS_CHANNEL_43_078: [ channel_create shall create a CHANNEL_INTERNAL object by calling THANDLE_MALLOC with channel_internal_dispose as dispose.]*/
        THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create_and_open(threadpool);
        if (channel_internal == NULL)
        {
            /*Codes_SRS_CHANNEL_43_002: [ If there are any failures, channel_create shall fail and return NULL. ]*/
            LogError("Failure in channel_internal_create_and_open(threadpool=%p)", threadpool);
        }
        else
        {
            /*Codes_SRS_CHANNEL_43_001: [ channel_create shall create a CHANNEL object by calling THANDLE_MALLOC with channel_dispose as dispose. ]*/
            THANDLE(CHANNEL) channel = THANDLE_MALLOC(CHANNEL)(channel_dispose);
            if (channel == NULL)
            {
                /*Codes_SRS_CHANNEL_43_002: [ If there are any failures, channel_create shall fail and return NULL. ]*/
                LogError("Failure in THANDLE_MALLOC(CHANNEL)(channel_dispose)");
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

IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_pull, THANDLE(CHANNEL), channel, PULL_CALLBACK, pull_callback, void*, pull_context, THANDLE(ASYNC_OP)*, out_op_pull)
{
    CHANNEL_RESULT result;

    /*Codes_SRS_CHANNEL_43_007: [ If channel is NULL, channel_pull shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
    /*Codes_SRS_CHANNEL_43_008: [ If pull_callback is NULL, channel_pull shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
    /*Codes_SRS_CHANNEL_43_009: [ If out_op_pull is NULL, channel_pull shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
    if (channel == NULL ||
        pull_callback == NULL ||
        out_op_pull == NULL
        )
    {
        LogError("Invalid arguments: THANDLE(CHANNEL) channel=%p, PULL_CALLBACK pull_callback=%p, void* pull_context=%p, THANDLE(ASYNC_OP)* out_op_pull=%p",
                   channel, pull_callback, pull_context, out_op_pull);
        result = CHANNEL_RESULT_INVALID_ARGS;
    }
    else
    {
        CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);

        /*Codes_SRS_CHANNEL_43_011: [ channel_pull shall call channel_internal_pull and return as it returns. ]*/
        result = channel_internal_pull(channel_ptr->channel_internal, pull_callback, pull_context, out_op_pull);
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_push, THANDLE(CHANNEL), channel, THANDLE(RC_PTR), data, PUSH_CALLBACK, push_callback, void*, push_context, THANDLE(ASYNC_OP)*, out_op_push)
{
    CHANNEL_RESULT result;

    /*Codes_SRS_CHANNEL_43_024: [ If channel is NULL, channel_push shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
    /*Codes_SRS_CHANNEL_43_025: [ If push_callback is NULL, channel_push shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
    /*Codes_SRS_CHANNEL_43_026: [ If out_op_push is NULL, channel_push shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
    if (channel == NULL ||
        push_callback == NULL ||
        out_op_push == NULL
        )
    {
        LogError("Invalid arguments: THANDLE(CHANNEL) channel=%p, THANDLE(RC_PTR) data = %p, PUSH_CALLBACK push_callback=%p, void* push_context=%p, THANDLE(ASYNC_OP)* out_op_push=%p", channel, data, push_callback, push_context, out_op_push);
        result = CHANNEL_RESULT_INVALID_ARGS;
    }
    else
    {
        CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);

        /*Codes_SRS_CHANNEL_43_041: [ channel_push shall call channel_internal_push and return as it returns. ]*/
        result = channel_internal_push(channel_ptr->channel_internal, data, push_callback, push_context, out_op_push);
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, channel_reset, THANDLE(CHANNEL), channel)
{
    /*Codes_SRS_CHANNEL_43_087: [If channel is NULL, channel_reset shall return.]*/
    if(channel == NULL)
    {
        LogError("Invalid arguments: THANDLE(CHANNEL) channel=%p", channel);
    }
    else
    {
        CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);
        /*Codes_SRS_CHANNEL_43_095: [channel_reset shall call channel_internal_close.]*/
        channel_internal_close(channel_ptr->channel_internal);
    }
}