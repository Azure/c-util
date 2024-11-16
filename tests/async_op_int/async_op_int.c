// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdlib.h>
#include <stddef.h>


#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/threadapi.h"
#include "c_pal/thandle.h"
#include "c_pal/timer.h"

#include "c_util/async_op.h"

#include "test_async_op_obj.h"

//TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
//TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(ASYNC_OP_STATE, ASYNC_OP_STATE_VALUES);

typedef struct TEST_CONTEXT_TAG
{
    THANDLE(TEST_ASYNC_OP) test_obj;
    volatile_atomic int32_t calls_started;
    volatile_atomic int32_t calls_executed;
    volatile_atomic int32_t stop_thread;
    uint32_t thread_sleep_value;
} TEST_CONTEXT;

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
}

TEST_FUNCTION_INITIALIZE(method_init)
{
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

static void test_start_cb(void* context)
{
    (void)context;
}

TEST_FUNCTION(async_op_test)
{
    ///arrange
    THANDLE(TEST_ASYNC_OP) test_obj = test_async_op_create(1);
    ASSERT_IS_NOT_NULL(test_obj);

    ///act
    (void)test_async_op_open(test_obj);

    THANDLE(ASYNC_OP) async_op = NULL;
    (void)test_async_op_start_call_async(test_obj, test_start_cb, NULL, &async_op);

    ThreadAPI_Sleep(5 * 1000);

    test_async_op_close(test_obj);
}

#define FIVE_SECONDS_RUNTIME        5 * 1000 // ms

static int test_thread_func(void* arg)
{
    TEST_CONTEXT* test_context = arg;

    volatile_atomic int32_t increment_value;

    do
    {
        (void)interlocked_exchange(&increment_value, 0);

        THANDLE(ASYNC_OP) async_op = NULL;

        ASSERT_ARE_EQUAL(int, 0, test_async_op_start_call_async(test_context->test_obj, test_start_cb, (void*)&test_context->calls_executed, &async_op));

        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);

        (void)interlocked_increment(&test_context->calls_started);

        ThreadAPI_Sleep(test_context->thread_sleep_value);
        //break;
    } while (interlocked_add(&test_context->stop_thread, 0) != 1);

    return 0;
}

TEST_FUNCTION(operation_limiter_executes_1_call_per_second_for_5_seconds)
{
    //arrange
    int32_t test_runtime = FIVE_SECONDS_RUNTIME;

    THANDLE(TEST_ASYNC_OP) test_obj = test_async_op_create(1);
    ASSERT_IS_NOT_NULL(test_obj);

    ASSERT_ARE_EQUAL(int, 0, test_async_op_open(test_obj));

    // start a thread that keeps posting calls to the op_limiter
    THREAD_HANDLE test_thread;
    TEST_CONTEXT test_context;
    test_context.thread_sleep_value = 10;

    (void)interlocked_exchange(&test_context.stop_thread, 0);
    (void)interlocked_exchange(&test_context.calls_started, 0);
    (void)interlocked_exchange(&test_context.calls_executed, 0);

    THANDLE_INITIALIZE_MOVE(TEST_ASYNC_OP)(&test_context.test_obj, &test_obj);

    (void)ThreadAPI_Create(&test_thread, test_thread_func, &test_context);

    // act
    // run for some time
    double start_time = timer_global_get_elapsed_ms();
    do
    {
        ThreadAPI_Sleep(1000);
        LogInfo("Made %" PRId32 " calls and it executed %" PRId32 " calls", interlocked_add(&test_context.calls_started, 0), interlocked_add(&test_context.calls_executed, 0));
    } while (timer_global_get_elapsed_ms() - start_time < test_runtime);

    (void)interlocked_exchange(&test_context.stop_thread, 1);

    int dont_care;
    (void)ThreadAPI_Join(test_thread, &dont_care);

    //assert

    //cleanup
    test_async_op_close(test_obj);

    THANDLE_ASSIGN(TEST_ASYNC_OP)(&test_context.test_obj, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
