// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_logging/logger.h"
#include "c_pal/execution_engine.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/interlocked_macros.h"
#include "c_pal/thandle.h"
#include "c_pal/threadapi.h"
#include "c_pal/threadpool.h"
#include "c_pal/timer.h"
#include "c_pal/sync.h"

#include "c_util/constbuffer.h"
#include "c_util/constbuffer_array.h"
#include "c_util/memory_data.h"

#include "batch_queue.h"

#define TEST_X_FUNCTION(A) static void A(void)

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_RESULT_VALUES);

#define HOLD_PROCESS_BATCH 0
#define ALLOW_PROCESS_BATCH 1
#define TEST_TIMEOUT_MS 15000

static EXECUTION_ENGINE_HANDLE g_default_execution_engine = NULL;
static struct GL_TAG /*gl comes from "global*/
{
    THANDLE(THREADPOOL) test_threadpool;
} gl;

static EXECUTION_ENGINE_HANDLE default_execution_engine_create()
{
    EXECUTION_ENGINE_PARAMETERS params = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&params);
    ASSERT_IS_NOT_NULL(execution_engine);

    return execution_engine;
}

BATCH_QUEUE_PROCESS_SYNC_RESULT empty_process_batch_func(void* process_batch_context, void** items, uint32_t item_count, BATCH_QUEUE_ON_LL_BATCH_COMPLETE on_ll_batch_complete, void* batch_context)
{
    (void)process_batch_context;
    (void)items;
    (void)item_count;
    (void)on_ll_batch_complete;
    (void)batch_context;

    return BATCH_QUEUE_PROCESS_SYNC_OK;
}

void empty_on_batch_faulted_func(void* context)
{
    (void)context;
}

void on_faulted_callback(void* context)
{
    ASSERT_IS_NOT_NULL(context);
    volatile_atomic int32_t* faulted = context;
    (void)interlocked_increment(faulted);
    (void)wake_by_address_single(faulted);
}

static CONSTBUFFER_ARRAY_HANDLE test_create_buffers_to_load(uint32_t count)
{
    CONSTBUFFER_ARRAY_HANDLE buffers;
    uint32_t buffer_count = count * 2;
    CONSTBUFFER_HANDLE* buffers_temp = malloc_2(buffer_count, sizeof(CONSTBUFFER_HANDLE));
    ASSERT_IS_NOT_NULL(buffers_temp);

    uint32_t buffer_index = 0;
    for (uint32_t i = 0; i < count; ++i)
    {
        char block_id[32];
        (void)sprintf(block_id, "block_%" PRIu32 "", i);
        uint32_t block_id_buffer_size = (uint32_t)(sizeof(uint16_t) + strlen(block_id) + 1);
        unsigned char* block_id_buffer = malloc(block_id_buffer_size);
        ASSERT_IS_NOT_NULL(block_id_buffer);

        write_uint16_t(block_id_buffer, (uint16_t)strlen(block_id) + 1);
        strcpy((char*)(block_id_buffer + sizeof(uint16_t)), block_id);

        buffers_temp[buffer_index] = CONSTBUFFER_CreateWithMoveMemory(block_id_buffer, block_id_buffer_size);
        ASSERT_IS_NOT_NULL(buffers_temp[buffer_index]);
        ++buffer_index;

        buffers_temp[buffer_index] = CONSTBUFFER_Create(NULL, 0);
        ASSERT_IS_NOT_NULL(buffers_temp[buffer_index]);
        ++buffer_index;
    }

    buffers = constbuffer_array_create_with_move_buffers(buffers_temp, buffer_count);
    ASSERT_IS_NOT_NULL(buffers);
    return buffers;
}

typedef struct ITEM_COMPLETE_CONTEXT_TAG
{
    double start_time;
    double end_time;
    int32_t batch_order_number;
    INTERLOCKED_DEFINE_VOLATILE_STATE_ENUM(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, result);
    volatile_atomic int32_t* item_complete_counter;
} ITEM_COMPLETE_CONTEXT;

typedef struct PER_BATCH_STATS_TAG
{
    uint32_t number_of_items;
    uint32_t total_item_size;

    THREADPOOL_WORK_ITEM_HANDLE work_item_handle;
} PER_BATCH_STATS;

typedef struct BATCH_QUEUE_PROCESSOR_BATCH_STATS_TAG
{
    THANDLE(THREADPOOL) process_threadpool;
    uint32_t max_batch_size;
    int32_t fail_on;
    BATCH_QUEUE_PROCESS_SYNC_RESULT batch_sync_result;
    volatile_atomic int32_t* allow_process_batch_to_continue;
    volatile_atomic int32_t* process_batch_called;
    volatile_atomic int32_t batch_order_number;

    int32_t expected_max_batches;
    PER_BATCH_STATS per_batch_stats[];
} BATCH_QUEUE_PROCESSOR_BATCH_STATS;

typedef struct BATCH_QUEUE_PROCESSOR_BATCH_STATS_WORK_TAG
{
    int32_t batch_order_number;
    BATCH_QUEUE_PROCESSOR_BATCH_STATS* processor_context;
    BATCH_QUEUE_PROCESS_COMPLETE_RESULT result_to_return;
    BATCH_QUEUE_ON_LL_BATCH_COMPLETE on_ll_batch_complete;
    void* batch_context;
} BATCH_QUEUE_PROCESSOR_BATCH_STATS_WORK;

static void processor_complete(void* context)
{
    ASSERT_IS_NOT_NULL(context);
    BATCH_QUEUE_PROCESSOR_BATCH_STATS_WORK* work_context = context;
    InterlockedHL_WaitForNotValue(work_context->processor_context->allow_process_batch_to_continue, HOLD_PROCESS_BATCH, UINT32_MAX);
    int32_t batch_order_number = work_context->batch_order_number;

    work_context->on_ll_batch_complete(work_context->batch_context, work_context->result_to_return, &batch_order_number);

    free(context);
}

static void process_batch_stats(BATCH_QUEUE_PROCESSOR_BATCH_STATS* processor_context, int32_t batch_order_number, void** items, uint32_t item_count)
{
    ASSERT_IS_TRUE(batch_order_number < processor_context->expected_max_batches);
    processor_context->per_batch_stats[batch_order_number].number_of_items = item_count;
    processor_context->per_batch_stats[batch_order_number].total_item_size = 0;
    for (uint32_t i = 0; i < item_count; i++)
    {
        ASSERT_IS_NOT_NULL(items[i]);
        CONSTBUFFER_ARRAY_HANDLE buffers = items[i];
        uint32_t batch_item_size;
        ASSERT_ARE_EQUAL(int, 0, constbuffer_array_get_all_buffers_size(buffers, &batch_item_size));
        processor_context->per_batch_stats[batch_order_number].total_item_size += batch_item_size;
    }
    ASSERT_IS_TRUE(processor_context->per_batch_stats[batch_order_number].total_item_size <= processor_context->max_batch_size);
}

static BATCH_QUEUE_PROCESS_SYNC_RESULT process_batch_with_stats_func(void* context, void** items, uint32_t item_count, BATCH_QUEUE_ON_LL_BATCH_COMPLETE on_ll_batch_complete, void* batch_context)
{
    ASSERT_IS_NOT_NULL(context);
    ASSERT_IS_NOT_NULL(items);
    ASSERT_ARE_NOT_EQUAL(uint32_t, 0, item_count);

    BATCH_QUEUE_PROCESSOR_BATCH_STATS* processor_context = context;
    int32_t batch_order_number = interlocked_increment(&processor_context->batch_order_number);
    process_batch_stats(processor_context, batch_order_number, items, item_count);

    // set that this was called first
    (void)interlocked_increment(processor_context->process_batch_called);
    wake_by_address_single(processor_context->process_batch_called);

    // set up a work item to run the rest of process batch async.
    BATCH_QUEUE_PROCESSOR_BATCH_STATS_WORK* processor_work_context = (BATCH_QUEUE_PROCESSOR_BATCH_STATS_WORK*)malloc(sizeof(BATCH_QUEUE_PROCESSOR_BATCH_STATS_WORK));
    ASSERT_IS_NOT_NULL(processor_work_context);
    processor_work_context->batch_order_number = batch_order_number;
    processor_work_context->batch_context = batch_context;
    processor_work_context->on_ll_batch_complete = on_ll_batch_complete;
    processor_work_context->processor_context = processor_context;
    if (processor_context->fail_on >= 0 && processor_context->fail_on == batch_order_number)
    {
        processor_work_context->result_to_return = BATCH_QUEUE_PROCESS_COMPLETE_ERROR;
    }
    else
    {
        processor_work_context->result_to_return = BATCH_QUEUE_PROCESS_COMPLETE_OK;
    }
    processor_context->per_batch_stats[batch_order_number].work_item_handle = threadpool_create_work_item(processor_context->process_threadpool, processor_complete, processor_work_context);
    ASSERT_IS_NOT_NULL(processor_context->per_batch_stats[batch_order_number].work_item_handle);
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(processor_context->process_threadpool, processor_context->per_batch_stats[batch_order_number].work_item_handle));

    // return OK sync
    return BATCH_QUEUE_PROCESS_SYNC_OK;
}

