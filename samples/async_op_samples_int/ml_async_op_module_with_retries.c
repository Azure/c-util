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

#include "ll_async_op_module_fake_cancel.h"
#include "ll_async_op_module_real_cancel.h"

#include "ml_async_op_module_with_retries.h"

typedef struct ML_ASYNC_OP_MODULE_WITH_RETRIES_TAG
{
    SM_HANDLE sm;
    EXECUTION_ENGINE_HANDLE execution_engine;
    THANDLE(THREADPOOL) threadpool;

    LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE ll_fake_cancel;
    LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE ll_real_cancel;
} ML_ASYNC_OP_MODULE_WITH_RETRIES;

typedef struct ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT_TAG
{
    ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CALLBACK callback;
    void* context;

    // ll call context
    THANDLE(ASYNC_OP) ll_async_op;

    // synchronization of which ll_async_op for retry iterations
    SRW_LOCK_LL ll_async_op_lock;
    volatile_atomic int32_t ll_async_op_epoch;

    // Cancel state of this module to avoid calling retries after cancel
    volatile_atomic int32_t is_canceled;

    // Anything needed for retries
    uint32_t complete_in_ms;
    ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE handle;
} ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT;

MU_DEFINE_ENUM_STRINGS(ML_ASYNC_OP_MODULE_WITH_RETRIES_RESULT, ML_ASYNC_OP_MODULE_WITH_RETRIES_RESULT_VALUES)

IMPLEMENT_MOCKABLE_FUNCTION(, ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE, ml_async_op_module_with_retries_create, EXECUTION_ENGINE_HANDLE, execution_engine, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, ll_fake_cancel, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE, ll_real_cancel)
{
    ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE result;

    if (execution_engine == NULL ||
        ll_fake_cancel == NULL ||
        ll_real_cancel == NULL)
    {
        LogError("Invalid arguments EXECUTION_ENGINE_HANDLE execution_engine=%p, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE ll_fake_cancel=%p, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE ll_real_cancel=%p",
            execution_engine, ll_fake_cancel, ll_real_cancel);
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(ML_ASYNC_OP_MODULE_WITH_RETRIES));
        if (result == NULL)
        {
            LogError("malloc ML_ASYNC_OP_MODULE_WITH_RETRIES failed");
        }
        else
        {
            result->sm = sm_create("ml_async_op_module_with_retries");
            if (result->sm == NULL)
            {
                LogError("sm_create failed");
            }
            else
            {
                execution_engine_inc_ref(execution_engine);
                result->execution_engine = execution_engine;
                THANDLE_INITIALIZE(THREADPOOL)(&result->threadpool, NULL);

                result->ll_fake_cancel = ll_fake_cancel;
                result->ll_real_cancel = ll_real_cancel;

                goto all_ok;
            }
            free(result);
            result = NULL;
        }
    }
all_ok:
    return result;
}

