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
    CHANNEL_OP_SCHEDULED, \
    CHANNEL_OP_PULL_CB_CALLED, \
    CHANNEL_OP_ABANDONED

MU_DEFINE_ENUM(CHANNEL_OP_STATE, CHANNEL_OP_STATE_VALUES)

typedef struct CHANNEL_INTERNAL_TAG
{
    THANDLE(THREADPOOL) threadpool;
    volatile_atomic int32_t state;
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

    volatile_atomic int32_t state;
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

    do
    {
        int32_t state = interlocked_add(&channel_internal_ptr->state, 0);
        if (state != CHANNEL_LOCKED)
        {
            if (interlocked_compare_exchange(&channel_internal_ptr->state, CHANNEL_LOCKED, state) != state)
            {
                /* state changed by another thread, try again */
            }
            else
            {
                /* channel is LOCKED */
                for (DLIST_ENTRY* entry = DList_RemoveHeadList(&channel_internal_ptr->op_list); entry != &channel_internal_ptr->op_list;)
                {
                    CHANNEL_OP* channel_op = CONTAINING_RECORD(entry, CHANNEL_OP, anchor);
                    (void)interlocked_exchange(&channel_op->state, CHANNEL_OP_ABANDONED);
                    if (threadpool_schedule_work(channel_internal_ptr->threadpool, execute_callbacks, channel_op) != 0)
                    {
                        LogError("Failure in threadpool_schedule_work(execute_callbacks, channel_op)");
                    }
                }
            }
        }
        else
        {
            /* state is CHANNEL_LOCKED, wait for it to change */
            if (InterlockedHL_WaitForNotValue(&channel_internal_ptr->state, CHANNEL_LOCKED, UINT32_MAX) != INTERLOCKED_HL_OK)
            {
                LogError("Failure in InterlockedHL_WaitForNotValue(&channel_ptr->CHANNEL_LOCKED, state, UINT32_MAX)");
            }
            else
            {
                /* state has changed, try again */
            }
        }
    } while(1);
}

static void channel_dispose(CHANNEL* channel)
{
    if (channel == NULL)
    {
        LogError("Invalid arguments: CHANNEL* channel=%p", channel);
    }
    else
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
            THANDLE_ASSIGN(THREADPOOL)(&channel->channel_internal->threadpool, NULL);
            THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel->channel_internal, NULL);
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
            THANDLE_INITIALIZE_MOVE(CHANNEL_INTERNAL)(&channel_ptr->channel_internal, &(THANDLE(CHANNEL_INTERNAL)){ THANDLE_MALLOC(CHANNEL_INTERNAL)(channel_internal_dispose) });
            if (channel_ptr->channel_internal == NULL)
            {
                LogError("Failure in THANDLE_MALLOC(CHANNEL_INTERNAL)(channel_internal_dispose)");
            }
            else
            {
                CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_ptr->channel_internal);
                THANDLE_INITIALIZE(THREADPOOL)(&channel_internal_ptr->threadpool, threadpool);

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
                        (void)interlocked_exchange(&channel_internal_ptr->state, CHANNEL_EMPTY);
                        THANDLE_INITIALIZE_MOVE(CHANNEL)(&result, &channel);
                        goto all_ok;
                    }
                }
                sm_destroy(channel_internal_ptr->sm);
                THANDLE_ASSIGN(THREADPOOL)(&channel_internal_ptr->threadpool, NULL);
            }
            THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_ptr->channel_internal, NULL);
        }
        THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
    }
all_ok:
    return result;
}


static void channel_op_init(CHANNEL_OP* channel_op, THANDLE(ASYNC_OP) parent_async_op, THANDLE(CHANNEL_INTERNAL) channel_internal, PULL_CALLBACK pull_callback, void* pull_context, PUSH_CALLBACK push_callback, void* push_context, THANDLE(RC_PTR) data)
{
    channel_op->pull_callback = pull_callback;
    channel_op->pull_context = pull_context;

    channel_op->push_callback = push_callback;
    channel_op->push_context = push_context;

    THANDLE_INITIALIZE(ASYNC_OP)(&channel_op->async_op, parent_async_op);
    THANDLE_INITIALIZE(CHANNEL_INTERNAL)(&channel_op->channel_internal, channel_internal);
    THANDLE_INITIALIZE(RC_PTR)(&channel_op->data, data);
    (void)interlocked_exchange(&channel_op->state, CHANNEL_OP_CREATED);
}