static BATCH_QUEUE_PROCESS_SYNC_RESULT process_batch_with_stats_sync_fail_func(void* context, void** items, uint32_t item_count, BATCH_QUEUE_ON_LL_BATCH_COMPLETE on_ll_batch_complete, void* batch_context)
{
    ASSERT_IS_NOT_NULL(context);
    ASSERT_IS_NOT_NULL(items);
    ASSERT_ARE_NOT_EQUAL(uint32_t, 0, item_count);

    BATCH_QUEUE_PROCESSOR_BATCH_STATS* processor_context = context;
    int32_t batch_order_number = interlocked_increment(&processor_context->batch_order_number);
    process_batch_stats(processor_context, batch_order_number, items, item_count);

    // set that this was called first
    (void)interlocked_increment(processor_context->process_batch_called);
    wake_by_address_single(processor_context->process_batch_called);

    BATCH_QUEUE_PROCESS_SYNC_RESULT result;
    if (processor_context->fail_on >= 0 && processor_context->fail_on == batch_order_number)
    {
        result = processor_context->batch_sync_result;
    }
    else
    {
        // set up a work item to run the rest of process batch async.
        BATCH_QUEUE_PROCESSOR_BATCH_STATS_WORK* processor_work_context = (BATCH_QUEUE_PROCESSOR_BATCH_STATS_WORK*)malloc(sizeof(BATCH_QUEUE_PROCESSOR_BATCH_STATS_WORK));
        ASSERT_IS_NOT_NULL(processor_work_context);
        processor_work_context->batch_order_number = batch_order_number;
        processor_work_context->batch_context = batch_context;
        processor_work_context->on_ll_batch_complete = on_ll_batch_complete;
        processor_work_context->processor_context = processor_context;
        processor_work_context->result_to_return = BATCH_QUEUE_PROCESS_COMPLETE_OK;
        processor_context->per_batch_stats[batch_order_number].work_item_handle = threadpool_create_work_item(processor_context->process_threadpool, processor_complete, processor_work_context);
        ASSERT_IS_NOT_NULL(processor_context->per_batch_stats[batch_order_number].work_item_handle);
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work_item(processor_context->process_threadpool, processor_context->per_batch_stats[batch_order_number].work_item_handle));

        // return OK sync
        result = BATCH_QUEUE_PROCESS_SYNC_OK;
    }
    return result;
}

static void counted_item_complete_callback(void* context, BATCH_QUEUE_PROCESS_COMPLETE_RESULT result, void* ll_result)
{
    ASSERT_IS_NOT_NULL(context);
    int32_t batch_order_number = -1;
    if (result == BATCH_QUEUE_PROCESS_COMPLETE_OK)
    {
        ASSERT_IS_NOT_NULL(ll_result);
        int32_t* result_batch_order_number = ll_result;
        ASSERT_IS_TRUE(*result_batch_order_number >= 0);
        batch_order_number = *result_batch_order_number;
    }
    else if (ll_result != NULL)
    {
        int32_t* result_batch_order_number = ll_result;
        ASSERT_IS_TRUE(*result_batch_order_number >= 0);
        batch_order_number = *result_batch_order_number;
    }
    ITEM_COMPLETE_CONTEXT* item_complete_context = context;
    (void)interlocked_exchange(&item_complete_context->result, result);
    item_complete_context->end_time = timer_global_get_elapsed_ms();
    item_complete_context->batch_order_number = batch_order_number;
    InterlockedHL_DecrementAndWake(item_complete_context->item_complete_counter);
}

static void setup_process_batch_context
(
    int32_t max_batch_size,
    int32_t expected_max_batches,
    int32_t fail_on,
    int32_t items_to_complete,
    int32_t batch_can_continue,
    volatile_atomic int32_t* item_complete_counter,
    volatile_atomic int32_t* process_batch_called,
    volatile_atomic int32_t* allow_process_batch_to_continue,
    BATCH_QUEUE_PROCESSOR_BATCH_STATS** processor_context
)
{
    (void)interlocked_exchange(item_complete_counter, items_to_complete);

    *processor_context = malloc_flex(sizeof(BATCH_QUEUE_PROCESSOR_BATCH_STATS), expected_max_batches, sizeof(PER_BATCH_STATS));
    ASSERT_IS_NOT_NULL(*processor_context);
    (*processor_context)->max_batch_size = max_batch_size;
    (*processor_context)->expected_max_batches = expected_max_batches;
    (*processor_context)->fail_on = fail_on;
    (*processor_context)->batch_sync_result = BATCH_QUEUE_PROCESS_SYNC_OK;
    THANDLE_INITIALIZE(THREADPOOL)(&(*processor_context)->process_threadpool, gl.test_threadpool);
    (void)interlocked_exchange(process_batch_called, 0);
    (void)interlocked_exchange(allow_process_batch_to_continue, batch_can_continue);
    (*processor_context)->process_batch_called = process_batch_called;
    (*processor_context)->allow_process_batch_to_continue = allow_process_batch_to_continue;
    (void)interlocked_exchange(&(*processor_context)->batch_order_number, -1);
    for (int batch = 0; batch < expected_max_batches; batch++)
    {
        (*processor_context)->per_batch_stats[batch].number_of_items = 0;
        (*processor_context)->per_batch_stats[batch].total_item_size = 0;
        (*processor_context)->per_batch_stats[batch].work_item_handle = NULL;
    }
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
    g_default_execution_engine = default_execution_engine_create();
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    execution_engine_dec_ref(g_default_execution_engine);
    g_default_execution_engine = NULL;
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    THANDLE(THREADPOOL) per_test_threadpool = threadpool_create(g_default_execution_engine);
    ASSERT_IS_NOT_NULL(per_test_threadpool);
    THANDLE_MOVE(THREADPOOL)(&gl.test_threadpool, &per_test_threadpool);
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    THANDLE_ASSIGN(THREADPOOL)(&gl.test_threadpool, NULL);
}

TEST_FUNCTION(batch_queue_queue_batch_func_is_not_triggered_when_max_pending_requests)
{
    // arrange
    const int32_t number_of_items_to_queue = 19;
    const int32_t max_pending_for_batch = 2;
    const int32_t max_batch_size_multiplier = 2;
    CONSTBUFFER_ARRAY_HANDLE buffers = test_create_buffers_to_load(10);
    uint32_t batch_item_size;
    ASSERT_ARE_EQUAL(int, 0, constbuffer_array_get_all_buffers_size(buffers, &batch_item_size));

    BATCH_QUEUE_SETTINGS settings =
    {
        .max_pending_requests = max_pending_for_batch, // much less than the total number of items queued
        .max_batch_size = max_batch_size_multiplier * batch_item_size, // small enough for multiple batches to possibly be called
        .min_batch_size = batch_item_size + 1, // Avoid triggering by min batch size by making larger than 1 batch
        .min_wait_time = 6000 // 1 minute is way longer than this test should run.
    };
    int32_t expected_max_batches = number_of_items_to_queue;
    volatile_atomic int32_t item_complete_counter;
    volatile_atomic int32_t process_batch_called;
    volatile_atomic int32_t allow_process_batch_to_continue;
    BATCH_QUEUE_PROCESSOR_BATCH_STATS* processor_context;

    ASSERT_IS_TRUE(expected_max_batches > max_pending_for_batch, "Test is invalid, expected_max_batches should be greater than max_pending_for_batch for this test to work");
    LogInfo("Starting test with settings: %" PRI_BATCH_QUEUE_SETTINGS "", BATCH_QUEUE_SETTINGS_VALUES(settings));

    setup_process_batch_context(settings.max_batch_size, expected_max_batches, -1, number_of_items_to_queue, HOLD_PROCESS_BATCH, &item_complete_counter, &process_batch_called, &allow_process_batch_to_continue, &processor_context);

    BATCH_QUEUE_HANDLE batch_queue = batch_queue_create(settings, g_default_execution_engine, process_batch_with_stats_func, processor_context, empty_on_batch_faulted_func, NULL);
    ASSERT_IS_NOT_NULL(batch_queue);
    ASSERT_ARE_EQUAL(int, 0, batch_queue_open(batch_queue));

    ITEM_COMPLETE_CONTEXT* item_contexts = (ITEM_COMPLETE_CONTEXT*)malloc_2(number_of_items_to_queue, sizeof(ITEM_COMPLETE_CONTEXT));
    int32_t call;
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        item_contexts[call].item_complete_counter = &item_complete_counter;
        (void)interlocked_exchange(&item_contexts[call].result, -1);
    }

    // act
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        BATCH_QUEUE_ENQUEUE_RESULT enqueue_result = batch_queue_enqueue(batch_queue, buffers, batch_item_size, counted_item_complete_callback, &item_contexts[call]);
        ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_OK, enqueue_result);
    }

    // assert
    // At this point, the process_batch_func should have been called twice and are waiting for allow_process_batch_to_continue
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&process_batch_called, max_pending_for_batch, TEST_TIMEOUT_MS));
    int batch;
    for (batch = 0; batch < expected_max_batches; batch++)
    {
        if (batch < max_pending_for_batch)
        {
            ASSERT_ARE_EQUAL(uint32_t, max_batch_size_multiplier, processor_context->per_batch_stats[batch].number_of_items);
            ASSERT_ARE_EQUAL(uint32_t, max_batch_size_multiplier * batch_item_size, processor_context->per_batch_stats[batch].total_item_size);
        }
        else
        {
            ASSERT_ARE_EQUAL(uint32_t, 0, processor_context->per_batch_stats[batch].number_of_items);
            ASSERT_ARE_EQUAL(uint32_t, 0, processor_context->per_batch_stats[batch].total_item_size);
        }
    }
    // release the existing batches and more batches should be called.
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWakeAll(&allow_process_batch_to_continue, ALLOW_PROCESS_BATCH));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&item_complete_counter, 0, TEST_TIMEOUT_MS));

    // cleanup
    batch_queue_close(batch_queue);
    batch_queue_destroy(batch_queue);
    for (batch = 0; batch < expected_max_batches; batch++)
    {
        if (processor_context->per_batch_stats[batch].work_item_handle != NULL)
        {
            threadpool_destroy_work_item(gl.test_threadpool, processor_context->per_batch_stats[batch].work_item_handle);
        }
    }

    THANDLE_ASSIGN(THREADPOOL)(&processor_context->process_threadpool, NULL);
    free(item_contexts);
    free(processor_context);
    constbuffer_array_dec_ref(buffers);
}

