// Copyright (c) Microsoft. All rights reserved.


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_pal/threadapi.h"
#include "c_pal/interlocked_hl.h"
#include "c_util/rc_string.h"
#include "c_pal/thandle.h"

#include "c_util/watchdog.h"
#include "c_util/watchdog_threadpool.h"

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

typedef struct TEST_TASK_CONTEXT_TAG
{
    volatile_atomic int32_t can_complete;
    volatile_atomic int32_t done;
} TEST_TASK_CONTEXT;

static int test_task_thread(void* context)
{
    LogInfo("begin \"long running\" task...");
    TEST_TASK_CONTEXT* task_context = context;
    (void)InterlockedHL_WaitForValue(&task_context->can_complete, 1, UINT32_MAX);

    LogInfo("...complete \"long running\" task");

    (void)interlocked_exchange(&task_context->done, 1);
    wake_by_address_single(&task_context->done);

    return 0;
}

typedef struct WATCHDOG_CONTEXT_TAG
{
    volatile_atomic int32_t count;
} WATCHDOG_CONTEXT;
static WATCHDOG_CONTEXT g_watchdog;

static void test_watchdog_handler(void* context, const char* message)
{
    LogInfo("Watchdog fired!!! (message=%s)", message);
    WATCHDOG_CONTEXT* watchdog_context = context;
    (void)interlocked_increment(&watchdog_context->count);
}

typedef struct TEST_WATCHDOG_EVENTS_TAG
{
    WATCHDOG_HANDLE watchdog;
    volatile_atomic int32_t completion_count;
} TEST_WATCHDOG_EVENTS;

static int test_watchdog_reset_thread(void* context)
{
    LogInfo("begin watchdog reset thread");
    ASSERT_IS_NOT_NULL(context);
    TEST_WATCHDOG_EVENTS* watchdog_events = context;
    watchdog_reset(watchdog_events->watchdog);
    (void)interlocked_increment(&watchdog_events->completion_count);
    return 0;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    (void)interlocked_exchange(&g_watchdog.count, 0);
    ASSERT_ARE_EQUAL(int, 0, watchdog_threadpool_init());
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    watchdog_threadpool_deinit();
}

