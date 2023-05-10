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
#include "c_pal/sm.h"
#include "c_pal/srw_lock.h"

#include "c_util/async_op.h"

#include "c_util/waiter.h"

THANDLE_TYPE_DEFINE(void_ptr);

typedef struct WAITER_TAG
{
    SM_HANDLE sm;
    SRW_LOCK_HANDLE lock;
    THANDLE(void_ptr) data;
    NOTIFICATION_CALLBACK notification_callback;
    void* notification_callback_context;
    NOTIFY_COMPLETE_CALLBACK notify_complete_callback;
    void* notify_complete_callback_context;
    bool registered;
    bool notified;
    bool callbacks_called;
} WAITER;

WAITER_HANDLE waiter_create(void)
{
    WAITER_HANDLE result;
    WAITER_HANDLE waiter = malloc(sizeof(WAITER));
    if (waiter == NULL)
    {
        LogError("Failure in malloc(sizeof(WAITER_HANDLE))");
        result = NULL;
    }
    else
    {
        waiter->sm = sm_create("waiter");
        if (waiter->sm == NULL)
        {
            LogError("Failure in sm_create(\"waiter\")");
            result = NULL;
        }
        else
        {
            SM_RESULT sm_open_begin_result = sm_open_begin(waiter->sm);
            if(sm_open_begin_result != SM_EXEC_GRANTED)
            {
                LogError("Failure in sm_open_begin(waiter->sm)");
                result = NULL;
            }
            else
            {
                sm_open_end(waiter->sm, true);
                waiter->lock = srw_lock_create("waiter", false);
                if (waiter->lock == NULL)
                {
                    LogError("Failure in srw_lock_create(\"waiter\", false)");
                    result = NULL;
                }
                else
                {
                    THANDLE_INITIALIZE(void_ptr)(&waiter->data, NULL);
                    waiter->notification_callback = NULL;
                    waiter->notification_callback_context = NULL;
                    waiter->notify_complete_callback = NULL;
                    waiter->notify_complete_callback_context = NULL;
                    waiter->registered = false;
                    waiter->notified = false;
                    waiter->callbacks_called = false;
                    result = waiter;
                    goto all_ok;
                }
            }
            sm_destroy(waiter->sm);
        }
        free(waiter);
    }
all_ok:
    return result;
}

static void abandon(void* context)
{
    WAITER_HANDLE waiter = (WAITER_HANDLE)context;
    srw_lock_acquire_exclusive(waiter->lock);
    if (!waiter->callbacks_called)
    {
        if (waiter->registered)
        {
            waiter->notification_callback(waiter->notification_callback_context, NULL, WAITER_RESULT_ABANDONED);
        }
        if (waiter->notified)
        {
            waiter->notify_complete_callback(waiter->notify_complete_callback_context, WAITER_RESULT_ABANDONED);
            THANDLE_ASSIGN(void_ptr)(&waiter->data, NULL);
        }
        waiter->callbacks_called = true;
    }
    srw_lock_release_exclusive(waiter->lock);
}

void waiter_destroy(WAITER_HANDLE waiter)
{
    if (waiter == NULL)
    {
        LogError("Invalid argument: WAITER_HANDLE waiter=%p", waiter);
    }
    else
    {
        sm_close_begin_with_cb(waiter->sm, abandon, waiter, NULL, NULL);
        srw_lock_destroy(waiter->lock);
        sm_destroy(waiter->sm);
        free(waiter);
    }
}

static void cancel_register_notification(void* context)
{
    WAITER_HANDLE waiter = context;
    srw_lock_acquire_exclusive(waiter->lock);
    {
        waiter->notification_callback(waiter->notification_callback_context, NULL, WAITER_RESULT_CANCELED);
        waiter->callbacks_called = true;
        sm_fault(waiter->sm);
    }
    srw_lock_release_exclusive(waiter->lock);
}

static void dispose(void* context)
{
    (void)context;
}

static void try_complete(WAITER_HANDLE waiter)
{
    if (waiter->registered && waiter->notified && !waiter->callbacks_called)
    {
        waiter->callbacks_called = true;
        sm_exec_end(waiter->sm);
        waiter->notification_callback(waiter->notification_callback_context, waiter->data, WAITER_RESULT_OK);
        THANDLE_ASSIGN(void_ptr)(&waiter->data, NULL);
        sm_exec_end(waiter->sm);
        waiter->notify_complete_callback(waiter->notify_complete_callback_context, WAITER_RESULT_OK);
    }
}

