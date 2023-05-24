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

#include "c_util/waiter.h"

#define WAITER_STATE_VALUES \
    WAITER_READY, \
    WAITER_NOTIFYING, \
    WAITER_NOTIFIED, \
    WAITER_REGISTERING, \
    WAITER_REGISTERED

MU_DEFINE_ENUM(WAITER_STATE, WAITER_STATE_VALUES)

#define WAITER_EPOCH_INCREMENT (1 << 3)

#define WAITER_STATE_MASK ((1 << 3) - 1)

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
    do
    {
        int32_t state = interlocked_add(&waiter->state, 0);
        if (
            (state & WAITER_STATE_MASK) == WAITER_REGISTERING ||
            (state & WAITER_STATE_MASK) == WAITER_NOTIFYING
            )
        {
            /* Operation in progress, wait */
            if (wait_on_address(&waiter->state, state, UINT32_MAX) != WAIT_ON_ADDRESS_OK)
            {
                LogError("Failure in wait_on_address(&waiter->state=%p, state=%" PRIx32 ", UINT32_MAX=%" PRIx32 ")", &waiter->state, state, UINT32_MAX);
                break;
            }
            else
            {
                /* Operation complete, try again */
            }
        }
        else if ((state & WAITER_STATE_MASK) == WAITER_REGISTERED)
        {
            waiter->notification_callback(waiter->context, NULL, WAITER_CALLBACK_RESULT_ABANDONED);
            break;
        }
        else if ((state & WAITER_STATE_MASK) == WAITER_NOTIFIED)
        {
            waiter->notify_complete_callback(waiter->context, WAITER_CALLBACK_RESULT_ABANDONED);
            THANDLE_ASSIGN(RC_PTR)(&waiter->data, NULL);
            break;
        }
        else
        {
            break;
        }
    } while(1);

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
        (void)interlocked_exchange(&waiter->state, WAITER_READY);
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
        if (interlocked_compare_exchange(&waiter->state, op_context->initial_state + WAITER_EPOCH_INCREMENT - WAITER_REGISTERED + WAITER_READY, op_context->initial_state) != op_context->initial_state)
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
        result = WAITER_RESULT_INVALID_ARGS;
    }
    else
    {
        WAITER* waiter_ptr = THANDLE_GET_T(WAITER)(waiter);
        do
        {
            int32_t state = interlocked_add(&waiter_ptr->state, 0);

            if (
                 (state & WAITER_STATE_MASK) == WAITER_REGISTERED ||
                 (state & WAITER_STATE_MASK) == WAITER_REGISTERING
                )
            {
                LogError("waiter_register_notification called twice on WAITER_HANDLE waiter=%p.", waiter);
                result = WAITER_RESULT_REFUSED;
                break;
            }
            else if ((state & WAITER_STATE_MASK) == WAITER_NOTIFYING)
            {
                /* waiter_notify is running, wait for it to finish */
                if (wait_on_address(&waiter_ptr->state, state, UINT32_MAX) != WAIT_ON_ADDRESS_OK)
                {
                    LogError("Failure in wait_on_address(&waiter_ptr->state = %p, state = %" PRIx32 ", UINT32_MAX=% " PRIx32 ")", &waiter_ptr->state, state, UINT32_MAX);
                    result = WAITER_RESULT_ERROR;
                    break;
                }
                else
                {
                    /* waiter_notify complete, try again*/
                }
            }
            else if ((state & WAITER_STATE_MASK) == WAITER_NOTIFIED)
            {
                NOTIFY_COMPLETE_CALLBACK notify_complete_callback = waiter_ptr->notify_complete_callback;
                void* notify_complete_context = waiter_ptr->context;
                THANDLE(RC_PTR) data = NULL;
                THANDLE_INITIALIZE_MOVE(RC_PTR)(&data, &waiter_ptr->data);
                if(interlocked_compare_exchange(&waiter_ptr->state, state + WAITER_EPOCH_INCREMENT - WAITER_NOTIFIED + WAITER_READY, state) != state)
                {
                    /*state changed by another thread, try again*/
                }
                else
                {
                    /* waiter_notify has been called previous, complete operation synchronously */
                    notification_callback(context, data, WAITER_CALLBACK_RESULT_OK);
                    notify_complete_callback(notify_complete_context, WAITER_CALLBACK_RESULT_OK);
                    THANDLE_ASSIGN(RC_PTR)(&data, NULL);
                    THANDLE_ASSIGN(ASYNC_OP)(out_op, NULL);
                    result = WAITER_RESULT_SYNC;
                    break;
                }
            }
            else if ((state & WAITER_STATE_MASK) == WAITER_READY)
            {
                if (interlocked_compare_exchange(&waiter_ptr->state, state - WAITER_READY + WAITER_REGISTERING, state) != state)
                {
                    /*state changed by another thread, try again*/
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
                        op_context->initial_state = state - WAITER_READY + WAITER_REGISTERED;
                        THANDLE_INITIALIZE(WAITER)(&op_context->waiter, waiter);
                        THANDLE_INITIALIZE_MOVE(ASYNC_OP)(out_op, &async_op);

                        (void)interlocked_exchange(&waiter_ptr->state, state - WAITER_READY + WAITER_REGISTERED);
                        wake_by_address_all(&waiter_ptr->state);

                        result = WAITER_RESULT_ASYNC;
                    }
                    break;
                }
            }
            else
            {
                LogError("waiter=%p in unexpected state waiter->state=%d", waiter, waiter_ptr->state);
                result = WAITER_RESULT_ERROR;
            }
        } while(1);
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
        if (interlocked_compare_exchange(&waiter->state, op_context->initial_state + WAITER_EPOCH_INCREMENT - WAITER_NOTIFIED + WAITER_READY, op_context->initial_state) != op_context->initial_state)
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
        result = WAITER_RESULT_INVALID_ARGS;
    }
    else
    {
        WAITER* waiter_ptr = THANDLE_GET_T(WAITER)(waiter);
        do
        {
            int32_t state = interlocked_add(&waiter_ptr->state, 0);
            if (
                (state & WAITER_STATE_MASK) == WAITER_NOTIFIED ||
                (state & WAITER_STATE_MASK) == WAITER_NOTIFYING
                )
            {
                LogError("waiter_notify called twice on WAITER_HANDLE waiter=%p.", waiter);
                result = WAITER_RESULT_REFUSED;
                break;
            }
            else if ((state & WAITER_STATE_MASK) == WAITER_REGISTERING)
            {
                /* waiter_register_notification is running, wait for it to finish */
                if (wait_on_address(&waiter_ptr->state, state, UINT32_MAX) != WAIT_ON_ADDRESS_OK)
                {
                    LogError("Failure in wait_on_address(&waiter_ptr->state = %p, state = %" PRIx32 ", UINT32_MAX=% " PRIx32 ")", &waiter_ptr->state, state, UINT32_MAX);
                    result = WAITER_RESULT_ERROR;
                    break;
                }
                else
                {
                    /* waiter_register_notification complete, try again*/
                }
            }
            else if ((state & WAITER_STATE_MASK) == WAITER_REGISTERED)
            {
                NOTIFICATION_CALLBACK notification_callback = waiter_ptr->notification_callback;
                void* notification_context = waiter_ptr->context;
                if (interlocked_compare_exchange(&waiter_ptr->state, state + WAITER_EPOCH_INCREMENT - WAITER_REGISTERED + WAITER_READY, state) != state)

                {
                    /*state changed by another thread, try again*/
                }
                else
                {
                    notification_callback(notification_context, data, WAITER_CALLBACK_RESULT_OK);
                    notify_complete_callback(context, WAITER_CALLBACK_RESULT_OK);
                    THANDLE_ASSIGN(ASYNC_OP)(out_op, NULL);
                    result = WAITER_RESULT_SYNC;
                    break;
                }
            }
            else if ((state & WAITER_STATE_MASK) == WAITER_READY)
            {
                if (interlocked_compare_exchange(&waiter_ptr->state, state - WAITER_READY + WAITER_NOTIFYING, state) != state)
                {
                    /*state changed by another thread, try again*/
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
                        op_context->initial_state = state - WAITER_READY + WAITER_NOTIFIED;
                        THANDLE_INITIALIZE(WAITER)(&op_context->waiter, waiter);

                        THANDLE_INITIALIZE_MOVE(ASYNC_OP)(out_op, &async_op);
                        (void)interlocked_exchange(&waiter_ptr->state, state - WAITER_READY + WAITER_NOTIFIED);

                        wake_by_address_all(&waiter_ptr->state);
                        result = WAITER_RESULT_ASYNC;
                    }
                    break;
                }
            }
            else
            {
                LogError("waiter=%p in unexpected state waiter->state=%d", waiter, waiter_ptr->state);
                result = WAITER_RESULT_ERROR;
            }
        } while(1);
    }
    return result;
}