TEST_FUNCTION(batch_queue_queue_a_batch_is_triggered_by_max_batch_size)
{
    // arrange

    //this test should choose values that evenly divide by max_batch_size_multiplier
    const int32_t number_of_items_to_queue = 20;
    const int32_t max_pending_for_batch = 20;
    const int32_t max_batch_size_multiplier = 5;
    CONSTBUFFER_ARRAY_HANDLE buffers = test_create_buffers_to_load(10);
    uint32_t batch_item_size;
    ASSERT_ARE_EQUAL(int, 0, constbuffer_array_get_all_buffers_size(buffers, &batch_item_size));

    BATCH_QUEUE_SETTINGS settings =
    {
        .max_pending_requests = max_pending_for_batch, // == number of items to queue, so we should always process batch.
        .max_batch_size = max_batch_size_multiplier * batch_item_size, // small enough for multiple batches to possibly be called
        .min_batch_size = (max_batch_size_multiplier + 1) * batch_item_size, // should not trigger by min batch size
        .min_wait_time = 6000 // 1 minute is way longer than this test should run.
    };
    int32_t expected_max_batches = number_of_items_to_queue/max_batch_size_multiplier;
    volatile_atomic int32_t item_complete_counter;
    volatile_atomic int32_t process_batch_called;
    volatile_atomic int32_t allow_process_batch_to_continue;
    BATCH_QUEUE_PROCESSOR_BATCH_STATS* processor_context;
    LogInfo("Starting test with settings: %" PRI_BATCH_QUEUE_SETTINGS "", BATCH_QUEUE_SETTINGS_VALUES(settings));

    setup_process_batch_context(settings.max_batch_size, expected_max_batches, -1, number_of_items_to_queue, ALLOW_PROCESS_BATCH, &item_complete_counter, &process_batch_called, &allow_process_batch_to_continue, &processor_context);

    BATCH_QUEUE_HANDLE batch_queue = batch_queue_create(settings, g_default_execution_engine, process_batch_with_stats_func, processor_context, empty_on_batch_faulted_func, NULL);
    ASSERT_IS_NOT_NULL(batch_queue);
    ASSERT_ARE_EQUAL(int, 0, batch_queue_open(batch_queue));

    ITEM_COMPLETE_CONTEXT* item_contexts = (ITEM_COMPLETE_CONTEXT*)malloc_2(number_of_items_to_queue, sizeof(ITEM_COMPLETE_CONTEXT));
    int32_t call;
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        item_contexts[call].item_complete_counter = &item_complete_counter;
        (void)interlocked_exchange(&item_contexts[call].result, -1);
    }

    // act
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        BATCH_QUEUE_ENQUEUE_RESULT enqueue_result = batch_queue_enqueue(batch_queue, buffers, batch_item_size, counted_item_complete_callback, &item_contexts[call]);
        ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_OK, enqueue_result);
    }

    // assert
    // At this point, we should have hit the max batch size multiple times. Wait for the process to complete.
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&process_batch_called, expected_max_batches, TEST_TIMEOUT_MS));
    int batch;
    for (batch = 0; batch < expected_max_batches; batch++)
    {
            ASSERT_ARE_EQUAL(uint32_t, max_batch_size_multiplier, processor_context->per_batch_stats[batch].number_of_items);
            ASSERT_ARE_EQUAL(uint32_t, max_batch_size_multiplier * batch_item_size, processor_context->per_batch_stats[batch].total_item_size);
    }
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&item_complete_counter, 0, TEST_TIMEOUT_MS));
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        ASSERT_ARE_EQUAL(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_OK, interlocked_add(&item_contexts[call].result, 0));
    }

    // cleanup
    batch_queue_close(batch_queue);
    batch_queue_destroy(batch_queue);
    for (batch = 0; batch < expected_max_batches; batch++)
    {
        if (processor_context->per_batch_stats[batch].work_item_handle != NULL)
        {
            threadpool_destroy_work_item(gl.test_threadpool, processor_context->per_batch_stats[batch].work_item_handle);
        }
    }

    THANDLE_ASSIGN(THREADPOOL)(&processor_context->process_threadpool, NULL);
    free(item_contexts);
    free(processor_context);
    constbuffer_array_dec_ref(buffers);
}

TEST_FUNCTION(batch_queue_batches_are_triggered_by_timeout)
{
    const int32_t number_of_items_to_queue = 1;
    const int32_t max_pending_for_batch = 20;
    const int32_t min_wait_time = 100;
    const unsigned char buffer_data = 0;
    CONSTBUFFER_HANDLE buffer = CONSTBUFFER_Create(&buffer_data, 1);
    CONSTBUFFER_ARRAY_HANDLE buffers = constbuffer_array_create(&buffer, 1);
    
    uint32_t batch_item_size;
    ASSERT_ARE_EQUAL(int, 0, constbuffer_array_get_all_buffers_size(buffers, &batch_item_size));
    ASSERT_ARE_EQUAL(uint32_t, 1, batch_item_size);

    BATCH_QUEUE_SETTINGS settings =
    {
        .max_pending_requests = max_pending_for_batch,
        .max_batch_size = 1048576,
        .min_batch_size = 1024,
        .min_wait_time = min_wait_time // a very short amount of time so this test doesn't take forever.
    };
    int32_t expected_max_batches = number_of_items_to_queue;
    volatile_atomic int32_t item_complete_counter;
    volatile_atomic int32_t process_batch_called;
    volatile_atomic int32_t allow_process_batch_to_continue;
    BATCH_QUEUE_PROCESSOR_BATCH_STATS* processor_context;
    LogInfo("Starting test with settings: %" PRI_BATCH_QUEUE_SETTINGS "", BATCH_QUEUE_SETTINGS_VALUES(settings));

    setup_process_batch_context(settings.max_batch_size, expected_max_batches, -1, number_of_items_to_queue, ALLOW_PROCESS_BATCH, &item_complete_counter, &process_batch_called, &allow_process_batch_to_continue, &processor_context);

    BATCH_QUEUE_HANDLE batch_queue = batch_queue_create(settings, g_default_execution_engine, process_batch_with_stats_func, processor_context, empty_on_batch_faulted_func, NULL);
    ASSERT_IS_NOT_NULL(batch_queue);
    ASSERT_ARE_EQUAL(int, 0, batch_queue_open(batch_queue));

    ITEM_COMPLETE_CONTEXT* item_contexts = (ITEM_COMPLETE_CONTEXT*)malloc_2(number_of_items_to_queue, sizeof(ITEM_COMPLETE_CONTEXT));
    item_contexts[0].item_complete_counter = &item_complete_counter;
    (void)interlocked_exchange(&item_contexts[0].result, -1);

    // act
    item_contexts[0].start_time = timer_global_get_elapsed_ms();
    BATCH_QUEUE_ENQUEUE_RESULT enqueue_result = batch_queue_enqueue(batch_queue, buffers, batch_item_size, counted_item_complete_callback, &item_contexts[0]);
    ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_OK, enqueue_result);

    // assert
    // At this point, each call to enqueue should have triggered a batch in the thread.
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&process_batch_called, expected_max_batches, TEST_TIMEOUT_MS));
    ASSERT_ARE_EQUAL(uint32_t, 1, processor_context->per_batch_stats[0].number_of_items);
    ASSERT_ARE_EQUAL(uint32_t, batch_item_size, processor_context->per_batch_stats[0].total_item_size);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&item_complete_counter, 0, TEST_TIMEOUT_MS));
    double time_to_complete = item_contexts[0].end_time - item_contexts[0].start_time;
    LogInfo("Item completed in %f ms", time_to_complete);
    ASSERT_IS_TRUE(time_to_complete >= min_wait_time, "Expected item to wait for min_wait_time");
    ASSERT_ARE_EQUAL(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_OK, interlocked_add(&item_contexts[0].result, 0));

    // cleanup
    batch_queue_close(batch_queue);
    batch_queue_destroy(batch_queue);
    if (processor_context->per_batch_stats[0].work_item_handle != NULL)
    {
        threadpool_destroy_work_item(gl.test_threadpool, processor_context->per_batch_stats[0].work_item_handle);
    }

    THANDLE_ASSIGN(THREADPOOL)(&processor_context->process_threadpool, NULL);
    free(item_contexts);
    free(processor_context);
    constbuffer_array_dec_ref(buffers);
    CONSTBUFFER_DecRef(buffer);
}

