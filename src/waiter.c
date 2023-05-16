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
#define WAITER_COMPLETING (1 << 2)

typedef struct OP_CONTEXT_TAG
{
    union
    {
        NOTIFICATION_CALLBACK notification_callback;
        NOTIFY_COMPLETE_CALLBACK notify_complete_callback;
    };
    void* context;
    THANDLE(RC_PTR) data;
    volatile_atomic int32_t* state;
} OP_CONTEXT;

typedef struct WAITER_TAG
{
    volatile_atomic int32_t state;
    THANDLE(ASYNC_OP) current_op;
} WAITER;

WAITER_HANDLE waiter_create(void)
{
    WAITER_HANDLE result;
    WAITER_HANDLE waiter = malloc(sizeof(WAITER));
    if (waiter == NULL)
    {
        LogError("Failure in malloc(sizeof(WAITER_HANDLE)=%zu)", sizeof(WAITER));
        result = NULL;
    }
    else
    {
        (void)interlocked_exchange(&waiter->state, 0);
        THANDLE_INITIALIZE(ASYNC_OP)(&waiter->current_op, NULL);
        result = waiter;
    }
    return result;
}

void waiter_destroy(WAITER_HANDLE waiter)
{
    if (waiter == NULL)
    {
        LogError("Invalid argument: WAITER_HANDLE waiter=%p", waiter);
    }
    else
    {
        int32_t state = interlocked_or(&waiter->state, WAITER_REGISTERED | WAITER_NOTIFIED);
        OP_CONTEXT* op_context = (OP_CONTEXT*)(waiter->current_op)->context;
        if (state & WAITER_REGISTERED)
        {
            op_context->notification_callback(op_context->context, NULL, WAITER_CALLBACK_RESULT_ABANDONED);
        }
        else if (state & WAITER_NOTIFIED)
        {
            op_context->notify_complete_callback(op_context->context, WAITER_CALLBACK_RESULT_ABANDONED);
            THANDLE_ASSIGN(RC_PTR)(&op_context->data, NULL);
        }
        THANDLE_ASSIGN(ASYNC_OP)(&waiter->current_op, NULL);
        free(waiter);
    }
}

static void cancel_register_notification(void* context)
{
    OP_CONTEXT* op_context = context;
    if (interlocked_or(op_context->state, WAITER_COMPLETING) & WAITER_COMPLETING)
    {
        LogError("Cannot cancel a waiter that is completing");
    }
    else
    {
        op_context->notification_callback(op_context->context, NULL, WAITER_CALLBACK_RESULT_CANCELLED);
        interlocked_and(op_context->state, ~WAITER_REGISTERED);
    }
}



WAITER_RESULT waiter_register_notification(WAITER_HANDLE waiter, NOTIFICATION_CALLBACK notification_callback, void* context, THANDLE(ASYNC_OP)* op)
{
    WAITER_RESULT result;
    THANDLE_ASSIGN(ASYNC_OP)(op, NULL);
    if (
        waiter == NULL ||
        notification_callback == NULL ||
        op == NULL
        )
    {
        LogError("Invalid arguments: WAITER_HANDLE waiter = %p, NOTIFICATION_CALLBACK notification_callback = %p, void* context = %p, THANDLE(ASYNC_OP)* op = %p",
            waiter, notification_callback, context, op);
        result = WAITER_RESULT_ERROR;
    }
    else
    {
        do
        {
            int32_t state = interlocked_or(&waiter->state, WAITER_REGISTERED);
            if (state & WAITER_REGISTERED)
            {
                OP_CONTEXT* op_context = waiter->current_op->context;
                LogError(
                    "waiter_register_notification called twice on WAITER_HANDLE waiter=%p."
                    " Previous arguments: NOTIFICATION_CALLBACK notification_callback=%p, void* context=%p"
                    " Current arguments: NOTIFICATION_CALLBACK notification_callback=%p, void* context=%p",
                    waiter,
                    op_context->notification_callback, op_context->notification_callback_context,
                    notification_callback, context
                );
                result = WAITER_RESULT_REFUSED;
                break;
            }
            else
            {
                if (state & WAITER_NOTIFIED)
                {
                    OP_CONTEXT* op_context = waiter->current_op->context;
                    if (interlocked_exchange(&op_context->completing, 1) != 0)
                    {
                        /* Previous operation is in progress, try again.*/
                    }
                    else
                    {
                        interlocked_and(op_context->state, ~WAITER_NOTIFIED);
                        op_context->notify_complete_callback(op_context->context, WAITER_CALLBACK_RESULT_OK);
                        interlocked_and(op_context->state, ~WAITER_REGISTERED);
                        notification_callback(context, op_context->data, WAITER_CALLBACK_RESULT_OK);
                        THANDLE_ASSIGN(RC_PTR)(&op_context->data, NULL);
                        THANDLE_ASSIGN(ASYNC_OP)(&waiter->current_op, NULL);
                        result = WAITER_RESULT_SYNC;
                    }
                }
                else
                {
                    THANDLE(ASYNC_OP) async_op = async_op_create(cancel_register_notification, sizeof(WAITER_HANDLE), alignof(WAITER_HANDLE), NULL);
                    if (async_op == NULL)
                    {
                        LogError("Failure in async_op_create(cancel_register_notification=%p, sizeof(WAITER_HANDLE)=%" PRIu32 ", alignof(WAITER_HANDLE)=%" PRIu32 ", NULL)", cancel_register_notification, (uint32_t)sizeof(WAITER_HANDLE), (uint32_t)alignof(WAITER_HANDLE));
                    }
                    else
                    {
                        ((ASYNC_OP*)async_op)->context = waiter;
                        complete(waiter);
                        THANDLE_INITIALIZE_MOVE(ASYNC_OP)(&result, &async_op);
                    }
                }
            }
        } while(1);
    }
    return result;
}