static void channel_op_deinit(CHANNEL_OP* channel_op)
{
    THANDLE_ASSIGN(RC_PTR)(&channel_op->data, NULL);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_op->channel_internal, NULL);

    // copy to local reference to avoid cutting the branch you are sitting on
    THANDLE(ASYNC_OP) temp = NULL;
    THANDLE_INITIALIZE_MOVE(ASYNC_OP)(&temp, &channel_op->async_op);
    THANDLE_ASSIGN(ASYNC_OP)(&temp, NULL);

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

        do
        {
            int32_t state = interlocked_add(&channel_op->state, 0);
            switch(state)
            {
                case CHANNEL_OP_SCHEDULED:
                {
                    if (interlocked_compare_exchange(&channel_op->state, state, CHANNEL_OP_PULL_CB_CALLED) != state)
                    {
                        /* state changed by another thread, try again */
                    }
                    else
                    {
                        channel_op->pull_callback(channel_op->pull_context, channel_op->data, CHANNEL_RESULT_OK);
                    }
                    break;
                }
                case CHANNEL_OP_PULL_CB_CALLED:
                {
                    channel_op->push_callback(channel_op->push_context, CHANNEL_RESULT_OK);
                    channel_op_deinit(channel_op);
                    break;
                }
                case CHANNEL_OP_ABANDONED:
                {
                    if (channel_op->pull_callback)
                    {
                        channel_op->pull_callback(channel_op->pull_context, NULL, CHANNEL_CALLBACK_RESULT_ABANDONED);
                    }
                    if (channel_op->push_callback)
                    {
                        channel_op->push_callback(channel_op->push_context, CHANNEL_CALLBACK_RESULT_ABANDONED);
                    }
                    channel_op_deinit(channel_op);
                    break;
                }
                default:
                {
                    LogError("Invalid state: CHANNEL_OP_STATE state=%" PRI_MU_ENUM "", MU_ENUM_VALUE(CHANNEL_OP_STATE, state));
                    break;
                }
            }
        } while(1);
        sm_exec_end(channel_op->channel_internal->sm);
    }
}


