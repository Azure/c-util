// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/threadapi.h"
#include "c_pal/sm.h"
#include "c_pal/sync.h"

#include "c_util/worker_thread.h"

MU_DEFINE_ENUM_STRINGS(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_RESULT_VALUES)

typedef struct WORKER_THREAD_TAG
{
    WORKER_FUNC worker_func;
    void* worker_func_context;
    THREAD_HANDLE thread_handle;
    volatile_atomic int32_t state;

    SM_HANDLE sm;
} WORKER_THREAD;

#define WORKER_THREAD_STATE_VALUES \
    WORKER_THREAD_STATE_IDLE, \
    WORKER_THREAD_STATE_PROCESS_ITEM, \
    WORKER_THREAD_STATE_CLOSE

MU_DEFINE_ENUM(WORKER_THREAD_STATE, WORKER_THREAD_STATE_VALUES)
MU_DEFINE_ENUM_STRINGS(WORKER_THREAD_STATE, WORKER_THREAD_STATE_VALUES);

static int worker_thread_func(void* arg)
{
    WORKER_THREAD_HANDLE worker_thread = arg;
    int result = 0;
    bool continue_run = true;

    do
    {
        // Codes_SRS_WORKER_THREAD_01_019: [ The worker thread started by worker_thread_create shall get the thread state. ]
        WORKER_THREAD_STATE current_state = interlocked_add(&worker_thread->state, 0);
        switch (current_state)
        {
            default:
                LogError("Invalid state: %" PRI_MU_ENUM "", MU_ENUM_VALUE(WORKER_THREAD_STATE, current_state));
                continue_run = false;
                break;

            case WORKER_THREAD_STATE_IDLE:
                // Codes_SRS_WORKER_THREAD_11_002: [ If the thread state is WORKER_THREAD_STATE_IDLE, the worker thread shall wait for the state to transition to something else. ]
                (void)InterlockedHL_WaitForNotValue(&worker_thread->state, WORKER_THREAD_STATE_IDLE, UINT32_MAX);
                break;
            case WORKER_THREAD_STATE_PROCESS_ITEM:
                // Codes_SRS_WORKER_THREAD_11_001: [ ... and set the thread state to WORKER_THREAD_STATE_IDLE if it has not been changed. ]
                (void)interlocked_compare_exchange(&worker_thread->state, WORKER_THREAD_STATE_IDLE, current_state);

                /* Codes_SRS_WORKER_THREAD_01_021: [ When the execute worker function event is signaled, the worker thread shall call the worker_func function passed to worker_thread_create and it shall pass worker_func_context as argument. ]*/
                worker_thread->worker_func(worker_thread->worker_func_context);
                break;
            case WORKER_THREAD_STATE_CLOSE:
                // Codes_SRS_WORKER_THREAD_01_020: [ If the thread state is WORKER_THREAD_STATE_CLOSE, the worker thread shall exit. ]
                continue_run = false;
                break;
        }
    } while (continue_run);

    return result;
}

WORKER_THREAD_HANDLE worker_thread_create(WORKER_FUNC worker_func, void* worker_func_context)
{
    WORKER_THREAD_HANDLE worker_thread;

    /* Codes_SRS_WORKER_THREAD_01_004: [ worker_func_context shall be allowed to be NULL. ]*/

    /* Codes_SRS_WORKER_THREAD_01_003: [ If worker_func is NULL, worker_thread_create shall fail and return NULL. ]*/
    if (worker_func == NULL)
    {
        LogError("NULL worker_func");
    }
    else
    {
        /* Codes_SRS_WORKER_THREAD_01_001: [ worker_thread_create shall allocate memory for a new worker thread object and on success return a non-NULL handle to it. ]*/
        worker_thread = malloc(sizeof(WORKER_THREAD));
        if (worker_thread == NULL)
        {
            /* Codes_SRS_WORKER_THREAD_01_008: [ If any error occurs, worker_thread_create shall fail and return NULL. ]*/
            LogError("Cannot allocate memory for BS worker thread");
        }
        else
        {
            worker_thread->worker_func = worker_func;
            worker_thread->worker_func_context = worker_func_context;

            /* Codes_SRS_WORKER_THREAD_01_037: [ worker_thread_create shall create a state manager object by calling sm_create with the name worker_thread. ]*/
            worker_thread->sm = sm_create("worker_thread");
            if (worker_thread->sm == NULL)
            {
                LogError("sm_create failed");
            }
            else
            {
                /* Codes_SRS_WORKER_THREAD_01_022: [ worker_thread_create shall perform the following actions in order: ]*/

                /* Codes_SRS_WORKER_THREAD_01_006: [ worker_thread_create shall initialize the state object. ]*/
                (void)interlocked_exchange(&worker_thread->state, WORKER_THREAD_STATE_IDLE);
                goto all_ok;
            }

            free(worker_thread);
        }
    }
    worker_thread = NULL;

all_ok:
    return worker_thread;
}

