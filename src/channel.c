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
#include "c_pal/interlocked_hl.h"
#include "c_pal/sync.h"
#include "c_pal/threadpool.h"
#include "c_pal/sm.h"
#include "c_pal/srw_lock.h"

#include "c_util/doublylinkedlist.h"
#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"

#include "c_util/channel.h"

#define CHANNEL_STATE_VALUES \
    CHANNEL_EMPTY, \
    CHANNEL_PUSHED, \
    CHANNEL_PULLED

MU_DEFINE_ENUM(CHANNEL_STATE, CHANNEL_STATE_VALUES)

#define CHANNEL_OP_STATE_VALUES \
    CHANNEL_OP_CREATED, \
    CHANNEL_OP_SCHEDULED, \
    CHANNEL_OP_CANCELLED, \
    CHANNEL_OP_ABANDONED

MU_DEFINE_ENUM(CHANNEL_OP_STATE, CHANNEL_OP_STATE_VALUES)
MU_DEFINE_ENUM_STRINGS(CHANNEL_OP_STATE, CHANNEL_OP_STATE_VALUES)

typedef struct CHANNEL_INTERNAL_TAG
{
    THANDLE(THREADPOOL) threadpool;
    int32_t state;
    SRW_LOCK_HANDLE lock;
    DLIST_ENTRY op_list;
    SM_HANDLE sm;
}CHANNEL_INTERNAL;

THANDLE_TYPE_DECLARE(CHANNEL_INTERNAL);
THANDLE_TYPE_DEFINE(CHANNEL_INTERNAL);

typedef struct CHANNEL_OP_TAG
{
    PUSH_CALLBACK push_callback;
    PULL_CALLBACK pull_callback;
    void* push_context;
    void* pull_context;
    THANDLE(RC_PTR) data;
    DLIST_ENTRY anchor;

    THANDLE(CHANNEL_INTERNAL) channel_internal;
    THANDLE(ASYNC_OP) async_op; // self reference to keep op alive even after user disposes (watch out for deadlocks)

    int32_t state;
}CHANNEL_OP;

typedef struct CHANNEL_TAG
{
    THANDLE(CHANNEL_INTERNAL) channel_internal;
}CHANNEL;

THANDLE_TYPE_DEFINE(CHANNEL);

static void execute_callbacks(void* context);

static void channel_internal_dispose(CHANNEL_INTERNAL* channel_internal)
{
    /* nothing to do here, actual cleanup is done in channel_close */
    (void)channel_internal;
}

static void channel_close(void* context)
{
    (void)context;
    // acquire lock over channel, call all callbacks with ABANDONED status
    // dispose reference to channel_internal
    CHANNEL* channel_ptr = (CHANNEL*)context;
    CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_ptr->channel_internal);

    srw_lock_acquire_exclusive(channel_internal_ptr->lock);
    /* channel is LOCKED */
    for (DLIST_ENTRY* entry = DList_RemoveHeadList(&channel_internal_ptr->op_list); entry != &channel_internal_ptr->op_list; entry = DList_RemoveHeadList(&channel_internal_ptr->op_list))
    {
        CHANNEL_OP* channel_op = CONTAINING_RECORD(entry, CHANNEL_OP, anchor);
        (void)interlocked_exchange(&channel_op->state, CHANNEL_OP_ABANDONED);
        if (threadpool_schedule_work(channel_internal_ptr->threadpool, execute_callbacks, channel_op) != 0)
        {
            LogError("Failure in threadpool_schedule_work(execute_callbacks, channel_op)");
        }
    }
    srw_lock_release_exclusive(channel_internal_ptr->lock);
}