TEST_FUNCTION(batch_queue_batches_are_triggered_by_min_batch_or_timeout)
{
    const int32_t number_of_items_to_queue = 20;
    const int32_t max_pending_for_batch = 20;
    const int32_t max_batch_size_multiplier = 20;
    const int32_t min_wait_time = 20;
    CONSTBUFFER_ARRAY_HANDLE buffers = test_create_buffers_to_load(21);
    uint32_t batch_item_size;
    ASSERT_ARE_EQUAL(int, 0, constbuffer_array_get_all_buffers_size(buffers, &batch_item_size));

    BATCH_QUEUE_SETTINGS settings =
    {
        .max_pending_requests = max_pending_for_batch, // == number of items to queue, so we should always process batch.
        .max_batch_size = max_batch_size_multiplier * batch_item_size, // == size of all item, so we won't trigger by max size
        .min_batch_size = batch_item_size, // we are going to queue slowly enough to trigger by timeout
        .min_wait_time = min_wait_time // a very short amount of time so this test doesn't take forever.
    };
    int32_t expected_max_batches = number_of_items_to_queue;
    volatile_atomic int32_t item_complete_counter;
    volatile_atomic int32_t process_batch_called;
    volatile_atomic int32_t allow_process_batch_to_continue;
    BATCH_QUEUE_PROCESSOR_BATCH_STATS* processor_context;
    LogInfo("Starting test with settings: %" PRI_BATCH_QUEUE_SETTINGS "", BATCH_QUEUE_SETTINGS_VALUES(settings));

    setup_process_batch_context(settings.max_batch_size, expected_max_batches, -1, number_of_items_to_queue, HOLD_PROCESS_BATCH, &item_complete_counter, &process_batch_called, &allow_process_batch_to_continue, &processor_context);

    BATCH_QUEUE_HANDLE batch_queue = batch_queue_create(settings, g_default_execution_engine, process_batch_with_stats_func, processor_context, empty_on_batch_faulted_func, NULL);
    ASSERT_IS_NOT_NULL(batch_queue);
    ASSERT_ARE_EQUAL(int, 0, batch_queue_open(batch_queue));

    ITEM_COMPLETE_CONTEXT* item_contexts = (ITEM_COMPLETE_CONTEXT*)malloc_2(number_of_items_to_queue, sizeof(ITEM_COMPLETE_CONTEXT));
    int32_t call;
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        item_contexts[call].item_complete_counter = &item_complete_counter;
        (void)interlocked_exchange(&item_contexts[call].result, -1);
    }

    // act
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        item_contexts[call].start_time = timer_global_get_elapsed_ms();
        BATCH_QUEUE_ENQUEUE_RESULT enqueue_result = batch_queue_enqueue(batch_queue, buffers, batch_item_size, counted_item_complete_callback, &item_contexts[call]);
        ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_OK, enqueue_result);
        // wait with the expectation that the batch worker thread will dequeue the item and stage it on one loop, and send the batch on the 2nd loop.
        ThreadAPI_Sleep(min_wait_time/2);
    }

    // assert
    // At this point, each call to enqueue should have triggered a batch in the thread.
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&process_batch_called, expected_max_batches, TEST_TIMEOUT_MS));
    int batch;
    for (batch = 0; batch < expected_max_batches; batch++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 1, processor_context->per_batch_stats[batch].number_of_items);
        ASSERT_ARE_EQUAL(uint32_t, batch_item_size, processor_context->per_batch_stats[batch].total_item_size);
    }
    // Allow the batches to complete.
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWakeAll(&allow_process_batch_to_continue, ALLOW_PROCESS_BATCH));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&item_complete_counter, 0, TEST_TIMEOUT_MS));
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        double time_to_complete = item_contexts[call].end_time - item_contexts[call].start_time;
        LogInfo("Item %d completed in %f ms", call, time_to_complete);
        ASSERT_ARE_EQUAL(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_OK, interlocked_add(&item_contexts[call].result, 0));
    }

    // cleanup
    batch_queue_close(batch_queue);
    batch_queue_destroy(batch_queue);
    for (batch = 0; batch < expected_max_batches; batch++)
    {
        if (processor_context->per_batch_stats[batch].work_item_handle != NULL)
        {
            threadpool_destroy_work_item(gl.test_threadpool, processor_context->per_batch_stats[batch].work_item_handle);
        }
    }

    THANDLE_ASSIGN(THREADPOOL)(&processor_context->process_threadpool, NULL);
    free(item_contexts);
    free(processor_context);
    constbuffer_array_dec_ref(buffers);
}

TEST_FUNCTION(batch_queue_batches_are_triggered_by_timeout_only)
{
    const int32_t number_of_items_to_queue = 2;
    const int32_t max_pending_for_batch = 3;
    const int32_t max_batch_size_multiplier = 20;
    const int32_t min_wait_time = 100;
    CONSTBUFFER_ARRAY_HANDLE buffers = test_create_buffers_to_load(23);
    uint32_t batch_item_size;
    ASSERT_ARE_EQUAL(int, 0, constbuffer_array_get_all_buffers_size(buffers, &batch_item_size));

    BATCH_QUEUE_SETTINGS settings =
    {
        .max_pending_requests = max_pending_for_batch, // >= number of items to queue, so we should always process batch.
        .max_batch_size = max_batch_size_multiplier* batch_item_size, // larger than the total size of all items, so we won't trigger by max size
        .min_batch_size = (number_of_items_to_queue + 1)* batch_item_size, // enqueue less than min batch size, so we should trigger by timeout
        .min_wait_time = min_wait_time // a short amount of time so this test doesn't take forever.
    };
    int32_t expected_max_batches = 1;
    volatile_atomic int32_t item_complete_counter;
    volatile_atomic int32_t process_batch_called;
    volatile_atomic int32_t allow_process_batch_to_continue;
    BATCH_QUEUE_PROCESSOR_BATCH_STATS* processor_context;
    LogInfo("Starting test with settings: %" PRI_BATCH_QUEUE_SETTINGS "", BATCH_QUEUE_SETTINGS_VALUES(settings));

    setup_process_batch_context(settings.max_batch_size, expected_max_batches, -1, number_of_items_to_queue, HOLD_PROCESS_BATCH, &item_complete_counter, &process_batch_called, &allow_process_batch_to_continue, &processor_context);

    BATCH_QUEUE_HANDLE batch_queue = batch_queue_create(settings, g_default_execution_engine, process_batch_with_stats_func, processor_context, empty_on_batch_faulted_func, NULL);
    ASSERT_IS_NOT_NULL(batch_queue);
    ASSERT_ARE_EQUAL(int, 0, batch_queue_open(batch_queue));

    ITEM_COMPLETE_CONTEXT* item_contexts = (ITEM_COMPLETE_CONTEXT*)malloc_2(number_of_items_to_queue, sizeof(ITEM_COMPLETE_CONTEXT));
    int32_t call;
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        item_contexts[call].item_complete_counter = &item_complete_counter;
        (void)interlocked_exchange(&item_contexts[call].result, -1);
    }

    // act
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        item_contexts[call].start_time = timer_global_get_elapsed_ms();
        BATCH_QUEUE_ENQUEUE_RESULT enqueue_result = batch_queue_enqueue(batch_queue, buffers, batch_item_size, counted_item_complete_callback, &item_contexts[call]);
        ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_OK, enqueue_result);
    }

    // assert
    // At this point, less than the min batch size has been queued, so we should wait for the timeout.
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&process_batch_called, expected_max_batches, TEST_TIMEOUT_MS));
    int batch;
    for (batch = 0; batch < expected_max_batches; batch++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 2, processor_context->per_batch_stats[batch].number_of_items);
        ASSERT_ARE_EQUAL(uint32_t, 2 * batch_item_size, processor_context->per_batch_stats[batch].total_item_size);
    }
    // Allow the batches to complete.
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWakeAll(&allow_process_batch_to_continue, ALLOW_PROCESS_BATCH));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&item_complete_counter, 0, TEST_TIMEOUT_MS));
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        double time_to_complete = item_contexts[call].end_time - item_contexts[call].start_time;
        LogInfo("Item %d completed in %f ms", call, time_to_complete);
        ASSERT_ARE_EQUAL(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_OK, interlocked_add(&item_contexts[call].result, 0));
    }

    // cleanup
    batch_queue_close(batch_queue);
    batch_queue_destroy(batch_queue);
    for (batch = 0; batch < expected_max_batches; batch++)
    {
        if (processor_context->per_batch_stats[batch].work_item_handle != NULL)
        {
            threadpool_destroy_work_item(gl.test_threadpool, processor_context->per_batch_stats[batch].work_item_handle);
        }
    }

    THANDLE_ASSIGN(THREADPOOL)(&processor_context->process_threadpool, NULL);
    free(item_contexts);
    free(processor_context);
    constbuffer_array_dec_ref(buffers);
}

