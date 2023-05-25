// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdalign.h>
#include <stdbool.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/thandle.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"

#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"

#include "c_util/channel.h"

#define CHANNEL_STATE_VALUES \
    CHANNEL_READY, \
    CHANNEL_PUSHING, \
    CHANNEL_PUSHED, \
    CHANNEL_PULLING, \
    CHANNEL_PULLED

MU_DEFINE_ENUM(CHANNEL_STATE, CHANNEL_STATE_VALUES)

#define CHANNEL_EPOCH_INCREMENT (1 << 3)

#define CHANNEL_STATE_MASK ((1 << 3) - 1)

struct CHANNEL_TAG{
    union
    {
        PULL_CALLBACK pull_callback;
        PUSH_CALLBACK push_callback;
    };
    void* context;
    THANDLE(RC_PTR) data;
    volatile_atomic int32_t state;
};

THANDLE_TYPE_DEFINE(CHANNEL);

typedef struct OP_CONTEXT_TAG
{
    int32_t initial_state;
    THANDLE(CHANNEL) channel;
} OP_CONTEXT;

void channel_dispose(CHANNEL* channel)
{
    do
    {
        int32_t state = interlocked_add(&channel->state, 0);
        if (
            (state & CHANNEL_STATE_MASK) == CHANNEL_PULLING ||
            (state & CHANNEL_STATE_MASK) == CHANNEL_PUSHING
            )
        {
            /* Operation in progress, wait */
            if (wait_on_address(&channel->state, state, UINT32_MAX) != WAIT_ON_ADDRESS_OK)
            {
                LogError("Failure in wait_on_address(&channel->state=%p, state=%" PRIx32 ", UINT32_MAX=%" PRIx32 ")", &channel->state, state, UINT32_MAX);
                break;
            }
            else
            {
                /* Operation complete, try again */
            }
        }
        else if ((state & CHANNEL_STATE_MASK) == CHANNEL_PULLED)
        {
            channel->pull_callback(channel->context, NULL, CHANNEL_CALLBACK_RESULT_ABANDONED);
            break;
        }
        else if ((state & CHANNEL_STATE_MASK) == CHANNEL_PUSHED)
        {
            channel->push_callback(channel->context, CHANNEL_CALLBACK_RESULT_ABANDONED);
            THANDLE_ASSIGN(RC_PTR)(&channel->data, NULL);
            break;
        }
        else
        {
            break;
        }
    } while(1);

}

THANDLE(CHANNEL) channel_create(void)
{
    THANDLE(CHANNEL) result = THANDLE_MALLOC(CHANNEL)(channel_dispose);
    if (result == NULL)
    {
        LogError("Failure in THANDLE_MALLOC(CHANNEL)(channel_dispose=%p)", channel_dispose);
    }
    else
    {
        CHANNEL* channel = THANDLE_GET_T(CHANNEL)(result);
        channel->pull_callback = NULL;
        channel->push_callback = NULL;
        channel->context = NULL;
        THANDLE_INITIALIZE(RC_PTR)(&channel->data, NULL);
        (void)interlocked_exchange(&channel->state, CHANNEL_READY);
    }
    return result;
}

static void cancel_pull(void* context)
{
    if (context == NULL)
    {
        LogError("Invalid argument: void* context=%p", context);
    }
    else
    {
        OP_CONTEXT* op_context = context;
        CHANNEL* channel = THANDLE_GET_T(CHANNEL)(op_context->channel);
        if (interlocked_compare_exchange(&channel->state, op_context->initial_state + CHANNEL_EPOCH_INCREMENT - CHANNEL_PULLED + CHANNEL_READY, op_context->initial_state) != op_context->initial_state)
        {
            LogError("Operation OP_CONTEXT* op_context = %p has been already cancelled or completed", op_context);
        }
        else
        {
            channel->pull_callback(channel->context, NULL, CHANNEL_CALLBACK_RESULT_CANCELLED);
        }
    }
}

static void dispose_async_op(void* context)
{
    if (context == NULL)
    {
        LogError("Invalid argument: void* context=%p", context);
    }
    else
    {
        OP_CONTEXT* op_context = context;
        THANDLE_ASSIGN(CHANNEL)(&op_context->channel, NULL);
    }
}