static void internal_close(WORKER_THREAD_HANDLE worker_thread)
{
    int dont_care;

    /* Codes_SRS_WORKER_THREAD_01_034: [ worker_thread_close shall set the worker thread state to close in order to indicate that the thread shall shutdown. ]*/
    (void)interlocked_exchange(&worker_thread->state, WORKER_THREAD_STATE_CLOSE);
    wake_by_address_single(&worker_thread->state);

    /* Codes_SRS_WORKER_THREAD_01_035: [ worker_thread_close shall wait for the thread to join by using ThreadAPI_Join. ]*/
    (void)ThreadAPI_Join(worker_thread->thread_handle, &dont_care);
}

void worker_thread_destroy(WORKER_THREAD_HANDLE worker_thread)
{
    if (worker_thread == NULL)
    {
        /* Codes_SRS_WORKER_THREAD_01_010: [ If worker_thread is NULL, worker_thread_destroy shall return. ]*/
        LogError("NULL worker_thread");
    }
    else
    {
        /* Codes_SRS_WORKER_THREAD_01_009: [ worker_thread_destroy shall free the resources associated with the worker thread handle. ]*/

        /* Codes_SRS_WORKER_THREAD_01_039: [ If the worker thread is open, worker_thread_destroy shall perform a close. ]*/
        if (sm_close_begin(worker_thread->sm) == SM_EXEC_GRANTED)
        {
            internal_close(worker_thread);

            sm_close_end(worker_thread->sm);
        }

        /* Codes_SRS_WORKER_THREAD_01_038: [ worker_thread_destroy shall destroy the state manager object created in worker_thread_create. ]*/
        sm_destroy(worker_thread->sm);

        free(worker_thread);
    }
}

int worker_thread_open(WORKER_THREAD_HANDLE worker_thread)
{
    int result;

    if (worker_thread == NULL)
    {
        /* Codes_SRS_WORKER_THREAD_01_026: [ If worker_thread is NULL, worker_thread_open shall fail and return a non-zero value. ]*/
        LogError("Invalid arguments: WORKER_THREAD_HANDLE worker_thread=%p", worker_thread);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_WORKER_THREAD_01_027: [ Otherwise, worker_thread_open shall call sm_open_begin. ]*/
        if (sm_open_begin(worker_thread->sm) != SM_EXEC_GRANTED)
        {
            /* Codes_SRS_WORKER_THREAD_01_031: [ If any error occurs, worker_thread_open shall fail and return a non-zero value. ]*/
            LogError("sm_open_begin did not grat execution of open");
            result = MU_FAILURE;
        }
        else
        {
            (void)interlocked_exchange(&worker_thread->state, WORKER_THREAD_STATE_IDLE);

            /* Codes_SRS_WORKER_THREAD_01_028: [ worker_thread_open shall start a worker_thread thread that will call worker_func by calling ThreadAPI_Create. ]*/
            if (ThreadAPI_Create(&worker_thread->thread_handle, worker_thread_func, worker_thread) != THREADAPI_OK)
            {
                /* Codes_SRS_WORKER_THREAD_01_031: [ If any error occurs, worker_thread_open shall fail and return a non-zero value. ]*/
                LogError("Cannot start worker thread");
                result = MU_FAILURE;
            }
            else
            {
                /* Codes_SRS_WORKER_THREAD_01_030: [ On success, worker_thread_open shall return 0. ]*/
                result = 0;
            }

            /* Codes_SRS_WORKER_THREAD_01_029: [ worker_thread_open shall call sm_open_end with success reflecting whether any error has occured during the open. ]*/
            sm_open_end(worker_thread->sm, result == 0 ? true : false);
        }
    }

    return result;
}