TEST_FUNCTION(batch_queue_batches_are_triggered_by_min_batch_or_timeout_timed)
{
    const int32_t number_of_items_to_queue = 20;
    const int32_t max_pending_for_batch = 20;
    const int32_t max_batch_size_multiplier = 20;
    const int32_t min_wait_time = 20;
    CONSTBUFFER_ARRAY_HANDLE buffers = test_create_buffers_to_load(21);
    uint32_t batch_item_size;
    ASSERT_ARE_EQUAL(int, 0, constbuffer_array_get_all_buffers_size(buffers, &batch_item_size));

    BATCH_QUEUE_SETTINGS settings =
    {
        .max_pending_requests = max_pending_for_batch, // == number of items to queue, so we should always process batch.
        .max_batch_size = max_batch_size_multiplier * batch_item_size, // == size of all item, so we won't trigger by max size
        .min_batch_size = batch_item_size, // we are going to queue slowly enough to trigger by timeout
        .min_wait_time = min_wait_time // a very short amount of time so this test doesn't take forever.
    };
    int32_t expected_max_batches = number_of_items_to_queue;
    volatile_atomic int32_t item_complete_counter;
    volatile_atomic int32_t process_batch_called;
    volatile_atomic int32_t allow_process_batch_to_continue;
    BATCH_QUEUE_PROCESSOR_BATCH_STATS* processor_context;
    LogInfo("Starting test with settings: %" PRI_BATCH_QUEUE_SETTINGS "", BATCH_QUEUE_SETTINGS_VALUES(settings));

    setup_process_batch_context(settings.max_batch_size, expected_max_batches, -1, number_of_items_to_queue, ALLOW_PROCESS_BATCH, &item_complete_counter, &process_batch_called, &allow_process_batch_to_continue, &processor_context);

    BATCH_QUEUE_HANDLE batch_queue = batch_queue_create(settings, g_default_execution_engine, process_batch_with_stats_func, processor_context, empty_on_batch_faulted_func, NULL);
    ASSERT_IS_NOT_NULL(batch_queue);
    ASSERT_ARE_EQUAL(int, 0, batch_queue_open(batch_queue));

    ITEM_COMPLETE_CONTEXT* item_contexts = (ITEM_COMPLETE_CONTEXT*)malloc_2(number_of_items_to_queue, sizeof(ITEM_COMPLETE_CONTEXT));
    int32_t call;
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        item_contexts[call].item_complete_counter = &item_complete_counter;
        (void)interlocked_exchange(&item_contexts[call].result, -1);
    }

    // act
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        item_contexts[call].start_time = timer_global_get_elapsed_ms();
        BATCH_QUEUE_ENQUEUE_RESULT enqueue_result = batch_queue_enqueue(batch_queue, buffers, batch_item_size, counted_item_complete_callback, &item_contexts[call]);
        // wait with the expectation that the batch worker thread will dequeue the item and stage it on one loop, and send the batch on the 2nd loop.
        ThreadAPI_Sleep(min_wait_time/2);
        ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_OK, enqueue_result);
    }

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&item_complete_counter, 0, TEST_TIMEOUT_MS));
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        double time_to_complete = item_contexts[call].end_time - item_contexts[call].start_time;
        double expected_wait_time = 4.0 * min_wait_time;
        LogInfo("Item %d completed in %f ms, expected %f ms", call, time_to_complete, expected_wait_time);
        ASSERT_IS_TRUE(time_to_complete < expected_wait_time, "Item should have executed very soon after min_wait_time expired");
        ASSERT_ARE_EQUAL(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_OK, interlocked_add(&item_contexts[call].result, 0));
    }

    // cleanup
    batch_queue_close(batch_queue);
    batch_queue_destroy(batch_queue);
    for (int batch = 0; batch < expected_max_batches; batch++)
    {
        if (processor_context->per_batch_stats[batch].work_item_handle != NULL)
        {
            threadpool_destroy_work_item(gl.test_threadpool, processor_context->per_batch_stats[batch].work_item_handle);
        }
    }

    THANDLE_ASSIGN(THREADPOOL)(&processor_context->process_threadpool, NULL);
    free(item_contexts);
    free(processor_context);
    constbuffer_array_dec_ref(buffers);
}

TEST_FUNCTION(batch_queue_batches_are_triggered_by_timeout_only_timed)
{
    const int32_t number_of_items_to_queue = 2;
    const int32_t max_pending_for_batch = 3;
    const int32_t max_batch_size_multiplier = 20;
    const int32_t min_wait_time = 100;
    CONSTBUFFER_ARRAY_HANDLE buffers = test_create_buffers_to_load(23);
    uint32_t batch_item_size;
    ASSERT_ARE_EQUAL(int, 0, constbuffer_array_get_all_buffers_size(buffers, &batch_item_size));

    BATCH_QUEUE_SETTINGS settings =
    {
        .max_pending_requests = max_pending_for_batch, // >= number of items to queue, so we should always process batch.
        .max_batch_size = max_batch_size_multiplier* batch_item_size, // larger than the total size of all items, so we won't trigger by max size
        .min_batch_size = (number_of_items_to_queue + 1)* batch_item_size, // enqueue less than min batch size, so we should trigger by timeout
        .min_wait_time = min_wait_time // a short amount of time so this test doesn't take forever.
    };
    int32_t expected_max_batches = 1;
    volatile_atomic int32_t item_complete_counter;
    volatile_atomic int32_t process_batch_called;
    volatile_atomic int32_t allow_process_batch_to_continue;
    BATCH_QUEUE_PROCESSOR_BATCH_STATS* processor_context;
    LogInfo("Starting test with settings: %" PRI_BATCH_QUEUE_SETTINGS "", BATCH_QUEUE_SETTINGS_VALUES(settings));

    setup_process_batch_context(settings.max_batch_size, expected_max_batches, -1, number_of_items_to_queue, ALLOW_PROCESS_BATCH, &item_complete_counter, &process_batch_called, &allow_process_batch_to_continue, &processor_context);

    BATCH_QUEUE_HANDLE batch_queue = batch_queue_create(settings, g_default_execution_engine, process_batch_with_stats_func, processor_context, empty_on_batch_faulted_func, NULL);
    ASSERT_IS_NOT_NULL(batch_queue);
    ASSERT_ARE_EQUAL(int, 0, batch_queue_open(batch_queue));

    ITEM_COMPLETE_CONTEXT* item_contexts = (ITEM_COMPLETE_CONTEXT*)malloc_2(number_of_items_to_queue, sizeof(ITEM_COMPLETE_CONTEXT));
    int32_t call;
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        item_contexts[call].item_complete_counter = &item_complete_counter;
        (void)interlocked_exchange(&item_contexts[call].result, -1);
    }

    // act
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        item_contexts[call].start_time = timer_global_get_elapsed_ms();
        BATCH_QUEUE_ENQUEUE_RESULT enqueue_result = batch_queue_enqueue(batch_queue, buffers, batch_item_size, counted_item_complete_callback, &item_contexts[call]);
        ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_OK, enqueue_result);
    }

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&item_complete_counter, 0, TEST_TIMEOUT_MS));
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        double time_to_complete = item_contexts[call].end_time - item_contexts[call].start_time;
        double expected_wait_time = 2.5 * min_wait_time;
        LogInfo("Item %d completed in %f ms, expected %f ms", call, time_to_complete, expected_wait_time);
        ASSERT_IS_TRUE(time_to_complete < expected_wait_time, "Item should have executed very soon after min_wait_time expired");
        ASSERT_IS_TRUE(time_to_complete >= (double)min_wait_time, "Item should have waitied until min_wait_time expired");
        ASSERT_ARE_EQUAL(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_OK, interlocked_add(&item_contexts[call].result, 0));
    }

    // cleanup
    batch_queue_close(batch_queue);
    batch_queue_destroy(batch_queue);
    for (int batch = 0; batch < expected_max_batches; batch++)
    {
        if (processor_context->per_batch_stats[batch].work_item_handle != NULL)
        {
            threadpool_destroy_work_item(gl.test_threadpool, processor_context->per_batch_stats[batch].work_item_handle);
        }
    }

    THANDLE_ASSIGN(THREADPOOL)(&processor_context->process_threadpool, NULL);
    free(item_contexts);
    free(processor_context);
    constbuffer_array_dec_ref(buffers);
}

TEST_FUNCTION(batch_queue_queue_a_batch_item_2nd_batch_fails_async_with_queued_items)
{
    const int32_t number_of_items_to_queue = 19;
    const int32_t max_pending_for_batch = 2;
    const int32_t max_batch_size_multiplier = 2;
    CONSTBUFFER_ARRAY_HANDLE buffers = test_create_buffers_to_load(10);
    uint32_t batch_item_size;
    ASSERT_ARE_EQUAL(int, 0, constbuffer_array_get_all_buffers_size(buffers, &batch_item_size));

    BATCH_QUEUE_SETTINGS settings =
    {
        .max_pending_requests = max_pending_for_batch, // much less than the total number of items queued
        .max_batch_size = max_batch_size_multiplier * batch_item_size, // small enough for multiple batches to possibly be called
        .min_batch_size = batch_item_size + 1, // Avoid triggering batch by min batch size so we know which items are in the batch.
        .min_wait_time = 6000 // 1 minute is way longer than this test should run.
    };
    int32_t expected_max_batches = number_of_items_to_queue;
    volatile_atomic int32_t item_complete_counter;
    volatile_atomic int32_t process_batch_called;
    volatile_atomic int32_t allow_process_batch_to_continue;
    BATCH_QUEUE_PROCESSOR_BATCH_STATS* processor_context;

    ASSERT_IS_TRUE(expected_max_batches > max_pending_for_batch, "Test is invalid, expected_max_batches should be greater than max_pending_for_batch for this test to work");
    LogInfo("Starting test with settings: %" PRI_BATCH_QUEUE_SETTINGS "", BATCH_QUEUE_SETTINGS_VALUES(settings));

    // set up so 2nd batch of items fail. should be call 2 and 3, based on max_batch_size_multiplier
    setup_process_batch_context(settings.max_batch_size, expected_max_batches, 1, number_of_items_to_queue, HOLD_PROCESS_BATCH, &item_complete_counter, &process_batch_called, &allow_process_batch_to_continue, &processor_context);

    BATCH_QUEUE_HANDLE batch_queue = batch_queue_create(settings, g_default_execution_engine, process_batch_with_stats_func, processor_context, empty_on_batch_faulted_func, NULL);
    ASSERT_IS_NOT_NULL(batch_queue);
    ASSERT_ARE_EQUAL(int, 0, batch_queue_open(batch_queue));

    ITEM_COMPLETE_CONTEXT* item_contexts = (ITEM_COMPLETE_CONTEXT*)malloc_2(number_of_items_to_queue, sizeof(ITEM_COMPLETE_CONTEXT));
    int32_t call;
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        item_contexts[call].item_complete_counter = &item_complete_counter;
        (void)interlocked_exchange(&item_contexts[call].result, -1);
    }

    // act
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        BATCH_QUEUE_ENQUEUE_RESULT enqueue_result = batch_queue_enqueue(batch_queue, buffers, batch_item_size, counted_item_complete_callback, &item_contexts[call]);
        ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_OK, enqueue_result);
    }

    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&process_batch_called, max_pending_for_batch, TEST_TIMEOUT_MS));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWakeAll(&allow_process_batch_to_continue, ALLOW_PROCESS_BATCH));

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&item_complete_counter, 0, TEST_TIMEOUT_MS));
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        BATCH_QUEUE_PROCESS_COMPLETE_RESULT item_result = interlocked_add(&item_contexts[call].result, 0);
        if (item_contexts[call].batch_order_number == 1)
        {
            ASSERT_ARE_EQUAL(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_ERROR, item_result);
        }
        else
        {
            ASSERT_ARE_EQUAL(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_OK, item_result);
        }
    }

    // cleanup
    batch_queue_close(batch_queue);
    batch_queue_destroy(batch_queue);
    for (int batch = 0; batch < expected_max_batches; batch++)
    {
        if (processor_context->per_batch_stats[batch].work_item_handle != NULL)
        {
            threadpool_destroy_work_item(gl.test_threadpool, processor_context->per_batch_stats[batch].work_item_handle);
        }
    }

    THANDLE_ASSIGN(THREADPOOL)(&processor_context->process_threadpool, NULL);
    free(item_contexts);
    free(processor_context);
    constbuffer_array_dec_ref(buffers);
}