static void cancel_notify(void* context)
{
    WAITER_HANDLE waiter = context;
    srw_lock_acquire_exclusive(waiter->lock);
    {
        waiter->notify_complete_callback(waiter->notify_complete_callback_context, WAITER_CALLBACK_RESULT_CANCELLED);
        waiter->callbacks_called = true;
        THANDLE_ASSIGN(RC_PTR)(&waiter->data, NULL);
    }
    srw_lock_release_exclusive(waiter->lock);
}

THANDLE(ASYNC_OP) waiter_notify(WAITER_HANDLE waiter, THANDLE(RC_PTR) data, NOTIFY_COMPLETE_CALLBACK notify_complete_callback, void* context)
{
    THANDLE(ASYNC_OP) result = NULL;
    if (
        waiter == NULL ||
        data == NULL ||
        notify_complete_callback == NULL
        )
    {
        LogError("Invalid arguments: WAITER_HANDLE waiter = %p, THANDLE(RC_PTR) data = %p, NOTIFY_COMPLETE_CALLBACK notify_complete_callback = %p, void* context = %p",
                   waiter, data, notify_complete_callback, context);
    }
    else
    {
        srw_lock_acquire_exclusive(waiter->lock);
        {
            if (waiter->notified)
            {
                LogError("waiter_notify called twice on WAITER_HANDLE waiter=%p."
                    "Previous arguments: THANDLE(RC_PTR) data=%" PRI_RC_PTR ", NOTIFY_COMPLETE_CALLBACK notify_complete_callback=%p, void* context=%p."
                    "Current arguments: THANDLE(RC_PTR) data=%" PRI_RC_PTR ", NOTIFY_COMPLETE_CALLBACK notify_complete_callback=%p, void* context=%p.",
                    waiter,
                    waiter->data, waiter->notify_complete_callback, waiter->notify_complete_callback_context,
                    data, notify_complete_callback, context
                );
            }
            else
            {
                THANDLE(ASYNC_OP) async_op = async_op_create(cancel_notify, sizeof(WAITER_HANDLE), alignof(WAITER_HANDLE), NULL);
                if (async_op == NULL)
                {
                    LogError("Failure in async_op_create(cancel_notify=%p, sizeof(WAITER_HANDLE)=%" PRIu32 ", alignof(WAITER_HANDLE)=%" PRIu32 ", NULL)", cancel_notify, (uint32_t)sizeof(WAITER_HANDLE), (uint32_t)alignof(WAITER_HANDLE));
                }
                else
                {
                    ((ASYNC_OP*)async_op)->context = waiter;
                    THANDLE_INITIALIZE(RC_PTR)(&waiter->data, data);
                    waiter->notify_complete_callback = notify_complete_callback;
                    waiter->notify_complete_callback_context = context;
                    waiter->notified = true;
                    try_complete(waiter);
                    THANDLE_INITIALIZE_MOVE(ASYNC_OP)(&result, &async_op);
                }
            }
        }
        srw_lock_release_exclusive(waiter->lock);
    }
    return result;
}