void worker_thread_close(WORKER_THREAD_HANDLE worker_thread)
{
    if (worker_thread == NULL)
    {
        /* Codes_SRS_WORKER_THREAD_01_032: [ If worker_thread is NULL, worker_thread_close shall return. ]*/
        LogError("Invalid arguments: WORKER_THREAD_HANDLE worker_thread=%p", worker_thread);
    }
    else
    {
        /* Codes_SRS_WORKER_THREAD_01_033: [ Otherwise, worker_thread_close shall call sm_close_begin. ]*/
        SM_RESULT sm_result = sm_close_begin(worker_thread->sm);
        if (sm_result != SM_EXEC_GRANTED)
        {
            /* Codes_SRS_WORKER_THREAD_01_040: [ If sm_close_begin does not return SM_EXEC_GRANTED, worker_thread_close shall return. ]*/
            LogError("sm_close_begin did not grant execution of close %" PRI_MU_ENUM, MU_ENUM_VALUE(SM_RESULT, sm_result));
        }
        else
        {
            internal_close(worker_thread);

            /* Codes_SRS_WORKER_THREAD_01_036: [ worker_thread_close shall call sm_close_end. ]*/
            sm_close_end(worker_thread->sm);
        }
    }
}

WORKER_THREAD_SCHEDULE_PROCESS_RESULT worker_thread_schedule_process(WORKER_THREAD_HANDLE worker_thread)
{
    WORKER_THREAD_SCHEDULE_PROCESS_RESULT result;

    if (worker_thread == NULL)
    {
        /* Codes_SRS_WORKER_THREAD_01_016: [ If worker_thread is NULL, worker_thread_schedule_process shall fail and return WORKER_THREAD_SCHEDULE_PROCESS_ERROR. ]*/
        LogError("NULL worker_thread");
        result = WORKER_THREAD_SCHEDULE_PROCESS_ERROR;
    }
    else
    {
        /* Codes_SRS_WORKER_THREAD_01_041: [ Otherwise worker_thread_schedule_process shall call sm_exec_begin. ]*/
        if (sm_exec_begin(worker_thread->sm) != SM_EXEC_GRANTED)
        {
            /* Codes_SRS_WORKER_THREAD_01_042: [ If sm_exec_begin does not grant the execution, worker_thread_schedule_process shall fail and return WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE. ]*/
            LogError("Cannot start execution of worker_thread_schedule_process");
            result = WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE;
        }
        else
        {
            /* Codes_SRS_WORKER_THREAD_01_017: [ worker_thread_schedule_process shall set the thread state to WORKER_THREAD_STATE_PROCESS_ITEM. ]*/
            WORKER_THREAD_STATE worker_thread_state = interlocked_compare_exchange(&worker_thread->state, WORKER_THREAD_STATE_PROCESS_ITEM, WORKER_THREAD_STATE_IDLE);
            if (
                (worker_thread_state != WORKER_THREAD_STATE_IDLE) &&
                (worker_thread_state != WORKER_THREAD_STATE_PROCESS_ITEM)
                )
            {
                // Codes_SRS_WORKER_THREAD_11_003: [ If the thread state is not WORKER_THREAD_STATE_IDLE, worker_thread_schedule_process shall fail and return WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE. ]
                LogError("Cannot start execution of worker_thread_schedule_process, worker_thread_state=%" PRI_MU_ENUM "", MU_ENUM_VALUE(WORKER_THREAD_STATE, worker_thread_state));
                result = WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE;
            }
            else
            {
                wake_by_address_single(&worker_thread->state);

                /* Codes_SRS_WORKER_THREAD_01_015: [ On success worker_thread_schedule_process shall return WORKER_THREAD_SCHEDULE_PROCESS_OK. ]*/
                result = WORKER_THREAD_SCHEDULE_PROCESS_OK;
            }
            /* Codes_SRS_WORKER_THREAD_01_043: [ worker_thread_schedule_process shall call sm_exec_end. ]*/
            sm_exec_end(worker_thread->sm);
        }
    }

    return result;
}
