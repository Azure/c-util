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
#include "c_pal/thandle.h"
#include "c_pal/threadapi.h"
#include "c_pal/threadpool.h"

#include "c_util/async_op.h"

#include "common_async_op_module_interface.h"

#include "hl_async_op_module.h"

typedef struct HL_ASYNC_OP_MODULE_TAG
{
    SM_HANDLE sm;
    EXECUTION_ENGINE_HANDLE execution_engine;
    THANDLE(THREADPOOL) threadpool;

    COMMON_OP_MODULE_INTERFACE_HANDLE ll_async_op_module;
} HL_ASYNC_OP_MODULE;

typedef struct HL_ASYNC_OP_MODULE_EXECUTE_CONTEXT_TAG
{
    COMMON_ASYNC_OP_MODULE_EXECUTE_CALLBACK callback;
    void* context;

    // ll call context
    THANDLE(ASYNC_OP) ll_async_op;

    // Pointer back for sm_exec_end
    HL_ASYNC_OP_MODULE_HANDLE handle;
} HL_ASYNC_OP_MODULE_EXECUTE_CONTEXT;

HL_ASYNC_OP_MODULE_HANDLE hl_async_op_module_create(EXECUTION_ENGINE_HANDLE execution_engine, COMMON_OP_MODULE_INTERFACE_HANDLE ll_async_op_module)
{
    HL_ASYNC_OP_MODULE_HANDLE result;

    if (execution_engine == NULL ||
        ll_async_op_module == NULL)
    {
        LogError("Invalid arguments EXECUTION_ENGINE_HANDLE execution_engine=%p, COMMON_OP_MODULE_INTERFACE_HANDLE ll_async_op_module=%p",
            execution_engine, ll_async_op_module);
        result = NULL;
    }
    else
    {
        result = malloc(sizeof(HL_ASYNC_OP_MODULE));
        if (result == NULL)
        {
            LogError("malloc HL_ASYNC_OP_MODULE failed");
        }
        else
        {
            result->sm = sm_create("hl_async_op_module");
            if (result->sm == NULL)
            {
                LogError("sm_create failed");
            }
            else
            {
                execution_engine_inc_ref(execution_engine);
                result->execution_engine = execution_engine;
                THANDLE_INITIALIZE(THREADPOOL)(&result->threadpool, NULL);

                result->ll_async_op_module = ll_async_op_module;

                goto all_ok;
            }
            free(result);
            result = NULL;
        }
    }
all_ok:
    return result;
}

static void hl_async_op_module_close_internal(HL_ASYNC_OP_MODULE_HANDLE handle)
{
    if (sm_close_begin(handle->sm) == SM_EXEC_GRANTED)
    {
        THANDLE_ASSIGN(THREADPOOL)(&handle->threadpool, NULL);
        sm_close_end(handle->sm);
    }
}

void hl_async_op_module_destroy(HL_ASYNC_OP_MODULE_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("Invalid argument HL_ASYNC_OP_MODULE_HANDLE handle=%p", handle);
    }
    else
    {
        hl_async_op_module_close_internal(handle);
        execution_engine_dec_ref(handle->execution_engine);
        sm_destroy(handle->sm);
        free(handle);
    }
}


