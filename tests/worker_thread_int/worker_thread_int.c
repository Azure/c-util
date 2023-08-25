// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/sync.h"
#include "c_pal/threadapi.h"

#include "c_util/worker_thread.h"

TEST_DEFINE_ENUM_TYPE(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

#define WORKER_THREAD_ITERATIONS                300
#define WORKER_THREAD_ITERATIONS_WITH_SLEEP     100

static void worker_thread_func(void* context)
{
    volatile_atomic int32_t* worker_ctx = (volatile_atomic int32_t*)context;

    (void)interlocked_increment(worker_ctx);
    wake_by_address_single(worker_ctx);

    //LogInfo("worker thread function call with value %" PRId32 "", value);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    time_t rand_time = time(NULL);
    srand((unsigned int)rand_time);
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

TEST_FUNCTION(MU_C3(worker_thread_runs, WORKER_THREAD_ITERATIONS, process_work_succeeds))
{
    volatile_atomic int32_t worker_ctx;
    (void)interlocked_exchange(&worker_ctx, 0);

    // arrange
    WORKER_THREAD_HANDLE worker_handle = worker_thread_create(worker_thread_func, (void*)&worker_ctx);
    ASSERT_IS_NOT_NULL(worker_handle);

    // act
    ASSERT_ARE_EQUAL(int32_t, 0, worker_thread_open(worker_handle));

    for (int32_t index = 0; index < WORKER_THREAD_ITERATIONS; index++)
    {
        ASSERT_ARE_EQUAL(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_OK, worker_thread_schedule_process(worker_handle));

        // assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&worker_ctx, index+1, UINT32_MAX));
    }

    // cleanup
    worker_thread_close(worker_handle);
    worker_thread_destroy(worker_handle);
}

TEST_FUNCTION(run_worker_thread_with_waits)
{
    volatile_atomic int32_t worker_ctx;
    (void)interlocked_exchange(&worker_ctx, 0);

    // arrange
    WORKER_THREAD_HANDLE worker_handle = worker_thread_create(worker_thread_func, (void*)&worker_ctx);
    ASSERT_IS_NOT_NULL(worker_handle);

    // act
    ASSERT_ARE_EQUAL(int32_t, 0, worker_thread_open(worker_handle));

    for (int32_t index = 0; index < WORKER_THREAD_ITERATIONS_WITH_SLEEP; index++)
    {
        // Sleep here to let the worker_thread_function to go idle sometimes
        uint32_t sleep_amt = rand() * 10 / (RAND_MAX + 1);
        ThreadAPI_Sleep(sleep_amt);

        ASSERT_ARE_EQUAL(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_OK, worker_thread_schedule_process(worker_handle));

        // assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&worker_ctx, index+1, UINT32_MAX));
    }

    // cleanup
    worker_thread_close(worker_handle);
    worker_thread_destroy(worker_handle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