CHANNEL_RESULT channel_pull(THANDLE(CHANNEL) channel, PULL_CALLBACK pull_callback, void* context, THANDLE(ASYNC_OP)* out_op)
{
    CHANNEL_RESULT result;
    if (
        channel == NULL ||
        pull_callback == NULL ||
        out_op == NULL
        )
    {
        LogError("Invalid arguments: CHANNEL_HANDLE channel = %p, PULL_CALLBACK pull_callback = %p, void* context = %p, THANDLE(ASYNC_OP)* out_op = %p",
            channel, pull_callback, context, out_op);
        result = CHANNEL_RESULT_INVALID_ARGS;
    }
    else
    {
        CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);
        do
        {
            int32_t state = interlocked_add(&channel_ptr->state, 0);

            if (
                 (state & CHANNEL_STATE_MASK) == CHANNEL_PULLED ||
                 (state & CHANNEL_STATE_MASK) == CHANNEL_PULLING
                )
            {
                LogError("channel_pull called twice on CHANNEL_HANDLE channel=%p.", channel);
                result = CHANNEL_RESULT_REFUSED;
                break;
            }
            else if ((state & CHANNEL_STATE_MASK) == CHANNEL_PUSHING)
            {
                /* channel_push is running, wait for it to finish */
                if (wait_on_address(&channel_ptr->state, state, UINT32_MAX) != WAIT_ON_ADDRESS_OK)
                {
                    LogError("Failure in wait_on_address(&channel_ptr->state = %p, state = %" PRIx32 ", UINT32_MAX=% " PRIx32 ")", &channel_ptr->state, state, UINT32_MAX);
                    result = CHANNEL_RESULT_ERROR;
                    break;
                }
                else
                {
                    /* channel_push complete, try again*/
                }
            }
            else if ((state & CHANNEL_STATE_MASK) == CHANNEL_PUSHED)
            {
                PUSH_CALLBACK push_callback = channel_ptr->push_callback;
                void* push_complete_context = channel_ptr->context;
                THANDLE(RC_PTR) data = NULL;
                THANDLE_INITIALIZE_MOVE(RC_PTR)(&data, &channel_ptr->data);
                if(interlocked_compare_exchange(&channel_ptr->state, state + CHANNEL_EPOCH_INCREMENT - CHANNEL_PUSHED + CHANNEL_READY, state) != state)
                {
                    /*state changed by another thread, try again*/
                }
                else
                {
                    /* channel_push has been called previous, complete operation synchronously */
                    pull_callback(context, data, CHANNEL_CALLBACK_RESULT_OK);
                    push_callback(push_complete_context, CHANNEL_CALLBACK_RESULT_OK);
                    THANDLE_ASSIGN(RC_PTR)(&data, NULL);
                    THANDLE_ASSIGN(ASYNC_OP)(out_op, NULL);
                    result = CHANNEL_RESULT_SYNC;
                    break;
                }
            }
            else if ((state & CHANNEL_STATE_MASK) == CHANNEL_READY)
            {
                if (interlocked_compare_exchange(&channel_ptr->state, state - CHANNEL_READY + CHANNEL_PULLING, state) != state)
                {
                    /*state changed by another thread, try again*/
                }
                else
                {
                    THANDLE(ASYNC_OP) async_op = async_op_create(cancel_pull, sizeof(OP_CONTEXT), alignof(OP_CONTEXT), dispose_async_op);
                    if (async_op == NULL)
                    {
                        LogError("Failure in async_op_create(cancel_pull=%p, sizeof(OP_CONTEXT)=%" PRIu32 ", alignof(OP_CONTEXT)=%" PRIu32 ", dispose_async_op=%p)", cancel_pull, (uint32_t)sizeof(OP_CONTEXT), (uint32_t)alignof(OP_CONTEXT), dispose_async_op);
                        result = CHANNEL_RESULT_ERROR;
                    }
                    else
                    {
                        channel_ptr->pull_callback = pull_callback;
                        channel_ptr->context = context;

                        OP_CONTEXT* op_context = async_op->context;
                        op_context->initial_state = state - CHANNEL_READY + CHANNEL_PULLED;
                        THANDLE_INITIALIZE(CHANNEL)(&op_context->channel, channel);
                        THANDLE_INITIALIZE_MOVE(ASYNC_OP)(out_op, &async_op);

                        (void)interlocked_exchange(&channel_ptr->state, state - CHANNEL_READY + CHANNEL_PULLED);
                        wake_by_address_all(&channel_ptr->state);

                        result = CHANNEL_RESULT_ASYNC;
                    }
                    break;
                }
            }
            else
            {
                LogError("channel=%p in unexpected state channel->state=%d", channel, channel_ptr->state);
                result = CHANNEL_RESULT_ERROR;
            }
        } while(1);
    }
    return result;
}

static void cancel_push(void* context)
{
    if (context == NULL)
    {
        LogError("Invalid argument: void* context=%p", context);
    }
    else
    {

        OP_CONTEXT* op_context = context;
        CHANNEL* channel = THANDLE_GET_T(CHANNEL)(op_context->channel);
        if (interlocked_compare_exchange(&channel->state, op_context->initial_state + CHANNEL_EPOCH_INCREMENT - CHANNEL_PUSHED + CHANNEL_READY, op_context->initial_state) != op_context->initial_state)
        {
            LogError("Operation OP_CONTEXT* op_context = %p has been already cancelled or completed", op_context);
        }
        else
        {
            channel->push_callback(channel->context, CHANNEL_CALLBACK_RESULT_CANCELLED);
            THANDLE_ASSIGN(RC_PTR)(&channel->data, NULL);
        }
    }
}

