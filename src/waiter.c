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

#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"

#include "c_util/waiter.h"

#define WAITER_REGISTERED (1 << 0)
#define WAITER_NOTIFIED (1 << 1)
#define WAITER_STATE_MASK (WAITER_REGISTERED | WAITER_NOTIFIED)
#define WAITER_EPOCH_INCREMENT (1 << 2)

struct WAITER_TAG{
    union
    {
        NOTIFICATION_CALLBACK notification_callback;
        NOTIFY_COMPLETE_CALLBACK notify_complete_callback;
    };
    void* context;
    THANDLE(RC_PTR) data;
    volatile_atomic int32_t state;
};

THANDLE_TYPE_DEFINE(WAITER);

typedef struct OP_CONTEXT_TAG
{
    int32_t initial_state;
    THANDLE(WAITER) waiter;
} OP_CONTEXT;

void waiter_dispose(WAITER* waiter)
{
    int32_t state = interlocked_add(&waiter->state, 0);
    if(state & WAITER_REGISTERED)
    {
        waiter->notification_callback(waiter->context, NULL, WAITER_CALLBACK_RESULT_ABANDONED);
    }
    else if (state & WAITER_NOTIFIED)
    {
        waiter->notify_complete_callback(waiter->context, WAITER_CALLBACK_RESULT_ABANDONED);
        THANDLE_ASSIGN(RC_PTR)(&waiter->data, NULL);
    }
}

THANDLE(WAITER) waiter_create(void)
{
    THANDLE(WAITER) result = THANDLE_MALLOC(WAITER)(waiter_dispose);
    if (result == NULL)
    {
        LogError("Failure in THANDLE_MALLOC(WAITER)(waiter_dispose=%p)", waiter_dispose);
    }
    else
    {
        WAITER* waiter = THANDLE_GET_T(WAITER)(result);
        waiter->notification_callback = NULL;
        waiter->notify_complete_callback = NULL;
        waiter->context = NULL;
        THANDLE_INITIALIZE(RC_PTR)(&waiter->data, NULL);
        (void)interlocked_exchange(&waiter->state, 0);
    }
    return result;
}

