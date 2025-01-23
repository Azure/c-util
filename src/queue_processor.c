// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/log_critical_and_terminate.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/sync.h"

#include "clds/mpsc_lock_free_queue.h"

#include "c_util/queue_processor.h"

#define QUEUE_PROCESSOR_STATE_VALUES \
    QUEUE_PROCESSOR_STATE_CLOSED, \
    QUEUE_PROCESSOR_STATE_OPENING, \
    QUEUE_PROCESSOR_STATE_OPEN, \
    QUEUE_PROCESSOR_STATE_CLOSING, \
    QUEUE_PROCESSOR_STATE_FAULTED


MU_DEFINE_ENUM(QUEUE_PROCESSOR_STATE, QUEUE_PROCESSOR_STATE_VALUES)
MU_DEFINE_ENUM_STRINGS(QUEUE_PROCESSOR_STATE, QUEUE_PROCESSOR_STATE_VALUES);

#define PROCESSING_STATE_VALUES \
    PROCESSING_STATE_IDLE, \
    PROCESSING_STATE_EXECUTING, \
    PROCESSING_STATE_NEW_DATA \

MU_DEFINE_ENUM(PROCESSING_STATE, PROCESSING_STATE_VALUES)

typedef struct QUEUE_PROCESSOR_WORK_ITEM_TAG
{
    MPSC_LOCK_FREE_QUEUE_ITEM queue_item;
    QUEUE_PROCESSOR_PROCESS_ITEM process_item_func;
    void* process_item_context;
} QUEUE_PROCESSOR_WORK_ITEM;

typedef struct QUEUE_PROCESSOR_TAG
{
    MPSC_LOCK_FREE_QUEUE_HANDLE queued_items;
    volatile_atomic int32_t state;
    volatile_atomic int32_t pending_call_count;
    ON_QUEUE_PROCESSOR_ERROR on_error;
    void* on_error_context;
    volatile_atomic int32_t processing_state;
    THANDLE(THREADPOOL) threadpool;
} QUEUE_PROCESSOR;

static void internal_process_all_items(QUEUE_PROCESSOR_HANDLE queue_processor)
{
    MPSC_LOCK_FREE_QUEUE_ITEM* queue_item;

    /* Codes_SRS_QUEUE_PROCESSOR_01_041: [ While there are items in the queue: ]*/
    do
    {
        /* Codes_SRS_QUEUE_PROCESSOR_01_042: [ on_threadpool_work shall dequeue an item from the queue. ]*/
        queue_item = mpsc_lock_free_queue_dequeue(queue_processor->queued_items);
        if (queue_item != NULL)
        {
            QUEUE_PROCESSOR_WORK_ITEM* queue_processor_work_item = (QUEUE_PROCESSOR_WORK_ITEM*)((unsigned char*)queue_item - offsetof(QUEUE_PROCESSOR_WORK_ITEM, queue_item));

            /* Codes_SRS_QUEUE_PROCESSOR_01_043: [ on_threadpool_work shall call the process_item_func with the context set to process_item_context. ]*/
            queue_processor_work_item->process_item_func(queue_processor_work_item->process_item_context);
            free(queue_processor_work_item);

            if (interlocked_decrement(&queue_processor->pending_call_count) == 0)
            {
                wake_by_address_single(&queue_processor->pending_call_count);
            }
        }
    } while (queue_item != NULL);
}

static void on_threadpool_work(void* context)
{
    if (context == NULL)
    {
        /* Codes_SRS_QUEUE_PROCESSOR_01_039: [ If context is NULL, on_threadpool_work shall return. ]*/
        LogError("Invalid arguments: void* context=%p", context);
    }
    else
    {
        /* Codes_SRS_QUEUE_PROCESSOR_01_040: [ Otherwise on_threadpool_work shall use context as the QUEUE_PROCESSOR_HANDLE passed in queue_processor_schedule_item. ]*/
        QUEUE_PROCESSOR_HANDLE queue_processor = context;

        do
        {
            internal_process_all_items(queue_processor);

            /* Codes_SRS_QUEUE_PROCESSOR_01_055: [ If the processing state is EXECUTING, it shall be set to IDLE. ]*/
            if (interlocked_compare_exchange(&queue_processor->processing_state, PROCESSING_STATE_IDLE, PROCESSING_STATE_EXECUTING) == PROCESSING_STATE_EXECUTING)
            {
                // If the state is not open then we need to signal that the processing state is Idle
                if (interlocked_add(&queue_processor->state, 0) != QUEUE_PROCESSOR_STATE_OPEN)
                {
                    wake_by_address_single(&queue_processor->processing_state);
                }
                // IDLE now, just break, next items will be processed by another work item
                break;
            }

            /* Codes_SRS_QUEUE_PROCESSOR_01_054: [ If the processing state is NEW_DATA, it shall be set to EXECUTING and the queue shall be re-examined and all new items in it processed. ]*/
            if (interlocked_compare_exchange(&queue_processor->processing_state, PROCESSING_STATE_EXECUTING, PROCESSING_STATE_NEW_DATA) == PROCESSING_STATE_NEW_DATA)
            {
                // have to keep executing
                continue;
            }
        } while (1);
    }
}

