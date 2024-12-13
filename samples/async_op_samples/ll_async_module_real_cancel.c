// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <inttypes.h>
#include <stdalign.h>
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
#include "c_pal/thandle.h"
#include "c_pal/threadapi.h"
#include "c_pal/threadpool.h"

#include "c_util/async_op.h"

#include "ll_async_module_real_cancel.h"

typedef struct LL_ASYNC_OP_MODULE_REAL_CANCEL_TAG
{
    SM_HANDLE sm;
    EXECUTION_ENGINE_HANDLE execution_engine;
    THANDLE(THREADPOOL) threadpool;
} LL_ASYNC_OP_MODULE_REAL_CANCEL;

typedef struct LL_ASYNC_OP_MODULE_REAL_CANCEL_EXECUTE_CONTEXT_TAG
{
    LL_ASYNC_OP_MODULE_REAL_CANCEL_EXECUTE_CALLBACK callback;
    void* context;

    // control the real execution
    uint32_t complete_in_ms;
    volatile_atomic uint32_t underlying_cancel_happened;

    // Pointer back for sm_exec_end
    LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE handle;
} LL_ASYNC_OP_MODULE_REAL_CANCEL_EXECUTE_CONTEXT;

MU_DEFINE_ENUM_STRINGS(LL_ASYNC_OP_MODULE_REAL_CANCEL_RESULT, LL_ASYNC_OP_MODULE_REAL_CANCEL_RESULT_VALUES)

IMPLEMENT_MOCKABLE_FUNCTION(, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE, ll_async_op_module_real_cancel_create, EXECUTION_ENGINE_HANDLE, execution_engine)
{
    LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE result;

    if (execution_engine == NULL)
    {
        LogError("Invalid arguments EXECUTION_ENGINE_HANDLE execution_engine=%p", execution_engine);
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(LL_ASYNC_OP_MODULE_REAL_CANCEL));
        if (result == NULL)
        {
            LogError("malloc LL_ASYNC_OP_MODULE_REAL_CANCEL failed");
        }
        else
        {
            result->sm = sm_create("ll_async_op_module_real_cancel");
            if (result->sm == NULL)
            {
                LogError("sm_create failed");
            }
            else
            {
                execution_engine_inc_ref(execution_engine);
                result->execution_engine = execution_engine;
                THANDLE_INITIALIZE(THREADPOOL)(&result->threadpool, NULL);
                goto all_ok;
            }
            free(result);
            result = NULL;
        }
    }
all_ok:
    return result;
}

