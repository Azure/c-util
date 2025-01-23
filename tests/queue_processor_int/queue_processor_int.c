// Copyright (c) Microsoft. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_pal/threadapi.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle.h"
#include "c_pal/interlocked_hl.h"

#include "c_util/queue_processor.h"

TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES)

static volatile_atomic int32_t actual_call_count;
#define CHAOS_TEST_RUN_TIME 10000

static void on_error(void* context)
{
    (void)context;
    ASSERT_FAIL("on_error called");
}

static void item_process_function(void* context)
{
    volatile_atomic int32_t* expected_call_count_ptr = context;
    ASSERT_ARE_EQUAL(int32_t, *expected_call_count_ptr, actual_call_count);
    (void)interlocked_increment(&actual_call_count);
    wake_by_address_single(&actual_call_count);
}

static void process_item_count_calls(void* context)
{
    volatile_atomic int32_t* call_count = context;
    (void)interlocked_increment(call_count);
    wake_by_address_single(call_count);
}

static THANDLE(THREADPOOL) create_threadpool(EXECUTION_ENGINE_HANDLE execution_engine)
{
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    return threadpool;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
    time_t seed = time(NULL);
    LogInfo("Test using random seed = %u", (unsigned int)seed);
    srand((unsigned int)seed);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    (void)interlocked_exchange(&actual_call_count, 0);
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

TEST_FUNCTION(one_schedule_works)
{
    // assert
    // create an execution engine
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);
    THANDLE(THREADPOOL) threadpool = create_threadpool(execution_engine);

    // create the queue processor
    QUEUE_PROCESSOR_HANDLE queue_processor = queue_processor_create(threadpool);

    // open
    ASSERT_ARE_EQUAL(int, 0, queue_processor_open(queue_processor, on_error, NULL));

    int64_t expected_call_count = 0;

    // act (schedule work one time)
    ASSERT_ARE_EQUAL(int, 0, queue_processor_schedule_item(queue_processor, item_process_function, &expected_call_count));

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&actual_call_count, 1, UINT32_MAX));

    // cleanup
    queue_processor_destroy(queue_processor);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

#define N_WORK_ITEMS 100
TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _work_items_works))
{
    // assert
    // create an execution engine
    int64_t* expected_call_ids = malloc(sizeof(int64_t) * N_WORK_ITEMS);
    ASSERT_IS_NOT_NULL(expected_call_ids);
    size_t i;
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);
    THANDLE(THREADPOOL) threadpool = create_threadpool(execution_engine);

    // create the queue processor
    QUEUE_PROCESSOR_HANDLE queue_processor = queue_processor_create(threadpool);

    // open
    ASSERT_ARE_EQUAL(int, 0, queue_processor_open(queue_processor, on_error, NULL));

    // act (schedule work one time)
    for (i = 0; i < N_WORK_ITEMS; i++)
    {
        expected_call_ids[i] = (int64_t)i;
        ASSERT_ARE_EQUAL(int, 0, queue_processor_schedule_item(queue_processor, item_process_function, &expected_call_ids[i]));
    }

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&actual_call_count, (int32_t)i, UINT32_MAX));

    // cleanup
    queue_processor_destroy(queue_processor);

    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
    free(expected_call_ids);
}

#define CHAOS_THREAD_COUNT 4

typedef struct CHAOS_TEST_DATA_TAG
{
    volatile_atomic int32_t expected_call_count;
    volatile_atomic int32_t executed_work_functions;
    volatile_atomic int32_t chaos_test_done;
    QUEUE_PROCESSOR_HANDLE queue_processor;
} CHAOS_TEST_DATA;

static int chaos_thread_func(void* arg)
{
    CHAOS_TEST_DATA* chaos_test_data = arg;

    while (interlocked_add(&chaos_test_data->chaos_test_done, 0) == 0)
    {
        int which_action = rand() * 3 / RAND_MAX;
        switch (which_action)
        {
        case 0:
            // perform an open
            (void)queue_processor_open(chaos_test_data->queue_processor, on_error, NULL);
            break;
        case 1:
            // perform a close
            queue_processor_close(chaos_test_data->queue_processor);
            break;
        case 2:
            // perform a schedule item
            if (queue_processor_schedule_item(chaos_test_data->queue_processor, process_item_count_calls, (void*)&chaos_test_data->executed_work_functions) == 0)
            {
                (void)interlocked_increment(&chaos_test_data->expected_call_count);
            }
            break;
        }
    }
    return 0;
}

TEST_FUNCTION(chaos_knight_test)
{
    // start a number of threads and each of them will do a random action on the queue_processor
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    THANDLE(THREADPOOL) threadpool = create_threadpool(execution_engine);
    THREAD_HANDLE thread_handles[CHAOS_THREAD_COUNT];
    size_t i;
    CHAOS_TEST_DATA chaos_test_data;
    chaos_test_data.queue_processor = queue_processor_create(threadpool);

    (void)interlocked_exchange(&chaos_test_data.expected_call_count, 0);
    (void)interlocked_exchange(&chaos_test_data.executed_work_functions, 0);
    (void)interlocked_exchange(&chaos_test_data.chaos_test_done, 0);

    for (i = 0; i < CHAOS_THREAD_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&thread_handles[i], chaos_thread_func, &chaos_test_data), "thread %zu failed to start", i);
    }

    // wait for some time
    ThreadAPI_Sleep(CHAOS_TEST_RUN_TIME);

    (void)interlocked_exchange(&chaos_test_data.chaos_test_done, 1);

    // wait for all threads to complete
    for (i = 0; i < CHAOS_THREAD_COUNT; i++)
    {
        int dont_care;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(thread_handles[i], &dont_care), "thread %zu failed to join", i);
    }

    LogInfo("Waiting for all expected calls to complete");
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&chaos_test_data.expected_call_count, chaos_test_data.executed_work_functions, UINT32_MAX));

    // call close
    queue_processor_close(chaos_test_data.queue_processor);

    // cleanup
    queue_processor_destroy(chaos_test_data.queue_processor);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