static int schedule_callbacks(CHANNEL_INTERNAL* channel_internal, CHANNEL_OP* channel_op)
{
    int result;

    if (threadpool_schedule_work(channel_internal->threadpool, execute_callbacks, channel_op) != 0)
    {
        LogError("Failure in threadpool_schedule_work(execute_callbacks, channel_op)");
        result = MU_FAILURE;
    }
    else
    {
        if (threadpool_schedule_work(channel_internal->threadpool, execute_callbacks, channel_op) != 0)
        {
            LogError("Failure in threadpool_schedule_work(execute_callbacks, channel_op)");
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
    CHANNEL_OP* channel_op = context;
    CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_op->channel_internal);
    do
    {
        int32_t op_state = interlocked_add(&channel_op->state, 0);
        if (op_state == CHANNEL_OP_CREATED)
        {
              int32_t channel_state = interlocked_add(&channel_internal_ptr->state, 0);
              if (channel_state != CHANNEL_LOCKED)
              {
                  if (interlocked_compare_exchange(&channel_internal_ptr->state, CHANNEL_LOCKED, channel_state) != channel_state)
                  {
                      /* state changed by another thread, try again */
                  }
                  else
                  {
                      /* channel is locked, no other thread can change op_state*/
                      int32_t op_state_2 = interlocked_add(&channel_op->state, 0);
                      if (op_state != op_state_2)
                      {
                          /* state changed by another thread, try again */
                      }
                      else
                      {
                          /* remove from the list */
                          if (DList_RemoveEntryList(&channel_op->anchor) != 0)
                          {
                              (void)interlocked_exchange(&channel_internal_ptr->state, CHANNEL_EMPTY);
                          }
                          else
                          {
                              (void)interlocked_exchange(&channel_internal_ptr->state, channel_state);
                          }
                          (void)wake_by_address_single(&channel_internal_ptr->state);

                          /* clean up channel_op */
                          channel_op_deinit(channel_op);
                      }
                  }
              }
              else
              {
                  /* state is CHANNEL_LOCKED, wait for it to change */
                  if (InterlockedHL_WaitForNotValue(&channel_internal_ptr->state, CHANNEL_LOCKED, UINT32_MAX) != INTERLOCKED_HL_OK)
                  {
                      LogError("Failure in InterlockedHL_WaitForNotValue(&channel_ptr->CHANNEL_LOCKED, state, UINT32_MAX)");
                  }
                  else
                  {
                      /* state has changed, try again */
                  }
              }
        }
        else
        {
            /* too late to cancel */
            LogInfo("THANDLE(ASYNC_OP) async_op = %p cannot be cancelled anymore.", channel_op->async_op);
            break;
        }
    } while(1);
}

static void dispose_channel_op(void* context)
{
    /* don't need to do anything here because actual cleanup happens in channel_op_deinit */
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
        CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_ptr->channel_internal);
        if (sm_exec_begin(channel_internal_ptr->sm) != SM_EXEC_GRANTED)
        {
            LogError("Failure in sm_exec_begin.");
            result = CHANNEL_RESULT_ERROR;
        }
        else
        {
            do
            {
                CHANNEL_STATE state = interlocked_add(&channel_internal_ptr->state, 0);
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
                        channel_op_init(channel_op, async_op, channel_ptr->channel_internal, pull_callback, pull_context, NULL, NULL, NULL);

                        if (interlocked_compare_exchange(&channel_internal_ptr->state, CHANNEL_LOCKED, state) != state)
                        {
                            /* state changed by another thread, try again */
                            channel_op_deinit(channel_op);
                            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
                        }
                        else
                        {
                            DList_InsertTailList(&channel_internal_ptr->op_list, &channel_op->anchor);
                            (void)interlocked_exchange(&channel_internal_ptr->state, CHANNEL_PULLED);
                            wake_by_address_single(&channel_internal_ptr->state);
                            THANDLE_INITIALIZE_MOVE(ASYNC_OP)(out_op_pull, &async_op);
                            result = CHANNEL_RESULT_OK;
                            break;
                        }
                    }
                }
                else if (state == CHANNEL_PUSHED)
                {
                    if (interlocked_compare_exchange(&channel_internal_ptr->state, CHANNEL_LOCKED, state) != state)
                    {
                        /* state changed by another thread, try again */
                    }
                    else
                    {
                        PDLIST_ENTRY op_entry = DList_RemoveHeadList(&channel_internal_ptr->op_list);

                        if (DList_IsListEmpty(&channel_internal_ptr->op_list))
                        {
                            (void)interlocked_exchange(&channel_internal_ptr->state, CHANNEL_EMPTY);
                        }
                        else
                        {
                            (void)interlocked_exchange(&channel_internal_ptr->state, CHANNEL_PULLED);
                        }
                        CHANNEL_OP* channel_op = CONTAINING_RECORD(op_entry, CHANNEL_OP, anchor);
                        (void)interlocked_exchange(&channel_op->state, CHANNEL_OP_SCHEDULED);

                        (void)wake_by_address_single(&channel_internal_ptr->state);

                        channel_op->pull_callback = pull_callback;
                        channel_op->pull_context = pull_context;
                        THANDLE_INITIALIZE(RC_PTR)(&channel_op->data, NULL);

                        THANDLE_INITIALIZE(ASYNC_OP)(out_op_pull, channel_op->async_op);

                        if (schedule_callbacks(channel_internal_ptr, channel_op) != 0)
                        {
                            LogError("Failure in schedule_callbacks(channel_internal_ptr, channel_op)");
                            THANDLE_ASSIGN(ASYNC_OP)(out_op_pull, NULL);
                            channel_op_deinit(channel_op);
                            result = CHANNEL_RESULT_ERROR;
                            break;
                        }
                        else
                        {
                            result = CHANNEL_RESULT_OK;
                            goto all_ok;
                        }
                    }
                }
                else
                {
                    /* state is CHANNEL_LOCKED, wait for it to change */
                    if (InterlockedHL_WaitForNotValue(&channel_internal_ptr->state, state, UINT32_MAX) != INTERLOCKED_HL_OK)
                    {
                        LogError("Failure in InterlockedHL_WaitForNotValue(&channel_ptr->state, state, UINT32_MAX)");
                        break;
                    }
                    else
                    {
                        /* state has changed, try again */
                    }
                }
            } while(1);
        }
        sm_exec_end(channel_internal_ptr->sm);
    }
