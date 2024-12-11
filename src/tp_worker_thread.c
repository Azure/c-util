// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/sync.h"
#include "c_pal/log_critical_and_terminate.h"
#include "c_pal/sm.h"
#include "c_pal/thandle.h"
#include "c_pal/threadpool.h"

#include "c_util/tp_worker_thread.h"

#define WORKER_THREAD_STATE_VALUES \
    WORKER_THREAD_STATE_IDLE, \
    WORKER_THREAD_STATE_EXECUTING, \
    WORKER_THREAD_STATE_SCHEDULE_REQUESTED \

MU_DEFINE_ENUM(WORKER_THREAD_STATE, WORKER_THREAD_STATE_VALUES)

typedef struct TP_WORKER_THREAD_TAG
{
    INTERLOCKED_DEFINE_VOLATILE_STATE_ENUM(WORKER_THREAD_STATE, processing_state);
    EXECUTION_ENGINE_HANDLE execution_engine;
    THANDLE(THREADPOOL) threadpool;
    THANDLE(THREADPOOL_WORK_ITEM) threadpool_work_item;
    SM_HANDLE sm;
    TP_WORKER_THREAD_FUNC worker_func;
    void* worker_func_context;
} TP_WORKER_THREAD;

MU_DEFINE_ENUM_STRINGS(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT_VALUES);

IMPLEMENT_MOCKABLE_FUNCTION(, TP_WORKER_THREAD_HANDLE, tp_worker_thread_create, EXECUTION_ENGINE_HANDLE, execution_engine, TP_WORKER_THREAD_FUNC, worker_func, void*, worker_func_context)
{
    TP_WORKER_THREAD_HANDLE result;

    if (
        /*Codes_SRS_TP_WORKER_THREAD_42_001: [ If execution_engine is NULL then tp_worker_thread_create shall fail and return NULL. ]*/
        execution_engine == NULL ||
        /*Codes_SRS_TP_WORKER_THREAD_42_002: [ If worker_func is NULL then tp_worker_thread_create shall fail and return NULL. ]*/
        worker_func == NULL
        )
    {
        LogError("Invalid args: EXECUTION_ENGINE_HANDLE execution_engine = %p, TP_WORKER_THREAD_FUNC worker_func = %p, void* worker_func_context = %p",
            execution_engine, worker_func, worker_func_context);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_TP_WORKER_THREAD_42_003: [ tp_worker_thread_create shall allocate memory for the worker thread. ]*/
        TP_WORKER_THREAD_HANDLE temp = malloc(sizeof(TP_WORKER_THREAD));
        if (temp == NULL)
        {
            /*Codes_SRS_TP_WORKER_THREAD_42_007: [ If there are any other errors then tp_worker_thread_create shall fail and return NULL. ]*/
            LogError("malloc of TP_WORKER_THREAD failed");
            result = NULL;
        }
        else
        {
            /*Codes_SRS_TP_WORKER_THREAD_42_004: [ tp_worker_thread_create shall call sm_create. ]*/
            temp->sm = sm_create("tp_worker_thread");
            if (temp->sm == NULL)
            {
                /*Codes_SRS_TP_WORKER_THREAD_42_007: [ If there are any other errors then tp_worker_thread_create shall fail and return NULL. ]*/
                LogError("sm_create failed");
                result = NULL;
            }
            else
            {
                temp->worker_func = worker_func;
                temp->worker_func_context = worker_func_context;
                THANDLE_INITIALIZE(THREADPOOL)(&temp->threadpool, NULL);
                THANDLE_INITIALIZE(THREADPOOL_WORK_ITEM)(&temp->threadpool_work_item, NULL);
                /*Codes_SRS_TP_WORKER_THREAD_45_001: [ tp_worker_thread_create shall save the execution_engine and call execution_engine_inc_ref. ]*/
                execution_engine_inc_ref(execution_engine);
                temp->execution_engine = execution_engine;
                (void)interlocked_exchange(&temp->processing_state, WORKER_THREAD_STATE_IDLE);

                /*Codes_SRS_TP_WORKER_THREAD_42_006: [ tp_worker_thread_create shall return the worker thread handle. ]*/
                result = temp;
                goto all_ok;
            }
            free(temp);
        }
    }
all_ok:
    return result;
}

