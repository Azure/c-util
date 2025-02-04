// Copyright(C) Microsoft Corporation.All rights reserved.

#include <math.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/timer.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"

#include "c_util/constbuffer.h"
#include "c_util/constbuffer_array.h"
#include "c_pal/interlocked_hl.h"

#include "testrunnerswitcher.h"

#include "queue_processor_perf_config.h"
#include "c_util/queue_processor.h"

TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    /*if nobody set the config (such is the case when these tests are run as a .dll in VsTest) then use the default config*/
    if (queue_processor_perf_config == NULL)
    {
        queue_processor_perf_config = &default_queue_processor_perf_config;
    }
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
}

TEST_FUNCTION_INITIALIZE(init)
{
}

TEST_FUNCTION_CLEANUP(cleanup)
{
}

static volatile_atomic int32_t pending_items;

static THANDLE(THREADPOOL) create_threadpool(EXECUTION_ENGINE_HANDLE execution_engine)
{
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    return threadpool;
}

static void do_nothing(void* context)
{
    (void)context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_DecrementAndWake(&pending_items));
}

static void queue_processor_on_error(void* context)
{
    (void)context;
}

TEST_FUNCTION(queue_processor_perf_test)
{
    //arrange
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);
    THANDLE(THREADPOOL) threadpool = create_threadpool(execution_engine);
    QUEUE_PROCESSOR_HANDLE queue_processor = queue_processor_create(threadpool);
    ASSERT_IS_NOT_NULL(queue_processor);

    volatile_atomic int32_t open_complete;

    (void)interlocked_exchange(&open_complete, 0);

    ASSERT_ARE_EQUAL(int, 0, queue_processor_open(queue_processor, queue_processor_on_error, NULL));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&open_complete, 1, UINT32_MAX));

    // create one thread that pushes data through the queue processor

    //act
    double start_time = timer_global_get_elapsed_ms();
    double current_time = start_time;
    int64_t submitted_items = 0;
    while (current_time - start_time < queue_processor_perf_config->test_runtime * 1000)
    {
        for (size_t i = 0; i < 1000; i++)
        {
            queue_processor_schedule_item(queue_processor, do_nothing, NULL);
            submitted_items++;
            (void)interlocked_increment(&pending_items);
        }

        current_time = timer_global_get_elapsed_ms();
    }

    double drain_start_time = timer_global_get_elapsed_ms();

    LogInfo("Draining items");

    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&pending_items, 0, UINT32_MAX));

    double drain_end_time = timer_global_get_elapsed_ms();

    LogInfo("Executed %" PRId64 " items in %.02f seconds, %.02f/s, drained in %.02f", submitted_items, (drain_start_time - start_time) / 1000, (double)submitted_items / ((drain_start_time - start_time) / 1000), (drain_end_time - drain_start_time) / 1000);

    //assert

    // cleanup
    queue_processor_destroy(queue_processor);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