THANDLE(ASYNC_OP) waiter_register_notification(WAITER_HANDLE waiter, NOTIFICATION_CALLBACK notification_callback, void* context)
{
    THANDLE(ASYNC_OP) result = NULL;
    if (waiter == NULL || notification_callback == NULL || context == NULL)
    {
        LogError("Invalid arguments: WAITER_HANDLE waiter = %p, NOTIFICATION_CALLBACK notification_callback = %p, void* context = %p",
            waiter, notification_callback, context);
    }
    else
    {
        SM_RESULT sm_exec_begin_result = sm_exec_begin(waiter->sm);
        if (sm_exec_begin_result != SM_EXEC_GRANTED)
        {
            LogError("Failure in sm_exec_begin(waiter->sm=%p)", waiter->sm);
        }
        else
        {
            THANDLE(ASYNC_OP) async_op = async_op_create(cancel_register_notification, sizeof(WAITER_HANDLE), alignof(WAITER_HANDLE), dispose);
            if (async_op == NULL)
            {
                LogError("Failure in async_op_create(cancel_register_notification=%p, sizeof(WAITER_HANDLE)=%" PRIu32 ", alignof(WAITER_HANDLE)=%" PRIu32 ", dispose=%p)", cancel_register_notification, (uint32_t)sizeof(WAITER_HANDLE), (uint32_t)alignof(WAITER_HANDLE), dispose);
            }
            else
            {
                ((ASYNC_OP*)async_op)->context = waiter;
                srw_lock_acquire_exclusive(waiter->lock);
                {
                    waiter->notification_callback = notification_callback;
                    waiter->notification_callback_context = context;
                    waiter->registered = true;
                    try_complete(waiter);
                }
                srw_lock_release_exclusive(waiter->lock);

                THANDLE_INITIALIZE_MOVE(ASYNC_OP)(&result, &async_op);
                goto all_ok;
            }
            sm_exec_end(waiter->sm);
        }
    }
all_ok:
    return result;
}

static void cancel_notify(void* context)
{
    WAITER_HANDLE waiter = context;
    srw_lock_acquire_exclusive(waiter->lock);
    {
        waiter->notify_complete_callback(waiter->notify_complete_callback_context, WAITER_RESULT_CANCELED);
        waiter->callbacks_called = true;
        sm_fault(waiter->sm);
    }
    srw_lock_release_exclusive(waiter->lock);
}

THANDLE(ASYNC_OP) waiter_notify(WAITER_HANDLE waiter, THANDLE(void_ptr) data, NOTIFY_COMPLETE_CALLBACK notify_complete_callback, void* context)
{
    THANDLE(ASYNC_OP) result = NULL;
    if (waiter == NULL || data == NULL || notify_complete_callback == NULL || context == NULL)
    {
        LogError("Invalid arguments: WAITER_HANDLE waiter = %p, THANDLE(void_ptr) data = %p, NOTIFY_COMPLETE_CALLBACK notify_complete_callback = %p, void* context = %p",
                   waiter, data, notify_complete_callback, context);
    }
    else
    {
        SM_RESULT sm_exec_begin_result = sm_exec_begin(waiter->sm);
        if (sm_exec_begin_result != SM_EXEC_GRANTED)
        {
            LogError("Failure in sm_exec_begin(waiter->sm=%p)", waiter->sm);
        }
        else
        {
            THANDLE(ASYNC_OP) async_op = async_op_create(cancel_notify, sizeof(WAITER_HANDLE), alignof(WAITER_HANDLE), dispose);
            if (async_op == NULL)
            {
                LogError("Failure in async_op_create(cancel_notify=%p, sizeof(WAITER_HANDLE)=%" PRIu32 ", alignof(WAITER_HANDLE)=%" PRIu32 ", dispose=%p)", cancel_notify, (uint32_t)sizeof(WAITER_HANDLE), (uint32_t)alignof(WAITER_HANDLE), dispose);
            }
            else
            {
                ((ASYNC_OP*)async_op)->context = waiter;
                srw_lock_acquire_exclusive(waiter->lock);
                {
                    THANDLE_ASSIGN(void_ptr)(&waiter->data, data);
                    waiter->notify_complete_callback = notify_complete_callback;
                    waiter->notify_complete_callback_context = context;
                    waiter->notified = true;
                    try_complete(waiter);
                }
                srw_lock_release_exclusive(waiter->lock);

                THANDLE_INITIALIZE_MOVE(ASYNC_OP)(&result, &async_op);
                goto all_ok;
            }
            sm_exec_end(waiter->sm);
        }
    }
all_ok:
    return result;
}