static void tp_worker_thread_close_internal(TP_WORKER_THREAD_HANDLE worker_thread)
{
    /*Codes_SRS_TP_WORKER_THREAD_42_022: [ tp_worker_thread_close shall call sm_close_begin. ]*/
    if (sm_close_begin(worker_thread->sm) == SM_EXEC_GRANTED)
    {
        /*Codes_SRS_TP_WORKER_THREAD_01_002: [ tp_worker_thread_close shall call THANDLE_ASSIGN(THREADPOOL_WORK_ITEM) with NULL. ]*/
        THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(&worker_thread->threadpool_work_item, NULL);

        /*Codes_SRS_TP_WORKER_THREAD_45_005: [ tp_worker_thread_close shall call THANDLE_ASSIGN(THREADPOOL) with NULL. ]*/
        THANDLE_ASSIGN(THREADPOOL)(&worker_thread->threadpool, NULL);

        /*Codes_SRS_TP_WORKER_THREAD_42_024: [ tp_worker_thread_close shall call sm_close_end. ]*/
        sm_close_end(worker_thread->sm);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, tp_worker_thread_destroy, TP_WORKER_THREAD_HANDLE, worker_thread)
{
    if (worker_thread == NULL)
    {
        /*Codes_SRS_TP_WORKER_THREAD_42_008: [ If worker_thread is NULL then tp_worker_thread_destroy shall return. ]*/
        LogError("Invalid args: TP_WORKER_THREAD_HANDLE worker_thread = %p", worker_thread);
    }
    else
    {
        /*Codes_SRS_TP_WORKER_THREAD_42_009: [ tp_worker_thread_destroy shall behave as if tp_worker_thread_close was called. ]*/
        tp_worker_thread_close_internal(worker_thread);

        /*Codes_SRS_TP_WORKER_THREAD_45_002: [ tp_worker_thread_destroy shall call execution_engine_dec_ref. ]*/
        execution_engine_dec_ref(worker_thread->execution_engine);
        worker_thread->execution_engine = NULL;

        /*Codes_SRS_TP_WORKER_THREAD_42_011: [ tp_worker_thread_destroy shall call sm_destroy. ]*/
        sm_destroy(worker_thread->sm);

        /*Codes_SRS_TP_WORKER_THREAD_42_012: [ tp_worker_thread_destroy shall free the memory allocated for the worker thread. ]*/
        free(worker_thread);
    }
}

static void tp_worker_on_threadpool_work(void* context)
{
    if (context == NULL)
    {
        /*Codes_SRS_TP_WORKER_THREAD_42_037: [ If context is NULL then tp_worker_on_threadpool_work shall terminate the process. ]*/
        LogCriticalAndTerminate("Invalid args: void* context = %p", context);
    }
    else
    {
        TP_WORKER_THREAD_HANDLE worker_thread = context;

        do
        {
            /*Codes_SRS_TP_WORKER_THREAD_42_038: [ tp_worker_on_threadpool_work shall call worker_func with worker_func_context. ]*/
            worker_thread->worker_func(worker_thread->worker_func_context);

            /*Codes_SRS_TP_WORKER_THREAD_42_039: [ If the state is EXECUTING then tp_worker_on_threadpool_work shall change the state to IDLE and return. ]*/
            if (interlocked_compare_exchange(&worker_thread->processing_state, WORKER_THREAD_STATE_IDLE, WORKER_THREAD_STATE_EXECUTING) == WORKER_THREAD_STATE_EXECUTING)
            {
                // IDLE now, just break, next schedule process call will start a new threadpool work thread
                wake_by_address_all(&worker_thread->processing_state);
                break;
            }

            /*Codes_SRS_TP_WORKER_THREAD_42_040: [ If the state is SCHEDULE_REQUESTED then tp_worker_on_threadpool_work shall change the state to EXECUTING and repeat. ]*/
            int32_t current_processing_state = interlocked_compare_exchange(&worker_thread->processing_state, WORKER_THREAD_STATE_EXECUTING, WORKER_THREAD_STATE_SCHEDULE_REQUESTED);
            if (current_processing_state == WORKER_THREAD_STATE_SCHEDULE_REQUESTED)
            {
                // have to keep executing
                wake_by_address_all(&worker_thread->processing_state);
                continue;
            }
        } while (true);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, tp_worker_thread_open, TP_WORKER_THREAD_HANDLE, worker_thread)
{
    int result;
    if (
        /*Codes_SRS_TP_WORKER_THREAD_42_013: [ If worker_thread is NULL then tp_worker_thread_open shall fail and return a non-zero value. ]*/
        worker_thread == NULL
        )
    {
        LogError("Invalid args: TP_WORKER_THREAD_HANDLE worker_thread = %p",
            worker_thread);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_TP_WORKER_THREAD_42_014: [ tp_worker_thread_open shall call sm_open_begin. ]*/
        SM_RESULT sm_result = sm_open_begin(worker_thread->sm);
        if (sm_result != SM_EXEC_GRANTED)
        {
            /*Codes_SRS_TP_WORKER_THREAD_42_020: [ If there are any errors then tp_worker_thread_open shall fail and return a non-zero value. ]*/
            LogError("sm_open_begin failed with %" PRI_MU_ENUM,
                MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_TP_WORKER_THREAD_42_015: [ tp_worker_thread_open shall initialize the state to IDLE. ]*/
            (void)interlocked_exchange(&worker_thread->processing_state, WORKER_THREAD_STATE_IDLE);

            /*Codes_SRS_TP_WORKER_THREAD_45_003: [ tp_worker_thread_open shall call threadpool_create with the saved execution_engine. ]*/
            THANDLE_INITIALIZE_MOVE(THREADPOOL)(&worker_thread->threadpool, &(THANDLE(THREADPOOL)){threadpool_create(worker_thread->execution_engine)});
            if (worker_thread->threadpool == NULL)
            {
                LogError("threadpool_open failed");
                /*Codes_SRS_TP_WORKER_THREAD_42_020: [ If there are any errors then tp_worker_thread_open shall fail and return a non-zero value. ]*/
                result = MU_FAILURE;
            }
            else
            {
                /*Codes_SRS_TP_WORKER_THREAD_42_041: [ tp_worker_thread_open shall call threadpool_create_work_item with the threadpool, tp_worker_on_threadpool_work and worker_thread. ]*/
                THANDLE(THREADPOOL_WORK_ITEM) threadpool_work_item = threadpool_create_work_item(worker_thread->threadpool, tp_worker_on_threadpool_work, worker_thread);
                if (threadpool_work_item == NULL)
                {
                    LogError("threadpool_create_work_item failed");
                    /*Codes_SRS_TP_WORKER_THREAD_42_020: [ If there are any errors then tp_worker_thread_open shall fail and return a non-zero value. ]*/
                    result = MU_FAILURE;
                }
                else
                {
                    /*Codes_SRS_TP_WORKER_THREAD_01_001: [ tp_worker_thread_open shall save the THANDLE(THREADPOOL_WORK_ITEM) for later use by using THANDLE_INITIALIZE_MOVE(THREADPOOL_WORK_ITEM). ]*/
                    THANDLE_INITIALIZE_MOVE(THREADPOOL_WORK_ITEM)(&worker_thread->threadpool_work_item, &threadpool_work_item);

                    /*Codes_SRS_TP_WORKER_THREAD_42_019: [ tp_worker_thread_open shall succeed and return 0. ]*/
                    result = 0;

                    /*Codes_SRS_TP_WORKER_THREAD_42_018: [ tp_worker_thread_open shall call sm_open_end with true. ]*/
                    sm_open_end(worker_thread->sm, true);

                    goto all_ok;
                }
                THANDLE_ASSIGN(THREADPOOL)(&worker_thread->threadpool, NULL);
            }
            /*Codes_SRS_TP_WORKER_THREAD_45_004: [ If threadpool_create fails then tp_worker_thread_open shall call sm_open_end with false. ]*/
            /*Codes_SRS_TP_WORKER_THREAD_42_043: [ If threadpool_create_work_item fails then tp_worker_thread_open shall call sm_open_end with false. ]*/
            sm_open_end(worker_thread->sm, false);
        }
    }
all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, tp_worker_thread_close, TP_WORKER_THREAD_HANDLE, worker_thread)
{
    if (worker_thread == NULL)
    {
        /*Codes_SRS_TP_WORKER_THREAD_42_021: [ If worker_thread is NULL then tp_worker_thread_close shall return. ]*/
        LogError("Invalid args: TP_WORKER_THREAD_HANDLE worker_thread = %p",
            worker_thread);
    }
    else
    {
        tp_worker_thread_close_internal(worker_thread);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, tp_worker_thread_schedule_process, TP_WORKER_THREAD_HANDLE, worker_thread)
{
    TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT result;
    if (
        /*Codes_SRS_TP_WORKER_THREAD_42_025: [ If worker_thread is NULL then tp_worker_thread_schedule_process shall fail and return TP_WORKER_THREAD_SCHEDULE_PROCESS_INVALID_ARGS. ]*/
        worker_thread == NULL
        )
    {
        LogError("Invalid args: TP_WORKER_THREAD_HANDLE worker_thread = %p",
            worker_thread);
        result = TP_WORKER_THREAD_SCHEDULE_PROCESS_INVALID_ARGS;
    }
    else
    {
        /*Codes_SRS_TP_WORKER_THREAD_42_026: [ tp_worker_thread_schedule_process shall call sm_exec_begin. ]*/
        SM_RESULT sm_result = sm_exec_begin(worker_thread->sm);
        if (sm_result != SM_EXEC_GRANTED)
        {
            /*Codes_SRS_TP_WORKER_THREAD_42_027: [ If sm_exec_begin fails then tp_worker_thread_schedule_process shall return TP_WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE. ]*/
            LogError("sm_exec_begin failed with %" PRI_MU_ENUM,
                MU_ENUM_VALUE(SM_RESULT, sm_result));
            result = TP_WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE;
        }
        else
        {
            do
            {
                /*Codes_SRS_TP_WORKER_THREAD_42_028: [ If the state is EXECUTING then tp_worker_thread_schedule_process shall set the state to SCHEDULE_REQUESTED and return TP_WORKER_THREAD_SCHEDULE_PROCESS_OK. ]*/
                int32_t current_processing_state = interlocked_compare_exchange(&worker_thread->processing_state, WORKER_THREAD_STATE_SCHEDULE_REQUESTED, WORKER_THREAD_STATE_EXECUTING);
                if ((current_processing_state == WORKER_THREAD_STATE_EXECUTING) ||
                    (current_processing_state == WORKER_THREAD_STATE_SCHEDULE_REQUESTED))
                {
                    wake_by_address_all(&worker_thread->processing_state);
                    break;
                }
                else
                {
                    current_processing_state = interlocked_compare_exchange(&worker_thread->processing_state, WORKER_THREAD_STATE_EXECUTING, WORKER_THREAD_STATE_IDLE);
                    if (current_processing_state == WORKER_THREAD_STATE_IDLE)
                    {
                        /*Codes_SRS_TP_WORKER_THREAD_42_029: [ If the state is IDLE then: ]*/
                        /*Codes_SRS_TP_WORKER_THREAD_42_030: [ tp_worker_thread_schedule_process shall set the state to EXECUTING. ]*/
                        /*Codes_SRS_TP_WORKER_THREAD_42_031: [ tp_worker_thread_schedule_process shall call threadpool_schedule_work_item on the work item created in the module open. ]*/
                        wake_by_address_all(&worker_thread->processing_state);
                        threadpool_schedule_work_item(worker_thread->threadpool, worker_thread->threadpool_work_item);
                        break;
                    }
                    else
                    {
                        // need to retry
                        (void)InterlockedHL_WaitForNotValue(&worker_thread->processing_state, current_processing_state, UINT32_MAX);
                    }
                }
            } while (1);

            /*Codes_SRS_TP_WORKER_THREAD_42_035: [ tp_worker_thread_schedule_process shall return TP_WORKER_THREAD_SCHEDULE_PROCESS_OK. ]*/
            result = TP_WORKER_THREAD_SCHEDULE_PROCESS_OK;

            /*Codes_SRS_TP_WORKER_THREAD_42_036: [ tp_worker_thread_schedule_process shall call sm_exec_end. ]*/
            sm_exec_end(worker_thread->sm);
        }
    }
    return result;
}
