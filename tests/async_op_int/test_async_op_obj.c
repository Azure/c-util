// Copyright (c) Microsoft. All rights reserved.

#include <stdint.h>
#include <stdlib.h>
#include <stdalign.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/log_critical_and_terminate.h"
#include "c_pal/sm.h"
#include "c_pal/timer.h"
#include "c_pal/thandle.h"
#include "c_pal/threadapi.h"

#include "c_util/async_op.h"
#include "c_util/doublylinkedlist.h"

#include "testrunnerswitcher.h"

#include "test_async_op_obj.h"

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

#define TEST_STATUS_VALUES  \
    TEST_STATUS_IDLE,    \
    TEST_STATUS_PROCESS,   \
    TEST_STATUS_CLOSE

MU_DEFINE_ENUM(TEST_STATUS, TEST_STATUS_VALUES)
MU_DEFINE_ENUM_STRINGS(TEST_STATUS, TEST_STATUS_VALUES)

static int test_async_op_thread(void* context);

typedef struct EXECUTING_CALL_CONTEXT_TAG
{
    volatile_atomic int32_t status;
    TEST_CALLBACK test_callback;
    void* test_ctx;

    DLIST_ENTRY link;
    THANDLE(ASYNC_OP) async_op;
} EXECUTING_CALL_CONTEXT;

typedef struct TEST_ASYNC_OP_TAG
{
    volatile_atomic int32_t test_state;
    DLIST_ENTRY executed_call_list;

    uint32_t id;
    THREAD_HANDLE thread_handle;
} TEST_ASYNC_OP;

THANDLE_TYPE_DEFINE(TEST_ASYNC_OP);

static void module_dispose(TEST_ASYNC_OP* test_async)
{
    (void)test_async;
}

THANDLE(TEST_ASYNC_OP) test_async_op_create(uint32_t id)
{
    TEST_ASYNC_OP* result;

    result = THANDLE_MALLOC(TEST_ASYNC_OP)(module_dispose);
    ASSERT_IS_NOT_NULL(result);

    DList_InitializeListHead(&result->executed_call_list);

    (void)interlocked_exchange(&result->test_state, TEST_STATUS_IDLE);
    result->id = id;

    return result;
}

int test_async_op_open(THANDLE(TEST_ASYNC_OP) test_async)
{
    int result = 0;
    TEST_ASYNC_OP* test_async_struct = THANDLE_GET_T(TEST_ASYNC_OP)(test_async);

    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&test_async_struct->thread_handle, test_async_op_thread, test_async_struct));
    return result;
}

void test_async_op_close(THANDLE(TEST_ASYNC_OP) test_async)
{
    TEST_ASYNC_OP* test_async_struct = THANDLE_GET_T(TEST_ASYNC_OP)(test_async);

    (void)InterlockedHL_SetAndWake(&test_async_struct->test_state, TEST_STATUS_CLOSE);

    int dont_care;
    (void)ThreadAPI_Join(test_async_struct->thread_handle, &dont_care);
}

static void dispose_async_operation_context(void* context)
{
    (void)context;
}

static void cancel_async_operation_call(void* cancellation_token)
{
    if (cancellation_token == NULL)
    {
        LogCriticalAndTerminate("invalid parameter: context: NULL");
    }
}

int test_async_op_start_call_async(THANDLE(TEST_ASYNC_OP) test_async, TEST_CALLBACK test_callback, void* test_ctx, THANDLE(ASYNC_OP)* async_op)
{
    TEST_ASYNC_OP* test_async_struct = THANDLE_GET_T(TEST_ASYNC_OP)(test_async);

    THANDLE(ASYNC_OP) temp_async_op = async_op_create(cancel_async_operation_call, sizeof(EXECUTING_CALL_CONTEXT), alignof(EXECUTING_CALL_CONTEXT), dispose_async_operation_context);
    ASSERT_IS_NOT_NULL(temp_async_op);

    EXECUTING_CALL_CONTEXT* exec_call_ctx = temp_async_op->context;
    exec_call_ctx->test_callback = test_callback;
    exec_call_ctx->test_ctx = test_ctx;

    THANDLE_INITIALIZE_MOVE(ASYNC_OP)(async_op, &temp_async_op);

    (void)InterlockedHL_SetAndWake(&test_async_struct->test_state, TEST_STATUS_PROCESS);
    THANDLE_INITIALIZE(ASYNC_OP)(&exec_call_ctx->async_op, temp_async_op);

    DList_InsertTailList(&test_async_struct->executed_call_list, &exec_call_ctx->link);
    return 0;
}

static int test_async_op_thread(void* context)
{
    uint32_t timeout_value = UINT32_MAX;

    TEST_ASYNC_OP* test_async = context;
    bool continue_run = true;
    do
    {
        TEST_STATUS current_operation = interlocked_add(&test_async->test_state, 0);
        switch (current_operation)
        {
            case TEST_STATUS_IDLE:
                (void)InterlockedHL_WaitForNotValue(&test_async->test_state, TEST_STATUS_IDLE, timeout_value);
                break;
            case TEST_STATUS_PROCESS:

                PDLIST_ENTRY list_entry = test_async->executed_call_list.Flink;

                while (list_entry != &test_async->executed_call_list)
                {
                    EXECUTING_CALL_CONTEXT* exec_ctx = CONTAINING_RECORD(list_entry, EXECUTING_CALL_CONTEXT, link);
                    list_entry = list_entry->Flink;

                    exec_ctx->test_callback(exec_ctx->test_ctx);

                    // Free the item
                    THANDLE_ASSIGN(ASYNC_OP)(&exec_ctx->async_op, NULL);
                }

                (void)interlocked_compare_exchange(&test_async->test_state, TEST_STATUS_IDLE, TEST_STATUS_PROCESS);
                break;
            case TEST_STATUS_CLOSE:
                continue_run = false;
                break;
        }
    } while (continue_run);
    return 0;
}
