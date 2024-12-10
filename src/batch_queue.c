// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"
#include "c_pal/log_critical_and_terminate.h"
#include "c_pal/malloc_multi_flex.h"
#include "c_pal/sm.h"
#include "c_pal/thandle.h"
#include "c_pal/threadpool.h"
#include "c_pal/timer.h"

#include "c_util/tarray.h"

#include "clds/mpsc_lock_free_queue.h"

#include "tp_worker_thread.h"

#include "batch_queue.h"
#include "batch_queue_tarray_types.h"

MU_DEFINE_ENUM_STRINGS(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS(BATCH_QUEUE_PROCESS_SYNC_RESULT, BATCH_QUEUE_PROCESS_SYNC_RESULT_VALUES);

typedef struct BATCH_ITEM_CONTEXT_TAG
{
    MPSC_LOCK_FREE_QUEUE_ITEM queue_item;
    uint32_t item_size;
    void* item_data;
    BATCH_QUEUE_ON_ITEM_COMPLETE callback;
    void* callback_context;
    double start_time;
} BATCH_ITEM_CONTEXT;

typedef struct BATCH_ITEM_CONTEXT_TAG* BATCH_ITEM_CONTEXT_HANDLE;

// We need to have 2 arrays in the batch
// First is an array of BATCH_ITEM_CONTEXT*, used internally by this module
// Second is an array of void*, which we pass down to the batch processor to handle (the same pointers given originally)
// Use malloc_multi_flex to allocate both arrays together

#define BATCH_CONTEXT_FIELDS \
    BATCH_QUEUE_HANDLE, batch_queue, \
    int64_t, batch_id, \
    uint32_t, item_count
#define BATCH_CONTEXT_ARRAY_FIELDS \
    BATCH_ITEM_CONTEXT*, items, \
    void*, item_datas
DECLARE_MALLOC_MULTI_FLEX_STRUCT(BATCH_CONTEXT, FIELDS(BATCH_CONTEXT_FIELDS), ARRAY_FIELDS(BATCH_CONTEXT_ARRAY_FIELDS));
DEFINE_MALLOC_MULTI_FLEX_STRUCT(BATCH_CONTEXT, FIELDS(BATCH_CONTEXT_FIELDS), ARRAY_FIELDS(BATCH_CONTEXT_ARRAY_FIELDS));

typedef struct BATCH_QUEUE_TAG
{
    SM_HANDLE sm;
    MPSC_LOCK_FREE_QUEUE_HANDLE queue;
    TP_WORKER_THREAD_HANDLE worker_thread;
    EXECUTION_ENGINE_HANDLE execution_engine;
    THANDLE(THREADPOOL) threadpool;
    TIMER_INSTANCE_HANDLE batching_timer;

    BATCH_QUEUE_SETTINGS settings;

    BATCH_QUEUE_PROCESS_BATCH process_batch_func;
    void* process_batch_context;
    BATCH_QUEUE_ON_BATCH_FAULTED on_batch_faulted_func;
    void* on_batch_faulted_context;

    volatile_atomic int64_t next_batch_id;
    volatile_atomic int32_t pending_batches;

    uint32_t batch_size;
    uint32_t batch_staging_count;
    TARRAY(BATCH_ITEM_CONTEXT_HANDLE) batch_staging_array;
} BATCH_QUEUE;


static void batch_queue_on_ll_batch_complete(void* context, BATCH_QUEUE_PROCESS_COMPLETE_RESULT result, void* ll_result)
{
    if (context == NULL)
    {
        /*Codes_SRS_BATCH_QUEUE_42_088: [ If context is NULL then batch_queue_on_ll_batch_complete shall terminate the process. ]*/
        LogCriticalAndTerminate("Invalid args: void* context = %p", context);
    }
    else
    {
        BATCH_CONTEXT* batch_context = context;

        for (uint32_t i = 0; i < batch_context->item_count; i++)
        {
            /*Codes_SRS_BATCH_QUEUE_42_089: [ For each item in the batch: ]*/
            BATCH_ITEM_CONTEXT* item = batch_context->items[i];

            /*Codes_SRS_BATCH_QUEUE_42_090: [ batch_queue_on_ll_batch_complete shall call on_item_complete with the result and ll_result for the item. ]*/
            item->callback(item->callback_context, result, ll_result);

            /*Codes_SRS_BATCH_QUEUE_42_091: [ batch_queue_on_ll_batch_complete shall free the memory allocated during enqueue of the item. ]*/
            free(item);
        }

        /*Codes_SRS_BATCH_QUEUE_42_092: [ batch_queue_on_ll_batch_complete shall decrement the number of pending batches. ]*/
        (void)interlocked_decrement(&batch_context->batch_queue->pending_batches);

        /*Codes_SRS_BATCH_QUEUE_42_093: [ batch_queue_on_ll_batch_complete shall call sm_exec_end. ]*/
        sm_exec_end(batch_context->batch_queue->sm);

        /*Codes_SRS_BATCH_QUEUE_42_094: [ batch_queue_on_ll_batch_complete shall call tp_worker_thread_schedule_process. ]*/
        TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT worker_thread_result = tp_worker_thread_schedule_process(batch_context->batch_queue->worker_thread);

        if (worker_thread_result != TP_WORKER_THREAD_SCHEDULE_PROCESS_OK)
        {
            LogError("Failed to schedule worker thread after completion of batch write with %" PRI_MU_ENUM ", likely queue is shutting down, do not panic!",
                MU_ENUM_VALUE(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, worker_thread_result));
        }

        /*Codes_SRS_BATCH_QUEUE_45_006: [ batch_queue_on_ll_batch_complete shall free the batch context. ]*/
        free(batch_context);
    }
}

static void batch_queue_timer_work(void* context)
{
    if (context == NULL)
    {
        /*Codes_SRS_BATCH_QUEUE_42_067: [ If context is NULL then batch_queue_timer_work shall terminate the process. ]*/
        LogCriticalAndTerminate("Invalid args: void* context = %p", context);
    }
    else
    {
        BATCH_QUEUE_HANDLE batch_queue = context;

        /*Codes_SRS_BATCH_QUEUE_42_068: [ batch_queue_timer_work shall call tp_worker_thread_schedule_process. ]*/
        TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT result = tp_worker_thread_schedule_process(batch_queue->worker_thread);

        if (result != TP_WORKER_THREAD_SCHEDULE_PROCESS_OK)
        {
            LogError("Failed to schedule worker thread in timer with %" PRI_MU_ENUM ", likely queue is shutting down, do not panic!",
                MU_ENUM_VALUE(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, result));
        }
    }
}

static int batch_queue_send_batch(BATCH_QUEUE_HANDLE batch_queue)
{
    int result;

    /*Codes_SRS_BATCH_QUEUE_42_072: [ batch_queue_send_batch shall increment the number of pending batches. ]*/
    (void)interlocked_increment(&batch_queue->pending_batches);

    /*Codes_SRS_BATCH_QUEUE_42_073: [ batch_queue_send_batch shall allocate a context for the batch and move the items from the batch staging array to the batch context. ]*/
    BATCH_CONTEXT* batch_context = MALLOC_MULTI_FLEX_STRUCT(BATCH_CONTEXT)(sizeof(BATCH_CONTEXT), batch_queue->batch_staging_count, batch_queue->batch_staging_count);
    if (batch_context == NULL)
    {
        /*Codes_SRS_BATCH_QUEUE_42_110: [ If there are any other errors then batch_queue_send_batch shall fail and return a non-zero value. ]*/
        LogError("malloc_flex of BATCH_CONTEXT failed");
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_BATCH_QUEUE_42_074: [ batch_queue_send_batch shall call sm_exec_begin. ]*/
        SM_RESULT sm_result = sm_exec_begin(batch_queue->sm);

        if (sm_result != SM_EXEC_GRANTED)
        {
            /*Codes_SRS_BATCH_QUEUE_42_110: [ If there are any other errors then batch_queue_send_batch shall fail and return a non-zero value. ]*/
            LogError("sm_exec_begin failed, coult not send batch %" PRI_MU_ENUM "",
                MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = MU_FAILURE;
        }
        else
        {
            batch_context->batch_queue = batch_queue;
            batch_context->batch_id = interlocked_increment_64(&batch_queue->next_batch_id) - 1;
            batch_context->item_count = batch_queue->batch_staging_count;
            for (uint32_t i = 0; i < batch_queue->batch_staging_count; i++)
            {
                BATCH_ITEM_CONTEXT* staging_item = batch_queue->batch_staging_array->arr[i];
                batch_context->items[i] = staging_item;
                batch_context->item_datas[i] = staging_item->item_data;
            }
            LogInfo("batch_queue %p sending batch %" PRIu64 " with %" PRIu32 " items, batch_size= %"PRIu32"", batch_queue, batch_context->batch_id, batch_context->item_count, batch_queue->batch_size);
            /*Codes_SRS_BATCH_QUEUE_42_075: [ batch_queue_send_batch shall call process_batch_func with the batch context. ]*/
            BATCH_QUEUE_PROCESS_SYNC_RESULT process_batch_result = batch_queue->process_batch_func(batch_queue->process_batch_context, (void**)batch_context->item_datas, batch_context->item_count, batch_queue_on_ll_batch_complete, batch_context);
            if (process_batch_result != BATCH_QUEUE_PROCESS_SYNC_OK)
            {
                /*Codes_SRS_BATCH_QUEUE_42_076: [ If process_batch_func returns anything other than BATCH_QUEUE_PROCESS_SYNC_OK then: ]*/
                LogError("process_batch_func failed with %" PRI_MU_ENUM " for batch %" PRIu64 "",
                    MU_ENUM_VALUE(BATCH_QUEUE_PROCESS_SYNC_RESULT, process_batch_result), batch_context->batch_id);

                BATCH_QUEUE_PROCESS_COMPLETE_RESULT callback_result;
                if (process_batch_result == BATCH_QUEUE_PROCESS_SYNC_NOT_OPEN)
                {
                    /*Codes_SRS_BATCH_QUEUE_42_077: [ If process_batch_func returns BATCH_QUEUE_PROCESS_SYNC_NOT_OPEN then batch_queue_send_batch shall call on_item_complete for each item in the batch with BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED and NULL for the ll_result. ]*/
                    callback_result = BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED;
                }
                else
                {
                    /*Codes_SRS_BATCH_QUEUE_42_079: [ If process_batch_func returns anything else then batch_queue_send_batch shall call on_item_complete for each item in the batch with BATCH_QUEUE_PROCESS_COMPLETE_ERROR and NULL for the ll_result. ]*/
                    callback_result = BATCH_QUEUE_PROCESS_COMPLETE_ERROR;
                }

                for (uint32_t i = 0; i < batch_context->item_count; i++)
                {
                    batch_context->items[i]->callback(batch_context->items[i]->callback_context, callback_result, NULL);
                    /*Codes_SRS_BATCH_QUEUE_45_007: [ batch_queue_send_batch shall free each item in the batch. ]*/
                    free(batch_context->items[i]);
                }

                /*Codes_SRS_BATCH_QUEUE_45_008: [ batch_queue_send_batch shall reset the batch staging array size to 0. ]*/
                batch_queue->batch_staging_count = 0;
                batch_queue->batch_size = 0;

                /*Codes_SRS_BATCH_QUEUE_45_009: [ batch_queue_send_batch shall call sm_fault. ]*/
                sm_fault(batch_queue->sm);

                /*Codes_SRS_BATCH_QUEUE_42_083: [ batch_queue_send_batch shall call on_batch_faulted_func with on_batch_faulted_context. ]*/
                batch_queue->on_batch_faulted_func(batch_queue->on_batch_faulted_context);

                /*Codes_SRS_BATCH_QUEUE_42_084: [ batch_queue_send_batch shall return a non-zero value. ]*/
                result = MU_FAILURE;
            }
            else
            {
                /*Codes_SRS_BATCH_QUEUE_42_085: [ Otherwise (process_batch_func returns BATCH_QUEUE_PROCESS_SYNC_OK): ]*/

                /*Codes_SRS_BATCH_QUEUE_42_086: [ batch_queue_send_batch shall reset the batch staging array size to 0. ]*/
                batch_queue->batch_staging_count = 0;
                batch_queue->batch_size = 0;

                /*Codes_SRS_BATCH_QUEUE_42_087: [ batch_queue_send_batch shall succeed and return 0. ]*/
                result = 0;
                goto all_ok;
            }

            /*Codes_SRS_BATCH_QUEUE_42_082: [ batch_queue_send_batch shall call sm_exec_end. ]*/
            sm_exec_end(batch_queue->sm);
        }
        free(batch_context);
    }

    /*Codes_SRS_BATCH_QUEUE_42_081: [ batch_queue_send_batch shall decrement the number of pending batches. ]*/
    (void)interlocked_decrement(&batch_queue->pending_batches);
all_ok:
    return result;
}

static void batch_queue_worker_thread(void* context)
{
    if (context == NULL)
    {
        /*Codes_SRS_BATCH_QUEUE_42_052: [ If context is NULL then batch_queue_worker_thread shall terminate the process. ]*/
        LogCriticalAndTerminate("Invalid arguments: void* context = %p", context);
    }
    else
    {
        BATCH_QUEUE_HANDLE batch_queue = context;
        /*Codes_SRS_BATCH_QUEUE_45_010: [ batch_queue_worker_thread shall call sm_exec_begin to check the timer. ]*/
        SM_RESULT sm_result = sm_exec_begin(batch_queue->sm);
        if (sm_result != SM_EXEC_GRANTED)
        {
            /*Codes_SRS_BATCH_QUEUE_42_109: [ If sm_exec_begin fails then batch_queue_worker_thread shall exit the loop. ]*/
            LogError("sm_exec_begin failed with %" PRI_MU_ENUM ", stop processing batches",
                MU_ENUM_VALUE(SM_RESULT, sm_result));
        }
        else
        {
            if (batch_queue->batching_timer != NULL)
            {
                /*Codes_SRS_BATCH_QUEUE_42_053: [ If a timer has been started then batch_queue_worker_thread shall call threadpool_timer_cancel. ]*/
                threadpool_timer_cancel(batch_queue->batching_timer);
            }
            /*Codes_SRS_BATCH_QUEUE_45_011: [ batch_queue_worker_thread shall call sm_exec_end when it is done checking the timer. ]*/
            sm_exec_end(batch_queue->sm);

            /*Codes_SRS_BATCH_QUEUE_42_054: [ batch_queue_worker_thread shall do the following until there are no more items in the queue: ]*/
            bool thread_should_continue = true;
            do
            {

                /*Codes_SRS_BATCH_QUEUE_42_105: [ batch_queue_worker_thread shall call sm_exec_begin. ]*/
                sm_result = sm_exec_begin(batch_queue->sm);
                if (sm_result != SM_EXEC_GRANTED)
                {
                    /*Codes_SRS_BATCH_QUEUE_42_109: [ If sm_exec_begin fails then batch_queue_worker_thread shall exit the loop. ]*/
                    LogError("sm_exec_begin failed with %" PRI_MU_ENUM ", stop processing batches",
                        MU_ENUM_VALUE(SM_RESULT, sm_result));
                    thread_should_continue = false;
                }
                else
                {
                    /*Codes_SRS_BATCH_QUEUE_45_012: [ If the pending requests are greater than or equal to max_pending_requests then batch_queue_worker_thread shall exit the loop. ]*/
                    if ((uint32_t)interlocked_add(&batch_queue->pending_batches, 0) < batch_queue->settings.max_pending_requests)
                    {
                        /*Codes_SRS_BATCH_QUEUE_42_055: [ batch_queue_worker_thread shall call mpsc_lock_free_queue_dequeue. ]*/
                        MPSC_LOCK_FREE_QUEUE_ITEM* queue_item = mpsc_lock_free_queue_dequeue(batch_queue->queue);

                        if (queue_item != NULL)
                        {
                            BATCH_ITEM_CONTEXT* batch_item_context = CONTAINING_RECORD(queue_item, BATCH_ITEM_CONTEXT, queue_item);

                            if (
                                batch_queue->batch_staging_count > 0 &&
                                (
                                    batch_queue->batch_staging_count == UINT32_MAX || // overflow check the count
                                    UINT32_MAX - batch_queue->batch_size < batch_item_context->item_size || // overflow check the size
                                    batch_queue->batch_size + batch_item_context->item_size  > batch_queue->settings.max_batch_size
                                )
                               )
                            {
                                /*Codes_SRS_BATCH_QUEUE_42_056: [ If there are items in the batch staging array and the size of the dequeued item would cause the staging batch to exceed max_batch_size then: ]*/
                                LogInfo("Batch staging array has %" PRIu32 " items with a size of %" PRIu32 ", next item size is %" PRIu32 ", max batch size is %" PRIu32 ", sending batch",
                                    batch_queue->batch_staging_count, batch_queue->batch_size, batch_item_context->item_size, batch_queue->settings.max_batch_size);
                                /*Codes_SRS_BATCH_QUEUE_42_057: [ batch_queue_worker_thread shall call batch_queue_send_batch. ]*/
                                if (batch_queue_send_batch(batch_queue) != 0)
                                {
                                    /*Codes_SRS_BATCH_QUEUE_42_058: [ If batch_queue_send_batch fails then batch_queue_worker_thread shall exit the loop. ]*/
                                    thread_should_continue = false;
                                }
                            }

                            /*Codes_SRS_BATCH_QUEUE_42_059: [ batch_queue_worker_thread shall call TARRAY_ENSURE_CAPACITY on the batch staging array to ensure it can hold the new item. ]*/
                            if (TARRAY_ENSURE_CAPACITY(BATCH_ITEM_CONTEXT_HANDLE)(batch_queue->batch_staging_array, batch_queue->batch_staging_count + 1) != 0)
                            {
                                /*Codes_SRS_BATCH_QUEUE_45_001: [ If TARRAY_ENSURE_CAPACITY fails then, ] */
                                LogError("TARRAY_ENSURE_CAPACITY(BATCH_ITEM_CONTEXT_HANDLE)(%p, %" PRIu32 ") failed",
                                    batch_queue->batch_staging_array, batch_queue->batch_staging_count + 1);
                                /*Codes_SRS_BATCH_QUEUE_45_002: [ batch_queue_timer_work shall call sm_fault. ] */
                                sm_fault(batch_queue->sm);
                                /*Codes_SRS_BATCH_QUEUE_45_003: [ batch_queue_timer_work shall call on_batch_faulted_func with on_batch_faulted_context. ] */
                                batch_queue->on_batch_faulted_func(batch_queue->on_batch_faulted_context);
                                /*Codes_SRS_BATCH_QUEUE_45_005: [ batch_queue_worker_thread shall free the batch_item_context. ]*/
                                free(batch_item_context);
                                /*Codes_SRS_BATCH_QUEUE_45_004: [ batch_queue_worker_thread shall exit the loop. ]*/
                                thread_should_continue = false;
                            }
                            else
                            {
                                /*Codes_SRS_BATCH_QUEUE_42_060: [ batch_queue_worker_thread shall add the dequeued item to the batch staging array, increment its count, and add the size of the item to the staging batch size. ]*/
                                batch_queue->batch_staging_array->arr[batch_queue->batch_staging_count] = batch_item_context;
                                batch_queue->batch_staging_count++;
                                batch_queue->batch_size += batch_item_context->item_size;
                            }
                        }
                        else
                        {
                            if (batch_queue->batch_staging_count > 0)
                            {
                                double time_now = timer_global_get_elapsed_ms();
                                double elapsed_time = time_now - batch_queue->batch_staging_array->arr[0]->start_time;
                                /*Codes_SRS_BATCH_QUEUE_42_061: [ If the batch staging array is not empty and the number of pending batches is less than max_pending_requests then: ]*/
                                if (
                                    /*Codes_SRS_BATCH_QUEUE_42_062: [ If the time since the first item in the batch staging array was enqueued is greater than or equal to min_wait_time then batch_queue_worker_thread shall call batch_queue_send_batch. ]*/
                                    elapsed_time >= batch_queue->settings.min_wait_time ||
                                    /*Codes_SRS_BATCH_QUEUE_42_063: [ Otherwise if the size of the batch staging array is greater than or equal to min_batch_size then batch_queue_worker_thread shall call batch_queue_send_batch. ]*/
                                    batch_queue->batch_size >= batch_queue->settings.min_batch_size
                                    )
                                {
                                    LogInfo("Oldest batch item elapsed time is %lf ms, batch staging array has %" PRIu32 " items with a size of %" PRIu32 ", min wait time is %" PRIu32 " ms, min batch size is %" PRIu32 ", sending batch",
                                        elapsed_time, batch_queue->batch_staging_count, batch_queue->batch_size, batch_queue->settings.min_wait_time, batch_queue->settings.min_batch_size);
                                    if (batch_queue_send_batch(batch_queue) != 0)
                                    {
                                        /*Codes_SRS_BATCH_QUEUE_42_058: [ If batch_queue_send_batch fails then batch_queue_worker_thread shall exit the loop. ]*/
                                        thread_should_continue = false;
                                    }
                                }
                                else
                                {
                                    // Wait for the oldest item to reach min_wait_time
                                    /*Codes_SRS_BATCH_QUEUE_42_064: [ Otherwise batch_queue_worker_thread shall call threadpool_timer_restart with the difference between min_wait_time and the elapsed time of the first operation as the start_delay_ms and 0 as the timer_period_ms. ]*/

                                    uint32_t delay_time = batch_queue->settings.min_wait_time - (uint32_t)(elapsed_time + 1);
                                    // Cannot fail unless the timer is NULL
                                    (void)threadpool_timer_restart(batch_queue->batching_timer, delay_time, 0);
                                }
                            }
                        }
                    }
                    else
                    {
                        thread_should_continue = false; // wait until something completes, and then we can start again.
                        LogInfo("Max pending requests reached, not processing more items");
                    }

                    /*Codes_SRS_BATCH_QUEUE_42_106: [ batch_queue_worker_thread shall call sm_exec_end. ]*/
                    sm_exec_end(batch_queue->sm);
                }
            } while (thread_should_continue);
        }
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, BATCH_QUEUE_HANDLE, batch_queue_create, BATCH_QUEUE_SETTINGS, settings, EXECUTION_ENGINE_HANDLE, execution_engine, BATCH_QUEUE_PROCESS_BATCH, process_batch_func, void*, process_batch_context, BATCH_QUEUE_ON_BATCH_FAULTED, on_batch_faulted_func, void*, on_batch_faulted_context)
{
    BATCH_QUEUE_HANDLE result;
    if (
        /*Codes_SRS_BATCH_QUEUE_42_001: [ If execution_engine is NULL then batch_queue_create shall fail and return NULL. ]*/
        execution_engine == NULL ||
        /*Codes_SRS_BATCH_QUEUE_42_002: [ If process_batch_func is NULL then batch_queue_create shall fail and return NULL. ]*/
        process_batch_func == NULL ||
        /*Codes_SRS_BATCH_QUEUE_42_003: [ If on_batch_faulted_func is NULL then batch_queue_create shall fail and return NULL. ]*/
        on_batch_faulted_func == NULL ||
        /*Codes_SRS_BATCH_QUEUE_42_004: [ If settings.max_pending_requests is 0 then batch_queue_create shall fail and return NULL. ]*/
        settings.max_pending_requests == 0
        )
    {
        LogError("Invalid arguments: BATCH_QUEUE_SETTINGS settings = %" PRI_BATCH_QUEUE_SETTINGS ", EXECUTION_ENGINE_HANDLE execution_engine = %p, BATCH_QUEUE_PROCESS_BATCH process_batch_func = %p, void* process_batch_context = %p, BATCH_QUEUE_ON_BATCH_FAULTED on_batch_faulted_func = %p, void* on_batch_faulted_context = %p",
            BATCH_QUEUE_SETTINGS_VALUES(settings), execution_engine, process_batch_func, process_batch_context, on_batch_faulted_func, on_batch_faulted_context);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_BATCH_QUEUE_42_005: [ batch_queue_create shall allocate memory for the batch queue. ]*/
        BATCH_QUEUE_HANDLE temp = malloc(sizeof(BATCH_QUEUE));
        if (temp == NULL)
        {
            /*Codes_SRS_BATCH_QUEUE_42_010: [ If there are any other errors then batch_queue_create shall fail and return NULL. ]*/
            LogError("malloc of BATCH_QUEUE failed");
            result = NULL;
        }
        else
        {
            /*Codes_SRS_BATCH_QUEUE_42_006: [ batch_queue_create shall call sm_create. ]*/
            temp->sm = sm_create("batch_queue");
            if (temp->sm == NULL)
            {
                /*Codes_SRS_BATCH_QUEUE_42_010: [ If there are any other errors then batch_queue_create shall fail and return NULL. ]*/
                LogError("sm_create failed");
                result = NULL;
            }
            else
            {
                /*Codes_SRS_BATCH_QUEUE_42_007: [ batch_queue_create shall create a multi producer single consumer queue by calling mpsc_lock_free_queue_create. ]*/
                temp->queue = mpsc_lock_free_queue_create();
                if (temp->queue == NULL)
                {
                    /*Codes_SRS_BATCH_QUEUE_42_010: [ If there are any other errors then batch_queue_create shall fail and return NULL. ]*/
                    LogError("mpsc_lock_free_queue_create failed");
                    result = NULL;
                }
                else
                {
                    /*Codes_SRS_BATCH_QUEUE_42_008: [ batch_queue_create shall create a worker thread by calling tp_worker_thread_create with batch_queue_worker_thread. ]*/
                    temp->worker_thread = tp_worker_thread_create(execution_engine, batch_queue_worker_thread, temp);
                    if (temp->worker_thread == NULL)
                    {
                        /*Codes_SRS_BATCH_QUEUE_42_010: [ If there are any other errors then batch_queue_create shall fail and return NULL. ]*/
                        LogError("tp_worker_thread_create failed");
                        result = NULL;
                    }
                    else
                    {
                        TARRAY_INITIALIZE(BATCH_ITEM_CONTEXT_HANDLE)(&temp->batch_staging_array, NULL);
                        temp->batching_timer = NULL;
                        temp->settings = settings;
                        temp->process_batch_func = process_batch_func;
                        temp->process_batch_context = process_batch_context;
                        temp->on_batch_faulted_func = on_batch_faulted_func;
                        temp->on_batch_faulted_context = on_batch_faulted_context;
                        (void)interlocked_exchange_64(&temp->next_batch_id, 0);
                        (void)interlocked_exchange(&temp->pending_batches, 0);
                        /*Codes_SRS_BATCH_QUEUE_42_020: [ batch_queue_create shall initialize the batch staging array item count and batch size to 0. ]*/
                        temp->batch_staging_count = 0;
                        temp->batch_size = 0;
                        THANDLE_INITIALIZE(THREADPOOL)(&temp->threadpool, NULL);
                        /*Codes_SRS_BATCH_QUEUE_45_013: [ batch_queue_create shall keep the execution engine, and call execution_engine_inc_ref. ]*/
                        execution_engine_inc_ref(execution_engine);
                        temp->execution_engine = execution_engine;

                        /*Codes_SRS_BATCH_QUEUE_42_009: [ batch_queue_create shall return the batch queue handle. ]*/
                        result = temp;
                        goto all_ok;
                        //tp_worker_thread_destroy(temp->worker_thread);
                    }
                    mpsc_lock_free_queue_destroy(temp->queue);
                }
                sm_destroy(temp->sm);
            }
            free(temp);
        }
    }

all_ok:
    return result;
}

static void batch_queue_close_internal(BATCH_QUEUE_HANDLE batch_queue)
{
    /*Codes_SRS_BATCH_QUEUE_42_028: [ batch_queue_close shall call sm_close_begin. ]*/
    if (sm_close_begin(batch_queue->sm) == SM_EXEC_GRANTED)
    {
        /*Codes_SRS_BATCH_QUEUE_42_029: [ batch_queue_close shall call threadpool_timer_destroy. ]*/
        threadpool_timer_destroy(batch_queue->batching_timer);
        batch_queue->batching_timer = NULL;

        /*Codes_SRS_BATCH_QUEUE_42_030: [ batch_queue_close shall call tp_worker_thread_close. ]*/
        tp_worker_thread_close(batch_queue->worker_thread);

        /*Codes_SRS_BATCH_QUEUE_45_016: [ batch_queue_close shall call THANDLE_ASSIGN(THREADPOOL) with NULL. ]*/
        THANDLE_ASSIGN(THREADPOOL)(&batch_queue->threadpool, NULL);

        /*Codes_SRS_BATCH_QUEUE_42_102: [ For each request in the batch staging array: ]*/
        for (uint32_t i = 0; i < batch_queue->batch_staging_count; i++)
        {
            BATCH_ITEM_CONTEXT* batch_item_context = batch_queue->batch_staging_array->arr[i];

            /*Codes_SRS_BATCH_QUEUE_42_103: [ batch_queue_close shall call on_item_complete with BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED and NULL for the ll_result. ]*/
            batch_item_context->callback(batch_item_context->callback_context, BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED, NULL);

            /*Codes_SRS_BATCH_QUEUE_42_104: [ batch_queue_close shall free the memory associated with the request. ]*/
            free(batch_item_context);
        }

        /*Codes_SRS_BATCH_QUEUE_42_031: [ For each request in the queue: ]*/
        MPSC_LOCK_FREE_QUEUE_ITEM* queue_item;
        do
        {
            queue_item = mpsc_lock_free_queue_dequeue(batch_queue->queue);
            if (queue_item != NULL)
            {
                BATCH_ITEM_CONTEXT* batch_item_context = CONTAINING_RECORD(queue_item, BATCH_ITEM_CONTEXT, queue_item);

                /*Codes_SRS_BATCH_QUEUE_42_032: [ batch_queue_close shall call on_item_complete with BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED and NULL for the ll_result. ]*/
                batch_item_context->callback(batch_item_context->callback_context, BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED, NULL);

                /*Codes_SRS_BATCH_QUEUE_42_033: [ batch_queue_close shall free the memory associated with the request. ]*/
                free(batch_item_context);
            }
        } while (queue_item != NULL);

        /*Codes_SRS_BATCH_QUEUE_42_034: [ batch_queue_close shall call TARRAY_ASSIGN with NULL to free the batch staging array. ]*/
        TARRAY_ASSIGN(BATCH_ITEM_CONTEXT_HANDLE)(&batch_queue->batch_staging_array, NULL);
        batch_queue->batch_staging_count = 0;
        batch_queue->batch_size = 0;

        /*Codes_SRS_BATCH_QUEUE_42_035: [ batch_queue_close shall call sm_close_end. ]*/
        sm_close_end(batch_queue->sm);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, batch_queue_destroy, BATCH_QUEUE_HANDLE, batch_queue)
{
    if (batch_queue == NULL)
    {
        /*Codes_SRS_BATCH_QUEUE_42_011: [ If batch_queue is NULL then batch_queue_destroy shall return. ]*/
        LogError("Invalid arguments: BATCH_QUEUE_HANDLE batch_queue = %p", batch_queue);
    }
    else
    {
        /*Codes_SRS_BATCH_QUEUE_42_012: [ batch_queue_destroy shall behave as if batch_queue_close was called. ]*/
        batch_queue_close_internal(batch_queue);

        /*Codes_SRS_BATCH_QUEUE_42_013: [ batch_queue_destroy shall call tp_worker_thread_destroy. ]*/
        tp_worker_thread_destroy(batch_queue->worker_thread);

        /*Codes_SRS_BATCH_QUEUE_42_014: [ batch_queue_destroy shall call mpsc_lock_free_queue_destroy. ]*/
        mpsc_lock_free_queue_destroy(batch_queue->queue);

        /*Codes_SRS_BATCH_QUEUE_45_014: [ batch_queue_destroy shall call execution_engine_dec_ref on the execution engine. ]*/
        execution_engine_dec_ref(batch_queue->execution_engine);

        /*Codes_SRS_BATCH_QUEUE_42_015: [ batch_queue_destroy shall call sm_destroy. ]*/
        sm_destroy(batch_queue->sm);

        /*Codes_SRS_BATCH_QUEUE_42_016: [ batch_queue_destroy shall free the memory allocated for the batch queue. ]*/
        free(batch_queue);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, batch_queue_open, BATCH_QUEUE_HANDLE, batch_queue)
{
    int result;
    if (batch_queue == NULL)
    {
        /*Codes_SRS_BATCH_QUEUE_42_017: [ If batch_queue is NULL then batch_queue_open shall fail and return a non-zero value. ]*/
        LogError("Invalid arguments: BATCH_QUEUE_HANDLE batch_queue = %p", batch_queue);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_BATCH_QUEUE_42_018: [ batch_queue_open shall call sm_open_begin. ]*/
        SM_RESULT sm_result = sm_open_begin(batch_queue->sm);

        if (sm_result != SM_EXEC_GRANTED)
        {
            /*Codes_SRS_BATCH_QUEUE_42_026: [ If there are any errors then batch_queue_open shall fail and return a non-zero value. ]*/
            LogError("sm_open_begin failed with %" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_BATCH_QUEUE_42_019: [ batch_queue_open shall call TARRAY_CREATE to create a batch staging array. ]*/
            TARRAY_MOVE(BATCH_ITEM_CONTEXT_HANDLE)(&batch_queue->batch_staging_array, &(TARRAY(BATCH_ITEM_CONTEXT_HANDLE)){TARRAY_CREATE(BATCH_ITEM_CONTEXT_HANDLE)()});
            if (batch_queue->batch_staging_array == NULL)
            {
                /*Codes_SRS_BATCH_QUEUE_42_026: [ If there are any errors then batch_queue_open shall fail and return a non-zero value. ]*/
                LogError("TARRAY_CREATE(BATCH_ITEM_CONTEXT_HANDLE) failed");
                result = MU_FAILURE;
            }
            else
            {
                /*Codes_SRS_BATCH_QUEUE_45_015: [ batch_queue_open shall call threadpool_create. ]*/
                THANDLE_INITIALIZE_MOVE(THREADPOOL)(&batch_queue->threadpool, &(THANDLE(THREADPOOL)){threadpool_create(batch_queue->execution_engine)});
                if (batch_queue->threadpool == NULL)
                {
                    /*Codes_SRS_BATCH_QUEUE_42_026: [ If there are any errors then batch_queue_open shall fail and return a non-zero value. ]*/
                    LogError("threadpool_create failed");
                    result = MU_FAILURE;
                }
                else
                {
                    /*Codes_SRS_BATCH_QUEUE_42_022: [ batch_queue_open shall call tp_worker_thread_open. ]*/
                    if (tp_worker_thread_open(batch_queue->worker_thread) != 0)
                    {
                        /*Codes_SRS_BATCH_QUEUE_42_026: [ If there are any errors then batch_queue_open shall fail and return a non-zero value. ]*/
                        LogError("tp_worker_thread_open failed");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        /*Codes_SRS_BATCH_QUEUE_42_107: [ batch_queue_open shall call threadpool_timer_start with UINT32_MAX as the start_delay_ms, 0 as the timer_period_ms, and batch_queue_timer_work as the work_function. ]*/
                        if (threadpool_timer_start(batch_queue->threadpool, UINT32_MAX, 0, batch_queue_timer_work, batch_queue, &batch_queue->batching_timer) != 0)
                        {
                            /*Codes_SRS_BATCH_QUEUE_42_026: [ If there are any errors then batch_queue_open shall fail and return a non-zero value. ]*/
                            LogError("threadpool_timer_start failed");
                            result = MU_FAILURE;
                        }
                        else
                        {
                            // NOTE: Ideally we should have a way to create a timer without the SetThreadpoolTimer part, but until then, we just cancel it immediately
                            /*Codes_SRS_BATCH_QUEUE_42_108: [ batch_queue_open shall call threadpool_timer_cancel. ]*/
                            threadpool_timer_cancel(batch_queue->batching_timer);

                            /*Codes_SRS_BATCH_QUEUE_42_025: [ batch_queue_open shall succeed and return 0. ]*/
                            result = 0;
                            /*Codes_SRS_BATCH_QUEUE_42_024: [ batch_queue_open shall call sm_open_end with true. ]*/
                            sm_open_end(batch_queue->sm, true);

                            goto all_ok;
                        }
                        tp_worker_thread_close(batch_queue->worker_thread);
                    }
                    THANDLE_ASSIGN(THREADPOOL)(&batch_queue->threadpool, NULL);
                }
                TARRAY_ASSIGN(BATCH_ITEM_CONTEXT_HANDLE)(&batch_queue->batch_staging_array, NULL);
            }

            /*Codes_SRS_BATCH_QUEUE_42_021: [ If TARRAY_CREATE fails then batch_queue_open shall call sm_open_end with false. ]*/
            /*Codes_SRS_BATCH_QUEUE_42_023: [ If anything fails after sm_open_begin then batch_queue_open shall call sm_open_end with false. ]*/
            sm_open_end(batch_queue->sm, false);
        }
    }
all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, batch_queue_close, BATCH_QUEUE_HANDLE, batch_queue)
{
    if (batch_queue == NULL)
    {
        /*Codes_SRS_BATCH_QUEUE_42_027: [ If batch_queue is NULL then batch_queue_close shall return. ]*/
        LogError("Invalid arguments: BATCH_QUEUE_HANDLE batch_queue = %p", batch_queue);
    }
    else
    {
        batch_queue_close_internal(batch_queue);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, BATCH_QUEUE_ENQUEUE_RESULT, batch_queue_enqueue, BATCH_QUEUE_HANDLE, batch_queue, void*, item, uint32_t, item_size, BATCH_QUEUE_ON_ITEM_COMPLETE, on_item_complete, void*, context)
{
    BATCH_QUEUE_ENQUEUE_RESULT result;
    if (
        /*Codes_SRS_BATCH_QUEUE_42_036: [ If batch_queue is NULL then batch_queue_enqueue shall fail and return BATCH_QUEUE_ENQUEUE_INVALID_ARGS. ]*/
        batch_queue == NULL ||
        /*Codes_SRS_BATCH_QUEUE_42_037: [ If item is NULL then batch_queue_enqueue shall fail and return BATCH_QUEUE_ENQUEUE_INVALID_ARGS. ]*/
        item == NULL ||
        /*Codes_SRS_BATCH_QUEUE_42_038: [ If item_size is 0 then batch_queue_enqueue shall fail and return BATCH_QUEUE_ENQUEUE_INVALID_ARGS. ]*/
        item_size == 0 ||
        /*Codes_SRS_BATCH_QUEUE_42_039: [ If on_item_complete is NULL then batch_queue_enqueue shall fail and return BATCH_QUEUE_ENQUEUE_INVALID_ARGS. ]*/
        on_item_complete == NULL
        )
    {
        LogError("Invalid args: BATCH_QUEUE_HANDLE batch_queue = %p, void* item = %p, uint32_t item_size = %" PRIu32 ", BATCH_QUEUE_ON_ITEM_COMPLETE on_item_complete = %p, void* context = %p",
            batch_queue, item, item_size, on_item_complete, context);
        result = BATCH_QUEUE_ENQUEUE_INVALID_ARGS;
    }
    else
    {
        /*Codes_SRS_BATCH_QUEUE_42_040: [ batch_queue_enqueue shall call sm_exec_begin. ]*/
        SM_RESULT sm_result = sm_exec_begin(batch_queue->sm);
        if (sm_result != SM_EXEC_GRANTED)
        {
            /*Codes_SRS_BATCH_QUEUE_42_041: [ If sm_exec_begin fails then batch_queue_enqueue shall return BATCH_QUEUE_ENQUEUE_INVALID_STATE. ]*/
            LogError("sm_exec_begin failed with %" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = BATCH_QUEUE_ENQUEUE_INVALID_STATE;
        }
        else
        {
            /*Codes_SRS_BATCH_QUEUE_42_042: [ batch_queue_enqueue shall get the current time to store with the request by calling timer_global_get_elapsed_ms. ]*/
            double start_time = timer_global_get_elapsed_ms();

            /*Codes_SRS_BATCH_QUEUE_42_043: [ batch_queue_enqueue shall allocate memory for the request context. ]*/
            BATCH_ITEM_CONTEXT* batch_item_context = malloc(sizeof(BATCH_ITEM_CONTEXT));
            if (batch_item_context == NULL)
            {
                /*Codes_SRS_BATCH_QUEUE_42_051: [ If any other error occurs then batch_queue_enqueue shall fail and return BATCH_QUEUE_ENQUEUE_ERROR. ]*/
                LogError("malloc of BATCH_ITEM_CONTEXT failed");
                result = BATCH_QUEUE_ENQUEUE_ERROR;
            }
            else
            {
                batch_item_context->item_size = item_size;
                batch_item_context->item_data = item;
                batch_item_context->callback = on_item_complete;
                batch_item_context->callback_context = context;
                batch_item_context->start_time = start_time;

                /*Codes_SRS_BATCH_QUEUE_42_044: [ batch_queue_enqueue shall call mpsc_lock_free_queue_enqueue. ]*/
                if (mpsc_lock_free_queue_enqueue(batch_queue->queue, &batch_item_context->queue_item) != 0)
                {
                    /*Codes_SRS_BATCH_QUEUE_42_051: [ If any other error occurs then batch_queue_enqueue shall fail and return BATCH_QUEUE_ENQUEUE_ERROR. ]*/
                    LogError("mpsc_lock_free_queue_enqueue failed");
                    free(batch_item_context);
                    result = BATCH_QUEUE_ENQUEUE_ERROR;
                }
                else
                {
                    /*Codes_SRS_BATCH_QUEUE_42_045: [ batch_queue_enqueue shall call tp_worker_thread_schedule_process. ]*/
                    TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT schedule_result = tp_worker_thread_schedule_process(batch_queue->worker_thread);
                    if (schedule_result != TP_WORKER_THREAD_SCHEDULE_PROCESS_OK)
                    {
                        LogError("tp_worker_thread_schedule_process failed with %" PRI_MU_ENUM ", this is not expected",
                            MU_ENUM_VALUE(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, schedule_result));
                    }

                    /*Codes_SRS_BATCH_QUEUE_42_050: [ batch_queue_enqueue shall succeed and return BATCH_QUEUE_ENQUEUE_OK. ]*/
                    result = BATCH_QUEUE_ENQUEUE_OK;
                }
            }
            /*Codes_SRS_BATCH_QUEUE_42_049: [ batch_queue_enqueue shall call sm_exec_end. ]*/
            sm_exec_end(batch_queue->sm);
        }
    }
    return result;
}