TEST_FUNCTION(batch_queue_queue_a_batch_fails_sync_not_open_with_queued_items)
{
    // Enqueue some items, make sure settings allow for some to remain in queue
    const int32_t number_of_items_to_queue = 19;
    const int32_t max_pending_for_batch = 2;
    const int32_t max_batch_size_multiplier = 2;
    CONSTBUFFER_ARRAY_HANDLE buffers = test_create_buffers_to_load(10);
    uint32_t batch_item_size;
    ASSERT_ARE_EQUAL(int, 0, constbuffer_array_get_all_buffers_size(buffers, &batch_item_size));

    BATCH_QUEUE_SETTINGS settings =
    {
        .max_pending_requests = max_pending_for_batch, // much less than the total number of items queued
        .max_batch_size = max_batch_size_multiplier * batch_item_size, // small enough for multiple batches to possibly be called
        .min_batch_size = batch_item_size + 1, // Avoid triggering by min batch size so we know which items are in the batch.
        .min_wait_time = 6000 // 1 minute is way longer than this test should run.
    };
    int32_t expected_max_batches = number_of_items_to_queue;
    volatile_atomic int32_t item_complete_counter;
    volatile_atomic int32_t process_batch_called;
    volatile_atomic int32_t allow_process_batch_to_continue;
    volatile_atomic int32_t on_fault_called;
    BATCH_QUEUE_PROCESSOR_BATCH_STATS* processor_context;

    ASSERT_IS_TRUE(expected_max_batches > max_pending_for_batch, "Test is invalid, expected_max_batches should be greater than max_pending_for_batch for this test to work");
    LogInfo("Starting test with settings: %" PRI_BATCH_QUEUE_SETTINGS "", BATCH_QUEUE_SETTINGS_VALUES(settings));

    (void)interlocked_exchange(&on_fault_called, 0);

    setup_process_batch_context(settings.max_batch_size, expected_max_batches, 1, number_of_items_to_queue, ALLOW_PROCESS_BATCH, &item_complete_counter, &process_batch_called, &allow_process_batch_to_continue, &processor_context);
    processor_context->batch_sync_result = BATCH_QUEUE_PROCESS_SYNC_NOT_OPEN;

    BATCH_QUEUE_HANDLE batch_queue = batch_queue_create(settings, g_default_execution_engine, process_batch_with_stats_sync_fail_func, processor_context, on_faulted_callback, (void*)&on_fault_called);
    ASSERT_IS_NOT_NULL(batch_queue);
    ASSERT_ARE_EQUAL(int, 0, batch_queue_open(batch_queue));

    ITEM_COMPLETE_CONTEXT* item_contexts = (ITEM_COMPLETE_CONTEXT*)malloc_2(number_of_items_to_queue, sizeof(ITEM_COMPLETE_CONTEXT));
    int32_t call;
    int32_t index_of_first_enqueued_error = INT32_MAX;
    int32_t expected_items_to_complete = INT32_MAX;
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        item_contexts[call].item_complete_counter = &item_complete_counter;
        (void)interlocked_exchange(&item_contexts[call].result, -1);
    }

    // act
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        BATCH_QUEUE_ENQUEUE_RESULT enqueue_result = batch_queue_enqueue(batch_queue, buffers, batch_item_size, counted_item_complete_callback, &item_contexts[call]);
        if (enqueue_result == BATCH_QUEUE_ENQUEUE_INVALID_STATE)
        {
            // fault can happen before all items are enqueued.
            interlocked_decrement(&item_complete_counter);
            if (index_of_first_enqueued_error == INT32_MAX)
            {
                index_of_first_enqueued_error = call;
            }
        }
        else
        {
            ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_OK, enqueue_result);
        }
    }
    if (index_of_first_enqueued_error == INT32_MAX)
    {
        expected_items_to_complete = number_of_items_to_queue;
    }
    else
    {
        expected_items_to_complete = index_of_first_enqueued_error;
    }
    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&on_fault_called, 1, TEST_TIMEOUT_MS));
    // After the fault, 2 calls should have succeeded, X calls got errors, Y got queued or staged,
    // and maybe (number_of_items_to_queue-index_of_first_enqueued_error) never were queued.
    // No more items will be processed until close is called.
    batch_queue_close(batch_queue);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&item_complete_counter, 0, TEST_TIMEOUT_MS));
    for (call = 0; call < number_of_items_to_queue; call++)
    {
        BATCH_QUEUE_PROCESS_COMPLETE_RESULT item_result = interlocked_add(&item_contexts[call].result, 0);
        if (call < 2)
        {
            ASSERT_ARE_EQUAL(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_OK, interlocked_add(&item_contexts[call].result, 0));
        }
        else if (call < expected_items_to_complete)
        {
            ASSERT_IS_TRUE(item_result == BATCH_QUEUE_PROCESS_COMPLETE_ERROR || item_result == BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED);
        }
        else
        {
            // never got queued
            ASSERT_ARE_EQUAL(int32_t, -1, interlocked_add(&item_contexts[call].result, 0));
        }
    }

    // cleanup
    batch_queue_destroy(batch_queue);
    for (int batch = 0; batch < expected_max_batches; batch++)
    {
        if (processor_context->per_batch_stats[batch].work_item_handle != NULL)
        {
            threadpool_destroy_work_item(gl.test_threadpool, processor_context->per_batch_stats[batch].work_item_handle);
        }
    }

    THANDLE_ASSIGN(THREADPOOL)(&processor_context->process_threadpool, NULL);
    free(item_contexts);
    free(processor_context);
    constbuffer_array_dec_ref(buffers);
}

TEST_FUNCTION(batch_queue_can_reopen_and_enqueue)
{
    // Enqueue some items, make sure settings allow for some to remain in queue
    const int32_t number_of_items_to_queue = 19;
    const int32_t max_pending_for_batch = 2;
    const int32_t max_batch_size_multiplier = 2;
    CONSTBUFFER_ARRAY_HANDLE buffers = test_create_buffers_to_load(10);
    uint32_t batch_item_size;
    ASSERT_ARE_EQUAL(int, 0, constbuffer_array_get_all_buffers_size(buffers, &batch_item_size));

    BATCH_QUEUE_SETTINGS settings =
    {
        .max_pending_requests = max_pending_for_batch, // much less than the total number of items queued
        .max_batch_size = max_batch_size_multiplier * batch_item_size, // small enough for multiple batches to possibly be called
        .min_batch_size = batch_item_size + 1, // Avoid triggering by min batch size so we know which items are in the batch.
        .min_wait_time = 6000 // 1 minute is way longer than this test should run.
    };
    int32_t expected_max_batches = number_of_items_to_queue;
    volatile_atomic int32_t item_complete_counter;
    volatile_atomic int32_t process_batch_called;
    volatile_atomic int32_t allow_process_batch_to_continue;
    volatile_atomic int32_t on_fault_called;
    BATCH_QUEUE_PROCESSOR_BATCH_STATS* processor_context;

    LogInfo("Starting test with settings: %" PRI_BATCH_QUEUE_SETTINGS "", BATCH_QUEUE_SETTINGS_VALUES(settings));

    (void)interlocked_exchange(&on_fault_called, 0);

    setup_process_batch_context(settings.max_batch_size, expected_max_batches, -1, number_of_items_to_queue, ALLOW_PROCESS_BATCH, &item_complete_counter, &process_batch_called, &allow_process_batch_to_continue, &processor_context);

    BATCH_QUEUE_HANDLE batch_queue = batch_queue_create(settings, g_default_execution_engine, process_batch_with_stats_func, processor_context, on_faulted_callback, (void*)&on_fault_called);
    ASSERT_IS_NOT_NULL(batch_queue);
    ASSERT_ARE_EQUAL(int, 0, batch_queue_open(batch_queue));

    ITEM_COMPLETE_CONTEXT* item_contexts = (ITEM_COMPLETE_CONTEXT*)malloc_2(number_of_items_to_queue, sizeof(ITEM_COMPLETE_CONTEXT));
    int32_t call;
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        item_contexts[call].item_complete_counter = &item_complete_counter;
        (void)interlocked_exchange(&item_contexts[call].result, -1);
    }
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        BATCH_QUEUE_ENQUEUE_RESULT enqueue_result = batch_queue_enqueue(batch_queue, buffers, batch_item_size, counted_item_complete_callback, &item_contexts[call]);
        ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_OK, enqueue_result);
    }
    batch_queue_close(batch_queue);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&item_complete_counter, 0, TEST_TIMEOUT_MS));
    for (int batch = 0; batch < expected_max_batches; batch++)
    {
        if (processor_context->per_batch_stats[batch].work_item_handle != NULL)
        {
            threadpool_destroy_work_item(gl.test_threadpool, processor_context->per_batch_stats[batch].work_item_handle);
        }
    }

    // reset process context
    processor_context->expected_max_batches = expected_max_batches;
    processor_context->fail_on = -1;
    processor_context->batch_sync_result = BATCH_QUEUE_PROCESS_SYNC_OK;
    (void)interlocked_exchange(&processor_context->batch_order_number, -1);
    (void)interlocked_exchange(&process_batch_called, 0);
    (void)interlocked_exchange(&allow_process_batch_to_continue, ALLOW_PROCESS_BATCH);

    for (int batch = 0; batch < expected_max_batches; batch++)
    {
        processor_context->per_batch_stats[batch].number_of_items = 0;
        processor_context->per_batch_stats[batch].total_item_size = 0;
        processor_context->per_batch_stats[batch].work_item_handle = NULL;
    }
    (void)interlocked_exchange(&item_complete_counter, number_of_items_to_queue);
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        item_contexts[call].item_complete_counter = &item_complete_counter;
        (void)interlocked_exchange(&item_contexts[call].result, -1);
    }

    // act
    ASSERT_ARE_EQUAL(int, 0, batch_queue_open(batch_queue));
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        BATCH_QUEUE_ENQUEUE_RESULT enqueue_result = batch_queue_enqueue(batch_queue, buffers, batch_item_size, counted_item_complete_callback, &item_contexts[call]);
        ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_OK, enqueue_result);
    }

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&item_complete_counter, 0, TEST_TIMEOUT_MS));
    for (call = 0; call < number_of_items_to_queue; call++)
    {
        ASSERT_ARE_EQUAL(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_OK, interlocked_add(&item_contexts[call].result, 0));
    }

    // cleanup
    batch_queue_destroy(batch_queue);
    for (int batch = 0; batch < expected_max_batches; batch++)
    {
        if (processor_context->per_batch_stats[batch].work_item_handle != NULL)
        {
            threadpool_destroy_work_item(gl.test_threadpool, processor_context->per_batch_stats[batch].work_item_handle);
        }
    }

    THANDLE_ASSIGN(THREADPOOL)(&processor_context->process_threadpool, NULL);
    free(item_contexts);
    free(processor_context);
    constbuffer_array_dec_ref(buffers);
}