static void internal_close(QUEUE_PROCESSOR_HANDLE queue_processor)
{
    do
    {
        // Wait for the threadpool work function to be completed
        if (InterlockedHL_WaitForValue(&queue_processor->processing_state, PROCESSING_STATE_IDLE, UINT32_MAX) != INTERLOCKED_HL_OK)
        {
            LogError("Failed waiting the threadpool work function to complete");
        }

        // Codes_SRS_QUEUE_PROCESSOR_11_003: [ If the process state is IDLE then queue_processor_close shall change it to PROCESSING_STATE_EXECUTING and call the on_threadpool_work function ]
        if (interlocked_compare_exchange(&queue_processor->processing_state, PROCESSING_STATE_EXECUTING, PROCESSING_STATE_IDLE) == PROCESSING_STATE_IDLE)
        {
            /* Codes_SRS_QUEUE_PROCESSOR_01_051: [ Any queued items that were not processed shall be processed by queue_processor_close. ]*/
            on_threadpool_work(queue_processor);
            break;
        }
    } while (1);

    /* Codes_SRS_QUEUE_PROCESSOR_01_027: [ queue_processor_close shall wait for any ongoing queue_processor_schedule_item API calls to complete. ]*/
    if (InterlockedHL_WaitForValue(&queue_processor->pending_call_count, 0, UINT32_MAX) != INTERLOCKED_HL_OK)
    {
        LogCriticalAndTerminate("Failed waiting for pending calls to reach 0");
    }

    /* Codes_SRS_QUEUE_PROCESSOR_01_029: [ queue_processor_close shall set the state to CLOSED. ]*/
    /* Codes_SRS_QUEUE_PROCESSOR_01_030: [ After a close, a successful call to queue_processor_open shall be possible. ]*/
    (void)interlocked_exchange(&queue_processor->state, QUEUE_PROCESSOR_STATE_CLOSED);
    wake_by_address_single(&queue_processor->state);
}

QUEUE_PROCESSOR_HANDLE queue_processor_create(THANDLE(THREADPOOL) threadpool)
{
    QUEUE_PROCESSOR_HANDLE result;

    // Codes_SRS_QUEUE_PROCESSOR_11_001: [ If threadpool is NULL, queue_processor_create shall fail and return NULL. ]
    if (threadpool == NULL)
    {
        LogError("Invalid arguments: THANDLE(THREADPOOL) threadpool: %p", threadpool);
    }
    else
    {
        /* Codes_SRS_QUEUE_PROCESSOR_01_002: [ Otherwise, queue_processor_create shall create a new queue processor and on success return a non-NULL handle to it. ]*/
        result = malloc(sizeof(QUEUE_PROCESSOR));
        if (result == NULL)
        {
            /* Codes_SRS_QUEUE_PROCESSOR_01_004: [ If any error occurs, queue_processor_create shall fail and return NULL. ]*/
            LogError("malloc failed");
        }
        else
        {
            /* Codes_SRS_QUEUE_PROCESSOR_01_045: [ queue_processor_create shall create a multi producer single consumer queue. ]*/
            result->queued_items = mpsc_lock_free_queue_create();
            if (result->queued_items == NULL)
            {
                /* Codes_SRS_QUEUE_PROCESSOR_01_004: [ If any error occurs, queue_processor_create shall fail and return NULL. ]*/
                LogError("mpsc_lock_free_queue_create failed");
            }
            else
            {
                (void)interlocked_exchange(&result->processing_state, PROCESSING_STATE_IDLE);
                (void)interlocked_exchange(&result->pending_call_count, 0);
                (void)interlocked_exchange(&result->state, QUEUE_PROCESSOR_STATE_CLOSED);
                // Codes_SRS_QUEUE_PROCESSOR_01_003: [ queue_processor_create shall initialize its threadpool object by calling THANDLE_INITIALIZE(THREADPOOL). ]
                THANDLE_INITIALIZE(THREADPOOL)(&result->threadpool, threadpool);
                goto all_ok;
            }
            free(result);
        }
    }

    result = NULL;

all_ok:
    return result;
}