CHANNEL_RESULT channel_push(THANDLE(CHANNEL) channel, THANDLE(RC_PTR) data, PUSH_CALLBACK push_callback, void* context, THANDLE(ASYNC_OP)* out_op)
{
    CHANNEL_RESULT result;
    if (
        channel == NULL ||
        push_callback == NULL ||
        out_op == NULL
        )
    {
        LogError("Invalid arguments: CHANNEL_HANDLE channel = %p, PUSH_CALLBACK push_callback = %p, void* context = %p, THANDLE(ASYNC_OP)* out_op = %p",
            channel, push_callback, context, out_op);
        result = CHANNEL_RESULT_INVALID_ARGS;
    }
    else
    {
        CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);
        do
        {
            int32_t state = interlocked_add(&channel_ptr->state, 0);
            if (
                (state & CHANNEL_STATE_MASK) == CHANNEL_PUSHED ||
                (state & CHANNEL_STATE_MASK) == CHANNEL_PUSHING
                )
            {
                LogError("channel_push called twice on CHANNEL_HANDLE channel=%p.", channel);
                result = CHANNEL_RESULT_REFUSED;
                break;
            }
            else if ((state & CHANNEL_STATE_MASK) == CHANNEL_PULLING)
            {
                /* channel_pull is running, wait for it to finish */
                if (wait_on_address(&channel_ptr->state, state, UINT32_MAX) != WAIT_ON_ADDRESS_OK)
                {
                    LogError("Failure in wait_on_address(&channel_ptr->state = %p, state = %" PRIx32 ", UINT32_MAX=% " PRIx32 ")", &channel_ptr->state, state, UINT32_MAX);
                    result = CHANNEL_RESULT_ERROR;
                    break;
                }
                else
                {
                    /* channel_pull complete, try again*/
                }
            }
            else if ((state & CHANNEL_STATE_MASK) == CHANNEL_PULLED)
            {
                PULL_CALLBACK pull_callback = channel_ptr->pull_callback;
                void* pull_context = channel_ptr->context;
                if (interlocked_compare_exchange(&channel_ptr->state, state + CHANNEL_EPOCH_INCREMENT - CHANNEL_PULLED + CHANNEL_READY, state) != state)

                {
                    /*state changed by another thread, try again*/
                }
                else
                {
                    pull_callback(pull_context, data, CHANNEL_CALLBACK_RESULT_OK);
                    push_callback(context, CHANNEL_CALLBACK_RESULT_OK);
                    THANDLE_ASSIGN(ASYNC_OP)(out_op, NULL);
                    result = CHANNEL_RESULT_SYNC;
                    break;
                }
            }
            else if ((state & CHANNEL_STATE_MASK) == CHANNEL_READY)
            {
                if (interlocked_compare_exchange(&channel_ptr->state, state - CHANNEL_READY + CHANNEL_PUSHING, state) != state)
                {
                    /*state changed by another thread, try again*/
                }
                else
                {
                    THANDLE(ASYNC_OP) async_op = async_op_create(cancel_push, sizeof(OP_CONTEXT), alignof(OP_CONTEXT), dispose_async_op);
                    if (async_op == NULL)
                    {
                        LogError("Failure in async_op_create(cancel_push=%p, sizeof(OP_CONTEXT)=%" PRIu32 ", alignof(OP_CONTEXT)=%" PRIu32 ", dispose_async_op=%p)", cancel_push, (uint32_t)sizeof(OP_CONTEXT), (uint32_t)alignof(OP_CONTEXT), dispose_async_op);
                        result = CHANNEL_RESULT_ERROR;
                    }
                    else
                    {
                        channel_ptr->push_callback = push_callback;
                        channel_ptr->context = context;
                        THANDLE_INITIALIZE(RC_PTR)(&channel_ptr->data, data);

                        OP_CONTEXT* op_context = async_op->context;
                        op_context->initial_state = state - CHANNEL_READY + CHANNEL_PUSHED;
                        THANDLE_INITIALIZE(CHANNEL)(&op_context->channel, channel);

                        THANDLE_INITIALIZE_MOVE(ASYNC_OP)(out_op, &async_op);
                        (void)interlocked_exchange(&channel_ptr->state, state - CHANNEL_READY + CHANNEL_PUSHED);

                        wake_by_address_all(&channel_ptr->state);
                        result = CHANNEL_RESULT_ASYNC;
                    }
                    break;
                }
            }
            else
            {
                LogError("channel=%p in unexpected state channel->state=%d", channel, channel_ptr->state);
                result = CHANNEL_RESULT_ERROR;
            }
        } while(1);
    }
    return result;
}