static void channel_dispose(CHANNEL* channel)
{
    SM_RESULT sm_close_begin_result = sm_close_begin_with_cb(channel->channel_internal->sm, channel_close, channel, NULL, NULL);
    if (sm_close_begin_result != SM_EXEC_GRANTED)
    {
        LogError("Failure in sm_close_begin(channel->sm): SM_RESULT sm_close_begin_result=%" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_RESULT, sm_close_begin_result));
    }
    else
    {
        sm_close_end(channel->channel_internal->sm);
        sm_destroy(channel->channel_internal->sm);
        srw_lock_destroy(channel->channel_internal->lock);
        THANDLE_ASSIGN(THREADPOOL)(&channel->channel_internal->threadpool, NULL);
        THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel->channel_internal, NULL);
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
            THANDLE_INITIALIZE_MOVE(CHANNEL_INTERNAL)(&channel_ptr->channel_internal, &(THANDLE(CHANNEL_INTERNAL)){ THANDLE_MALLOC(CHANNEL_INTERNAL)(channel_internal_dispose) });
            if (channel_ptr->channel_internal == NULL)
            {
                LogError("Failure in THANDLE_MALLOC(CHANNEL_INTERNAL)(channel_internal_dispose)");
            }
            else
            {
                CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_ptr->channel_internal);
                THANDLE_INITIALIZE(THREADPOOL)(&channel_internal_ptr->threadpool, threadpool);

                channel_internal_ptr->lock = srw_lock_create(false, "channel");
                if (channel_internal_ptr->lock == NULL)
                {
                    LogError("Failure in srw_lock_create(false, \"channel\")");
                }
                else
                {
                    channel_internal_ptr->sm = sm_create("channel");
                    if (channel_internal_ptr->sm == NULL)
                    {
                        LogError("Failure in sm_create(\"channel\")");
                    }
                    else
                    {
                        SM_RESULT sm_open_begin_result = sm_open_begin(channel_internal_ptr->sm);
                        if (sm_open_begin_result != SM_EXEC_GRANTED)
                        {
                            LogError("Failure in sm_open_begin(channel_internal_ptr->sm): SM_RESULT sm_open_begin_result=%" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_RESULT, sm_open_begin_result));
                        }
                        else
                        {
                            sm_open_end(channel_internal_ptr->sm, true);
                            DList_InitializeListHead(&channel_internal_ptr->op_list);
                            channel_internal_ptr->state = CHANNEL_EMPTY;
                            THANDLE_INITIALIZE_MOVE(CHANNEL)(&result, &channel);
                            goto all_ok;
                        }
                    }
                    sm_destroy(channel_internal_ptr->sm);
                }
                srw_lock_destroy(channel_internal_ptr->lock);
                THANDLE_ASSIGN(THREADPOOL)(&channel_internal_ptr->threadpool, NULL);
            }
            THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_ptr->channel_internal, NULL);
        }
        THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
    }
all_ok:
    return result;
}

static void execute_callbacks(void* context)
{
    if (context == NULL)
    {
        LogError("Invalid arguments: void* context=%p", context);
    }
    else
    {
        CHANNEL_OP* channel_op = (CHANNEL_OP*)context;

        switch(channel_op->state)
        {
            case CHANNEL_OP_SCHEDULED:
            {
                channel_op->pull_callback(channel_op->pull_context, channel_op->data, CHANNEL_RESULT_OK);
                channel_op->push_callback(channel_op->push_context, CHANNEL_RESULT_OK);
                break;
            }
            case CHANNEL_OP_ABANDONED:
            case CHANNEL_OP_CANCELLED:
            {
                CHANNEL_CALLBACK_RESULT callback_result = (channel_op->state == CHANNEL_OP_ABANDONED) ? CHANNEL_CALLBACK_RESULT_ABANDONED : CHANNEL_CALLBACK_RESULT_CANCELLED;
                if (channel_op->pull_callback)
                {
                    channel_op->pull_callback(channel_op->pull_context, NULL, callback_result);
                }
                if (channel_op->push_callback)
                {
                    channel_op->push_callback(channel_op->push_context, callback_result);
                }
                break;
            }
            default:
            {
                LogError("Invalid channel_op->state=%" PRI_MU_ENUM "", MU_ENUM_VALUE(CHANNEL_OP_STATE, channel_op->state));
                break;
            }
        }
        sm_exec_end(channel_op->channel_internal->sm);

        THANDLE_ASSIGN(RC_PTR)(&channel_op->data, NULL);
        THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_op->channel_internal, NULL);

        // copy to local reference to avoid cutting the branch you are sitting on
        THANDLE(ASYNC_OP) temp = NULL;
        THANDLE_INITIALIZE_MOVE(ASYNC_OP)(&temp, &channel_op->async_op);
        THANDLE_ASSIGN(ASYNC_OP)(&temp, NULL);
    }
}

