// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/sync.h"
#include "c_pal/threadapi.h"

#include "c_util/worker_thread.h"

TEST_DEFINE_ENUM_TYPE(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

#define WORKER_THREAD_REENTRANCY                10

#define WORKER_THREAD_ITERATIONS                500
#define WORKER_THREAD_ITERATIONS_WITH_SLEEP     100

static void worker_thread_func(void* context)
{
    volatile_atomic int32_t* worker_ctx = (volatile_atomic int32_t*)context;

    (void)interlocked_increment(worker_ctx);
    wake_by_address_single(worker_ctx);
}

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

TEST_FUNCTION(MU_C3(worker_thread_runs_, WORKER_THREAD_ITERATIONS, _process_work_succeeds))
{
    volatile_atomic int32_t worker_ctx;
    (void)interlocked_exchange(&worker_ctx, 0);

    // arrange
    WORKER_THREAD_HANDLE worker_handle = worker_thread_create(worker_thread_func, (void*)&worker_ctx);
    ASSERT_IS_NOT_NULL(worker_handle);

    // act
    ASSERT_ARE_EQUAL(int, 0, worker_thread_open(worker_handle));

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
    ASSERT_ARE_EQUAL(int, 0, worker_thread_open(worker_handle));

    for (int32_t index = 0; index < WORKER_THREAD_ITERATIONS_WITH_SLEEP; index++)
    {
        // Sleep here to let the worker_thread_function to go idle sometimes
        ThreadAPI_Sleep(10);

        ASSERT_ARE_EQUAL(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_OK, worker_thread_schedule_process(worker_handle));

        // assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&worker_ctx, index+1, UINT32_MAX));
    }

    // cleanup
    worker_thread_close(worker_handle);
    worker_thread_destroy(worker_handle);
}

typedef struct WORKER_THREAD_DATA_TAG
{
    volatile_atomic int32_t worker_ctx;
    WORKER_THREAD_HANDLE worker_handle;
} WORKER_THREAD_DATA;

static void worker_thread_schedule_thread_func(void* context)
{
    WORKER_THREAD_DATA* worker_data = (WORKER_THREAD_DATA*)context;

    if (interlocked_add(&worker_data->worker_ctx, 0) < WORKER_THREAD_REENTRANCY)
    {
        ASSERT_ARE_EQUAL(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_OK, worker_thread_schedule_process(worker_data->worker_handle));
        if (interlocked_increment(&worker_data->worker_ctx) == WORKER_THREAD_REENTRANCY)
        {
            wake_by_address_single(&worker_data->worker_ctx);
        }
    }
}

TEST_FUNCTION(run_worker_thread_from_within_worker_thread_schedule_process)
{
    WORKER_THREAD_DATA worker_data;
    (void)interlocked_exchange(&worker_data.worker_ctx, 0);

    // arrange
    worker_data.worker_handle = worker_thread_create(worker_thread_schedule_thread_func, (void*)&worker_data);
    ASSERT_IS_NOT_NULL(worker_data.worker_handle);

    // act
    ASSERT_ARE_EQUAL(int, 0, worker_thread_open(worker_data.worker_handle));

    ASSERT_ARE_EQUAL(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_OK, worker_thread_schedule_process(worker_data.worker_handle));

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&worker_data.worker_ctx, WORKER_THREAD_REENTRANCY, UINT32_MAX));

    // cleanup
    worker_thread_close(worker_data.worker_handle);
    worker_thread_destroy(worker_data.worker_handle);
}

#define WORKER_THREAD_CHAOS_THREADS 16
#define WORKER_THREAD_CHAOS_RUNTIME 30000 // ms
#define WORKER_THREAD_CHAOS_SCHEDULE_MAX_DELAY 50

static volatile_atomic int32_t shutdown_attack_threads;

static int worker_thread_attack_thread(void* arg)
{
    WORKER_THREAD_HANDLE worker_thread = arg;

    while (interlocked_add(&shutdown_attack_threads, 0) == 0)
    {
        ASSERT_ARE_EQUAL(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_OK, worker_thread_schedule_process(worker_thread));
        int delay = rand() * WORKER_THREAD_CHAOS_SCHEDULE_MAX_DELAY / RAND_MAX;
        if (delay > 0)
        {
            ThreadAPI_Sleep(delay);
        }
    }

    return 0;
}

TEST_FUNCTION(worker_thread_chaos_knight)
{
    THREAD_HANDLE threads[WORKER_THREAD_CHAOS_THREADS];

    volatile_atomic int32_t worker_ctx;
    (void)interlocked_exchange(&worker_ctx, 0);

    // arrange
    WORKER_THREAD_HANDLE worker_handle = worker_thread_create(worker_thread_func, (void*)&worker_ctx);
    ASSERT_IS_NOT_NULL(worker_handle);

    ASSERT_ARE_EQUAL(int, 0, worker_thread_open(worker_handle));

    (void)interlocked_exchange(&shutdown_attack_threads, 0);

    // act
    // assert
    for (int32_t index = 0; index < WORKER_THREAD_CHAOS_THREADS; index++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&threads[index], worker_thread_attack_thread, worker_handle));
    }

    ThreadAPI_Sleep(WORKER_THREAD_CHAOS_RUNTIME);

    (void)interlocked_exchange(&shutdown_attack_threads, 1);

    for (int32_t index = 0; index < WORKER_THREAD_CHAOS_THREADS; index++)
    {
        int dont_care;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(threads[index], &dont_care));
    }

    // cleanup
    worker_thread_close(worker_handle);
    worker_thread_destroy(worker_handle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