static void ml_async_op_module_with_retries_close_internal(ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE handle)
{
    if (sm_close_begin(handle->sm) == SM_EXEC_GRANTED)
    {
        THANDLE_ASSIGN(THREADPOOL)(&handle->threadpool, NULL);
        sm_close_end(handle->sm);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, ml_async_op_module_with_retries_destroy, ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE, handle)
{
    if (handle == NULL)
    {
        LogError("Invalid argument ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE handle=%p", handle);
    }
    else
    {
        ml_async_op_module_with_retries_close_internal(handle);
        execution_engine_dec_ref(handle->execution_engine);
        sm_destroy(handle->sm);
        free(handle);
    }
}


IMPLEMENT_MOCKABLE_FUNCTION(, int, ml_async_op_module_with_retries_open, ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE, handle)
{
    int result;
    if (handle == NULL)
    {
        LogError("Invalid argument ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE handle=%p", handle);
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

IMPLEMENT_MOCKABLE_FUNCTION(, void, ml_async_op_module_with_retries_close, ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE, handle)
{
    if (handle == NULL)
    {
        LogError("Invalid argument ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE handle=%p", handle);
    }
    else
    {
        ml_async_op_module_with_retries_close_internal(handle);
    }
}

static void on_async_op_ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT_cancel(void* context)
{
    ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT* call_context = context;

    THANDLE(ASYNC_OP) async_op_ll_temp = NULL;

    // 1. Take a lock to synchronize with calls in the chain completing and the async_op_ll changing
    srw_lock_ll_acquire_exclusive(&call_context->ll_async_op_lock);
    {
        LogInfo("Call was canceled, set the state to canceled");
        // 2. Set the state of this module to canceled so that we do not begin new async calls for lower-layer(s)
        (void)interlocked_exchange(&call_context->is_canceled, 1);
        // 3. Get the current lower layer async op under the lock, note that it may be NULL if in between calls
        THANDLE_ASSIGN(ASYNC_OP)(&async_op_ll_temp, call_context->ll_async_op);

        srw_lock_ll_release_exclusive(&call_context->ll_async_op_lock);
    }

    if (async_op_ll_temp != NULL)
    {
        LogInfo("...and cancel the async_op for the lower layer");
        // 4. Call cancel on the lower layer async_op if we had a non-NULL
        (void)async_op_cancel(async_op_ll_temp);

        // Clean up the temporary reference
        THANDLE_ASSIGN(ASYNC_OP)(&async_op_ll_temp, NULL);
    }
}

static void on_async_op_ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT_dispose(void* context)
{
    ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT* call_context = context;
    // Clean up the underlying ll_async_op
    THANDLE_ASSIGN(ASYNC_OP)(&call_context->ll_async_op, NULL);
    srw_lock_ll_deinit(&call_context->ll_async_op_lock);
}

static void ml_async_op_module_with_retries_on_ll_complete_with_fake_cancel_do_retry(void* context, LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT result)
{
    if (context == NULL)
    {
        LogCriticalAndTerminate("Invalid arguments: void* context=%p, LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT result=%" PRI_MU_ENUM "",
            context, MU_ENUM_VALUE(LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT, result));
    }
    else
    {
        THANDLE(ASYNC_OP) async_op = context;
        ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT* async_op_context = async_op->context;

        bool is_canceled = false;
        bool failed = false;
        bool must_call_callback = false;

        if (result == enum_value_typedef_LL_ASYNC_OP_MODULE_FAKE_CANCEL_ERROR_CAN_RETRY)
        {
            // In case retry is required...

            int32_t ll_async_op_epoch;

            // 1. Take a lock to synchronize with calls in the chain completing and the async_op_ll changing
            srw_lock_ll_acquire_exclusive(&async_op_context->ll_async_op_lock);
            {
                // 2. Increment the epoch so that we do not try to store the old async_op from before this call
                ll_async_op_epoch = interlocked_increment(&async_op_context->ll_async_op_epoch);
                // 3. Reset the ll_async_op, it is complete and we don't need it anymore. Note that this is an optimization and not strictly necessary
                THANDLE_ASSIGN(ASYNC_OP)(&async_op_context->ll_async_op, NULL);
                // 4. Check if cancel has already been called, if so, we should call the callback instead of calling the chained async operation
                if (interlocked_add(&async_op_context->is_canceled, 0) != 0)
                {
                    is_canceled = true;
                    must_call_callback = true;
                }
                srw_lock_ll_release_exclusive(&async_op_context->ll_async_op_lock);
            }

            if (!is_canceled)
            {
                // 5. Store the async_op from the LL in a temporary variable
                THANDLE(ASYNC_OP) ll_async_op = NULL;

                // 6. Call the next step in the chain
                if (ll_async_op_module_fake_cancel_execute_async(async_op_context->handle->ll_fake_cancel, async_op_context->complete_in_ms, &ll_async_op, ml_async_op_module_with_retries_on_ll_complete_with_fake_cancel_do_retry, context) != 0)
                {
                    LogError("ll_async_op_module_fake_cancel_execute_async failed");
                    failed = true;
                    must_call_callback = true;
                }
                else
                {
                    // 7. Take a lock to synchronize with calls in the chain completing and the async_op_ll changing
                    srw_lock_ll_acquire_exclusive(&async_op_context->ll_async_op_lock);
                    {
                        // 8. Check if cancel has already been called, if so, we should cancel the new ll_async_op which wasn't stored in the context yet
                        if (interlocked_add(&async_op_context->is_canceled, 0) != 0)
                        {
                            is_canceled = true;
                            // Note that we do not set must_call_callback because the lower layer call has started and its callback will come
                        }
                        else
                        {
                            // 9. Make sure we only store the latest ll_async_op by synchronizing on ll_async_op_epoch
                            if (interlocked_compare_exchange(&async_op_context->ll_async_op_epoch, ll_async_op_epoch + 1, ll_async_op_epoch) == ll_async_op_epoch)
                            {
                                // 10. Store the ll_async_op in the context on success so that it can be canceled
                                THANDLE_ASSIGN(ASYNC_OP)(&async_op_context->ll_async_op, ll_async_op);
                            }
                        }
                        srw_lock_ll_release_exclusive(&async_op_context->ll_async_op_lock);
                    }

                    if (is_canceled)
                    {
                        // 11. If we are canceled, we need to cancel the lower layer async_op which was just started in this call
                        (void)async_op_cancel(ll_async_op);
                    }
                    THANDLE_ASSIGN(ASYNC_OP)(&ll_async_op, NULL);
                }
            }
        }
        else
        {
            // normal case, just call the callback
            must_call_callback = true;
        }

        if (must_call_callback)
        {
            // Note that sm_exec_end is called here so that the callback could call close on the module without a deadlock
            sm_exec_end(async_op_context->handle->sm);

            // 12. In case of no retry needed, failure, or cancellation, call the callback now
            //     All other cases indicate a retry was started and this callback will come again
            if (is_canceled)
            {
                async_op_context->callback(async_op_context->context, ML_ASYNC_OP_MODULE_WITH_RETRIES_CANCELED);
            }
            else if (failed)
            {
                async_op_context->callback(async_op_context->context, ML_ASYNC_OP_MODULE_WITH_RETRIES_ERROR);
            }
            else
            {
                switch (result)
                {
                    default:
                    {
                        LogInfo("LL Call completed with an error: %" PRI_MU_ENUM "",
                            MU_ENUM_VALUE(LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT, result));
                        async_op_context->callback(async_op_context->context, ML_ASYNC_OP_MODULE_WITH_RETRIES_ERROR);
                        break;
                    }
                    case LL_ASYNC_OP_MODULE_FAKE_CANCEL_OK:
                    {
                        LogInfo("LL Call completed normally");
                        async_op_context->callback(async_op_context->context, ML_ASYNC_OP_MODULE_WITH_RETRIES_OK);
                        break;
                    }
                    case LL_ASYNC_OP_MODULE_FAKE_CANCEL_CANCELED:
                    {
                        LogInfo("LL Call was canceled");
                        async_op_context->callback(async_op_context->context, ML_ASYNC_OP_MODULE_WITH_RETRIES_CANCELED);
                        break;
                    }
                }
            }

            // 13. ...and clean up the async_op
            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        }
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, ml_async_op_module_with_retries_execute_underlying_fake_cancel_async, ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE, handle, uint32_t, complete_in_ms, THANDLE(ASYNC_OP)*, async_op_out, ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CALLBACK, callback, void*, context)
{
    int result;
    if (
        handle == NULL ||
        async_op_out == NULL ||
        callback == NULL
        )
    {
        LogError("Invalid arguments ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE handle=%p, uint32_t complete_in_ms=%" PRIu32 ", THANDLE(ASYNC_OP)* async_op_out=%p, ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CALLBACK callback=%p, void* context=%p",
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
            THANDLE(ASYNC_OP) async_op = async_op_create(on_async_op_ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT_cancel, sizeof(ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT), alignof(ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT), on_async_op_ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT_dispose);
            if (async_op == NULL)
            {
                LogError("async_op_create failed");
                result = MU_FAILURE;
            }
            else
            {
                // 2. Any context needed is stored in async_op->context
                ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT* async_op_context = async_op->context;

                async_op_context->callback = callback;
                async_op_context->context = context;
                async_op_context->handle = handle;
                async_op_context->complete_in_ms = complete_in_ms;

                // 3. Synchronization of ll_async_op between chained async calls
                (void)srw_lock_ll_init(&async_op_context->ll_async_op_lock);
                (void)interlocked_exchange(&async_op_context->ll_async_op_epoch, 0);
                (void)interlocked_exchange(&async_op_context->is_canceled, 0);

                // 4. Initialize the ll_async_op
                THANDLE_INITIALIZE(ASYNC_OP)(&async_op_context->ll_async_op, NULL);

                // 5. Store the async_op from the LL in a temporary variable
                THANDLE(ASYNC_OP) ll_async_op = NULL;

                // 6. Take an additional reference on the async_op, we have 1 reference for returning to the caller, and 1 reference for the async work callback
                //    Need to take the reference before starting the operation below because its callback may come immediately
                THANDLE(ASYNC_OP) async_op_ref_for_callback = NULL;
                THANDLE_ASSIGN(ASYNC_OP)(&async_op_ref_for_callback, async_op);

                // 7. Start the first step of the async work, passing the async_op_ref_for_callback as the context
                if (ll_async_op_module_fake_cancel_execute_async(handle->ll_fake_cancel, complete_in_ms, &ll_async_op, ml_async_op_module_with_retries_on_ll_complete_with_fake_cancel_do_retry, (void*)async_op_ref_for_callback) != 0)
                {
                    LogError("ll_async_op_module_fake_cancel_execute_async failed");
                    result = MU_FAILURE;
                }
                else
                {
                    // 8. Take a lock to synchronize with the possibility of the step 1 callback already being called (or any subsequent steps)
                    srw_lock_ll_acquire_exclusive(&async_op_context->ll_async_op_lock);
                    {
                        // 9. Make sure we only store the latest ll_async_op by synchronizing on ll_async_op_epoch
                        //    This handles the case where the callback for step 1 has already been called and now the ll_async_op in this scope is complete and we don't need it
                        if (interlocked_compare_exchange(&async_op_context->ll_async_op_epoch, 1, 0) == 0)
                        {
                            // 10. Store the ll_async_op in the context on success so that it can be canceled
                            //     This must happen before the next line so that the ll_async_op is set before cancel is called
                            THANDLE_MOVE(ASYNC_OP)(&async_op_context->ll_async_op, &ll_async_op);
                        }
                        srw_lock_ll_release_exclusive(&async_op_context->ll_async_op_lock);
                    }

                    // 11. Provide the async_op to the caller
                    //    After this (but before returning) it is possible that the caller may call cancel because they may have access to the async_op_out pointer in another thread
                    THANDLE_MOVE(ASYNC_OP)(async_op_out, &async_op);
                    result = 0;
                    goto all_ok;
                }
            }
            sm_exec_end(handle->sm);
        }
    }
all_ok:
    return result;
}

static void ml_async_op_module_with_retries_on_ll_complete_with_real_cancel_do_retry(void* context, LL_ASYNC_OP_MODULE_REAL_CANCEL_RESULT result)
{
    if (context == NULL)
    {
        LogCriticalAndTerminate("Invalid arguments: void* context=%p, LL_ASYNC_OP_MODULE_REAL_CANCEL_RESULT result=%" PRI_MU_ENUM "",
            context, MU_ENUM_VALUE(LL_ASYNC_OP_MODULE_REAL_CANCEL_RESULT, result));
    }
    else
    {
        THANDLE(ASYNC_OP) async_op = context;
        ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT* async_op_context = async_op->context;

        bool is_canceled = false;
        bool failed = false;
        bool must_call_callback = false;

        if (result == enum_value_typedef_LL_ASYNC_OP_MODULE_REAL_CANCEL_ERROR_CAN_RETRY)
        {
            // In case retry is required...

            int32_t ll_async_op_epoch;

            // 1. Take a lock to synchronize with calls in the chain completing and the async_op_ll changing
            srw_lock_ll_acquire_exclusive(&async_op_context->ll_async_op_lock);
            {
                // 2. Increment the epoch so that we do not try to store the old async_op from before this call
                ll_async_op_epoch = interlocked_increment(&async_op_context->ll_async_op_epoch);
                // 3. Reset the ll_async_op, it is complete and we don't need it anymore. Note that this is an optimization and not strictly necessary
                THANDLE_ASSIGN(ASYNC_OP)(&async_op_context->ll_async_op, NULL);
                // 4. Check if cancel has already been called, if so, we should call the callback instead of calling the chained async operation
                if (interlocked_add(&async_op_context->is_canceled, 0) != 0)
                {
                    is_canceled = true;
                    must_call_callback = true;
                }
                srw_lock_ll_release_exclusive(&async_op_context->ll_async_op_lock);
            }

            if (!is_canceled)
            {
                // 5. Store the async_op from the LL in a temporary variable
                THANDLE(ASYNC_OP) ll_async_op = NULL;

                // 6. Call the next step in the chain
                if (ll_async_op_module_real_cancel_execute_async(async_op_context->handle->ll_real_cancel, async_op_context->complete_in_ms, &ll_async_op, ml_async_op_module_with_retries_on_ll_complete_with_real_cancel_do_retry, context) != 0)
                {
                    LogError("ll_async_op_module_real_cancel_execute_async failed");
                    failed = true;
                    must_call_callback = true;
                }
                else
                {
                    // 7. Take a lock to synchronize with calls in the chain completing and the async_op_ll changing
                    srw_lock_ll_acquire_exclusive(&async_op_context->ll_async_op_lock);
                    {
                        // 8. Check if cancel has already been called, if so, we should cancel the new ll_async_op which wasn't stored in the context yet
                        if (interlocked_add(&async_op_context->is_canceled, 0) != 0)
                        {
                            is_canceled = true;
                            // Note that we do not set must_call_callback because the lower layer call has started and its callback will come
                        }
                        else
                        {
                            // 9. Make sure we only store the latest ll_async_op by synchronizing on ll_async_op_epoch
                            if (interlocked_compare_exchange(&async_op_context->ll_async_op_epoch, ll_async_op_epoch + 1, ll_async_op_epoch) == ll_async_op_epoch)
                            {
                                // 10. Store the ll_async_op in the context on success so that it can be canceled
                                THANDLE_ASSIGN(ASYNC_OP)(&async_op_context->ll_async_op, ll_async_op);
                            }
                        }
                        srw_lock_ll_release_exclusive(&async_op_context->ll_async_op_lock);
                    }

                    if (is_canceled)
                    {
                        // 11. If we are canceled, we need to cancel the lower layer async_op which was just started in this call
                        (void)async_op_cancel(ll_async_op);
                    }
                    THANDLE_ASSIGN(ASYNC_OP)(&ll_async_op, NULL);
                }
            }
        }
        else
        {
            // normal case, just call the callback
            must_call_callback = true;
        }

        if (must_call_callback)
        {
            // Note that sm_exec_end is called here so that the callback could call close on the module without a deadlock
            sm_exec_end(async_op_context->handle->sm);

            // 12. In case of no retry needed, failure, or cancellation, call the callback now
            //     All other cases indicate a retry was started and this callback will come again
            if (is_canceled)
            {
                async_op_context->callback(async_op_context->context, ML_ASYNC_OP_MODULE_WITH_RETRIES_CANCELED);
            }
            else if (failed)
            {
                async_op_context->callback(async_op_context->context, ML_ASYNC_OP_MODULE_WITH_RETRIES_ERROR);
            }
            else
            {
                switch (result)
                {
                    default:
                    {
                        LogInfo("LL Call completed with an error: %" PRI_MU_ENUM "",
                            MU_ENUM_VALUE(LL_ASYNC_OP_MODULE_REAL_CANCEL_RESULT, result));
                        async_op_context->callback(async_op_context->context, ML_ASYNC_OP_MODULE_WITH_RETRIES_ERROR);
                        break;
                    }
                    case LL_ASYNC_OP_MODULE_REAL_CANCEL_OK:
                    {
                        LogInfo("LL Call completed normally");
                        async_op_context->callback(async_op_context->context, ML_ASYNC_OP_MODULE_WITH_RETRIES_OK);
                        break;
                    }
                    case LL_ASYNC_OP_MODULE_REAL_CANCEL_CANCELED:
                    {
                        LogInfo("LL Call was canceled");
                        async_op_context->callback(async_op_context->context, ML_ASYNC_OP_MODULE_WITH_RETRIES_CANCELED);
                        break;
                    }
                }
            }

            // 13. ...and clean up the async_op
            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        }
    }
}
IMPLEMENT_MOCKABLE_FUNCTION(, int, ml_async_op_module_with_retries_execute_underlying_real_cancel_async, ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE, handle, uint32_t, complete_in_ms, THANDLE(ASYNC_OP)*, async_op_out, ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CALLBACK, callback, void*, context)
{
    int result;
    if (
        handle == NULL ||
        async_op_out == NULL ||
        callback == NULL
        )
    {
        LogError("Invalid arguments ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE handle=%p, uint32_t complete_in_ms=%" PRIu32 ", THANDLE(ASYNC_OP)* async_op_out=%p, ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CALLBACK callback=%p, void* context=%p",
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
            THANDLE(ASYNC_OP) async_op = async_op_create(on_async_op_ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT_cancel, sizeof(ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT), alignof(ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT), on_async_op_ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT_dispose);
            if (async_op == NULL)
            {
                LogError("async_op_create failed");
                result = MU_FAILURE;
            }
            else
            {
                // 2. Any context needed is stored in async_op->context
                ML_ASYNC_OP_MODULE_WITH_RETRIES_EXECUTE_CONTEXT* async_op_context = async_op->context;

                async_op_context->callback = callback;
                async_op_context->context = context;
                async_op_context->handle = handle;
                async_op_context->complete_in_ms = complete_in_ms;

                // 3. Synchronization of ll_async_op between chained async calls
                (void)srw_lock_ll_init(&async_op_context->ll_async_op_lock);
                (void)interlocked_exchange(&async_op_context->ll_async_op_epoch, 0);
                (void)interlocked_exchange(&async_op_context->is_canceled, 0);

                // 4. Initialize the ll_async_op
                THANDLE_INITIALIZE(ASYNC_OP)(&async_op_context->ll_async_op, NULL);

                // 5. Store the async_op from the LL in a temporary variable
                THANDLE(ASYNC_OP) ll_async_op = NULL;

                // 6. Take an additional reference on the async_op, we have 1 reference for returning to the caller, and 1 reference for the async work callback
                //    Need to take the reference before starting the operation below because its callback may come immediately
                THANDLE(ASYNC_OP) async_op_ref_for_callback = NULL;
                THANDLE_ASSIGN(ASYNC_OP)(&async_op_ref_for_callback, async_op);

                // 7. Start the first step of the async work, passing the async_op_ref_for_callback as the context
                if (ll_async_op_module_real_cancel_execute_async(handle->ll_real_cancel, complete_in_ms, &ll_async_op, ml_async_op_module_with_retries_on_ll_complete_with_real_cancel_do_retry, (void*)async_op_ref_for_callback) != 0)
                {
                    LogError("ll_async_op_module_real_cancel_execute_async failed");
                    result = MU_FAILURE;
                }
                else
                {
                    // 8. Take a lock to synchronize with the possibility of the step 1 callback already being called (or any subsequent steps)
                    srw_lock_ll_acquire_exclusive(&async_op_context->ll_async_op_lock);
                    {
                        // 9. Make sure we only store the latest ll_async_op by synchronizing on ll_async_op_epoch
                        //    This handles the case where the callback for step 1 has already been called and now the ll_async_op in this scope is complete and we don't need it
                        if (interlocked_compare_exchange(&async_op_context->ll_async_op_epoch, 1, 0) == 0)
                        {
                            // 10. Store the ll_async_op in the context on success so that it can be canceled
                            //     This must happen before the next line so that the ll_async_op is set before cancel is called
                            THANDLE_MOVE(ASYNC_OP)(&async_op_context->ll_async_op, &ll_async_op);
                        }
                        srw_lock_ll_release_exclusive(&async_op_context->ll_async_op_lock);
                    }

                    // 11. Provide the async_op to the caller
                    //    After this (but before returning) it is possible that the caller may call cancel because they may have access to the async_op_out pointer in another thread
                    THANDLE_MOVE(ASYNC_OP)(async_op_out, &async_op);
                    result = 0;
                    goto all_ok;
                }
            }
            sm_exec_end(handle->sm);
        }
    }
all_ok:
    return result;
}