static void cancel_channel_op(void* context)
{
    CHANNEL_OP* channel_op = context;
    CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_op->channel_internal);

    srw_lock_acquire_exclusive(channel_internal_ptr->lock);
    {
        if (channel_op->state == CHANNEL_OP_CREATED)
        {
            channel_op->state = CHANNEL_OP_CANCELLED;

            if (DList_RemoveEntryList(&channel_op->anchor) != 0)
            {
                channel_internal_ptr->state = CHANNEL_EMPTY;
            }

            if (threadpool_schedule_work(channel_internal_ptr->threadpool, execute_callbacks, channel_op) != 0)
            {
                LogError("Failure in threadpool_schedule_work(execute_callbacks, channel_op)");
            }
        }
        else
        {
            /* too late to cancel */
            LogInfo("THANDLE(ASYNC_OP) async_op = %p cannot be cancelled anymore.", channel_op->async_op);
        }
    }
    srw_lock_release_exclusive(channel_internal_ptr->lock);
}

static void dispose_channel_op(void* context)
{
    /* don't need to do anything here because actual cleanup happens in execute_callbacks */
    (void)context;
}

static int enqueue_operation(CHANNEL_INTERNAL* channel_internal, THANDLE(ASYNC_OP)* out_op, PULL_CALLBACK pull_callback, void* pull_context, PUSH_CALLBACK push_callback, void* push_context, THANDLE(RC_PTR) data)
{
    int result;
    if (sm_exec_begin(channel_internal->sm) != SM_EXEC_GRANTED)
    {
        LogError("Failure in sm_exec_begin.");
        result = MU_FAILURE;
    }
    else
    {
        THANDLE(ASYNC_OP) async_op = async_op_create(cancel_channel_op, sizeof(CHANNEL_OP), alignof(CHANNEL_OP), dispose_channel_op);
        if (async_op == NULL)
        {
            LogError("Failure in async_op_create(cancel_channel_op, sizeof(CHANNEL_OP), alignof(CHANNEL_OP), dispose_channel_op)");
            result = MU_FAILURE;
        }
        else
        {
            CHANNEL_OP* channel_op = async_op->context;
            channel_op->pull_callback = pull_callback;
            channel_op->pull_context = pull_context;
            channel_op->push_callback = push_callback;
            channel_op->push_context = push_context;

            THANDLE_INITIALIZE(ASYNC_OP)(&channel_op->async_op, async_op);
            THANDLE_INITIALIZE(CHANNEL_INTERNAL)(&channel_op->channel_internal, channel_internal);
            THANDLE_INITIALIZE(RC_PTR)(&channel_op->data, data);
            channel_op->state = CHANNEL_OP_CREATED;

            DList_InsertTailList(&channel_internal->op_list, &channel_op->anchor);
            channel_internal->state = (pull_context != NULL) ? CHANNEL_PULLED : CHANNEL_PUSHED;
            THANDLE_INITIALIZE_MOVE(ASYNC_OP)(out_op, &async_op);
            result = 0;
            goto all_ok;
        }
    }
    sm_exec_end(channel_internal->sm);
all_ok:
    return result;
}

static int dequeue_operation(CHANNEL_INTERNAL* channel_internal, THANDLE(ASYNC_OP)* out_op, PULL_CALLBACK pull_callback, void* pull_context, PUSH_CALLBACK push_callback, void* push_context, THANDLE(RC_PTR) data)
{
    int result;

    PDLIST_ENTRY op_entry = DList_RemoveHeadList(&channel_internal->op_list);
    if (DList_IsListEmpty(&channel_internal->op_list))
    {
        channel_internal->state = CHANNEL_EMPTY;
    }

    CHANNEL_OP* channel_op = CONTAINING_RECORD(op_entry, CHANNEL_OP, anchor);
    channel_op->state = CHANNEL_OP_SCHEDULED;
    if (pull_callback)
    {
        channel_op->pull_callback = pull_callback;
        channel_op->pull_context = pull_context;
    }
    else if (push_callback)
    {
        channel_op->push_callback = push_callback;
        channel_op->push_context = push_context;
        THANDLE_ASSIGN(RC_PTR)(&channel_op->data, data);
    }

    if (threadpool_schedule_work(channel_internal->threadpool, execute_callbacks, channel_op) != 0)
    {
        LogError("Failure in threadpool_schedule_work(execute_callbacks, channel_op)");
        result = MU_FAILURE;
    }
    else
    {
        THANDLE_INITIALIZE(ASYNC_OP)(out_op, channel_op->async_op);
        result = 0;
    }
    return result;
}


IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_pull, THANDLE(CHANNEL), channel, PULL_CALLBACK, pull_callback, void*, pull_context, THANDLE(ASYNC_OP)*, out_op_pull)
{
    CHANNEL_RESULT result;
    if (channel == NULL ||
        pull_callback == NULL ||
        pull_context == NULL ||
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
        CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_ptr->channel_internal);

        srw_lock_acquire_exclusive(channel_ptr->channel_internal->lock);
        {
            CHANNEL_STATE state = channel_internal_ptr->state;
            if (
                state == CHANNEL_PULLED ||
                state == CHANNEL_EMPTY
                )
            {
                if (enqueue_operation(channel_internal_ptr, out_op_pull, pull_callback, pull_context, NULL, NULL, NULL) != 0)
                {
                    LogError("Failure in enqueue_operation(channel_internal_ptr, out_op_pull, pull_callback, pull_context, NULL, NULL, NULL)");
                    result = CHANNEL_RESULT_ERROR;
                }
                else
                {
                    result = CHANNEL_RESULT_OK;
                    goto all_ok;
                }
            }
            else
            {
                if (dequeue_operation(channel_internal_ptr, out_op_pull, pull_callback, pull_context, NULL, NULL, NULL) != 0)
                {
                    LogError("Failure in dequeue_operation(channel_internal_ptr, out_op_pull, pull_callback, pull_context, NULL, NULL, NULL)");
                    result = CHANNEL_RESULT_ERROR;
                }
                else
                {
                    result = CHANNEL_RESULT_OK;
                    goto all_ok;
                }
            }
        }
all_ok:
        srw_lock_release_exclusive(channel->channel_internal->lock);
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_push, THANDLE(CHANNEL), channel, THANDLE(RC_PTR), data, PUSH_CALLBACK, push_callback, void*, push_context, THANDLE(ASYNC_OP)*, out_op_push)
{
    CHANNEL_RESULT result;
    if (channel == NULL ||
        data == NULL ||
        push_callback == NULL ||
        push_context == NULL ||
        out_op_push == NULL
        )
    {
        LogError("Invalid arguments: THANDLE(CHANNEL) channel=%p, THANDLE(RC_PTR) data = %p, PUSH_CALLBACK push_callback=%p, void* push_context=%p, THANDLE(ASYNC_OP)* out_op_push=%p", channel, data, push_callback, push_context, out_op_push);
        result = CHANNEL_RESULT_INVALID_ARGS;
    }
    else
    {
        CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);
        CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_ptr->channel_internal);

        srw_lock_acquire_exclusive(channel_ptr->channel_internal->lock);
        {
            CHANNEL_STATE state = channel_internal_ptr->state;
            if (
                state == CHANNEL_PUSHED ||
                state == CHANNEL_EMPTY
                )
            {
                if (enqueue_operation(channel_internal_ptr, out_op_push, NULL, NULL, push_callback, push_context, data) != 0)
                {
                    LogError("Failure in enqueue_operation(channel_internal_ptr, out_op_push, NULL, NULL, push_callback, push_context, data)");
                    result = CHANNEL_RESULT_ERROR;
                }
                else
                {
                    result = CHANNEL_RESULT_OK;
                    goto all_ok;
                }
            }
            else
            {
                if (dequeue_operation(channel_internal_ptr, out_op_push, NULL, NULL, push_callback, push_context, data) != 0)
                {
                    LogError("Failure in dequeue_operation(channel_internal_ptr, out_op_push, NULL, NULL, push_callback, push_context, data)");
                    result = CHANNEL_RESULT_ERROR;
                }
                else
                {
                    result = CHANNEL_RESULT_OK;
                    goto all_ok;
                }
            }
        }
    all_ok:
        srw_lock_release_exclusive(channel->channel_internal->lock);
    }
    return result;
}
