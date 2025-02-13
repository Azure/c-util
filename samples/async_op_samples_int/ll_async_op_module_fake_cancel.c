// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <inttypes.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/log_critical_and_terminate.h"
#include "c_pal/sm.h"
#include "c_pal/srw_lock_ll.h"
#include "c_pal/thandle.h"
#include "c_pal/threadapi.h"
#include "c_pal/threadpool.h"

#include "c_util/async_op.h"
#include "c_util/doublylinkedlist.h"

#include "common_async_op_module_interface.h"
#include "ll_async_op_module_fake_cancel.h"

typedef struct LL_ASYNC_OP_MODULE_FAKE_CANCEL_TAG
{
    SM_HANDLE sm;
    EXECUTION_ENGINE_HANDLE execution_engine;
    THANDLE(THREADPOOL) threadpool;

    // Test hook settings
    bool next_call_completes_synchronously;
    COMMON_ASYNC_OP_MODULE_RESULT next_result;
    volatile_atomic int32_t retry_result_remaining;

    SRW_LOCK_LL result_settings_queue_lock;
    DLIST_ENTRY result_settings_queue;
} LL_ASYNC_OP_MODULE_FAKE_CANCEL;

typedef struct LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT_SETTINGS_TAG
{
    DLIST_ENTRY dlist_entry;
    bool completes_synchronously;
    COMMON_ASYNC_OP_MODULE_RESULT result;
} LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT_SETTINGS;

typedef struct LL_ASYNC_OP_MODULE_FAKE_CANCEL_EXECUTE_CONTEXT_TAG
{
    volatile_atomic int32_t callback_was_called; // synchronize the fake cancel
    COMMON_ASYNC_OP_MODULE_EXECUTE_CALLBACK callback;
    void* context;

    // control the fake execution
    uint32_t complete_in_ms;

    // Pointer back for sm_exec_end
    LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE handle;
} LL_ASYNC_OP_MODULE_FAKE_CANCEL_EXECUTE_CONTEXT;

