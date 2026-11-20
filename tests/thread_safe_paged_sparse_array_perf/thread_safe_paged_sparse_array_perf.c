// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/interlocked.h"
#include "c_pal/thandle.h"
#include "c_pal/threadapi.h"
#include "c_pal/timer.h"

#include "c_util/thread_safe_paged_sparse_array.h"

/*
 * Performance test for THREAD_SAFE_PAGED_SPARSE_ARRAY used the way bs_ordered_op_writer uses its queue of operations:
 * - every operation takes the next sequence number, and its element is at index sequence_number % PERF_MAX_SIZE
 * - two producer threads, running at different rates, allocate the elements at the head of the queue (like write_async)
 * - one process thread gets the elements in sequence number order behind the producers (like process_write_operations),
 *   it waits for an element whose sequence number was taken but that is not allocated yet
 * - one consumer thread gets and releases the processed elements in sequence number order at the tail of the queue (like indicate_completions)
 * - a producer waits while the queue is full: its sequence number is PERF_MAX_SIZE ahead of the tail
 * The same workload also runs on a fixed array of slots, which is what bs_ordered_op_writer uses today, as a baseline, and on an array of
 * THANDLEs, which is what bs_ordered_op_writer would store: every operation is a new object, the array and the threads that get it hold references to it.
 * Timing every operation slows down the threads that run as fast as possible, so the scenarios where a thread runs as fast as possible only
 * measure the throughput.
 */

/*the queue size of bs_ordered_op_writer*/
#define PERF_MAX_SIZE (512 * 1024)
#define PERF_RUN_DURATION_MS 1000
#define PERF_PRODUCER_COUNT 2
#define PERF_SLOW_OPERATION_US 10.0

/*the item has the size of QUEUED_OPERATION_SLOT in bs_ordered_op_writer (80 bytes)*/
typedef struct PERF_ITEM_TAG
{
    uint64_t sequence_number;
    unsigned char payload[72];
} PERF_ITEM;

THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(PERF_ITEM);
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(PERF_ITEM));
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(PERF_ITEM));

THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(PERF_ITEM);
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(PERF_ITEM);

/*an operation as an object that is referenced by THANDLEs, it has the size of QUEUED_OPERATION_SLOT in bs_ordered_op_writer (80 bytes)*/
typedef struct PERF_OPERATION_TAG
{
    uint64_t sequence_number;
    unsigned char payload[72];
} PERF_OPERATION;

THANDLE_TYPE_DECLARE(PERF_OPERATION);
THANDLE_TYPE_DEFINE(PERF_OPERATION);

THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(THANDLE(PERF_OPERATION));
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(THANDLE(PERF_OPERATION)));
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(THANDLE(PERF_OPERATION)));

THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(THANDLE(PERF_OPERATION));
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(THANDLE(PERF_OPERATION));

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES)

/*the queue of operations, implemented by a fixed array of slots or by THREAD_SAFE_PAGED_SPARSE_ARRAY*/
typedef struct PERF_QUEUE_INTERFACE_TAG
{
    const char* name;
    void* (*create)(uint64_t max_size, uint64_t page_size);
    void (*destroy)(void* queue);
    /*allocates the element at index and copies item in it, returns false if the element could not be allocated*/
    bool (*push)(void* queue, uint64_t index, PERF_ITEM* item);
    /*returns false if the element at index is not allocated or its item is not written yet*/
    bool (*try_get)(void* queue, uint64_t index, uint64_t* sequence_number);
    void (*release)(void* queue, uint64_t index);
} PERF_QUEUE_INTERFACE;

/*fixed array of slots, like the queue of bs_ordered_op_writer today*/
#define FIXED_SLOT_STATE_FREE 0
#define FIXED_SLOT_STATE_WRITING 1
#define FIXED_SLOT_STATE_READY 2

typedef struct FIXED_SLOT_TAG
{
    volatile_atomic int32_t state;
    PERF_ITEM item;
} FIXED_SLOT;

typedef struct FIXED_ARRAY_QUEUE_TAG
{
    uint64_t max_size;
    FIXED_SLOT slots[];
} FIXED_ARRAY_QUEUE;