all_ok:
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

        if (sm_exec_begin(channel_internal_ptr->sm) != SM_EXEC_GRANTED)
        {
            LogError("Failure in sm_exec_begin.");
            result = CHANNEL_RESULT_ERROR;
        }
        else
        {
            do
            {
                CHANNEL_STATE state = interlocked_add(&channel_internal_ptr->state, 0);
                if (
                    state == CHANNEL_PUSHED ||
                    state == CHANNEL_EMPTY
                    )
                {
                    THANDLE(ASYNC_OP) async_op = async_op_create(cancel_channel_op, sizeof(CHANNEL_OP), alignof(CHANNEL_OP), dispose_channel_op);
                    if (async_op == NULL)
                    {
                        LogError("Failure in async_op_create(cancel_channel_op, sizeof(CHANNEL_OP), alignof(CHANNEL_OP), dispose_channel_op)");
                        result = CHANNEL_RESULT_ERROR;
                        break;
                    }
                    else
                    {
                        CHANNEL_OP* channel_op = async_op->context;

                        channel_op_init(channel_op, async_op, channel_ptr->channel_internal, NULL, NULL, push_callback, push_context, data);

                        if (interlocked_compare_exchange(&channel_internal_ptr->state, CHANNEL_LOCKED, state) != state)
                        {
                            /* state changed by another thread, try again */
                            channel_op_deinit(channel_op);
                            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
                        }
                        else
                        {
                            DList_InsertTailList(&channel_internal_ptr->op_list, &channel_op->anchor);
                            (void)interlocked_exchange(&channel_internal_ptr->state, CHANNEL_PULLED);
                            wake_by_address_single(&channel_internal_ptr->state);
                            THANDLE_INITIALIZE_MOVE(ASYNC_OP)(out_op_push, &async_op);
                            result = CHANNEL_RESULT_OK;
                            goto all_ok;
                        }
                    }
                }
                else if (state == CHANNEL_PULLED)
                {
                    if (interlocked_compare_exchange(&channel_internal_ptr->state, CHANNEL_LOCKED, state) != state)
                    {
                        /* state changed by another thread, try again */
                    }
                    else
                    {
                        PDLIST_ENTRY op_entry = DList_RemoveHeadList(&channel_internal_ptr->op_list);

                        if (DList_IsListEmpty(&channel_internal_ptr->op_list))
                        {
                            (void)interlocked_exchange(&channel_internal_ptr->state, CHANNEL_EMPTY);
                        }
                        else
                        {
                            (void)interlocked_exchange(&channel_internal_ptr->state, CHANNEL_PULLED);
                        }
                        CHANNEL_OP* channel_op = CONTAINING_RECORD(op_entry, CHANNEL_OP, anchor);
                        (void)interlocked_exchange(&channel_op->state, CHANNEL_OP_SCHEDULED);

                        (void)wake_by_address_single(&channel_internal_ptr->state);
                        channel_op->push_callback = push_callback;
                        channel_op->push_context = push_context;
                        THANDLE_INITIALIZE(RC_PTR)(&channel_op->data, data);

                        THANDLE_INITIALIZE(ASYNC_OP)(out_op_push, channel_op->async_op);

                        if (schedule_callbacks(channel_internal_ptr, channel_op) != 0)
                        {
                            LogError("Failure in schedule_callbacks(channel_internal_ptr, channel_op)");
                            THANDLE_ASSIGN(ASYNC_OP)(out_op_push, NULL);
                            channel_op_deinit(channel_op);
                            result = CHANNEL_RESULT_ERROR;
                            break;
                        }
                        else
                        {
                            result = CHANNEL_RESULT_OK;
                            goto all_ok;
                        }
                    }
                }
                else
                {
                    /* state is CHANNEL_LOCKED, wait for it to change */
                    if (InterlockedHL_WaitForNotValue(&channel_internal_ptr->state, state, UINT32_MAX) != INTERLOCKED_HL_OK)
                    {
                        LogError("Failure in InterlockedHL_WaitForNotValue(&channel_ptr->state, state, UINT32_MAX)");
                        break;
                    }
                    else
                    {
                        /* state has changed, try again */
                    }
                }
            } while (1);
        }
        sm_exec_end(channel_internal_ptr->sm);
    }
all_ok:
    return result;
}