IMPLEMENT_MOCKABLE_FUNCTION(, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, ll_async_op_module_fake_cancel_create, EXECUTION_ENGINE_HANDLE, execution_engine)
{
    LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE result;

    if (execution_engine == NULL)
    {
        LogError("Invalid arguments EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(LL_ASYNC_OP_MODULE_FAKE_CANCEL));
        if (result == NULL)
        {
            LogError("malloc LL_ASYNC_OP_MODULE_FAKE_CANCEL failed");
        }
        else
        {
            result->sm = sm_create("ll_async_op_module_fake_cancel");
            if (result->sm == NULL)
            {
                LogError("sm_create failed");
            }
            else
            {
                if (srw_lock_ll_init(&result->result_settings_queue_lock) != 0)
                {
                    LogError("srw_lock_ll_init failed");
                }
                else
                {
                    DList_InitializeListHead(&result->result_settings_queue);

                    execution_engine_inc_ref(execution_engine);
                    result->execution_engine = execution_engine;
                    THANDLE_INITIALIZE(THREADPOOL)(&result->threadpool, NULL);

                    result->next_call_completes_synchronously = false;
                    result->next_result = COMMON_ASYNC_OP_MODULE_OK;
                    (void)interlocked_exchange(&result->retry_result_remaining, 0);

                    goto all_ok;
                }
                sm_destroy(result->sm);
            }
            free(result);
            result = NULL;
        }
    }
all_ok:
    return result;
}

static void ll_async_op_module_fake_cancel_close_internal(LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE handle)
{
    if (sm_close_begin(handle->sm) == SM_EXEC_GRANTED)
    {
        THANDLE_ASSIGN(THREADPOOL)(&handle->threadpool, NULL);
        sm_close_end(handle->sm);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, ll_async_op_module_fake_cancel_destroy, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, handle)
{
    if (handle == NULL)
    {
        LogError("Invalid argument LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE handle=%p", handle);
    }
    else
    {
        ll_async_op_module_fake_cancel_close_internal(handle);
        execution_engine_dec_ref(handle->execution_engine);

        PDLIST_ENTRY entry;
        while ((entry = DList_RemoveTailList(&handle->result_settings_queue)) != &handle->result_settings_queue)
        {
            LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT_SETTINGS* settings = CONTAINING_RECORD(entry, LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT_SETTINGS, dlist_entry);
            free(settings);
        }

        srw_lock_ll_deinit(&handle->result_settings_queue_lock);
        sm_destroy(handle->sm);
        free(handle);
    }
}


IMPLEMENT_MOCKABLE_FUNCTION(, int, ll_async_op_module_fake_cancel_open, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, handle)
{
    int result;
    if (handle == NULL)
    {
        LogError("Invalid argument LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE handle=%p", handle);
        result = MU_FAILURE;
    }
    else
    {
        if (sm_open_begin(handle->sm) != SM_EXEC_GRANTED)
        {
            LogError("sm_open failed");
            result = MU_FAILURE;
        }
        else
        {
            THANDLE_INITIALIZE_MOVE(THREADPOOL)(&handle->threadpool, &(THANDLE(THREADPOOL)){ threadpool_create(handle->execution_engine) });
            if (handle->threadpool == NULL)
            {
                LogError("threadpool_create failed");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
            sm_open_end(handle->sm, result == 0);
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, ll_async_op_module_fake_cancel_close, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, handle)
{
    if (handle == NULL)
    {
        LogError("Invalid argument LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE handle=%p", handle);
    }
    else
    {
        ll_async_op_module_fake_cancel_close_internal(handle);
    }
}

static void on_async_op_LL_ASYNC_OP_MODULE_FAKE_CANCEL_EXECUTE_CONTEXT_cancel(void* context)
{
    LL_ASYNC_OP_MODULE_FAKE_CANCEL_EXECUTE_CONTEXT* call_context = context;
    // It is not possible to cancel this operation, so we just call the callback
    // We need to synchronize with possible races with the operation completing

    // 1. Check our synchronization flag to call the callback exactly once
    if (interlocked_compare_exchange(&call_context->callback_was_called, 1, 0) == 0)
    {
        // 2. If the callback has not been called yet, we call it now with a canceled/abandoned result
        LogInfo("Call was canceled, calling callback and real operation will complete some time later");
        call_context->callback(call_context->context, COMMON_ASYNC_OP_MODULE_CANCELED);
    }
    else
    {
        // 3. Otherwise, the operation is already complete and this is a no-op
        LogInfo("Call was canceled but already complete, do nothing");
    }
}

static void on_async_op_LL_ASYNC_OP_MODULE_FAKE_CANCEL_EXECUTE_CONTEXT_dispose(void* context)
{
    // This context has nothing additional to cleanup
    (void)context;
}

static COMMON_ASYNC_OP_MODULE_RESULT get_underlying_result(LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE handle)
{
    COMMON_ASYNC_OP_MODULE_RESULT result;

    bool do_retry = false;
    do
    {
        int32_t retry_result_remaining = interlocked_add(&handle->retry_result_remaining, 0);
        if (retry_result_remaining > 0)
        {
            if (interlocked_compare_exchange(&handle->retry_result_remaining, retry_result_remaining - 1, retry_result_remaining) == retry_result_remaining)
            {
                do_retry = true;
                break;
            }
        }
        else
        {
            break;
        }
    } while (true);

    if (do_retry)
    {
        LogError("Retry still required");
        result = COMMON_ASYNC_OP_MODULE_ERROR_CAN_RETRY;
    }
    else
    {
        result = handle->next_result;
    }
    return result;
}

static void ll_async_op_module_fake_cancel_execute_the_async_worker_thread(void* context)
{
    if (context == NULL)
    {
        LogCriticalAndTerminate("Invalid arguments: void* context=%p", context);
    }
    else
    {
        THANDLE(ASYNC_OP) async_op = context;
        LL_ASYNC_OP_MODULE_FAKE_CANCEL_EXECUTE_CONTEXT* async_op_context = async_op->context;

        // Simulate the operation taking some time to complete, real operation would do some IO for example
        ThreadAPI_Sleep(async_op_context->complete_in_ms);

        // Note that sm_exec_end is called here so that the callback could call close on the module without a deadlock
        sm_exec_end(async_op_context->handle->sm);

        // 1. Check our synchronization flag to call the callback exactly once
        if (interlocked_compare_exchange(&async_op_context->callback_was_called, 1, 0) == 0)
        {
            // 2. If the callback has not been called yet, we call it now with the result value
            LogInfo("Call completed");
            async_op_context->callback(async_op_context->context, get_underlying_result(async_op_context->handle));
        }
        else
        {
            // 3. Otherwise, the operation was canceled and this is a no-op
            LogInfo("Call completed but was canceled, do nothing");
        }

        // 4. Clean up the async_op
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    }
}

// This function is really just a test hook to simulate an underlying async call
// Real module would not have this
static int call_underlying_async(LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE handle, THREADPOOL_WORK_FUNCTION callback, void* context)
{
    int result;

    // Check queue which controls an ordered set of results. Note if the queue is used, the last entry will be used repeatedly
    srw_lock_ll_acquire_exclusive(&handle->result_settings_queue_lock);
    {
        PDLIST_ENTRY entry;
        if ((entry = DList_RemoveTailList(&handle->result_settings_queue)) != &handle->result_settings_queue)
        {
            LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT_SETTINGS* settings = CONTAINING_RECORD(entry, LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT_SETTINGS, dlist_entry);
            handle->next_result = settings->result;
            handle->next_call_completes_synchronously = settings->completes_synchronously;
            free(settings);
        }
        srw_lock_ll_release_exclusive(&handle->result_settings_queue_lock);
    }

    if (handle->next_call_completes_synchronously)
    {
        callback(context);
        result = 0;
    }
    else
    {
        result = threadpool_schedule_work(handle->threadpool, callback, context);
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, ll_async_op_module_fake_cancel_execute_async, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, handle, uint32_t, complete_in_ms, THANDLE(ASYNC_OP)*, async_op_out, COMMON_ASYNC_OP_MODULE_EXECUTE_CALLBACK, callback, void*, context)
{
    int result;
    if (
        handle == NULL ||
        async_op_out == NULL ||
        callback == NULL
        )
    {
        LogError("Invalid arguments LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE handle=%p, uint32_t complete_in_ms=%" PRIu32 ", THANDLE(ASYNC_OP)* async_op_out=%p, COMMON_ASYNC_OP_MODULE_EXECUTE_CALLBACK callback=%p, void* context=%p",
            handle, complete_in_ms, async_op_out, callback, context);
        result = MU_FAILURE;
    }
    else
    {
        if (sm_exec_begin(handle->sm) != SM_EXEC_GRANTED)
        {
            LogError("sm_exec_begin failed");
            result = MU_FAILURE;
        }
        else
        {
            // 1. Create an ASYNC_OP for the operation context
            THANDLE(ASYNC_OP) async_op = async_op_create(on_async_op_LL_ASYNC_OP_MODULE_FAKE_CANCEL_EXECUTE_CONTEXT_cancel, sizeof(LL_ASYNC_OP_MODULE_FAKE_CANCEL_EXECUTE_CONTEXT), alignof(LL_ASYNC_OP_MODULE_FAKE_CANCEL_EXECUTE_CONTEXT), on_async_op_LL_ASYNC_OP_MODULE_FAKE_CANCEL_EXECUTE_CONTEXT_dispose);
            if (async_op == NULL)
            {
                LogError("async_op_create failed");
                result = MU_FAILURE;
            }
            else
            {
                // 2. Any context needed is stored in async_op->context
                LL_ASYNC_OP_MODULE_FAKE_CANCEL_EXECUTE_CONTEXT* async_op_context = async_op->context;

                async_op_context->callback = callback;
                async_op_context->context = context;
                async_op_context->complete_in_ms = complete_in_ms;
                async_op_context->handle = handle;
                // 3. Need to synchronize fake cancellation with the real underlying operation completion, set a flag to indicate callback has not been called
                (void)interlocked_exchange(&async_op_context->callback_was_called, 0);

                // 4. Take an additional reference on the async_op, we have 1 reference for returning to the caller (async_op), and 1 reference for the async work callback (async_op_ref_for_callback)
                //    Need to take the reference before starting the operation below because its callback may come immediately
                THANDLE(ASYNC_OP) async_op_ref_for_callback = NULL;
                THANDLE_ASSIGN(ASYNC_OP)(&async_op_ref_for_callback, async_op);

                // 5. Start the async work, passing the async_op_ref_for_callback as the context
                if (call_underlying_async(handle, ll_async_op_module_fake_cancel_execute_the_async_worker_thread, (void*)async_op_ref_for_callback) != 0)
                {
                    LogError("call_underlying_async failed");
                    result = MU_FAILURE;
                }
                else
                {
                    // 6. Provide the async_op to the caller
                    //    Note that another option is to return the ASYNC_OP rather than using an out argument
                    THANDLE_MOVE(ASYNC_OP)(async_op_out, &async_op);
                    result = 0;
                    goto all_ok;
                }
                THANDLE_ASSIGN(ASYNC_OP)(&async_op_ref_for_callback, NULL);
                THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
            }
            sm_exec_end(handle->sm);
        }
    }
all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, ll_async_op_module_fake_cancel_next_call_completes_synchronously, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, handle, bool, is_synchronous)
{
    if (handle == NULL)
    {
        LogError("Invalid arguments: LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE handle=%p, bool is_synchronous=%" PRI_BOOL,
            handle, MU_BOOL_VALUE(is_synchronous));
    }
    else
    {
        handle->next_call_completes_synchronously = is_synchronous;
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, ll_async_op_module_fake_cancel_set_report_retry_result_count, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, handle, uint32_t, retry_result_count)
{
    if (handle == NULL)
    {
        LogError("Invalid arguments: LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE handle=%p, uint32_t retry_result_count=%" PRIu32,
            handle, retry_result_count);
    }
    else
    {
        (void)interlocked_exchange(&handle->retry_result_remaining, retry_result_count);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, ll_async_op_module_fake_cancel_set_next_async_result, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, handle, COMMON_ASYNC_OP_MODULE_RESULT, next_result)
{
    if (handle == NULL)
    {
        LogError("Invalid arguments: LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE handle=%p, COMMON_ASYNC_OP_MODULE_RESULT next_result=%" PRI_MU_ENUM "",
            handle, MU_ENUM_VALUE(COMMON_ASYNC_OP_MODULE_RESULT, next_result));
    }
    else
    {
        handle->next_result = next_result;
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, ll_async_op_module_fake_cancel_add_result_settings_to_queue, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, handle, bool, is_synchronous, COMMON_ASYNC_OP_MODULE_RESULT, next_result)
{
    int result;

    if (handle == NULL)
    {
        LogError("Invalid arguments: LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE handle=%p, bool is_synchronous=%" PRI_BOOL ", COMMON_ASYNC_OP_MODULE_RESULT next_result=%" PRI_MU_ENUM "",
            handle, MU_BOOL_VALUE(is_synchronous), MU_ENUM_VALUE(COMMON_ASYNC_OP_MODULE_RESULT, next_result));
        result = MU_FAILURE;
    }
    else
    {
        LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT_SETTINGS* settings = malloc(sizeof(LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT_SETTINGS));
        if (settings == NULL)
        {
            LogError("malloc LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT_SETTINGS failed");
            result = MU_FAILURE;
        }
        else
        {
            settings->completes_synchronously = is_synchronous;
            settings->result = next_result;

            srw_lock_ll_acquire_exclusive(&handle->result_settings_queue_lock);
            {
                DList_InsertHeadList(&handle->result_settings_queue, &settings->dlist_entry);
                srw_lock_ll_release_exclusive(&handle->result_settings_queue_lock);
            }

            result = 0;
        }
    }
    return result;
}

static int ll_async_op_module_fake_cancel_open_interface_adapter(void* context)
{
    return ll_async_op_module_fake_cancel_open(context);
}

static void ll_async_op_module_fake_cancel_close_interface_adapter(void* context)
{
    ll_async_op_module_fake_cancel_close(context);
}

static void ll_async_op_module_fake_cancel_destroy_interface_adapter(void* context)
{
    ll_async_op_module_fake_cancel_destroy(context);
}

static int ll_async_op_module_fake_cancel_execute_async_interface_adapter(void* context, uint32_t complete_in_ms, THANDLE(ASYNC_OP)* async_op_out, COMMON_ASYNC_OP_MODULE_EXECUTE_CALLBACK callback, void* context_callback)
{
    return ll_async_op_module_fake_cancel_execute_async(context, complete_in_ms, async_op_out, callback, context_callback);
}

IMPLEMENT_MOCKABLE_FUNCTION(, COMMON_ASYNC_OP_MODULE_INTERFACE, ll_async_op_module_fake_cancel_get_interface, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, handle)
{
    COMMON_ASYNC_OP_MODULE_INTERFACE result = (COMMON_ASYNC_OP_MODULE_INTERFACE)
    {
        .handle = handle,
        .open = ll_async_op_module_fake_cancel_open_interface_adapter,
        .close = ll_async_op_module_fake_cancel_close_interface_adapter,
        .destroy = ll_async_op_module_fake_cancel_destroy_interface_adapter,
        .execute_async = ll_async_op_module_fake_cancel_execute_async_interface_adapter,
        .execute_async_no_async_op_out = NULL
    };
    return result;
}