static void* fixed_array_create(uint64_t max_size, uint64_t page_size)
{
    (void)page_size;
    FIXED_ARRAY_QUEUE* result = malloc_flex(sizeof(FIXED_ARRAY_QUEUE), max_size, sizeof(FIXED_SLOT));
    if (result != NULL)
    {
        result->max_size = max_size;
        for (uint64_t index = 0; index < max_size; index++)
        {
            (void)interlocked_exchange(&result->slots[index].state, FIXED_SLOT_STATE_FREE);
        }
    }
    return result;
}

static void fixed_array_destroy(void* queue)
{
    free(queue);
}

static bool fixed_array_push(void* queue, uint64_t index, PERF_ITEM* item)
{
    bool result;
    FIXED_SLOT* slot = &((FIXED_ARRAY_QUEUE*)queue)->slots[index];
    if (interlocked_compare_exchange(&slot->state, FIXED_SLOT_STATE_WRITING, FIXED_SLOT_STATE_FREE) != FIXED_SLOT_STATE_FREE)
    {
        result = false;
    }
    else
    {
        slot->item = *item;
        (void)interlocked_exchange(&slot->state, FIXED_SLOT_STATE_READY);
        result = true;
    }
    return result;
}

static bool fixed_array_try_get(void* queue, uint64_t index, uint64_t* sequence_number)
{
    bool result;
    FIXED_SLOT* slot = &((FIXED_ARRAY_QUEUE*)queue)->slots[index];
    if (interlocked_add(&slot->state, 0) != FIXED_SLOT_STATE_READY)
    {
        result = false;
    }
    else
    {
        *sequence_number = slot->item.sequence_number;
        result = true;
    }
    return result;
}

static void fixed_array_release(void* queue, uint64_t index)
{
    (void)interlocked_exchange(&((FIXED_ARRAY_QUEUE*)queue)->slots[index].state, FIXED_SLOT_STATE_FREE);
}

static const PERF_QUEUE_INTERFACE fixed_array_queue_interface =
{
    "fixed array of slots",
    fixed_array_create,
    fixed_array_destroy,
    fixed_array_push,
    fixed_array_try_get,
    fixed_array_release
};

/*THREAD_SAFE_PAGED_SPARSE_ARRAY*/
typedef struct PAGED_QUEUE_TAG
{
    THREAD_SAFE_PAGED_SPARSE_ARRAY(PERF_ITEM) array;
} PAGED_QUEUE;

static void* paged_create(uint64_t max_size, uint64_t page_size)
{
    PAGED_QUEUE* result = malloc(sizeof(PAGED_QUEUE));
    if (result != NULL)
    {
        THREAD_SAFE_PAGED_SPARSE_ARRAY(PERF_ITEM) array = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(PERF_ITEM)(max_size, page_size, NULL, NULL);
        if (array == NULL)
        {
            free(result);
            result = NULL;
        }
        else
        {
            THREAD_SAFE_PAGED_SPARSE_ARRAY_INITIALIZE_MOVE(PERF_ITEM)(&result->array, &array);
        }
    }
    return result;
}

static void paged_destroy(void* queue)
{
    PAGED_QUEUE* paged_queue = queue;
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(PERF_ITEM)(&paged_queue->array, NULL);
    free(paged_queue);
}

static bool paged_push(void* queue, uint64_t index, PERF_ITEM* item)
{
    return THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(PERF_ITEM)(((PAGED_QUEUE*)queue)->array, index, item) == PAGED_SPARSE_ARRAY_ALLOCATE_OK;
}

static bool paged_try_get(void* queue, uint64_t index, uint64_t* sequence_number)
{
    bool result;
    PERF_ITEM item;
    if (THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(PERF_ITEM)(((PAGED_QUEUE*)queue)->array, index, &item) != PAGED_SPARSE_ARRAY_GET_OK)
    {
        result = false;
    }
    else
    {
        *sequence_number = item.sequence_number;
        result = true;
    }
    return result;
}

static void paged_release(void* queue, uint64_t index)
{
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(PERF_ITEM)(((PAGED_QUEUE*)queue)->array, index);
}

static const PERF_QUEUE_INTERFACE paged_queue_interface =
{
    "THREAD_SAFE_PAGED_SPARSE_ARRAY",
    paged_create,
    paged_destroy,
    paged_push,
    paged_try_get,
    paged_release
};

