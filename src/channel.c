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

#include "c_util/doublylinkedlist.h"
#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"

#include "c_util/channel.h"

#define CHANNEL_STATE_VALUES \
    CHANNEL_EMPTY, \
    CHANNEL_LOCKED, \
    CHANNEL_PUSHED, \
    CHANNEL_PULLED

MU_DEFINE_ENUM(CHANNEL_STATE, CHANNEL_STATE_VALUES)

#define CHANNEL_OP_STATE_VALUES \
    CHANNEL_OP_CREATED, \
    CHANNEL_OP_QUEUED, \
    CHANNEL_OP_PULLED, \
    CHANNEL_OP_PULL_CB_CALLED, \
    CHANNEL_OP_PUSH_CB_CALLED, \
    CHANNEL_OP_CANCELLED

MU_DEFINE_ENUM(CHANNEL_OP_STATE, CHANNEL_OP_STATE_VALUES)

typedef struct CHANNEL_OP_TAG
{
    THANDLE(ASYNC_OP) async_op;
    PUSH_CALLBACK push_callback;
    PULL_CALLBACK pull_callback;
    void* push_context;
    void* pull_context;
    THANDLE(RC_PTR) data;
    DLIST_ENTRY anchor;

    volatile_atomic int32_t state;
}CHANNEL_OP;

typedef struct CHANNEL_TAG
{
    THANDLE(THREADPOOL) threadpool;
    volatile_atomic int32_t state;
    DLIST_ENTRY op_list;
    SM_HANDLE sm;
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
        SM_RESULT sm_close_begin_result = sm_close_begin(channel->sm);
        if (sm_close_begin_result != SM_EXEC_GRANTED)
        {
            LogError("Failure in sm_close_begin(channel->sm): SM_RESULT sm_close_begin_result=%" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_RESULT, sm_close_begin_result));
        }
        else
        {
            THANDLE_ASSIGN(THREADPOOL)(&channel->threadpool, NULL);
            sm_close_end(channel->sm);
            sm_destroy(channel->sm);
        }
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
            channel_ptr->sm = sm_create("channel");
            if (channel_ptr->sm == NULL)
            {
                LogError("Failure in sm_create(\"channel\")");
            }
            else
            {
                SM_RESULT sm_open_begin_result = sm_open_begin(channel->sm);
                if (sm_open_begin_result != SM_EXEC_GRANTED)
                {
                    LogError("Failure in sm_open_begin(channel->sm): SM_RESULT sm_open_begin_result=%" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_RESULT, sm_open_begin_result));
                }
                else
                {
                    sm_open_end(channel_ptr->sm, true);
                    THANDLE_INITIALIZE(THREADPOOL)(&channel_ptr->threadpool, threadpool);
                    DList_InitializeListHead(&channel_ptr->op_list);
                    (void)interlocked_exchange(&channel_ptr->state, CHANNEL_EMPTY);
                    THANDLE_INITIALIZE_MOVE(CHANNEL)(&result, &channel);
                    goto all_ok;
                }
            }
            sm_destroy(channel_ptr->sm);
        }
        THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
    }
all_ok:
    return result;
}


static void pull_work_function(void* context)
{
    PULL_WORK* pull_work = (PULL_WORK*)context;
    pull_work->pull_callback(pull_work->pull_context, pull_work->data, CHANNEL_CALLBACK_RESULT_OK);
    THANDLE_ASSIGN(RC_PTR)(&pull_work->data, NULL);
    sm_exec_end(pull_work->sm);
    free(pull_work);
}

