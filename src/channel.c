// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdalign.h>
#include <stdbool.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/thandle.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_pal/threadpool.h"
#include "c_pal/sm.h"
#include "c_pal/srw_lock.h"

#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"

#include "c_util/channel.h"

#define OP_STATE_VALUES \
    OP_READY, \
    OP_PUSHING, \
    OP_PUSHED, \
    OP_PULLING, \
    OP_PULLED

MU_DEFINE_ENUM(OP_STATE, OP_STATE_VALUES)

#define OP_EPOCH_INCREMENT (1 << 3)

#define OP_STATE_MASK ((1 << 3) - 1)

#define DEFAULT_OP_ARRAY_SIZE 2048

typedef struct OP_TAG{
    union
    {
        PULL_CALLBACK pull_callback;
        PUSH_CALLBACK push_callback;
    };
    void* context;
    THANDLE(RC_PTR) data;
    volatile_atomic int32_t state;
}OP;

typedef struct CHANNEL_TAG
{
    THANDLE(THREADPOOL) threadpool;
    OP* op_array;
    SRW_LOCK_HANDLE lock;
    SM_HANDLE sm;

    volatile_atomic int32_t push_slot;
    volatile_atomic int32_t pull_slot;
}CHANNEL;

THANDLE_TYPE_DEFINE(CHANNEL);

static void channel_dispose(CHANNEL* channel)
{
    if (channel == NULL)
    {
        LogError("Invalid arguments: CHANNEL* channel=%p", channel);
    }
    else
    {
        THANDLE_ASSIGN(THREADPOOL)(&channel->threadpool, NULL);
        sm_destroy(channel->sm);
        srw_lock_destroy(channel->lock);
        free(channel->op_array);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CHANNEL), channel_create, THANDLE(THREADPOOL), threadpool)
{
    THANDLE(CHANNEL) result = NULL;
    if (threadpool == NULL)
    {
        LogError("Invalid arguments: THANDLE(THREADPOOL) threadpool=%p", threadpool);
    }
    else
    {
        THANDLE(CHANNEL) channel = THANDLE_MALLOC(CHANNEL)(channel_dispose);
        if (channel == NULL)
        {
            LogError("Failure in THANDLE_MALLOC(CHANNEL)(channel_dispose)");
        }
        else
        {
            CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);
            channel_ptr->op_array = (OP*)malloc(sizeof(OP) * DEFAULT_OP_ARRAY_SIZE);
            if (channel_ptr->op_array == NULL)
            {
                LogError("Failure in malloc(sizeof(OP) * DEFAULT_OP_ARRAY_SIZE)");
            }
            else
            {
                channel_ptr->lock = srw_lock_create(false, "channel");
                if (channel_ptr->lock == NULL)
                {
                    LogError("Failure in srw_lock_create(false, \"channel\")");
                }
                else
                {
                    channel_ptr->sm = sm_create("channel");
                    if (channel_ptr->sm == NULL)
                    {
                        LogError("Failure in sm_create(\"channel\")");
                    }
                    else
                    {
                        THANDLE_INITIALIZE(THREADPOOL)(&channel_ptr->threadpool, threadpool);
                        (void)interlocked_exchange(&channel_ptr->push_slot, 0);
                        (void)interlocked_exchange(&channel_ptr->pull_slot, 0);
                        THANDLE_INITIALIZE(CHANNEL)(&result, channel);
                        goto all_ok;

                    }
                    sm_destroy(channel_ptr->sm);
                }
                srw_lock_destroy(channel_ptr->lock);
            }
            free(channel_ptr->op_array);
        }
        THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
    }
all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, channel_open, THANDLE(CHANNEL), channel)
{
    int result;

    if (channel == NULL)
    {
        LogError("Invalid arguments: THANDLE(CHANNEL) channel=%p", channel);
        result = MU_FAILURE;
    }
    else
    {
        CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);
        if (threadpool_open(channel_ptr->threadpool) != 0)
        {
            LogError("Failure in threadpool_open(channel_ptr->threadpool=%p)", channel_ptr->threadpool);
            result = MU_FAILURE;
        }
        else
        {
            if (sm_open_begin(channel_ptr->sm) != SM_EXEC_GRANTED)
            {
                LogError("Failure in sm_open(channel_ptr->sm)");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, channel_close, THANDLE(CHANNEL), channel)
{
    (void)channel;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_pull, THANDLE(CHANNEL), channel, PULL_CALLBACK, pull_callback, void*, pull_context, THANDLE(ASYNC_OP)*, out_op_pull)
{
    (void)channel;
    (void)pull_callback;
    (void)pull_context;
    (void)out_op_pull;
    return CHANNEL_RESULT_ERROR;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_push, THANDLE(CHANNEL), channel, THANDLE(RC_PTR), data, PUSH_CALLBACK, push_callback, void*, push_context, THANDLE(ASYNC_OP)*, out_op_push)
{
    (void)channel;
    (void)data;
    (void)push_callback;
    (void)push_context;
    (void)out_op_push;
    return CHANNEL_RESULT_ERROR;
}