/*THREAD_SAFE_PAGED_SPARSE_ARRAY of THANDLE(PERF_OPERATION): push creates the operation, the array and the threads that get the operation hold references to it*/
typedef struct PAGED_HANDLE_QUEUE_TAG
{
    THREAD_SAFE_PAGED_SPARSE_ARRAY(THANDLE(PERF_OPERATION)) array;
} PAGED_HANDLE_QUEUE;

static void operation_handle_copy(THANDLE(PERF_OPERATION)* dst, THANDLE(PERF_OPERATION)* src)
{
    THANDLE_INITIALIZE(PERF_OPERATION)(dst, *src);
}

static void operation_handle_dispose(THANDLE(PERF_OPERATION)* operation)
{
    THANDLE_ASSIGN(PERF_OPERATION)(operation, NULL);
}

static void* paged_handle_create(uint64_t max_size, uint64_t page_size)
{
    PAGED_HANDLE_QUEUE* result = malloc(sizeof(PAGED_HANDLE_QUEUE));
    if (result != NULL)
    {
        THREAD_SAFE_PAGED_SPARSE_ARRAY(THANDLE(PERF_OPERATION)) array = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(THANDLE(PERF_OPERATION))(max_size, page_size, operation_handle_copy, operation_handle_dispose);
        if (array == NULL)
        {
            free(result);
            result = NULL;
        }
        else
        {
            THREAD_SAFE_PAGED_SPARSE_ARRAY_INITIALIZE_MOVE(THANDLE(PERF_OPERATION))(&result->array, &array);
        }
    }
    return result;
}

static void paged_handle_destroy(void* queue)
{
    PAGED_HANDLE_QUEUE* paged_handle_queue = queue;
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(THANDLE(PERF_OPERATION))(&paged_handle_queue->array, NULL);
    free(paged_handle_queue);
}

static bool paged_handle_push(void* queue, uint64_t index, PERF_ITEM* item)
{
    bool result;
    PERF_OPERATION* operation = THANDLE_MALLOC(PERF_OPERATION)(NULL);
    if (operation == NULL)
    {
        result = false;
    }
    else
    {
        operation->sequence_number = item->sequence_number;
        (void)memcpy(operation->payload, item->payload, sizeof(operation->payload));
        THANDLE(PERF_OPERATION) operation_handle = operation;
        result = (THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(THANDLE(PERF_OPERATION))(((PAGED_HANDLE_QUEUE*)queue)->array, index, &operation_handle) == PAGED_SPARSE_ARRAY_ALLOCATE_OK);
        /*the array holds its own reference*/
        THANDLE_ASSIGN(PERF_OPERATION)(&operation_handle, NULL);
    }
    return result;
}

static bool paged_handle_try_get(void* queue, uint64_t index, uint64_t* sequence_number)
{
    bool result;
    THANDLE(PERF_OPERATION) operation = NULL;
    if (THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(THANDLE(PERF_OPERATION))(((PAGED_HANDLE_QUEUE*)queue)->array, index, &operation) != PAGED_SPARSE_ARRAY_GET_OK)
    {
        result = false;
    }
    else
    {
        *sequence_number = operation->sequence_number;
        THANDLE_ASSIGN(PERF_OPERATION)(&operation, NULL);
        result = true;
    }
    return result;
}

static void paged_handle_release(void* queue, uint64_t index)
{
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(THANDLE(PERF_OPERATION))(((PAGED_HANDLE_QUEUE*)queue)->array, index);
}

static const PERF_QUEUE_INTERFACE paged_handle_queue_interface =
{
    "THREAD_SAFE_PAGED_SPARSE_ARRAY of THANDLE",
    paged_handle_create,
    paged_handle_destroy,
    paged_handle_push,
    paged_handle_try_get,
    paged_handle_release
};

typedef struct PERF_QUEUE_CONFIG_TAG
{
    const PERF_QUEUE_INTERFACE* queue_interface;
    uint64_t page_size;
} PERF_QUEUE_CONFIG;

static const PERF_QUEUE_CONFIG queue_configs[] =
{
    { &fixed_array_queue_interface, 0 },
    { &paged_queue_interface, 64 },
    { &paged_queue_interface, 1024 },
    { &paged_queue_interface, 16384 },
    { &paged_handle_queue_interface, 1024 }
};