void queue_processor_destroy(QUEUE_PROCESSOR_HANDLE queue_processor)
{
    /* Codes_SRS_QUEUE_PROCESSOR_01_005: [ If queue_processor is NULL, queue_processor_destroy shall return. ]*/
    if (queue_processor == NULL)
    {
        LogError("QUEUE_PROCESSOR_HANDLE queue_processor=%p", queue_processor);
    }
    else
    {
        bool exit_loop = false;
        do
        {
            /* Codes_SRS_QUEUE_PROCESSOR_01_006: [ Otherwise queue_processor_destroy shall wait until the state is either CLOSED or OPEN. ]*/
            int32_t current_state = interlocked_add(&queue_processor->state, 0);
            switch (current_state)
            {
                case QUEUE_PROCESSOR_STATE_OPEN:
                case QUEUE_PROCESSOR_STATE_FAULTED:
                {
                    int32_t changed_state = interlocked_compare_exchange(&queue_processor->state, QUEUE_PROCESSOR_STATE_CLOSING, current_state);
                    if (changed_state == current_state)
                    {
                        /* Codes_SRS_QUEUE_PROCESSOR_01_007: [ queue_processor_destroy shall perform a close in case the queue_processor is not CLOSED. ]*/
                        internal_close(queue_processor);
                        exit_loop = true;
                        break;
                    }
                    else if (changed_state == QUEUE_PROCESSOR_STATE_CLOSED)
                    {
                        exit_loop = true;
                        break;
                    }
                    (void)wait_on_address(&queue_processor->state, current_state, UINT32_MAX);
                    break;
                }
                default:
                case QUEUE_PROCESSOR_STATE_CLOSED:
                    exit_loop = true;
                    break;
            }
            if (exit_loop)
            {
                break;
            }
        } while (1);

        /* Codes_SRS_QUEUE_PROCESSOR_01_008: [ queue_processor_destroy shall assign NULL the threadpool initialized in queue_processor_create. ]*/
        mpsc_lock_free_queue_destroy(queue_processor->queued_items);
        /* Codes_SRS_QUEUE_PROCESSOR_01_046: [ queue_processor_destroy shall destroy the multi producer single consumer queue created in queue_processor_create. ]*/
        THANDLE_ASSIGN(THREADPOOL)(&queue_processor->threadpool, NULL);

        /* Codes_SRS_QUEUE_PROCESSOR_01_009: [ queue_processor_destroy shall free the memory associated with the worker. ]*/
        free(queue_processor);
    }
}

