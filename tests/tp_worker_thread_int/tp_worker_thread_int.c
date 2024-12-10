// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/sync.h"
#include "c_pal/threadapi.h"

#include "c_util/tp_worker_thread.h"

TEST_DEFINE_ENUM_TYPE(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

#define WORKER_THREAD_REENTRANCY                10

#define WORKER_THREAD_ITERATIONS                500
#define WORKER_THREAD_ITERATIONS_WITH_SLEEP     100

typedef struct WORKER_THREAD_CONTEXT_TAG
{
    volatile_atomic int32_t counter;
    volatile_atomic int32_t is_active;
} WORKER_THREAD_CONTEXT;

static void worker_thread_func(void* context)
{
    WORKER_THREAD_CONTEXT* worker_ctx = context;

    if (interlocked_increment(&worker_ctx->is_active) != 1)
    {
        ASSERT_FAIL("worker_thread_func should not be called concurrently!!!");
    }

    (void)interlocked_increment(&worker_ctx->counter);
    wake_by_address_single(&worker_ctx->counter);

    if (interlocked_decrement(&worker_ctx->is_active) != 0)
    {
        ASSERT_FAIL("worker_thread_func should not be called concurrently!!!");
    }
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
    // arrange
    WORKER_THREAD_CONTEXT worker_ctx;
    (void)interlocked_exchange(&worker_ctx.counter, 0);
    (void)interlocked_exchange(&worker_ctx.is_active, 0);

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);

    TP_WORKER_THREAD_HANDLE worker_handle = tp_worker_thread_create(execution_engine, worker_thread_func, &worker_ctx);
    ASSERT_IS_NOT_NULL(worker_handle);

    // act
    ASSERT_ARE_EQUAL(int, 0, tp_worker_thread_open(worker_handle));

    for (int32_t index = 0; index < WORKER_THREAD_ITERATIONS; index++)
    {
        ASSERT_ARE_EQUAL(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_OK, tp_worker_thread_schedule_process(worker_handle));

        // assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&worker_ctx.counter, index+1, UINT32_MAX));
    }

    // cleanup
    tp_worker_thread_close(worker_handle);
    tp_worker_thread_destroy(worker_handle);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(run_worker_thread_with_waits)
{
    // arrange
    WORKER_THREAD_CONTEXT worker_ctx;
    (void)interlocked_exchange(&worker_ctx.counter, 0);
    (void)interlocked_exchange(&worker_ctx.is_active, 0);

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);

    TP_WORKER_THREAD_HANDLE worker_handle = tp_worker_thread_create(execution_engine, worker_thread_func, &worker_ctx);
    ASSERT_IS_NOT_NULL(worker_handle);

    // act
    ASSERT_ARE_EQUAL(int, 0, tp_worker_thread_open(worker_handle));

    for (int32_t index = 0; index < WORKER_THREAD_ITERATIONS_WITH_SLEEP; index++)
    {
        // Sleep here to let the worker_thread_function to go idle sometimes
        ThreadAPI_Sleep(10);

        ASSERT_ARE_EQUAL(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_OK, tp_worker_thread_schedule_process(worker_handle));

        // assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&worker_ctx.counter, index+1, UINT32_MAX));
    }

    // cleanup
    tp_worker_thread_close(worker_handle);
    tp_worker_thread_destroy(worker_handle);
    execution_engine_dec_ref(execution_engine);
}

typedef struct WORKER_THREAD_DATA_TAG
{
    WORKER_THREAD_CONTEXT worker_ctx;
    TP_WORKER_THREAD_HANDLE worker_handle;
} WORKER_THREAD_DATA;

static void worker_thread_schedule_thread_func(void* context)
{
    WORKER_THREAD_DATA* worker_data = (WORKER_THREAD_DATA*)context;

    if (interlocked_add(&worker_data->worker_ctx.counter, 0) < WORKER_THREAD_REENTRANCY)
    {
        ASSERT_ARE_EQUAL(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_OK, tp_worker_thread_schedule_process(worker_data->worker_handle));
        if (interlocked_increment(&worker_data->worker_ctx.counter) == WORKER_THREAD_REENTRANCY)
        {
            wake_by_address_single(&worker_data->worker_ctx.counter);
        }
    }
}

TEST_FUNCTION(run_worker_thread_from_within_worker_thread_schedule_process)
{
    // arrange
    WORKER_THREAD_DATA worker_data;
    (void)interlocked_exchange(&worker_data.worker_ctx.counter, 0);
    (void)interlocked_exchange(&worker_data.worker_ctx.is_active, 0);

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);

    worker_data.worker_handle = tp_worker_thread_create(execution_engine, worker_thread_schedule_thread_func, &worker_data);
    ASSERT_IS_NOT_NULL(worker_data.worker_handle);

    // act
    ASSERT_ARE_EQUAL(int, 0, tp_worker_thread_open(worker_data.worker_handle));

    ASSERT_ARE_EQUAL(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_OK, tp_worker_thread_schedule_process(worker_data.worker_handle));

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&worker_data.worker_ctx.counter, WORKER_THREAD_REENTRANCY, UINT32_MAX));

    // cleanup
    tp_worker_thread_close(worker_data.worker_handle);
    tp_worker_thread_destroy(worker_data.worker_handle);
    execution_engine_dec_ref(execution_engine);
}

#define WORKER_THREAD_CHAOS_THREADS 16
#define WORKER_THREAD_CHAOS_RUNTIME 30000 // ms
#define WORKER_THREAD_CHAOS_SCHEDULE_MAX_DELAY 50

static volatile_atomic int32_t shutdown_attack_threads;

static int worker_thread_attack_thread(void* arg)
{
    TP_WORKER_THREAD_HANDLE worker_thread = arg;

    while (interlocked_add(&shutdown_attack_threads, 0) == 0)
    {
        ASSERT_ARE_EQUAL(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_OK, tp_worker_thread_schedule_process(worker_thread));
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
    // arrange
    THREAD_HANDLE threads[WORKER_THREAD_CHAOS_THREADS];

    WORKER_THREAD_CONTEXT worker_ctx;
    (void)interlocked_exchange(&worker_ctx.counter, 0);
    (void)interlocked_exchange(&worker_ctx.is_active, 0);

    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);

    TP_WORKER_THREAD_HANDLE worker_handle = tp_worker_thread_create(execution_engine, worker_thread_func, &worker_ctx);
    ASSERT_IS_NOT_NULL(worker_handle);

    ASSERT_ARE_EQUAL(int, 0, tp_worker_thread_open(worker_handle));

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
    tp_worker_thread_close(worker_handle);
    tp_worker_thread_destroy(worker_handle);
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