typedef struct PERF_SCENARIO_TAG
{
    const char* name;
    double producer_interval_us[PERF_PRODUCER_COUNT]; /*time between the operations of each producer, 0 means as fast as possible*/
    uint32_t consumer_batch_size; /*the consumer gets up to this many ready elements from the tail, then releases them*/
    double consumer_interval_us; /*time between the elements released by the consumer, 0 means as fast as possible*/
} PERF_SCENARIO;

typedef struct PERF_LATENCY_TAG
{
    double sum_us;
    uint64_t count;
    double max_us;
    uint64_t slow_count; /*operations that took longer than PERF_SLOW_OPERATION_US*/
} PERF_LATENCY;

typedef struct PERF_THREAD_STATS_TAG
{
    uint64_t operation_count;
    uint64_t wait_count; /*producer: times the queue was full, process and consumer: times no element was ready*/
    uint64_t batch_count;
    uint64_t in_flight_sum; /*consumer: head - tail before each batch*/
    uint64_t in_flight_max;
    PERF_LATENCY latency; /*producer: push of 1 element, consumer: get and release of a batch*/
} PERF_THREAD_STATS;

typedef struct PERF_RUN_TAG
{
    const PERF_QUEUE_INTERFACE* queue_interface;
    void* queue;
    const PERF_SCENARIO* scenario;
    bool measure_latency;
    volatile_atomic int32_t stop; /*1 when the producers shall stop taking sequence numbers*/
    volatile_atomic int32_t running_producer_count;
    volatile_atomic int64_t next_sequence_number; /*the head of the queue*/
    volatile_atomic int64_t processed; /*the process thread got all the elements with a lower sequence number*/
    volatile_atomic int64_t tail; /*all the elements with a lower sequence number were released*/
    volatile_atomic int32_t failures;
    double end_time_us; /*when the consumer released the last element*/
} PERF_RUN;

typedef struct PERF_THREAD_CONTEXT_TAG
{
    PERF_RUN* run;
    double interval_us;
    PERF_THREAD_STATS stats;
} PERF_THREAD_CONTEXT;

static void record_latency(PERF_LATENCY* latency, double start_us, double end_us)
{
    double duration_us = end_us - start_us;
    latency->sum_us += duration_us;
    latency->count++;
    if (duration_us > latency->max_us)
    {
        latency->max_us = duration_us;
    }
    if (duration_us > PERF_SLOW_OPERATION_US)
    {
        latency->slow_count++;
    }
}

/*spins until next_time_us and returns the time when the next operation shall start, a thread that fell behind does not run faster to catch up*/
static double wait_until(double next_time_us, double interval_us)
{
    double now_us;
    do
    {
        now_us = timer_global_get_elapsed_us();
    } while (now_us < next_time_us);
    return now_us + interval_us;
}

static int producer_thread(void* arg)
{
    PERF_THREAD_CONTEXT* context = arg;
    PERF_RUN* run = context->run;
    PERF_ITEM item;
    (void)memset(&item, 0, sizeof(item));
    double next_time_us = timer_global_get_elapsed_us();
    while (interlocked_add(&run->stop, 0) == 0)
    {
        if (context->interval_us > 0)
        {
            next_time_us = wait_until(next_time_us, context->interval_us);
        }
        int64_t sequence_number = interlocked_increment_64(&run->next_sequence_number) - 1;
        if (sequence_number - interlocked_add_64(&run->tail, 0) >= PERF_MAX_SIZE)
        {
            context->stats.wait_count++;
            while (
                (sequence_number - interlocked_add_64(&run->tail, 0) >= PERF_MAX_SIZE) &&
                (interlocked_add(&run->failures, 0) == 0)
                )
            {
                /*the queue is full, wait for the consumer to release the element that has this index*/
            }
        }
        item.sequence_number = (uint64_t)sequence_number;
        double start_us = run->measure_latency ? timer_global_get_elapsed_us() : 0;
        if (!run->queue_interface->push(run->queue, (uint64_t)sequence_number % PERF_MAX_SIZE, &item))
        {
            LogError("push failed for sequence_number=%" PRId64 "", sequence_number);
            (void)interlocked_increment(&run->failures);
            break;
        }
        if (run->measure_latency)
        {
            record_latency(&context->stats.latency, start_us, timer_global_get_elapsed_us());
        }
        context->stats.operation_count++;
    }
    (void)interlocked_decrement(&run->running_producer_count);
    return 0;
}