TEST_FUNCTION(batch_queue_queue_a_batch_fails_sync_error_with_queued_items)
{
    // Enqueue some items, make sure settings allow for some to remain in queue
    const int32_t number_of_items_to_queue = 23;
    const int32_t max_pending_for_batch = 3;
    const int32_t max_batch_size_multiplier = 2;
    CONSTBUFFER_ARRAY_HANDLE buffers = test_create_buffers_to_load(10);
    uint32_t batch_item_size;
    ASSERT_ARE_EQUAL(int, 0, constbuffer_array_get_all_buffers_size(buffers, &batch_item_size));

    BATCH_QUEUE_SETTINGS settings =
    {
        .max_pending_requests = max_pending_for_batch, // much less than the total number of items queued
        .max_batch_size = max_batch_size_multiplier * batch_item_size, // small enough for multiple batches to possibly be called
        .min_batch_size = batch_item_size + 1, // Avoid triggering by min batch size by setting this to slightly larger.
        .min_wait_time = 6000 // 1 minute is way longer than this test should run.
    };
    int32_t expected_max_batches = number_of_items_to_queue;
    volatile_atomic int32_t item_complete_counter;
    volatile_atomic int32_t process_batch_called;
    volatile_atomic int32_t allow_process_batch_to_continue;
    volatile_atomic int32_t on_fault_called;
    BATCH_QUEUE_PROCESSOR_BATCH_STATS* processor_context;

    ASSERT_IS_TRUE(expected_max_batches > max_pending_for_batch, "Test is invalid, expected_max_batches should be greater than max_pending_for_batch for this test to work");
    LogInfo("Starting test with settings: %" PRI_BATCH_QUEUE_SETTINGS "", BATCH_QUEUE_SETTINGS_VALUES(settings));

    (void)interlocked_exchange(&on_fault_called, 0);

    setup_process_batch_context(settings.max_batch_size, expected_max_batches, 1, number_of_items_to_queue, ALLOW_PROCESS_BATCH, &item_complete_counter, &process_batch_called, &allow_process_batch_to_continue, &processor_context);
    processor_context->batch_sync_result = BATCH_QUEUE_PROCESS_SYNC_ERROR;

    BATCH_QUEUE_HANDLE batch_queue = batch_queue_create(settings, g_default_execution_engine, process_batch_with_stats_sync_fail_func, processor_context, on_faulted_callback, (void*)&on_fault_called);
    ASSERT_IS_NOT_NULL(batch_queue);
    ASSERT_ARE_EQUAL(int, 0, batch_queue_open(batch_queue));

    ITEM_COMPLETE_CONTEXT* item_contexts = (ITEM_COMPLETE_CONTEXT*)malloc_2(number_of_items_to_queue, sizeof(ITEM_COMPLETE_CONTEXT));
    int32_t call;
    int32_t index_of_first_enqueued_error = INT32_MAX;
    int32_t expected_items_to_complete;
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        item_contexts[call].item_complete_counter = &item_complete_counter;
        (void)interlocked_exchange(&item_contexts[call].result, -1);
    }

    // act
    for (call = 0; call < number_of_items_to_queue; ++call)
    {
        BATCH_QUEUE_ENQUEUE_RESULT enqueue_result = batch_queue_enqueue(batch_queue, buffers, batch_item_size, counted_item_complete_callback, &item_contexts[call]);
        if (enqueue_result == BATCH_QUEUE_ENQUEUE_INVALID_STATE)
        {
            // fault can happen before all items are enqueued.
            interlocked_decrement(&item_complete_counter);
            if (index_of_first_enqueued_error == INT32_MAX)
            {
                index_of_first_enqueued_error = call;
            }
        }
        else
        {
            ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_OK, enqueue_result);
        }
    }
    if (index_of_first_enqueued_error == INT32_MAX)
    {
        expected_items_to_complete = number_of_items_to_queue;
    }
    else
    {
        expected_items_to_complete = index_of_first_enqueued_error;
    }

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&on_fault_called, 1, TEST_TIMEOUT_MS));
    // After the fault, 2(max_batch_size_multiplier) calls should have succeeded, X calls got errors, Y got queued or staged,
    // and maybe (number_of_items_to_queue-index_of_first_enqueued_error) never were queued.
    // No more items will be processed until close is called.
    batch_queue_close(batch_queue);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&item_complete_counter, 0, TEST_TIMEOUT_MS));
    for (call = 0; call < number_of_items_to_queue; call++)
    {
        BATCH_QUEUE_PROCESS_COMPLETE_RESULT item_result = interlocked_add(&item_contexts[call].result, 0);
        if (call < 2)
        {
            ASSERT_ARE_EQUAL(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_OK, interlocked_add(&item_contexts[call].result, 0));
        }
        else if (call < expected_items_to_complete)
        {
            ASSERT_IS_TRUE(item_result == BATCH_QUEUE_PROCESS_COMPLETE_ERROR || item_result == BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED);
        }
        else
        {
            // never got queued
            ASSERT_ARE_EQUAL(int32_t, -1, interlocked_add(&item_contexts[call].result, 0));
        }
    }

    // cleanup
    batch_queue_destroy(batch_queue);
    for (int batch = 0; batch < expected_max_batches; batch++)
    {
        if (processor_context->per_batch_stats[batch].work_item_handle != NULL)
        {
            threadpool_destroy_work_item(gl.test_threadpool, processor_context->per_batch_stats[batch].work_item_handle);
        }
    }

    THANDLE_ASSIGN(THREADPOOL)(&processor_context->process_threadpool, NULL);
    free(item_contexts);
    free(processor_context);
    constbuffer_array_dec_ref(buffers);
}


TEST_FUNCTION(batch_queue_can_be_opened_and_closed)
{
    // arrange
    BATCH_QUEUE_SETTINGS settings = 
    {
        .max_pending_requests = 8,
        .max_batch_size = 16192,
        .min_batch_size = 128,
        .min_wait_time = 10
    };

    BATCH_QUEUE_HANDLE batch_queue = batch_queue_create(settings, g_default_execution_engine, empty_process_batch_func, NULL, empty_on_batch_faulted_func, NULL);
    ASSERT_IS_NOT_NULL(batch_queue);

    // act
    int result = batch_queue_open(batch_queue);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    batch_queue_close(batch_queue);
    batch_queue_destroy(batch_queue);
}

TEST_FUNCTION(batch_queue_can_be_created_and_destroyed)
{
    // arrange
    BATCH_QUEUE_SETTINGS settings = 
    {
        .max_pending_requests = 8,
        .max_batch_size = 16192,
        .min_batch_size = 128,
        .min_wait_time = 10
    };

    // act
    BATCH_QUEUE_HANDLE batch_queue = batch_queue_create(settings, g_default_execution_engine, empty_process_batch_func, NULL, empty_on_batch_faulted_func, NULL);

    // assert
    ASSERT_IS_NOT_NULL(batch_queue);

    // cleanup
    batch_queue_destroy(batch_queue);
}


// Chaos test

typedef struct CHAOS_ITEM_TAG
{
    uint32_t delay_in_ms; // delay before calling on_ll_batch_complete
    BATCH_QUEUE_ON_LL_BATCH_COMPLETE on_ll_batch_complete; // callback to call when item is complete
    void* batch_context; // context to pass to callback

    BATCH_QUEUE_PROCESS_COMPLETE_RESULT result; // result of the item
    int32_t batch_order_number; // order in which item was batched
    volatile_atomic int32_t* batch_order_counter; // increment when item batch is called
    TIMER_INSTANCE_HANDLE timer; // hold onto this item's timer for cleanup
} CHAOS_ITEM;