static int schedule_pull_callback(PULL_CALLBACK pull_callback, void* pull_context, THANDLE(RC_PTR) data, THANDLE(CHANNEL) channel)
{
    int result;
    CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);

    SM_RESULT sm_exec_begin_result = sm_exec_begin(channel_ptr->sm);
    if (sm_exec_begin_result != SM_EXEC_GRANTED)
    {
        LogError("Failure in sm_exec_begin(channel_ptr->sm): SM_RESULT sm_exec_begin_result=%" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_RESULT, sm_exec_begin_result));
        result = MU_FAILURE;
    }
    else
    {
        PULL_WORK* pull_work = malloc(sizeof(PULL_WORK));
        if (pull_work == NULL)
        {
            LogError("Failure in malloc(sizeof(PULL_WORK))");
            result = MU_FAILURE;
        }
        else
        {
            pull_work->pull_callback = pull_callback;
            pull_work->pull_context = pull_context;
            THANDLE_INITIALIZE(RC_PTR)(&pull_work->data, data);
            pull_work->sm = channel_ptr->sm;

            if (threadpool_schedule_work(channel_ptr->threadpool, pull_work_function, pull_work) != 0)
            {
                LogError("Failure in threadpool_schedule_work(channel_ptr->threadpool, pull_work_function, pull_work)");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
                goto all_ok;
            }
            THANDLE_ASSIGN(RC_PTR)(&pull_work->data, NULL);
        }
        free(pull_work);
    }
    sm_exec_end(channel_ptr->sm);

all_ok:
    return result;
}

static void push_work_function(void* context)
{
    PUSH_WORK* push_work = (PUSH_WORK*)context;
    push_work->push_callback(push_work->push_context, CHANNEL_CALLBACK_RESULT_OK);
    sm_exec_end(push_work->sm);
    free(push_work);
}

static int schedule_push_callback(PUSH_CALLBACK push_callback, void* push_context, THANDLE(CHANNEL) channel)
{
    int result;
    CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);

    SM_RESULT sm_exec_begin_result = sm_exec_begin(channel_ptr->sm);
    if (sm_exec_begin_result != SM_EXEC_GRANTED)
    {
        LogError("Failure in sm_exec_begin(channel_ptr->sm): SM_RESULT sm_exec_begin_result=%" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_RESULT, sm_exec_begin_result));
        result = MU_FAILURE;
    }
    else
    {
        PUSH_WORK* push_work = malloc(sizeof(PUSH_WORK));
        if (push_work == NULL)
        {
            LogError("Failure in malloc(sizeof(PUSH_WORK))");
            result = MU_FAILURE;
        }
        else
        {
            push_work->push_callback = push_callback;
            push_work->context = push_context;
            push_work->sm = channel_ptr->sm;

            if (threadpool_schedule_work(channel_ptr->threadpool, push_work_function, push_work) != 0)
            {
                LogError("Failure in threadpool_schedule_work(channel_ptr->threadpool, push_work_function, push_work)");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
                goto all_ok;
            }
        }
        free(push_work);
    }
    sm_exec_end(channel_ptr->sm);

all_ok:
    return result;
}