static int process_thread(void* arg)
{
    PERF_THREAD_CONTEXT* context = arg;
    PERF_RUN* run = context->run;
    int64_t processed = 0;
    while (interlocked_add(&run->failures, 0) == 0)
    {
        int64_t head = interlocked_add_64(&run->next_sequence_number, 0);
        int64_t processed_before = processed;
        while (processed < head)
        {
            uint64_t sequence_number;
            if (!run->queue_interface->try_get(run->queue, (uint64_t)processed % PERF_MAX_SIZE, &sequence_number))
            {
                /*the producer that took this sequence number did not push it yet*/
                break;
            }
            if ((sequence_number + PERF_MAX_SIZE) == (uint64_t)processed)
            {
                /*the consumer did not release the element of the previous lap yet, so the producer that took this sequence number did not push it yet*/
                break;
            }
            if (sequence_number != (uint64_t)processed)
            {
                LogError("process thread expected sequence_number=%" PRId64 ", got %" PRIu64 "", processed, sequence_number);
                (void)interlocked_increment(&run->failures);
            }
            processed++;
            (void)interlocked_exchange_64(&run->processed, processed);
        }
        if (processed == processed_before)
        {
            if (
                (interlocked_add(&run->running_producer_count, 0) == 0) &&
                (processed == interlocked_add_64(&run->next_sequence_number, 0))
                )
            {
                break;
            }
            context->stats.wait_count++;
        }
        else
        {
            context->stats.operation_count += (uint64_t)(processed - processed_before);
        }
    }
    return 0;
}

static int consumer_thread(void* arg)
{
    PERF_THREAD_CONTEXT* context = arg;
    PERF_RUN* run = context->run;
    uint32_t batch_size = run->scenario->consumer_batch_size;
    int64_t tail = 0;
    double next_time_us = timer_global_get_elapsed_us();
    while (interlocked_add(&run->failures, 0) == 0)
    {
        int64_t head = interlocked_add_64(&run->next_sequence_number, 0);
        /*an element is released only after the process thread got it*/
        int64_t processed = interlocked_add_64(&run->processed, 0);
        double start_us = run->measure_latency ? timer_global_get_elapsed_us() : 0;
        uint32_t count = 0;
        while ((count < batch_size) && (tail + count < processed))
        {
            uint64_t sequence_number;
            if (!run->queue_interface->try_get(run->queue, (uint64_t)(tail + count) % PERF_MAX_SIZE, &sequence_number))
            {
                /*the process thread got this element and only the consumer releases it*/
                LogError("consumer could not get sequence_number=%" PRId64 ", processed=%" PRId64 ", head=%" PRId64 "", tail + count, processed, head);
                (void)interlocked_increment(&run->failures);
                break;
            }
            if (sequence_number != (uint64_t)(tail + count))
            {
                LogError("consumer expected sequence_number=%" PRId64 ", got %" PRIu64 "", tail + count, sequence_number);
                (void)interlocked_increment(&run->failures);
            }
            count++;
        }
        if (count == 0)
        {
            if (
                (interlocked_add(&run->running_producer_count, 0) == 0) &&
                (tail == interlocked_add_64(&run->next_sequence_number, 0))
                )
            {
                break;
            }
            context->stats.wait_count++;
        }
        else
        {
            uint64_t in_flight = (uint64_t)(head - tail);
            context->stats.in_flight_sum += in_flight;
            if (in_flight > context->stats.in_flight_max)
            {
                context->stats.in_flight_max = in_flight;
            }
            for (uint32_t i = 0; i < count; i++)
            {
                run->queue_interface->release(run->queue, (uint64_t)(tail + i) % PERF_MAX_SIZE);
            }
            tail += count;
            (void)interlocked_exchange_64(&run->tail, tail);
            if (run->measure_latency)
            {
                record_latency(&context->stats.latency, start_us, timer_global_get_elapsed_us());
            }
            context->stats.operation_count += count;
            context->stats.batch_count++;
            if (context->interval_us > 0)
            {
                next_time_us = wait_until(next_time_us, context->interval_us * count);
            }
        }
    }
    run->end_time_us = timer_global_get_elapsed_us();
    return 0;
}