static void ll_async_op_module_real_cancel_close_internal(LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE handle)
{
    if (sm_close_begin(handle->sm) == SM_EXEC_GRANTED)
    {
        THANDLE_ASSIGN(THREADPOOL)(&handle->threadpool, NULL);
        sm_close_end(handle->sm);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, ll_async_op_module_real_cancel_destroy, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE, handle)
{
    if (handle == NULL)
    {
        LogError("Invalid argument LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE handle=%p", handle);
    }
    else
    {
        ll_async_op_module_real_cancel_close_internal(handle);
        sm_destroy(handle->sm);
        free(handle);
    }
}


IMPLEMENT_MOCKABLE_FUNCTION(, int, ll_async_op_module_real_cancel_open, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE, handle)
{
    int result;
    if (handle == NULL)
    {
        LogError("Invalid argument LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE handle=%p", handle);
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

IMPLEMENT_MOCKABLE_FUNCTION(, void, ll_async_op_module_real_cancel_close, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE, handle)
{
    if (handle == NULL)
    {
        LogError("Invalid argument LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE handle=%p", handle);
    }
    else
    {
        ll_async_op_module_real_cancel_close_internal(handle);
    }
}

static void on_async_op_LL_ASYNC_OP_MODULE_REAL_CANCEL_EXECUTE_CONTEXT_cancel(void* context)
{
    LL_ASYNC_OP_MODULE_REAL_CANCEL_EXECUTE_CONTEXT* call_context = context;
    // 1. Just signal to the underlying operation that it should cancel (e.g. stop the IO)
    LogInfo("Call was canceled, force the underlayer to stop what it's doing");
    InterlockedHL_SetAndWake(&call_context->underlying_cancel_happened, 1);
}

static void on_async_op_LL_ASYNC_OP_MODULE_REAL_CANCEL_EXECUTE_CONTEXT_dispose(void* context)
{
    // This context has nothing additional to cleanup
    (void)context;
}

static void ll_async_op_module_real_cancel_execute_the_async_worker_thread(void* context)
{
    if (context == NULL)
    {
        LogCriticalAndTerminate("Invalid arguments: void* context=%p", context);
    }
    else
    {
        THANDLE(ASYNC_OP) async_op = context;
        LL_ASYNC_OP_MODULE_REAL_CANCEL_EXECUTE_CONTEXT* async_op_context = async_op->context;

        // Simulate the operation taking some time to complete, real operation would do some IO for example
        INTERLOCKED_HL_RESULT interlocked_result = InterlockedHL_WaitForValue(&async_op_context->underlying_cancel_happened, 1, async_op_context->complete_in_ms);

        // Note that sm_exec_end is called here so that the callback could call close on the module without a deadlock
        sm_exec_end(async_op_context->handle->sm);

        if (interlocked_result == INTERLOCKED_HL_OK)
        {
            // 1. If the underlayer reported the reason was cancel, then report the result as canceled
            LogInfo("Call was canceled");
            async_op_context->callback(async_op_context->context, LL_ASYNC_OP_MODULE_REAL_CANCEL_CANCELLED);
        }
        else
        {
            // 2. Otherwise, complete normally
            LogInfo("Call completed normally");
            async_op_context->callback(async_op_context->context, LL_ASYNC_OP_MODULE_REAL_CANCEL_OK);
        }

        // 3. Can clean up the async_op now
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, ll_async_op_module_real_cancel_execute_async, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE, handle, uint32_t, complete_in_ms, THANDLE(ASYNC_OP)*, async_op_out, LL_ASYNC_OP_MODULE_REAL_CANCEL_EXECUTE_CALLBACK, callback, void*, context)
{
    int result;
    if (
        handle == NULL ||
        async_op_out == NULL ||
        callback == NULL
        )
    {
        LogError("Invalid arguments LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE handle=%p, uint32_t complete_in_ms=%" PRIu32 ", THANDLE(ASYNC_OP)* async_op_out=%p, LL_ASYNC_OP_MODULE_REAL_CANCEL_EXECUTE_CALLBACK callback=%p, void* context=%p",
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
            THANDLE(ASYNC_OP) async_op = async_op_create(on_async_op_LL_ASYNC_OP_MODULE_REAL_CANCEL_EXECUTE_CONTEXT_cancel, sizeof(LL_ASYNC_OP_MODULE_REAL_CANCEL_EXECUTE_CONTEXT), alignof(LL_ASYNC_OP_MODULE_REAL_CANCEL_EXECUTE_CONTEXT), on_async_op_LL_ASYNC_OP_MODULE_REAL_CANCEL_EXECUTE_CONTEXT_dispose);
            if (async_op == NULL)
            {
                LogError("async_op_create failed");
                result = MU_FAILURE;
            }
            else
            {
                // 2. Any context needed is stored in async_op->context
                LL_ASYNC_OP_MODULE_REAL_CANCEL_EXECUTE_CONTEXT* async_op_context = async_op->context;

                async_op_context->callback = callback;
                async_op_context->context = context;
                async_op_context->complete_in_ms = complete_in_ms;
                async_op_context->handle = handle;
                // The interlocked below would not be needed in a real program, assume there is some "thing" in the layer below we call to tell the operation to complete
                (void)interlocked_exchange(&async_op_context->underlying_cancel_happened, 0);

                // 3. Take an additional reference on the async_op, we have 1 reference for returning to the caller, and 1 reference for the async work callback
                //    Need to take the reference before starting the operation below because its callback may come immediately
                THANDLE(ASYNC_OP) async_op_ref_for_callback = NULL;
                THANDLE_ASSIGN(ASYNC_OP)(&async_op_ref_for_callback, async_op);

                // 4. Start the async work, passing the async_op_ref_for_callback as the context
                if (threadpool_schedule_work(handle->threadpool, ll_async_op_module_real_cancel_execute_the_async_worker_thread, (void*)async_op_ref_for_callback) != 0)
                {
                    LogError("threadpool_schedule_work failed");
                    result = MU_FAILURE;
                }
                else
                {
                    // 5. Provide the async_op to the caller
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