TEST_FUNCTION(when_something_completes_watchdog_does_not_fire)
{
    // arrange
    THREAD_HANDLE work_thread;
    TEST_TASK_CONTEXT task_context;
    (void)interlocked_exchange(&task_context.can_complete, 0);
    (void)interlocked_exchange(&task_context.done, 0);

    LogInfo("Begin watchdog");
    THANDLE(RC_STRING) message = rc_string_create("test watchdog should never fire");
    THANDLE(THREADPOOL) threadpool = watchdog_threadpool_get();
    ASSERT_IS_NOT_NULL(threadpool);
    WATCHDOG_HANDLE watchdog = watchdog_start(threadpool, 5000, message, test_watchdog_handler, &g_watchdog);

    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&work_thread, test_task_thread, &task_context));

    // act
    ThreadAPI_Sleep(1000);
    (void)interlocked_exchange(&task_context.can_complete, 1);
    wake_by_address_single(&task_context.can_complete);

    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&task_context.done, 1, 10000));
    watchdog_stop(watchdog);
    LogInfo("Stopped watchdog");

    // assert
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&g_watchdog.count, 0));

    // cleanup
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(work_thread, NULL));
    THANDLE_ASSIGN(RC_STRING)(&message, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

TEST_FUNCTION(when_something_completes_watchdog_does_not_fire_in_loop_with_reset)
{
    // arrange
    THREAD_HANDLE work_thread[4];
    TEST_TASK_CONTEXT task_context;
    (void)interlocked_exchange(&task_context.can_complete, 0);
    (void)interlocked_exchange(&task_context.done, 0);

    LogInfo("Begin watchdog");
    THANDLE(RC_STRING) message = rc_string_create("test watchdog should never fire");
    THANDLE(THREADPOOL) threadpool = watchdog_threadpool_get();
    ASSERT_IS_NOT_NULL(threadpool);
    WATCHDOG_HANDLE watchdog = watchdog_start(threadpool, 5000, message, test_watchdog_handler, &g_watchdog);

    for (uint32_t i = 0; i < 4; ++i)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&work_thread[i], test_task_thread, &task_context));

        // act
        ThreadAPI_Sleep(1000);
        (void)interlocked_exchange(&task_context.can_complete, 1);
        wake_by_address_single(&task_context.can_complete);

        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&task_context.done, 1, 10000));
        watchdog_reset(watchdog);
        LogInfo("Reset watchdog");
    }

    watchdog_stop(watchdog);
    LogInfo("Stopped watchdog");

    // assert
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&g_watchdog.count, 0));

    // cleanup
    for (uint32_t i = 0; i < 4; ++i)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(work_thread[i], NULL));
    }
    THANDLE_ASSIGN(RC_STRING)(&message, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

TEST_FUNCTION(when_something_takes_too_long_watchdog_fires_exactly_once)
{
    // arrange
    THREAD_HANDLE work_thread;
    TEST_TASK_CONTEXT task_context;
    (void)interlocked_exchange(&task_context.can_complete, 0);
    (void)interlocked_exchange(&task_context.done, 0);

    LogInfo("Begin watchdog");
    THANDLE(RC_STRING) message = rc_string_create("test watchdog is expected");
    THANDLE(THREADPOOL) threadpool = watchdog_threadpool_get();
    ASSERT_IS_NOT_NULL(threadpool);
    WATCHDOG_HANDLE watchdog = watchdog_start(threadpool, 100, message, test_watchdog_handler, &g_watchdog);

    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&work_thread, test_task_thread, &task_context));

    // act
    ThreadAPI_Sleep(3000);
    (void)interlocked_exchange(&task_context.can_complete, 1);
    wake_by_address_single(&task_context.can_complete);

    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&task_context.done, 1, 10000));

    watchdog_stop(watchdog);
    LogInfo("Stopped watchdog");

    // assert
    ASSERT_ARE_EQUAL(int32_t, 1, interlocked_add(&g_watchdog.count, 0));

    // cleanup
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(work_thread, NULL));
    THANDLE_ASSIGN(RC_STRING)(&message, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

TEST_FUNCTION(when_something_takes_too_long_watchdog_fires_exactly_once_after_reset)
{
    // arrange
    THREAD_HANDLE work_thread;
    TEST_TASK_CONTEXT task_context;
    (void)interlocked_exchange(&task_context.can_complete, 0);
    (void)interlocked_exchange(&task_context.done, 0);

    LogInfo("Begin watchdog");
    THANDLE(RC_STRING) message = rc_string_create("test watchdog is expected");
    THANDLE(THREADPOOL) threadpool = watchdog_threadpool_get();
    ASSERT_IS_NOT_NULL(threadpool);

    WATCHDOG_HANDLE watchdog = watchdog_start(threadpool, 3000, message, test_watchdog_handler, &g_watchdog);

    ThreadAPI_Sleep(50);
    watchdog_reset(watchdog);

    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&work_thread, test_task_thread, &task_context));

    // act
    ThreadAPI_Sleep(5000);
    (void)interlocked_exchange(&task_context.can_complete, 1);
    wake_by_address_single(&task_context.can_complete);

    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&task_context.done, 1, 10000));
    watchdog_stop(watchdog);
    LogInfo("Stopped watchdog");

    // assert
    ASSERT_ARE_EQUAL(int32_t, 1, interlocked_add(&g_watchdog.count, 0));

    // cleanup
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(work_thread, NULL));
    THANDLE_ASSIGN(RC_STRING)(&message, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

TEST_FUNCTION(can_run_watchdog_reset_concurrently)
{
    // arrange
    THREAD_HANDLE reset_thread[2];

    TEST_WATCHDOG_EVENTS watchdog_event;
    (void)interlocked_exchange(&watchdog_event.completion_count, 0);

    LogInfo("Begin watchdog");
    THANDLE(RC_STRING) message = rc_string_create("test watchdog is expected after dueling resets");
    THANDLE(THREADPOOL) threadpool = watchdog_threadpool_get();
    ASSERT_IS_NOT_NULL(threadpool);

    WATCHDOG_HANDLE watchdog = watchdog_start(threadpool, 500, message, test_watchdog_handler, &g_watchdog);
    watchdog_event.watchdog = watchdog;

    // act 
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&reset_thread[0], test_watchdog_reset_thread, &watchdog_event));
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&reset_thread[1], test_watchdog_reset_thread, &watchdog_event));

    // wait long enough to allow expiration after reset.
    ThreadAPI_Sleep(1000);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&watchdog_event.completion_count, 2, 500));
    ASSERT_ARE_EQUAL(int32_t, 1, interlocked_add(&g_watchdog.count, 0));

    // cleanup
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(reset_thread[0], NULL));
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(reset_thread[1], NULL));
    watchdog_stop(watchdog);
    THANDLE_ASSIGN(RC_STRING)(&message, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