int hl_async_op_module_open(HL_ASYNC_OP_MODULE_HANDLE handle)
{
    int result;
    if (handle == NULL)
    {
        LogError("Invalid argument HL_ASYNC_OP_MODULE_HANDLE handle=%p", handle);
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

void hl_async_op_module_close(HL_ASYNC_OP_MODULE_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("Invalid argument HL_ASYNC_OP_MODULE_HANDLE handle=%p", handle);
    }
    else
    {
        hl_async_op_module_close_internal(handle);
    }
}

static void on_async_op_HL_ASYNC_OP_MODULE_EXECUTE_CONTEXT_cancel(void* context)
{
    HL_ASYNC_OP_MODULE_EXECUTE_CONTEXT* call_context = context;
    // 1. Just call cancel on the lower layer async_op, nothing we need to do with the return value
    //    Note that ll_async_op is always set to a valid value before it is possible to call cancel (cannot be NULL)
    LogInfo("Call was canceled, cancel the async_op for the lower layer");
    (void)async_op_cancel(call_context->ll_async_op);
}

static void on_async_op_HL_ASYNC_OP_MODULE_EXECUTE_CONTEXT_dispose(void* context)
{
    HL_ASYNC_OP_MODULE_EXECUTE_CONTEXT* call_context = context;
    // Clean up the underlying ll_async_op
    THANDLE_ASSIGN(ASYNC_OP)(&call_context->ll_async_op, NULL);
}

static void hl_async_op_module_on_ml_complete(void* context, COMMON_ASYNC_OP_MODULE_RESULT result)
{
    if (context == NULL)
    {
        LogCriticalAndTerminate("Invalid arguments: void* context=%p, COMMON_ASYNC_OP_MODULE_RESULT result=%" PRI_MU_ENUM "",
            context, MU_ENUM_VALUE(COMMON_ASYNC_OP_MODULE_RESULT, result));
    }
    else
    {
        THANDLE(ASYNC_OP) async_op = context;
        HL_ASYNC_OP_MODULE_EXECUTE_CONTEXT* async_op_context = async_op->context;

        // Note that sm_exec_end is called here so that the callback could call close on the module without a deadlock
        sm_exec_end(async_op_context->handle->sm);

        // 1. Do any processing and call the callback based on the result
        switch (result)
        {
            default:
            {
                LogInfo("ML Call completed with an error: %" PRI_MU_ENUM "",
                    MU_ENUM_VALUE(COMMON_ASYNC_OP_MODULE_RESULT, result));
                async_op_context->callback(async_op_context->context, COMMON_ASYNC_OP_MODULE_ERROR);
                break;
            }
            case COMMON_ASYNC_OP_MODULE_OK:
            {
                LogInfo("ML Call completed normally");
                async_op_context->callback(async_op_context->context, COMMON_ASYNC_OP_MODULE_OK);
                break;
            }
            case COMMON_ASYNC_OP_MODULE_CANCELED:
            {
                LogInfo("ML Call was canceled");
                async_op_context->callback(async_op_context->context, COMMON_ASYNC_OP_MODULE_CANCELED);
                break;
            }
        }

        // 2. Clean up the async_op
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    }
}

int hl_async_op_module_execute_async(HL_ASYNC_OP_MODULE_HANDLE handle, uint32_t complete_in_ms, THANDLE(ASYNC_OP)* async_op_out, COMMON_ASYNC_OP_MODULE_EXECUTE_CALLBACK callback, void* context)
{
    int result;
    if (
        handle == NULL ||
        async_op_out == NULL ||
        callback == NULL
        )
    {
        LogError("Invalid arguments HL_ASYNC_OP_MODULE_HANDLE handle=%p, uint32_t complete_in_ms=%" PRIu32 ", THANDLE(ASYNC_OP)* async_op_out=%p, COMMON_ASYNC_OP_MODULE_EXECUTE_CALLBACK callback=%p, void* context=%p",
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
            THANDLE(ASYNC_OP) async_op = async_op_create(on_async_op_HL_ASYNC_OP_MODULE_EXECUTE_CONTEXT_cancel, sizeof(HL_ASYNC_OP_MODULE_EXECUTE_CONTEXT), alignof(HL_ASYNC_OP_MODULE_EXECUTE_CONTEXT), on_async_op_HL_ASYNC_OP_MODULE_EXECUTE_CONTEXT_dispose);
            if (async_op == NULL)
            {
                LogError("async_op_create failed");
                result = MU_FAILURE;
            }
            else
            {
                // 2. Any context needed is stored in async_op->context
                HL_ASYNC_OP_MODULE_EXECUTE_CONTEXT* async_op_context = async_op->context;

                async_op_context->callback = callback;
                async_op_context->context = context;
                async_op_context->handle = handle;

                // 3. Initialize the ll_async_op to NULL
                THANDLE_INITIALIZE(ASYNC_OP)(&async_op_context->ll_async_op, NULL);

                // 4. Store the async_op from the LL in a temporary variable
                THANDLE(ASYNC_OP) ll_async_op = NULL;

                // 5. Take an additional reference on the async_op, we have 1 reference for returning to the caller (async_op), and 1 reference for the async work callback (async_op_ref_for_callback)
                //    Need to take the reference before starting the operation below because its callback may come immediately
                THANDLE(ASYNC_OP) async_op_ref_for_callback = NULL;
                THANDLE_ASSIGN(ASYNC_OP)(&async_op_ref_for_callback, async_op);

                // 6. Start the async work, passing the async_op_ref_for_callback as the context
                if (handle->ll_async_op_module->execute_async(handle->ll_async_op_module->handle, complete_in_ms, &ll_async_op, hl_async_op_module_on_ml_complete, (void*)async_op_ref_for_callback) != 0)
                {
                    LogError("ll_execute_async for lower module failed");
                    result = MU_FAILURE;
                }
                else
                {
                    // 7. Store the ll_async_op in the context on success so that it can be canceled
                    //    This must happen before the next line so that the ll_async_op is set before cancel is called
                    THANDLE_MOVE(ASYNC_OP)(&async_op_context->ll_async_op, &ll_async_op);

                    // 8. Provide the async_op to the caller
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

static int hl_async_op_module_open_interface_adapter(void* context)
{
    return hl_async_op_module_open(context);
}

static void hl_async_op_module_close_interface_adapter(void* context)
{
    hl_async_op_module_close(context);
}

static void hl_async_op_module_destroy_interface_adapter(void* context)
{
    hl_async_op_module_destroy(context);
}

static int hl_async_op_module_execute_async_interface_adapter(void* context, uint32_t complete_in_ms, THANDLE(ASYNC_OP)* async_op_out, COMMON_ASYNC_OP_MODULE_EXECUTE_CALLBACK callback, void* context_callback)
{
    return hl_async_op_module_execute_async(context, complete_in_ms, async_op_out, callback, context_callback);
}

COMMON_ASYNC_OP_MODULE_INTERFACE hl_async_op_module_get_interface(HL_ASYNC_OP_MODULE_HANDLE handle)
{
    COMMON_ASYNC_OP_MODULE_INTERFACE result = (COMMON_ASYNC_OP_MODULE_INTERFACE)
    {
        .handle = handle,
        .open = hl_async_op_module_open_interface_adapter,
        .close = hl_async_op_module_close_interface_adapter,
        .destroy = hl_async_op_module_destroy_interface_adapter,
        .execute_async = hl_async_op_module_execute_async_interface_adapter,
        .execute_async_no_async_op_out = NULL
    };
    return result;
}