static void run_scenario(const PERF_QUEUE_CONFIG* queue_config, const PERF_SCENARIO* scenario)
{
    ///arrange
    PERF_RUN run;
    run.queue_interface = queue_config->queue_interface;
    run.scenario = scenario;
    /*a thread that runs as fast as possible is slowed down by the timer, so only the throughput is measured then*/
    run.measure_latency = (scenario->producer_interval_us[0] > 0) && (scenario->producer_interval_us[1] > 0);
    run.queue = run.queue_interface->create(PERF_MAX_SIZE, queue_config->page_size);
    ASSERT_IS_NOT_NULL(run.queue);
    (void)interlocked_exchange(&run.stop, 0);
    (void)interlocked_exchange(&run.running_producer_count, PERF_PRODUCER_COUNT);
    (void)interlocked_exchange_64(&run.next_sequence_number, 0);
    (void)interlocked_exchange_64(&run.processed, 0);
    (void)interlocked_exchange_64(&run.tail, 0);
    (void)interlocked_exchange(&run.failures, 0);

    PERF_THREAD_CONTEXT producers[PERF_PRODUCER_COUNT];
    THREAD_HANDLE producer_threads[PERF_PRODUCER_COUNT];
    for (uint32_t i = 0; i < PERF_PRODUCER_COUNT; i++)
    {
        (void)memset(&producers[i], 0, sizeof(producers[i]));
        producers[i].run = &run;
        producers[i].interval_us = scenario->producer_interval_us[i];
    }
    PERF_THREAD_CONTEXT process;
    THREAD_HANDLE process_thread_handle;
    (void)memset(&process, 0, sizeof(process));
    process.run = &run;
    PERF_THREAD_CONTEXT consumer;
    THREAD_HANDLE consumer_thread_handle;
    (void)memset(&consumer, 0, sizeof(consumer));
    consumer.run = &run;
    consumer.interval_us = scenario->consumer_interval_us;

    ///act
    double start_time_us = timer_global_get_elapsed_us();
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&consumer_thread_handle, consumer_thread, &consumer));
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&process_thread_handle, process_thread, &process));
    for (uint32_t i = 0; i < PERF_PRODUCER_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&producer_threads[i], producer_thread, &producers[i]));
    }
    ThreadAPI_Sleep(PERF_RUN_DURATION_MS);
    (void)interlocked_exchange(&run.stop, 1);
    for (uint32_t i = 0; i < PERF_PRODUCER_COUNT; i++)
    {
        int thread_result;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(producer_threads[i], &thread_result));
    }
    int process_result;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(process_thread_handle, &process_result));
    int consumer_result;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(consumer_thread_handle, &consumer_result));

    ///assert
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&run.failures, 0));
    uint64_t produced_count = 0;
    uint64_t full_wait_count = 0;
    for (uint32_t i = 0; i < PERF_PRODUCER_COUNT; i++)
    {
        produced_count += producers[i].stats.operation_count;
        full_wait_count += producers[i].stats.wait_count;
    }
    ASSERT_ARE_EQUAL(uint64_t, produced_count, process.stats.operation_count);
    ASSERT_ARE_EQUAL(uint64_t, produced_count, consumer.stats.operation_count);
    ASSERT_ARE_EQUAL(int64_t, interlocked_add_64(&run.next_sequence_number, 0), interlocked_add_64(&run.tail, 0));

    double duration_us = run.end_time_us - start_time_us;
    char page_size_string[32];
    if (queue_config->page_size == 0)
    {
        (void)snprintf(page_size_string, sizeof(page_size_string), "no pages");
    }
    else
    {
        (void)snprintf(page_size_string, sizeof(page_size_string), "page_size=%" PRIu64 "", queue_config->page_size);
    }
    LogInfo("[%s] %s, %s: %" PRIu64 " ops (%" PRIu64 " + %" PRIu64 ") in %.0f ms = %.2f Mops/s, %.1f ns/op | avg in flight %.1f, max %" PRIu64 " | full waits %" PRIu64 ", process empty polls %" PRIu64 ", consumer empty polls %" PRIu64 ", batches %" PRIu64 "",
        scenario->name, run.queue_interface->name, page_size_string,
        produced_count, producers[0].stats.operation_count, producers[1].stats.operation_count,
        duration_us / 1000.0, (double)produced_count / duration_us, (duration_us * 1000.0) / (double)produced_count,
        (consumer.stats.batch_count == 0) ? 0.0 : (double)consumer.stats.in_flight_sum / (double)consumer.stats.batch_count, consumer.stats.in_flight_max,
        full_wait_count, process.stats.wait_count, consumer.stats.wait_count, consumer.stats.batch_count);
    if (run.measure_latency)
    {
        PERF_LATENCY push_latency = { 0 };
        for (uint32_t i = 0; i < PERF_PRODUCER_COUNT; i++)
        {
            push_latency.sum_us += producers[i].stats.latency.sum_us;
            push_latency.count += producers[i].stats.latency.count;
            push_latency.slow_count += producers[i].stats.latency.slow_count;
            if (producers[i].stats.latency.max_us > push_latency.max_us)
            {
                push_latency.max_us = producers[i].stats.latency.max_us;
            }
        }
        LogInfo("[%s] %s, %s: push avg %.0f ns, max %.1f us, %" PRIu64 " over %.0f us | get+release avg %.0f ns per element, max %.1f us per batch, %" PRIu64 " batches over %.0f us",
            scenario->name, run.queue_interface->name, page_size_string,
            (push_latency.count == 0) ? 0.0 : (push_latency.sum_us * 1000.0) / (double)push_latency.count, push_latency.max_us, push_latency.slow_count, PERF_SLOW_OPERATION_US,
            (consumer.stats.operation_count == 0) ? 0.0 : (consumer.stats.latency.sum_us * 1000.0) / (double)consumer.stats.operation_count, consumer.stats.latency.max_us,
            consumer.stats.latency.slow_count, PERF_SLOW_OPERATION_US);
    }

    ///cleanup
    run.queue_interface->destroy(run.queue);
}