typedef struct CHAOS_ITEM_CONTEXT_TAG
{
    CHAOS_ITEM* chaos_item;
    volatile_atomic int32_t* item_complete_counter; // decrement and wake when item is complete
} CHAOS_ITEM_CONTEXT;

typedef struct BATCH_PROCESSOR_CONTEXT_TAG 
{
    THANDLE(THREADPOOL) threadpool;
} BATCH_PROCESSOR_CONTEXT;

typedef struct CHAOS_THREAD_CONTEXT_TAG
{
    int32_t number_of_items_to_queue;
    uint32_t per_item_pause_max;
    BATCH_QUEUE_HANDLE batch_queue;
} CHAOS_THREAD_CONTEXT;

// Processor:

static void complete_batch(void* context)
{
    ASSERT_IS_NOT_NULL(context);
    CHAOS_ITEM* chaos_item = context;
    chaos_item->on_ll_batch_complete(chaos_item->batch_context, BATCH_QUEUE_PROCESS_COMPLETE_OK, NULL);
}

static BATCH_QUEUE_PROCESS_SYNC_RESULT chaos_process_batch_func(void* context, void** items, uint32_t item_count, BATCH_QUEUE_ON_LL_BATCH_COMPLETE on_ll_batch_complete, void* batch_context)
{

    ASSERT_IS_NOT_NULL(context);
    BATCH_PROCESSOR_CONTEXT* process_batch_context = context;

    THANDLE(THREADPOOL) threadpool = NULL;
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, process_batch_context->threadpool);
    ASSERT_IS_NOT_NULL(items);

    ASSERT_ARE_NOT_EQUAL(uint32_t, 0, item_count);

    uint32_t i;
    for (i = 0; i < item_count; i++)
    {
        ASSERT_IS_NOT_NULL(items[i]);
    }
    // Expectation is that all the items are processed in one batch with one final callback.
    for (i = 0; i < item_count; i++)
    {
        CHAOS_ITEM_CONTEXT* chaos_item_context = items[i];
        CHAOS_ITEM* chaos_item = chaos_item_context->chaos_item;
        // this will help prove we receieved the queued items in queued order.
        chaos_item->batch_order_number= interlocked_increment(chaos_item_context->chaos_item->batch_order_counter);
    }
    // pick one item to complete the batch
    CHAOS_ITEM_CONTEXT* chaos_item_context = items[0];
    CHAOS_ITEM* chaos_item = chaos_item_context->chaos_item;
    chaos_item->on_ll_batch_complete = on_ll_batch_complete;
    chaos_item->batch_context = batch_context;
    ASSERT_ARE_EQUAL(int, 0, threadpool_timer_start(threadpool, chaos_item->delay_in_ms, UINT32_MAX, complete_batch, chaos_item, &chaos_item->timer));
    ASSERT_IS_NOT_NULL(chaos_item->timer);
    THANDLE_ASSIGN(THREADPOOL)(&threadpool, NULL);

    return BATCH_QUEUE_PROCESS_SYNC_OK;
}

// Requestor:

static void chaos_item_complete(void* context, BATCH_QUEUE_PROCESS_COMPLETE_RESULT result, void* ll_result)
{
    (void)ll_result;
    ASSERT_IS_NOT_NULL(context);
    CHAOS_ITEM_CONTEXT* chaos_item_context = context;
    chaos_item_context->chaos_item->result = result;
    if (result == BATCH_QUEUE_PROCESS_COMPLETE_OK)
    {
        InterlockedHL_DecrementAndWake(chaos_item_context->item_complete_counter);
    }
    else
    {
        LogError("Item failed with result %" PRI_MU_ENUM "", MU_ENUM_VALUE(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, result));
    }
    free(chaos_item_context);
}

static int chaos_thread_worker_function(void* context)
{
    int thread_result = 0;
    ASSERT_IS_NOT_NULL(context);
    CHAOS_THREAD_CONTEXT* chaos_thread_context = context;

    volatile_atomic int32_t item_complete_counter; 
    (void)interlocked_exchange(&item_complete_counter, chaos_thread_context->number_of_items_to_queue);
    volatile_atomic int32_t batch_order_counter;
    (void)interlocked_exchange(&batch_order_counter, 0);
    uint32_t random_per_item_pause_max = chaos_thread_context->per_item_pause_max;

    CHAOS_ITEM* chaos_items = (CHAOS_ITEM *)malloc_2(chaos_thread_context->number_of_items_to_queue, sizeof(CHAOS_ITEM));
    ASSERT_IS_NOT_NULL(chaos_items);

    for (int i = 0; i < chaos_thread_context->number_of_items_to_queue; i++)
    {
        CHAOS_ITEM* chaos_item = &chaos_items[i];
        chaos_item->delay_in_ms = (rand() % 100) + 1;
        chaos_item->batch_order_counter = &batch_order_counter;
        chaos_item->timer = NULL;

        CHAOS_ITEM_CONTEXT* chaos_item_context = (CHAOS_ITEM_CONTEXT*)malloc(sizeof(CHAOS_ITEM_CONTEXT));
        ASSERT_IS_NOT_NULL(chaos_item_context);
        chaos_item_context->chaos_item = chaos_item;
        chaos_item_context->item_complete_counter = &item_complete_counter;


        BATCH_QUEUE_ENQUEUE_RESULT enqueue_result = batch_queue_enqueue(chaos_thread_context->batch_queue, chaos_item_context, sizeof(CHAOS_ITEM_CONTEXT), chaos_item_complete, chaos_item_context);
        if (enqueue_result != BATCH_QUEUE_ENQUEUE_OK)
        {
            thread_result = MU_FAILURE;
            break;
        }

        ThreadAPI_Sleep((rand() % random_per_item_pause_max) + 1);
    }

    if (thread_result == 0)
    {
        INTERLOCKED_HL_RESULT wait_result = InterlockedHL_WaitForValue(&item_complete_counter, 0, chaos_thread_context->number_of_items_to_queue * 200);
        if (wait_result != INTERLOCKED_HL_OK)
        {
            thread_result = MU_FAILURE;
        }
        else
        {
            int i;
            int batch_order_number = chaos_items[0].batch_order_number;
            if (chaos_items[0].timer != NULL)
            {
                threadpool_timer_destroy(chaos_items[0].timer);
            }
            for (i = 1; i < chaos_thread_context->number_of_items_to_queue; i++)
            {
                ASSERT_IS_TRUE(chaos_items[i].batch_order_number > batch_order_number);
                ASSERT_ARE_EQUAL(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_OK, chaos_items[i].result);
                batch_order_number = chaos_items[i].batch_order_number;
                if (chaos_items[i].timer != NULL)
                {
                    threadpool_timer_destroy(chaos_items[i].timer);
                }
            }
        }
    }
    free(chaos_items);

    return thread_result;
}

#define NUMBER_OF_CHAOS_THREADS 12
#define MIN_ITEMS_PER_THREAD 200
#define MAX_ITEMS_PER_THREAD 400
#define MIN_WAIT_TIME 50

// Make sure the batch sizes make sense.
TEST_FUNCTION(batch_queue_chaos_multiple_threads_one_batch_queue)
{
    // arrange

    BATCH_PROCESSOR_CONTEXT processor_context;
    THANDLE_INITIALIZE(THREADPOOL)(&processor_context.threadpool, gl.test_threadpool);
    uint32_t batch_item_size = sizeof(CHAOS_ITEM_CONTEXT);

    BATCH_QUEUE_SETTINGS settings = {
        .max_pending_requests = 6,
        .max_batch_size = 9*batch_item_size,
        .min_batch_size = 3*batch_item_size,
        .min_wait_time = MIN_WAIT_TIME };
    LogInfo("Starting test with settings: %" PRI_BATCH_QUEUE_SETTINGS "", BATCH_QUEUE_SETTINGS_VALUES(settings));
    BATCH_QUEUE_HANDLE batch_queue = batch_queue_create(settings, g_default_execution_engine, chaos_process_batch_func, &processor_context, empty_on_batch_faulted_func, NULL);
    ASSERT_IS_NOT_NULL(batch_queue);
    ASSERT_ARE_EQUAL(int, 0, batch_queue_open(batch_queue));

    CHAOS_THREAD_CONTEXT thread_contexts[NUMBER_OF_CHAOS_THREADS];
    THREAD_HANDLE thread_handles[NUMBER_OF_CHAOS_THREADS];

    int thread;
    for (thread = 0; thread < NUMBER_OF_CHAOS_THREADS; thread++)
    {
        thread_contexts[thread].number_of_items_to_queue = (rand() % (MAX_ITEMS_PER_THREAD - MIN_ITEMS_PER_THREAD)) + MIN_ITEMS_PER_THREAD;
        thread_contexts[thread].per_item_pause_max = (rand() % (MIN_WAIT_TIME/2)) + 10;
        thread_contexts[thread].batch_queue = batch_queue;
        LogInfo("Starting thread %" PRId32 ", number of items to queue=%" PRId32 ", per_item_pause_max=%" PRIu32 "", thread, thread_contexts[thread].number_of_items_to_queue, thread_contexts[thread].per_item_pause_max);

        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&thread_handles[thread], chaos_thread_worker_function, &thread_contexts[thread]));
    }

    for (thread = 0; thread < NUMBER_OF_CHAOS_THREADS; thread++)
    {
        int thread_result;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(thread_handles[thread], &thread_result));
        ASSERT_ARE_EQUAL(int, 0, thread_result);
    }

    // cleanup
    THANDLE_ASSIGN(THREADPOOL)(&processor_context.threadpool, NULL);
    batch_queue_close(batch_queue);
    batch_queue_destroy(batch_queue);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