int queue_processor_open(QUEUE_PROCESSOR_HANDLE queue_processor, ON_QUEUE_PROCESSOR_ERROR on_error, void* on_error_context)
{
    int result;

    /* Codes_SRS_QUEUE_PROCESSOR_01_049: [ on_error_context shall be allowed to be NULL. ]*/

    if (
        /* Codes_SRS_QUEUE_PROCESSOR_01_010: [ If queue_processor is NULL, queue_processor_open shall fail and return a non-zero value. ]*/
        (queue_processor == NULL) ||
        /* Codes_SRS_QUEUE_PROCESSOR_01_048: [ If on_error is NULL, queue_processor_open shall fail and return a non-zero value. ]*/
        (on_error == NULL)
        )
    {
        LogError("QUEUE_PROCESSOR_HANDLE queue_processor=%p, ON_QUEUE_PROCESSOR_ERROR on_error=%p, void* on_error_context=%p",
            queue_processor, on_error, on_error_context);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_QUEUE_PROCESSOR_01_013: [ Otherwise, queue_processor_open shall switch the state to QUEUE_PROCESSOR_STATE_OPENING. ]*/
        int32_t current_state = interlocked_compare_exchange(&queue_processor->state, QUEUE_PROCESSOR_STATE_OPENING, QUEUE_PROCESSOR_STATE_CLOSED);
        if (current_state != QUEUE_PROCESSOR_STATE_CLOSED)
        {
            /* Codes_SRS_QUEUE_PROCESSOR_01_014: [ If the queue_processor state was not QUEUE_PROCESSOR_STATE_CLOSED, queue_processor_open shall fail and return non-zero value. ]*/
            LogError("Not closed, cannot open, current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(QUEUE_PROCESSOR_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {
            queue_processor->on_error = on_error;
            queue_processor->on_error_context = on_error_context;

            (void)interlocked_exchange(&queue_processor->state, QUEUE_PROCESSOR_STATE_OPEN);
            wake_by_address_single(&queue_processor->state);

            /* Codes_SRS_QUEUE_PROCESSOR_01_016: [ On success, queue_processor_open shall return 0. ]*/
            result = 0;
        }
    }
    return result;
}

void queue_processor_close(QUEUE_PROCESSOR_HANDLE queue_processor)
{
    /* Codes_SRS_QUEUE_PROCESSOR_01_024: [ If queue_processor is NULL, queue_processor_close shall return. ]*/
    if (queue_processor == NULL)
    {
        LogError("Invalid arguments: QUEUE_PROCESSOR_HANDLE queue_processor=%p", queue_processor);
    }
    else
    {
        /* Codes_SRS_QUEUE_PROCESSOR_01_026: [ Otherwise, queue_processor_close shall switch the state to QUEUE_PROCESSOR_STATE_CLOSING. ]*/
        int32_t current_state_1 = interlocked_compare_exchange(&queue_processor->state, QUEUE_PROCESSOR_STATE_CLOSING, QUEUE_PROCESSOR_STATE_OPEN);
        int32_t current_state_2 = interlocked_compare_exchange(&queue_processor->state, QUEUE_PROCESSOR_STATE_CLOSING, QUEUE_PROCESSOR_STATE_FAULTED);
        if (current_state_1 != QUEUE_PROCESSOR_STATE_OPEN &&
            current_state_2 != QUEUE_PROCESSOR_STATE_FAULTED)
        {
            /* Codes_SRS_QUEUE_PROCESSOR_01_025: [ If the state of queue_processor is not OPEN, queue_processor_close shall return. ]*/
            LogWarning("queue processor close called but state is neither OPEN or FAULTED current state 1 is %" PRI_MU_ENUM " current state 2 is %" PRI_MU_ENUM "",
                MU_ENUM_VALUE(QUEUE_PROCESSOR_STATE, current_state_1), MU_ENUM_VALUE(QUEUE_PROCESSOR_STATE, current_state_2));
        }
        else
        {
            internal_close(queue_processor);
        }
    }
}

int queue_processor_schedule_item(QUEUE_PROCESSOR_HANDLE queue_processor, QUEUE_PROCESSOR_PROCESS_ITEM process_item_func, void* process_item_context)
{
    int result;

    /* Codes_SRS_QUEUE_PROCESSOR_01_052: [ process_item_context shall be allowed to be NULL. ]*/

    if (
        /* Codes_SRS_QUEUE_PROCESSOR_01_031: [ If queue_processor is NULL, queue_processor_schedule_item shall fail and return non-zero value. ]*/
        (queue_processor == NULL) ||
        /* Codes_SRS_QUEUE_PROCESSOR_01_047: [ If process_item_func is NULL, queue_processor_schedule_item shall fail and return non-zero value. ]*/
        (process_item_func == NULL)
        )
    {
        LogError("Invalid arguments: QUEUE_PROCESSOR_HANDLE queue_processor=%p, QUEUE_PROCESSOR_PROCESS_ITEM process_item_func=%p, void* process_item_context=%p",
            queue_processor, process_item_func, process_item_context);
        result = MU_FAILURE;
    }
    else
    {
        (void)interlocked_increment(&queue_processor->pending_call_count);

        int32_t current_state = interlocked_add(&queue_processor->state, 0);
        if (current_state != QUEUE_PROCESSOR_STATE_OPEN)
        {
            /* Codes_SRS_QUEUE_PROCESSOR_01_032: [ If the queue_processor state is not OPEN, queue_processor_schedule_item shall fail and return non-zero value. ]*/
            LogWarning("Not open, current state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(QUEUE_PROCESSOR_STATE, current_state));
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_QUEUE_PROCESSOR_01_033: [ Otherwise queue_processor_schedule_item shall allocate a context where process_item_func and process_item_context shall be stored. ]*/
            QUEUE_PROCESSOR_WORK_ITEM* queue_processor_work_item = malloc(sizeof(QUEUE_PROCESSOR_WORK_ITEM));
            if (queue_processor_work_item == NULL)
            {
                /* Codes_SRS_QUEUE_PROCESSOR_01_038: [ If any error occurs, queue_processor_schedule_item shall fail and return non-zero value. ]*/
                LogError("malloc failed");
                result = MU_FAILURE;
            }
            else
            {
                queue_processor_work_item->process_item_func = process_item_func;
                queue_processor_work_item->process_item_context = process_item_context;

                /* Codes_SRS_QUEUE_PROCESSOR_01_034: [ queue_processor_schedule_item shall enqueue the context in the multi producer single consumer queue created in queue_processor_create. ]*/
                if (mpsc_lock_free_queue_enqueue(queue_processor->queued_items, &queue_processor_work_item->queue_item) != 0)
                {
                    /* Codes_SRS_QUEUE_PROCESSOR_01_038: [ If any error occurs, queue_processor_schedule_item shall fail and return non-zero value. ]*/
                    LogError("mpsc_lock_free_queue_enqueue failed");
                    result = MU_FAILURE;
                }
                else
                {
                    bool call_on_error = false;

                    do
                    {
                        /* Codes_SRS_QUEUE_PROCESSOR_01_053: [ If the processing state is EXECUTING, queue_processor_schedule_item shall switch the processing state to NEW_DATA to signal on_threadpool_work that new items need to be processed. ]*/
                        int32_t current_processing_state = interlocked_compare_exchange(&queue_processor->processing_state, PROCESSING_STATE_NEW_DATA, PROCESSING_STATE_EXECUTING);
                        if ((current_processing_state == PROCESSING_STATE_EXECUTING) ||
                            (current_processing_state == PROCESSING_STATE_NEW_DATA))
                        {
                            break;
                        }

                        if (interlocked_compare_exchange(&queue_processor->processing_state, PROCESSING_STATE_EXECUTING, PROCESSING_STATE_IDLE) == PROCESSING_STATE_IDLE)
                        {
                            /* Codes_SRS_QUEUE_PROCESSOR_01_036: [ If the processing state is IDLE, queue_processor_schedule_item shall switch the processing state to EXECUTING queue_processor_schedule_item shall call threadpool_schedule_work to schedule a work item and pass as callback on_threadpool_work and queue_processor as context. ]*/
                            if (threadpool_schedule_work(queue_processor->threadpool, on_threadpool_work, queue_processor) != 0)
                            {
                                // scheduling the item failed, but we cannot pull the item back from the queue, thus indicate an error happened by calling on_error.
                                LogError("threadpool_schedule_work failed");

                                /* Codes_SRS_QUEUE_PROCESSOR_01_050: [ If scheduling the threadpool work item fails, an error shall be indicated by calling the on_error callback passed to queue_processor_open. ]*/
                                call_on_error = true;
                            }
                            break;
                        }
                    } while (1);

                    if (call_on_error)
                    {
                        // Codes_SRS_QUEUE_PROCESSOR_11_002: [ ... and shall set the state to QUEUE_PROCESSOR_STATE_FAULTED
                        (void)interlocked_exchange(&queue_processor->state, QUEUE_PROCESSOR_STATE_FAULTED);

                        queue_processor->on_error(queue_processor->on_error_context);
                        // Since threadpool_schedule_work failed then we need to go from executing back to idle
                        (void)interlocked_compare_exchange(&queue_processor->processing_state, PROCESSING_STATE_IDLE, PROCESSING_STATE_EXECUTING);
                    }

                    /* Codes_SRS_QUEUE_PROCESSOR_01_037: [ On success, queue_processor_schedule_item returns 0. ]*/
                    result = 0;

                    goto all_ok;
                }

                free(queue_processor_work_item);
            }
        }

        if (interlocked_decrement(&queue_processor->pending_call_count) == 0)
        {
            wake_by_address_single(&queue_processor->pending_call_count);
        }
    }

all_ok:
    return result;
}