static void run_scenario_for_all_queues(const PERF_SCENARIO* scenario)
{
    for (size_t i = 0; i < MU_COUNT_ARRAY_ITEMS(queue_configs); i++)
    {
        run_scenario(&queue_configs[i], scenario);
    }
}

/*
 * Readers on the same page: reader_count threads call ACQUIRE_ITEM on elements of one page of the queue as fast as possible, like the callers that read
 * the elements of other callers (for example bsi_sequence_tracker_is_undoing). It measures the cost of an ACQUIRE_ITEM when several threads use the same page.
 */
#define PERF_READER_ELEMENT_COUNT 64
#define PERF_READERS_DURATION_MS 500
#define PERF_MAX_READER_COUNT 8

typedef struct PERF_READERS_RUN_TAG
{
    const PERF_QUEUE_INTERFACE* queue_interface;
    void* queue;
    volatile_atomic int32_t stop;
    volatile_atomic int32_t failures;
} PERF_READERS_RUN;

typedef struct PERF_READER_CONTEXT_TAG
{
    PERF_READERS_RUN* run;
    uint64_t first_index;
    uint64_t operation_count;
} PERF_READER_CONTEXT;

static int reader_thread(void* arg)
{
    PERF_READER_CONTEXT* context = arg;
    PERF_READERS_RUN* run = context->run;
    uint64_t index = context->first_index;
    while (interlocked_add(&run->stop, 0) == 0)
    {
        uint64_t sequence_number;
        uint64_t element_index = index % PERF_READER_ELEMENT_COUNT;
        if (
            (!run->queue_interface->try_get(run->queue, element_index, &sequence_number)) ||
            (sequence_number != element_index)
            )
        {
            LogError("ACQUIRE_ITEM of element_index=%" PRIu64 " failed", element_index);
            (void)interlocked_increment(&run->failures);
            break;
        }
        context->operation_count++;
        index++;
    }
    return 0;
}