static void cancel_register_notification(void* context)
{
    if (context == NULL)
    {
        LogError("Invalid argument: void* context=%p", context);
    }
    else
    {
        OP_CONTEXT* op_context = context;
        WAITER* waiter = THANDLE_GET_T(WAITER)(op_context->waiter);
        if (interlocked_compare_exchange(&waiter->state, op_context->initial_state - WAITER_REGISTERED + WAITER_EPOCH_INCREMENT, op_context->initial_state) != op_context->initial_state)
        {
            LogError("Operation OP_CONTEXT* op_context = %p has been already cancelled or completed", op_context);
        }
        else
        {
            waiter->notification_callback(waiter->context, NULL, WAITER_CALLBACK_RESULT_CANCELLED);
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
        THANDLE_ASSIGN(WAITER)(&op_context->waiter, NULL);
    }
}

WAITER_RESULT waiter_register_notification(THANDLE(WAITER) waiter, NOTIFICATION_CALLBACK notification_callback, void* context, THANDLE(ASYNC_OP)* out_op)
{
    WAITER_RESULT result;
    if (
        waiter == NULL ||
        notification_callback == NULL ||
        out_op == NULL
        )
    {
        LogError("Invalid arguments: WAITER_HANDLE waiter = %p, NOTIFICATION_CALLBACK notification_callback = %p, void* context = %p, THANDLE(ASYNC_OP)* out_op = %p",
            waiter, notification_callback, context, out_op);
        result = WAITER_RESULT_ERROR;
    }
    else
    {
        WAITER* waiter_ptr = THANDLE_GET_T(WAITER)(waiter);
        int32_t state = interlocked_or(&waiter_ptr->state, WAITER_REGISTERED);
        if (state & WAITER_REGISTERED)
        {
            LogError("waiter_register_notification called twice on WAITER_HANDLE waiter=%p.", waiter);
            result = WAITER_RESULT_REFUSED;
        }
        else
        {
            if (state & WAITER_NOTIFIED)
            {
                if(interlocked_compare_exchange(&waiter_ptr->state, state + WAITER_EPOCH_INCREMENT - WAITER_NOTIFIED, (state | WAITER_REGISTERED)) != (state | WAITER_REGISTERED))
                {
                    LogError("State should not have changed");
                    result = WAITER_RESULT_ERROR;
                }
                else
                {
                    notification_callback(context, waiter_ptr->data, WAITER_CALLBACK_RESULT_OK);
                    waiter_ptr->notify_complete_callback(waiter_ptr->context, WAITER_CALLBACK_RESULT_OK);
                    THANDLE_ASSIGN(RC_PTR)(&waiter_ptr->data, NULL);
                    result = WAITER_RESULT_SYNC;
                }
            }
            else
            {
                THANDLE(ASYNC_OP) async_op = async_op_create(cancel_register_notification, sizeof(OP_CONTEXT), alignof(OP_CONTEXT), dispose_async_op);
                if (async_op == NULL)
                {
                    LogError("Failure in async_op_create(cancel_register_notification=%p, sizeof(OP_CONTEXT)=%" PRIu32 ", alignof(OP_CONTEXT)=%" PRIu32 ", dispose_async_op=%p)", cancel_register_notification, (uint32_t)sizeof(OP_CONTEXT), (uint32_t)alignof(OP_CONTEXT), dispose_async_op);
                    result = WAITER_RESULT_ERROR;
                }
                else
                {
                    waiter_ptr->notification_callback = notification_callback;
                    waiter_ptr->context = context;

                    OP_CONTEXT* op_context = async_op->context;
                    op_context->initial_state = state | WAITER_REGISTERED;
                    THANDLE_INITIALIZE(WAITER)(&op_context->waiter, waiter);
                    THANDLE_INITIALIZE_MOVE(ASYNC_OP)(out_op, &async_op);

                    result = WAITER_RESULT_ASYNC;
                }
            }
        }
    }
    return result;
}

static void cancel_notify(void* context)
{
    if (context == NULL)
    {
        LogError("Invalid argument: void* context=%p", context);
    }
    else
    {

        OP_CONTEXT* op_context = context;
        WAITER* waiter = THANDLE_GET_T(WAITER)(op_context->waiter);
        if (interlocked_compare_exchange(&waiter->state, op_context->initial_state - WAITER_NOTIFIED + WAITER_EPOCH_INCREMENT, op_context->initial_state) != op_context->initial_state)
        {
            LogError("Operation OP_CONTEXT* op_context = %p has been already cancelled or completed", op_context);
        }
        else
        {
            waiter->notify_complete_callback(waiter->context, WAITER_CALLBACK_RESULT_CANCELLED);
            THANDLE_ASSIGN(RC_PTR)(&waiter->data, NULL);
        }
    }
}

WAITER_RESULT waiter_notify(THANDLE(WAITER) waiter, THANDLE(RC_PTR) data, NOTIFY_COMPLETE_CALLBACK notify_complete_callback, void* context, THANDLE(ASYNC_OP)* out_op)
{
    WAITER_RESULT result;
    if (
        waiter == NULL ||
        notify_complete_callback == NULL ||
        out_op == NULL
        )
    {
        LogError("Invalid arguments: WAITER_HANDLE waiter = %p, NOTIFY_COMPLETE_CALLBACK notify_complete_callback = %p, void* context = %p, THANDLE(ASYNC_OP)* out_op = %p",
            waiter, notify_complete_callback, context, out_op);
        result = WAITER_RESULT_ERROR;
    }
    else
    {
        WAITER* waiter_ptr = THANDLE_GET_T(WAITER)(waiter);
        int32_t state = interlocked_or(&waiter_ptr->state, WAITER_NOTIFIED);
        if (state & WAITER_NOTIFIED)
        {
            LogError("waiter_notify called twice on WAITER_HANDLE waiter=%p.", waiter);
            result = WAITER_RESULT_REFUSED;
        }
        else
        {
            if (state & WAITER_REGISTERED)
            {
                if (interlocked_compare_exchange(&waiter_ptr->state, state + WAITER_EPOCH_INCREMENT - WAITER_REGISTERED, (state | WAITER_NOTIFIED)) != (state | WAITER_NOTIFIED))
                {
                    LogError("State should not have changed");
                    result = WAITER_RESULT_ERROR;
                }
                else
                {
                    waiter_ptr->notification_callback(waiter_ptr->context, data, WAITER_CALLBACK_RESULT_OK);
                    notify_complete_callback(context, WAITER_CALLBACK_RESULT_OK);
                    result = WAITER_RESULT_SYNC;
                }
            }
            else
            {
                THANDLE(ASYNC_OP) async_op = async_op_create(cancel_notify, sizeof(OP_CONTEXT), alignof(OP_CONTEXT), dispose_async_op);
                if (async_op == NULL)
                {
                    LogError("Failure in async_op_create(cancel_notify=%p, sizeof(OP_CONTEXT)=%" PRIu32 ", alignof(OP_CONTEXT)=%" PRIu32 ", dispose_async_op=%p)", cancel_notify, (uint32_t)sizeof(OP_CONTEXT), (uint32_t)alignof(OP_CONTEXT), dispose_async_op);
                    result = WAITER_RESULT_ERROR;
                }
                else
                {
                    waiter_ptr->notify_complete_callback = notify_complete_callback;
                    waiter_ptr->context = context;
                    THANDLE_INITIALIZE(RC_PTR)(&waiter_ptr->data, data);

                    OP_CONTEXT* op_context = async_op->context;
                    op_context->initial_state = state | WAITER_NOTIFIED;
                    THANDLE_INITIALIZE(WAITER)(&op_context->waiter, waiter);

                    THANDLE_INITIALIZE_MOVE(ASYNC_OP)(out_op, &async_op);
                    result = WAITER_RESULT_ASYNC;
                }
            }
        }
    }
    return result;
}