static int schedule_callbacks(PULL_CALLBACK pull_callback, void* pull_context, PUSH_CALLBACK push_callback, void* push_context, THANDLE(RC_PTR) data, THANDLE(CHANNEL) channel)
{
    int result;
    if (schedule_pull_callback(pull_callback, pull_context, data, channel) != 0)
    {
        LogError("Failure in schedule_pull_callback(pull_callback, pull_context, data, channel)");
        result = MU_FAILURE;
    }
    else
    {
        if (schedule_push_callback(push_callback, push_context, channel) != 0)
        {
            LogError("Failure in schedule_push_callback(push_callback, push_context, channel)");
            result = MU_FAILURE;
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

static void cancel_channel_op(void* context)
{
    (void)context;
}

static void dispose_channel_op(void* context)
{
    (void)context;
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
        do
        {
            CHANNEL_STATE state = interlocked_add(&channel_ptr->state, 0);
            if (
                state == CHANNEL_PULLED ||
                state == CHANNEL_EMPTY
               )
            {
                THANDLE(ASYNC_OP) async_op = async_op_create(cancel_channel_op, sizeof(CHANNEL_OP), alignof(CHANNEL_OP), dispose_channel_op);
                if(async_op == NULL)
                {
                    LogError("Failure in async_op_create(cancel_channel_op, sizeof(CHANNEL_OP), alignof(CHANNEL_OP), dispose_channel_op)");
                    result = CHANNEL_RESULT_ERROR;
                    break;
                }
                else
                {
                    CHANNEL_OP* channel_op = async_op->context;
                    channel_op->pull_callback = pull_callback;
                    channel_op->pull_context = pull_context;
                    (void)interlocked_exchange(&channel_op->state, CHANNEL_OP_CREATED);
                    THANDLE_INITIALIZE(ASYNC_OP)(&channel_op->async_op, async_op); // obtain circular reference
                    if (interlocked_compare_exchange(&channel_ptr->state, CHANNEL_LOCKED, state) != state)
                    {
                        /* state changed by another thread, try again */
                        THANDLE_ASSIGN(ASYNC_OP)(&channel_op->async_op, NULL); // release circular reference
                        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
                    }
                    else
                    {
                        DList_InsertTailList(&channel_ptr->op_list, &channel_op->anchor);
                        (void)interlocked_exchange(&channel_ptr->state, CHANNEL_PULLED);
                        wake_by_address_single(&channel_ptr->state);
                        THANDLE_INITIALIZE_MOVE(ASYNC_OP)(out_op_pull, &async_op);
                        result = CHANNEL_RESULT_WILL_COMPLETE_ASYNC;
                        break;
                    }
                }
            }
            else if (state == CHANNEL_PUSHED)
            {
                if (interlocked_compare_exchange(&channel_ptr->state, CHANNEL_LOCKED, state) != state)
                {
                    /* state changed by another thread, try again */
                }
                else
                {
                    do
                    {
                        if (push_op_entry == NULL)
                        {
                            LogError("Failure in DList_RemoveHeadList(&channel_ptr->op_list)");
                        }
                        else
                        {
                            CHANNEL_OP* channel_op = CONTAINING_RECORD(push_op_entry, CHANNEL_OP, anchor);
                            interlocked_compinterlocked_exchange(&push_op->done, 1);
                            if (DList_IsListEmpty(&channel_ptr->op_list))
                            {
                                (void)interlocked_exchange(&channel_ptr->state, CHANNEL_EMPTY);
                            }
                            else
                            {
                                (void)interlocked_exchange(&channel_ptr->state, CHANNEL_PUSHED);
                            }
                            (void)wake_by_address_single(&channel_ptr->state);


                            if (schedule_callbacks(pull_callback, pull_context, push_op->push_callback, push_op->push_context, push_op->data, channel) != 0)
                            {
                                LogError("Failure in schedule_callbacks(pull_callback, pull_context, push_op->push_callback, push_op->push_context, push_op->data, channel)");
                                result = CHANNEL_RESULT_ERROR;
                            }
                            else
                            {
                                THANDLE_INITIALIZE(ASYNC_OP)(out_op_pull, NULL);
                                result = CHANNEL_RESULT_COMPLETED_SYNC;
                            }

                            THANDLE(ASYNC_OP) temp = NULL;
                            THANDLE_INITIALIZE_MOVE(ASYNC_OP)(&temp, &push_op->async_op);
                            THANDLE_ASSIGN(ASYNC_OP)(&temp, NULL);

                            break;
                        }
                    } while(1);
                    break;
                }
            }
            else
            {
                /* state is CHANNEL_LOCKED, wait for it to change */
                if (InterlockedHL_WaitForNotValue(&channel_ptr->state, state, UINT32_MAX) != INTERLOCKED_HL_OK)
                {
                    LogError("Failure in InterlockedHL_WaitForNotValue(&channel_ptr->state, state, UINT32_MAX)");
                }
                else
                {
                    /* state has changed, try again */
                }
            }
        } while(1);
    }
}

static void cancel_push(void* context)
{
    (void)context;
}

static void dispose_push(void* context)
{
    (void)context;
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
        do
        {
            CHANNEL_STATE state = interlocked_add(&channel_ptr->state, 0);
            if (
                state == CHANNEL_PUSHED ||
                state == CHANNEL_EMPTY
                )
            {
                THANDLE(ASYNC_OP) async_op = async_op_create(cancel_push, sizeof(PUSH_OP), alignof(PUSH_OP), dispose_push);
                if (async_op == NULL)
                {
                    LogError("Failure in async_op_create(cancel_push, sizeof(PUSH_OP), alignof(PUSH_OP), dispose_push)");
                    result = CHANNEL_RESULT_ERROR;
                    break;
                }
                else
                {
                    PUSH_OP* push_op = async_op->context;
                    push_op->push_callback = push_callback;
                    push_op->push_context = push_context;
                    (void)interlocked_exchange(&push_op->done, 0);
                    THANDLE_INITIALIZE(RC_PTR)(&push_op->data, data);
                    THANDLE_INITIALIZE(ASYNC_OP)(&push_op->async_op, async_op);
                    if (interlocked_compare_exchange(&channel_ptr->state, CHANNEL_LOCKED, state) != state)
                    {
                        /* state changed by another thread, try again */
                        THANDLE_ASSIGN(RC_PTR)(&push_op->data, NULL);
                        THANDLE_ASSIGN(ASYNC_OP)(&push_op->async_op, NULL);
                        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
                    }
                    else
                    {
                        DList_InsertTailList(&channel_ptr->op_list, &push_op->anchor);
                        (void)interlocked_exchange(&channel_ptr->state, CHANNEL_PUSHED);
                        wake_by_address_single(&channel_ptr->state);
                        THANDLE_INITIALIZE_MOVE(ASYNC_OP)(out_op_push, &async_op);
                        result = CHANNEL_RESULT_WILL_COMPLETE_ASYNC;
                        break;
                    }
                }
            }
            else if (state == CHANNEL_PULLED)
            {
                if (interlocked_compare_exchange(&channel_ptr->state, CHANNEL_LOCKED, state) != state)
                {
                    /* state changed by another thread, try again */
                }
                else
                {
                    PDLIST_ENTRY pull_op_entry = DList_RemoveHeadList(&channel_ptr->op_list);
                    if (pull_op_entry == NULL)
                    {
                        LogError("Failure in DList_RemoveHeadList(&channel_ptr->op_list)");
                    }
                    else
                    {
                        PULL_OP* pull_op = CONTAINING_RECORD(pull_op_entry, PULL_OP, anchor);
                        (void)interlocked_exchange(&pull_op->done, 1);
                        if (DList_IsListEmpty(&channel_ptr->op_list))
                        {
                            (void)interlocked_exchange(&channel_ptr->state, CHANNEL_EMPTY);
                        }
                        else
                        {
                            (void)interlocked_exchange(&channel_ptr->state, CHANNEL_PULLED);
                        }
                        (void)wake_by_address_single(&channel_ptr->state);


                        if (schedule_callbacks(pull_op->pull_callback, pull_op->pull_context, push_callback, push_context, data, channel) != 0)
                        {
                            LogError("Failure in schedule_callbacks(pull_op->pull_callback, pull_op->pull_context, push_callback, push_context, data, channel)");
                            result = CHANNEL_RESULT_ERROR;
                        }
                        else
                        {
                            THANDLE_INITIALIZE(ASYNC_OP)(out_op_push, NULL);
                            result = CHANNEL_RESULT_COMPLETED_SYNC;
                        }

                        THANDLE(ASYNC_OP) temp = NULL;
                        THANDLE_INITIALIZE_MOVE(ASYNC_OP)(&temp, &pull_op->async_op);
                        THANDLE_ASSIGN(ASYNC_OP)(&temp, NULL);

                        break;
                    }
                }
            }
            else
            {
                /* state is CHANNEL_LOCKED, wait for it to change */
                if (InterlockedHL_WaitForNotValue(&channel_ptr->state, state, UINT32_MAX) != INTERLOCKED_HL_OK)
                {
                    LogError("Failure in InterlockedHL_WaitForNotValue(&channel_ptr->state, state, UINT32_MAX)");
                }
                else
                {
                    /* state has changed, try again */
                }
            }
        } while (1);
    }
}