static void run_readers(const PERF_QUEUE_CONFIG* queue_config, uint32_t reader_count)
{
    ///arrange
    PERF_READERS_RUN run;
    run.queue_interface = queue_config->queue_interface;
    run.queue = run.queue_interface->create(PERF_MAX_SIZE, queue_config->page_size);
    ASSERT_IS_NOT_NULL(run.queue);
    (void)interlocked_exchange(&run.stop, 0);
    (void)interlocked_exchange(&run.failures, 0);
    PERF_ITEM item;
    (void)memset(&item, 0, sizeof(item));
    for (uint64_t index = 0; index < PERF_READER_ELEMENT_COUNT; index++)
    {
        item.sequence_number = index;
        ASSERT_IS_TRUE(run.queue_interface->push(run.queue, index, &item));
    }
    PERF_READER_CONTEXT readers[PERF_MAX_READER_COUNT];
    THREAD_HANDLE reader_threads[PERF_MAX_READER_COUNT];
    ASSERT_IS_TRUE(reader_count <= PERF_MAX_READER_COUNT);

    ///act
    double start_time_us = timer_global_get_elapsed_us();
    for (uint32_t i = 0; i < reader_count; i++)
    {
        readers[i].run = &run;
        readers[i].first_index = (uint64_t)i * 7;
        readers[i].operation_count = 0;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&reader_threads[i], reader_thread, &readers[i]));
    }
    ThreadAPI_Sleep(PERF_READERS_DURATION_MS);
    (void)interlocked_exchange(&run.stop, 1);
    uint64_t operation_count = 0;
    for (uint32_t i = 0; i < reader_count; i++)
    {
        int thread_result;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(reader_threads[i], &thread_result));
        operation_count += readers[i].operation_count;
    }
    double duration_us = timer_global_get_elapsed_us() - start_time_us;

    ///assert
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&run.failures, 0));
    char page_size_string[32];
    if (queue_config->page_size == 0)
    {
        (void)snprintf(page_size_string, sizeof(page_size_string), "no pages");
    }
    else
    {
        (void)snprintf(page_size_string, sizeof(page_size_string), "page_size=%" PRIu64 "", queue_config->page_size);
    }
    LogInfo("[readers on the same page] %s, %s, %" PRIu32 " readers: %.2f Mops/s total, %.1f ns per ACQUIRE_ITEM per reader",
        run.queue_interface->name, page_size_string, reader_count, (double)operation_count / duration_us, (duration_us * 1000.0 * reader_count) / (double)operation_count);

    ///cleanup
    for (uint64_t index = 0; index < PERF_READER_ELEMENT_COUNT; index++)
    {
        run.queue_interface->release(run.queue, index);
    }
    run.queue_interface->destroy(run.queue);
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
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

/*one producer runs as fast as possible, so the queue is full most of the time and the throughput is the throughput of the consumer*/
TEST_FUNCTION(perf_producers_faster_than_the_consumer_releasing_each_element)
{
    PERF_SCENARIO scenario = { "saturated, consumer releases each element", { 0, 1 }, 1, 0 };
    run_scenario_for_all_queues(&scenario);
}

TEST_FUNCTION(perf_producers_faster_than_the_consumer_releasing_batches)
{
    PERF_SCENARIO scenario = { "saturated, consumer releases batches of 64", { 0, 1 }, 64, 0 };
    run_scenario_for_all_queues(&scenario);
}

/*the producers run at 2 and 0.5 million operations per second, the consumer keeps up so few elements are in flight and pages are allocated and freed as the queue moves*/
TEST_FUNCTION(perf_consumer_keeps_up_releasing_each_element)
{
    PERF_SCENARIO scenario = { "consumer keeps up, releases each element", { 0.5, 2 }, 1, 0 };
    run_scenario_for_all_queues(&scenario);
}

TEST_FUNCTION(perf_consumer_keeps_up_releasing_batches)
{
    PERF_SCENARIO scenario = { "consumer keeps up, releases batches of 64", { 0.5, 2 }, 64, 0 };
    run_scenario_for_all_queues(&scenario);
}

/*the consumer releases 1 million elements per second, slower than the producers, so the queue fills up and the producers wait for it*/
TEST_FUNCTION(perf_consumer_slower_than_the_producers)
{
    PERF_SCENARIO scenario = { "consumer slower than producers", { 0.5, 2 }, 1, 1 };
    run_scenario_for_all_queues(&scenario);
}

/*the producers run at 200 and 50 thousand operations per second, so the queue is empty most of the time*/
TEST_FUNCTION(perf_light_load)
{
    PERF_SCENARIO scenario = { "light load", { 5, 20 }, 1, 0 };
    run_scenario_for_all_queues(&scenario);
}

/*1 reader measures the cost of an ACQUIRE_ITEM, PERF_MAX_READER_COUNT readers measure it when they all use the same page*/
TEST_FUNCTION(perf_readers_on_the_same_page)
{
    for (size_t i = 0; i < MU_COUNT_ARRAY_ITEMS(queue_configs); i++)
    {
        run_readers(&queue_configs[i], 1);
        run_readers(&queue_configs[i], PERF_MAX_READER_COUNT);
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
