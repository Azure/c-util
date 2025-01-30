// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdalign.h>
#include <stdbool.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/thandle.h"
#include "c_pal/srw_lock.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle_log_context_handle.h"
#include "c_pal/sm.h"
#include "c_pal/log_critical_and_terminate.h"

#include "c_util/doublylinkedlist.h"
#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"
#include "c_util/rc_string.h"

#include "../inc/c_util/channel_internal.h"

typedef struct CHANNEL_INTERNAL_TAG
{
    THANDLE(THREADPOOL) threadpool;
    SM_HANDLE sm;
    SRW_LOCK_HANDLE lock;
    DLIST_ENTRY op_list;
    THANDLE(PTR(LOG_CONTEXT_HANDLE)) log_context;
}CHANNEL_INTERNAL;

THANDLE_TYPE_DEFINE(CHANNEL_INTERNAL);

typedef struct CHANNEL_OP_TAG
{
    ON_DATA_CONSUMED_CB on_data_consumed_cb;
    ON_DATA_AVAILABLE_CB on_data_available_cb;
    void* push_context;
    void* pull_context;
    THANDLE(RC_PTR) data;
    DLIST_ENTRY anchor;

    THANDLE(RC_STRING) push_correlation_id;
    THANDLE(RC_STRING) pull_correlation_id;

    THANDLE(CHANNEL_INTERNAL) channel_internal;
    THANDLE(ASYNC_OP) async_op; // self reference to keep op alive even after user disposes

    CHANNEL_CALLBACK_RESULT result;
}CHANNEL_OP;


static void execute_callbacks(void* context);

static void channel_internal_dispose(CHANNEL_INTERNAL* channel_internal)
{
    /*Codes_SRS_CHANNEL_INTERNAL_43_150: [ channel_internal_dispose shall release the reference to the log_context ]*/
    THANDLE_ASSIGN(PTR(LOG_CONTEXT_HANDLE))(&channel_internal->log_context, NULL);

    /*Codes_SRS_CHANNEL_INTERNAL_43_091: [ channel_internal_dispose shall release the reference to THANDLE(THREADPOOL). ]*/
    THANDLE_ASSIGN(THREADPOOL)(&channel_internal->threadpool, NULL);

    /*Codes_SRS_CHANNEL_INTERNAL_43_099: [ channel_internal_dispose shall call srw_lock_destroy. ]*/
    srw_lock_destroy(channel_internal->lock);

    /*Codes_SRS_CHANNEL_INTERNAL_43_165: [ channel_internal_dispose shall call sm_destroy. ]*/
    sm_destroy(channel_internal->sm);
}

static void abandon_pending_operations(void* context)
{
    CHANNEL_INTERNAL* channel_internal_ptr = context;
    /*Codes_SRS_CHANNEL_INTERNAL_43_095: [ abandon_pending_operations shall iterate over the list of pending operations and do the following: ]*/
    for (DLIST_ENTRY* entry = DList_RemoveHeadList(&channel_internal_ptr->op_list); entry != &channel_internal_ptr->op_list; entry = DList_RemoveHeadList(&channel_internal_ptr->op_list))
    {
        CHANNEL_OP* channel_op = CONTAINING_RECORD(entry, CHANNEL_OP, anchor);

        /*Codes_SRS_CHANNEL_INTERNAL_43_096: [ set the result of the operation to CHANNEL_CALLBACK_RESULT_ABANDONED. ]*/
        channel_op->result = CHANNEL_CALLBACK_RESULT_ABANDONED;

        /*Codes_SRS_CHANNEL_INTERNAL_43_097: [ call threadpool_schedule_work with execute_callbacks as work_function. ]*/
        if (threadpool_schedule_work(channel_internal_ptr->threadpool, execute_callbacks, channel_op) != 0)
        {
            LogError("Failure in threadpool_schedule_work(channel_internal_ptr->threadpool=%p, execute_callbacks=%p, channel_op=%p)", channel_internal_ptr->threadpool, execute_callbacks, channel_op);
        }
        else
        {
            /* all ok */
        }
    }
}

void channel_internal_close(THANDLE(CHANNEL_INTERNAL) channel_internal)
{
    CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_internal);

    /*Codes_SRS_CHANNEL_INTERNAL_43_094: [ channel_internal_close shall call sm_close_begin_with_cb with abandon_pending_operation as the callback. ]*/
    SM_RESULT sm_close_begin_result = sm_close_begin_with_cb(channel_internal_ptr->sm, abandon_pending_operations, channel_internal_ptr, NULL, NULL);
    if (sm_close_begin_result != SM_EXEC_GRANTED)
    {
        LogError("Failure in sm_close_begin_with_cb(channel_internal_ptr->sm=%p, abandon_pending_operations=%p, channel_internal_ptr=%p, NULL, NULL). SM_RESULT sm_close_begin_result = %" PRI_MU_ENUM "", channel_internal_ptr->sm, abandon_pending_operations, channel_internal_ptr, MU_ENUM_VALUE(SM_RESULT, sm_close_begin_result));
    }
    else
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_100: [ channel_internal_close shall call sm_close_end. ]*/
        sm_close_end(channel_internal_ptr->sm);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CHANNEL_INTERNAL), channel_internal_create, THANDLE(PTR(LOG_CONTEXT_HANDLE)), log_context, THANDLE(THREADPOOL), threadpool)
{
    THANDLE(CHANNEL_INTERNAL) result = NULL;

    /*Codes_SRS_CHANNEL_INTERNAL_43_151: [ channel_internal_create shall call sm_create. ]*/
    SM_HANDLE sm = sm_create("channel");
    if (sm == NULL)
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_002: [ If there are any failures, channel_internal_create shall fail and return NULL. ]*/
        LogError("Failure in sm_create(\"channel\")");
    }
    else
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_098: [ channel_internal_create shall call srw_lock_create. ]*/
        SRW_LOCK_HANDLE lock = srw_lock_create(false, "channel");
        if (lock == NULL)
        {
            /*Codes_SRS_CHANNEL_INTERNAL_43_002: [ If there are any failures, channel_internal_create shall fail and return NULL. ]*/
            LogError("Failure in srw_lock_create(false, \"channel\")");
        }
        else
        {
            /*Codes_SRS_CHANNEL_INTERNAL_43_078: [ channel_internal_create shall create a CHANNEL_INTERNAL object by calling THANDLE_MALLOC with channel_internal_dispose as dispose.]*/
            THANDLE(CHANNEL_INTERNAL) channel_internal = THANDLE_MALLOC(CHANNEL_INTERNAL)(channel_internal_dispose);
            if (channel_internal == NULL)
            {
                /*Codes_SRS_CHANNEL_INTERNAL_43_002: [ If there are any failures, channel_internal_create shall fail and return NULL. ]*/
                LogError("Failure in THANDLE_MALLOC(CHANNEL_INTERNAL)(channel_internal_dispose=%p)", channel_internal_dispose);
            }
            else
            {
                CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_internal);

                channel_internal_ptr->sm = sm;
                channel_internal_ptr->lock = lock;

                /*Codes_SRS_CHANNEL_INTERNAL_43_080: [ channel_internal_create shall store given threadpool in the created CHANNEL_INTERNAL. ]*/
                THANDLE_INITIALIZE(THREADPOOL)(&channel_internal_ptr->threadpool, threadpool);

                /*Codes_SRS_CHANNEL_INTERNAL_43_149: [ channel_internal_create shall store the given log_context in the created CHANNEL_INTERNAL. ]*/
                THANDLE_INITIALIZE(PTR(LOG_CONTEXT_HANDLE))(&channel_internal_ptr->log_context, log_context);

                /*Codes_SRS_CHANNEL_INTERNAL_43_084: [ channel_internal_create shall call DList_InitializeListHead. ]*/
                DList_InitializeListHead(&channel_internal_ptr->op_list);

                /*Codes_SRS_CHANNEL_INTERNAL_43_086: [ channel_internal_create shall succeed and return the created THANDLE(CHANNEL). ]*/
                THANDLE_INITIALIZE_MOVE(CHANNEL_INTERNAL)(&result, &channel_internal);
                goto all_ok;
            }
            srw_lock_destroy(lock);
        }
        sm_destroy(sm);
    }
all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, channel_internal_open, THANDLE(CHANNEL_INTERNAL), channel_internal)
{
    int result;
    /*Codes_SRS_CHANNEL_INTERNAL_43_159: [channel_internal_open shall call sm_open_begin.]*/
    SM_RESULT sm_result = sm_open_begin(channel_internal->sm);
    if (sm_result != SM_EXEC_GRANTED)
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_161 : [If there are any failures, channel_internal_open shall fail and return a non-zero value.]*/
        LogError("Failure in sm_open_begin(channel_internal->sm=%p). SM_RESULT sm_result = %" PRI_MU_ENUM "", channel_internal->sm, MU_ENUM_VALUE(SM_RESULT, sm_result));
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_160 : [channel_internal_open shall call sm_open_end.]*/
        sm_open_end(channel_internal->sm, true);
        /*Codes_SRS_CHANNEL_INTERNAL_43_162 : [channel_internal_open shall succeed and return 0.]*/
        result = 0;
    }
    return result;
}

static void execute_callbacks(void* context)
{
    /*Codes_SRS_CHANNEL_INTERNAL_43_148: [ If channel_internal_op_context is NULL, execute_callbacks shall terminate the process. ]*/
    if (context == NULL)
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_148: [ If channel_internal_op_context is NULL, execute_callbacks shall terminate the process. ]*/
        LogCriticalAndTerminate("Invalid arguments: void* context=%p", context);
    }
    else
    {
        CHANNEL_OP* channel_op = (CHANNEL_OP*)context;
        CHANNEL_CALLBACK_RESULT result = channel_op->result; // local copy to make sure both callbacks are called with the same result

        LOGGER_LOG(LOG_LEVEL_VERBOSE, T_PTR_VALUE_OR_NULL(channel_op->channel_internal->log_context), "Executing callbacks for pull_correlation_id=%" PRI_RC_STRING ", push_correlation_id=%" PRI_RC_STRING ", callback_result=%" PRI_MU_ENUM "", RC_STRING_VALUE_OR_NULL(channel_op->pull_correlation_id), RC_STRING_VALUE_OR_NULL(channel_op->push_correlation_id), MU_ENUM_VALUE(CHANNEL_CALLBACK_RESULT, result));

        /*Codes_SRS_CHANNEL_INTERNAL_43_145: [ execute_callbacks shall call the stored callback(s) with the result of the operation. ]*/
        if (channel_op->on_data_available_cb != NULL)
        {
            channel_op->on_data_available_cb(channel_op->pull_context, result, channel_op->pull_correlation_id, channel_op->push_correlation_id, channel_op->data);
            /*Codes_SRS_CHANNEL_INTERNAL_43_157: [ execute_callbacks shall call sm_exec_end for each callback that is called. ]*/
            sm_exec_end(channel_op->channel_internal->sm);
        }
        if (channel_op->on_data_consumed_cb != NULL)
        {
            channel_op->on_data_consumed_cb(channel_op->push_context, result, channel_op->pull_correlation_id, channel_op->push_correlation_id);
            /*Codes_SRS_CHANNEL_INTERNAL_43_157: [ execute_callbacks shall call sm_exec_end for each callback that is called. ]*/
            sm_exec_end(channel_op->channel_internal->sm);
        }

        /*Codes_SRS_CHANNEL_INTERNAL_43_147: [ execute_callbacks shall perform cleanup of the operation. ]*/
        THANDLE_ASSIGN(RC_STRING)(&channel_op->pull_correlation_id, NULL);
        THANDLE_ASSIGN(RC_STRING)(&channel_op->push_correlation_id, NULL);
        THANDLE_ASSIGN(RC_PTR)(&channel_op->data, NULL);
        THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_op->channel_internal, NULL);

        // copy to local reference to avoid cutting the branch you are sitting on
        THANDLE(ASYNC_OP) temp = NULL;
        THANDLE_INITIALIZE_MOVE(ASYNC_OP)(&temp, &channel_op->async_op);
        THANDLE_ASSIGN(ASYNC_OP)(&temp, NULL);
    }
}

static void cancel_op(void* context)
{
    CHANNEL_OP* channel_op = context;
    CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_op->channel_internal);

    bool call_callback = false;

    /*Codes_SRS_CHANNEL_INTERNAL_43_154: [ cancel_op shall call sm_exec_begin. ]*/
    SM_RESULT sm_result = sm_exec_begin(channel_internal_ptr->sm);
    if (sm_result != SM_EXEC_GRANTED)
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_155: [ If there are any failures, cancel_op shall fail. ]*/
        LogError("Failure in sm_exec_begin(channel_internal_ptr->sm=%p). SM_RESULT sm_result = %" PRI_MU_ENUM "", channel_internal_ptr->sm, MU_ENUM_VALUE(SM_RESULT, sm_result));
    }
    else
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_134: [ cancel_op shall call srw_lock_acquire_exclusive. ]*/
        srw_lock_acquire_exclusive(channel_internal_ptr->lock);
        {
            /*Codes_SRS_CHANNEL_INTERNAL_43_135: [ If the operation is in the list of pending operations, cancel_op shall call DList_RemoveEntryList to remove it. ]*/
            if (
                (channel_op->anchor.Flink != &channel_op->anchor) &&
                (&channel_op->anchor == channel_op->anchor.Flink->Blink)
                )
            {
                (void)DList_RemoveEntryList(&channel_op->anchor);
                call_callback = true;
            }
            /*Codes_SRS_CHANNEL_INTERNAL_43_139: [ cancel_op shall call srw_lock_release_exclusive. ]*/
            srw_lock_release_exclusive(channel_internal_ptr->lock);
        }

        /*Codes_SRS_CHANNEL_INTERNAL_43_136: [ If the result of the operation is CHANNEL_CALLBACK_RESULT_OK, cancel_op shall set it to CHANNEL_CALLBACK_RESULT_CANCELLED. ]*/
        if (channel_op->result == CHANNEL_CALLBACK_RESULT_OK)
        {
            channel_op->result = CHANNEL_CALLBACK_RESULT_CANCELLED;
        }

        /*Codes_SRS_CHANNEL_INTERNAL_43_138: [ If the operation had been found in the list of pending operations, cancel_op shall call threadpool_schedule_work with execute_callbacks as work_function and the operation as work_function_context. ]*/
        if (call_callback)
        {
            if (threadpool_schedule_work(channel_internal_ptr->threadpool, execute_callbacks, channel_op) != 0)
            {
                LogError("Failure in threadpool_schedule_work(channel_internal_ptr->threadpool=%p, execute_callbacks=%p, channel_op=%p)", channel_internal_ptr->threadpool, execute_callbacks, channel_op);
            }
            else
            {
                /* all ok */
            }
        }

        /*Codes_SRS_CHANNEL_INTERNAL_43_156: [ cancel_op shall call sm_exec_end. ]*/
        sm_exec_end(channel_internal_ptr->sm);
    }
}

static void dispose_channel_op(void* context)
{
    /* don't need to do anything here because actual cleanup happens in execute_callbacks */
    (void)context;
}

static int enqueue_operation(THANDLE(CHANNEL_INTERNAL) channel_internal, THANDLE(ASYNC_OP)* out_op, THANDLE(RC_STRING) pull_correlation_id, ON_DATA_AVAILABLE_CB on_data_available_cb, void* pull_context, THANDLE(RC_STRING) push_correlation_id, ON_DATA_CONSUMED_CB on_data_consumed_cb, void* push_context, THANDLE(RC_PTR) data)
{
    int result;

    /*Codes_SRS_CHANNEL_INTERNAL_43_103: [ channel_internal_pull shall create a THANDLE(ASYNC_OP) by calling async_op_create with cancel_op as cancel. ]*/
    /*Codes_SRS_CHANNEL_INTERNAL_43_119: [ channel_internal_push shall create a THANDLE(ASYNC_OP) by calling async_op_create with cancel_op as cancel. ]*/
    THANDLE(ASYNC_OP) async_op = async_op_create(cancel_op, sizeof(CHANNEL_OP), alignof(CHANNEL_OP), dispose_channel_op);
    if (async_op == NULL)
    {
        LogError("Failure in async_op_create(cancel_op=%p, sizeof(CHANNEL_OP)=%zu, alignof(CHANNEL_OP)=%zu, dispose_channel_op=%p)", cancel_op, sizeof(CHANNEL_OP), alignof(CHANNEL_OP), dispose_channel_op);
        result = MU_FAILURE;
    }
    else
    {
        CHANNEL_OP* channel_op = async_op->context;

        /*Codes_SRS_CHANNEL_INTERNAL_43_104: [ channel_internal_pull shall store the correlation_id, on_data_available_cb and pull_context in the THANDLE(ASYNC_OP). ]*/
        THANDLE_INITIALIZE(RC_STRING)(&channel_op->pull_correlation_id, pull_correlation_id);
        channel_op->on_data_available_cb = on_data_available_cb;
        channel_op->pull_context = pull_context;

        /*Codes_SRS_CHANNEL_INTERNAL_43_120: [ channel_internal_push shall store the correlation_id, on_data_consumed_cb, push_context and data in the THANDLE(ASYNC_OP). ]*/
        THANDLE_INITIALIZE(RC_STRING)(&channel_op->push_correlation_id, push_correlation_id);
        channel_op->on_data_consumed_cb = on_data_consumed_cb;
        channel_op->push_context = push_context;
        THANDLE_INITIALIZE(RC_PTR)(&channel_op->data, data);

        THANDLE_INITIALIZE(ASYNC_OP)(&channel_op->async_op, async_op);
        THANDLE_INITIALIZE(CHANNEL_INTERNAL)(&channel_op->channel_internal, channel_internal);

        /*Codes_SRS_CHANNEL_INTERNAL_43_111: [ channel_internal_pull shall set the result of the created operation to CHANNEL_CALLBACK_RESULT_OK. ]*/
        /*Codes_SRS_CHANNEL_INTERNAL_43_127: [ channel_internal_push shall set the result of the created operation to CHANNEL_CALLBACK_RESULT_OK. ]*/
        channel_op->result = CHANNEL_CALLBACK_RESULT_OK;

        CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_internal);
        /*Codes_SRS_CHANNEL_INTERNAL_43_105: [ channel_internal_pull shall insert the created THANDLE(ASYNC_OP) in the list of pending operations by calling DList_InsertTailList. ]*/
        /*Codes_SRS_CHANNEL_INTERNAL_43_121: [ channel_internal_push shall insert the created THANDLE(ASYNC_OP) in the list of pending operations by calling DList_InsertTailList. ]*/
        DList_InsertTailList(&channel_internal_ptr->op_list, &channel_op->anchor);

        /*Codes_SRS_CHANNEL_INTERNAL_43_107: [ channel_internal_pull shall set *out_op_pull to the created THANDLE(ASYNC_OP). ]*/
        /*Codes_SRS_CHANNEL_INTERNAL_43_123: [ channel_internal_push shall set *out_op_push to the created THANDLE(ASYNC_OP). ]*/
        THANDLE_INITIALIZE_MOVE(ASYNC_OP)(out_op, &async_op);
        result = 0;
    }
    return result;
}

static int dequeue_operation(THANDLE(CHANNEL_INTERNAL) channel_internal, THANDLE(ASYNC_OP)* out_op, THANDLE(RC_STRING) pull_correlation_id, ON_DATA_AVAILABLE_CB on_data_available_cb, void* pull_context, THANDLE(RC_STRING) push_correlation_id, ON_DATA_CONSUMED_CB on_data_consumed_cb, void* push_context, THANDLE(RC_PTR) data)
{
    int result;

    CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_internal);

    /*Codes_SRS_CHANNEL_INTERNAL_43_109: [ channel_internal_pull shall call DList_RemoveHeadList on the list of pending operations to obtain the operation. ]*/
    /*Codes_SRS_CHANNEL_INTERNAL_43_125: [ channel_internal_push shall call DList_RemoveHeadList on the list of pending operations to obtain the operation. ]*/
    PDLIST_ENTRY op_entry = DList_RemoveHeadList(&channel_internal_ptr->op_list);

    CHANNEL_OP* channel_op = CONTAINING_RECORD(op_entry, CHANNEL_OP, anchor);
    if (on_data_available_cb != NULL)
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_112: [ channel_internal_pull shall store the correlation_id, on_data_available_cb and pull_context in the obtained operation. ]*/
        THANDLE_INITIALIZE(RC_STRING)(&channel_op->pull_correlation_id, pull_correlation_id);
        channel_op->on_data_available_cb = on_data_available_cb;
        channel_op->pull_context = pull_context;
    }
    else if (on_data_consumed_cb != NULL)
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_128: [ channel_internal_push shall store the correlation_id, on_data_consumed_cb, push_context and data in the obtained operation. ]*/
        THANDLE_INITIALIZE(RC_STRING)(&channel_op->push_correlation_id, push_correlation_id);
        channel_op->on_data_consumed_cb = on_data_consumed_cb;
        channel_op->push_context = push_context;
        THANDLE_ASSIGN(RC_PTR)(&channel_op->data, data);
    }
    THANDLE(ASYNC_OP) temp = NULL;
    THANDLE_INITIALIZE(ASYNC_OP)(&temp, channel_op->async_op);
    /*Codes_SRS_CHANNEL_INTERNAL_43_113: [ channel_internal_pull shall call threadpool_schedule_work with execute_callbacks as work_function and the obtained operation as work_function_context. ]*/
    /*Codes_SRS_CHANNEL_INTERNAL_43_129: [ channel_internal_push shall call threadpool_schedule_work with execute_callbacks as work_function and the obtained operation as work_function_context. ]*/
    if (threadpool_schedule_work(channel_internal_ptr->threadpool, execute_callbacks, channel_op) != 0)
    {
        LogError("Failure in threadpool_schedule_work(channel_internal_ptr->threadpool=%p, execute_callbacks=%p, channel_op=%p)", channel_internal_ptr->threadpool, execute_callbacks, channel_op);
        result = MU_FAILURE;
        // For testing purposes, we must unset the callback
        if (on_data_available_cb != NULL)
        {
            channel_op->on_data_available_cb = NULL;
        }
        else if (on_data_consumed_cb != NULL)
        {
            channel_op->on_data_consumed_cb = NULL;
        }
    }
    else
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_114: [ channel_internal_pull shall set *out_op_pull to the THANDLE(ASYNC_OP) of the obtained operation. ]*/
        /*Codes_SRS_CHANNEL_INTERNAL_43_130: [ channel_internal_push shall set *out_op_push to the THANDLE(ASYNC_OP) of the obtained operation. ]*/
        THANDLE_INITIALIZE(ASYNC_OP)(out_op, temp);
        result = 0;
    }
    THANDLE_ASSIGN(ASYNC_OP)(&temp, NULL);
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_internal_pull, THANDLE(CHANNEL_INTERNAL), channel_internal, THANDLE(RC_STRING), correlation_id, ON_DATA_AVAILABLE_CB, on_data_available_cb, void*, pull_context, THANDLE(ASYNC_OP)*, out_op_pull)
{
    CHANNEL_RESULT result;
    CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_internal);

    /*Codes_SRS_CHANNEL_INTERNAL_43_152: [ channel_internal_pull shall call sm_exec_begin. ]*/
    SM_RESULT sm_result = sm_exec_begin(channel_internal_ptr->sm);
    if (sm_result != SM_EXEC_GRANTED)
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_023: [ If there are any failures, channel_internal_pull shall fail and return CHANNEL_RESULT_ERROR. ]*/
        LogError("Failure in sm_exec_begin(channel_internal_ptr->sm=%p). SM_RESULT sm_result = %" PRI_MU_ENUM "", channel_internal_ptr->sm, MU_ENUM_VALUE(SM_RESULT, sm_result));
        result = CHANNEL_RESULT_ERROR;
    }
    else
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_010: [ channel_internal_pull shall call srw_lock_acquire_exclusive. ]*/
        srw_lock_acquire_exclusive(channel_internal_ptr->lock);
        {
            /*Codes_SRS_CHANNEL_INTERNAL_43_101: [ If the list of pending operations is empty or the first operation in the list of pending operations contains a non-NULL on_data_available_cb: ]*/
            if (
                DList_IsListEmpty(&channel_internal_ptr->op_list) ||
                CONTAINING_RECORD(channel_internal_ptr->op_list.Flink, CHANNEL_OP, anchor)->on_data_available_cb != NULL
                )
            {
                if (enqueue_operation(channel_internal, out_op_pull, correlation_id, on_data_available_cb, pull_context, NULL, NULL, NULL, NULL) != 0)
                {
                    /*Codes_SRS_CHANNEL_INTERNAL_43_023: [ If there are any failures, channel_internal_pull shall fail and return CHANNEL_RESULT_ERROR. ]*/
                    LogError("Failure in enqueue_operation(channel_internal=%p, out_op_pull=%p, correlation_id=%" PRI_RC_STRING ", on_data_available_cb=%p, pull_context=%p, NULL, NULL, NULL, NULL)", channel_internal, out_op_pull, RC_STRING_VALUE_OR_NULL(correlation_id), on_data_available_cb, pull_context);
                    result = CHANNEL_RESULT_ERROR;
                }
                else
                {
                    /*Codes_SRS_CHANNEL_INTERNAL_43_011: [ channel_internal_pull shall succeeds and return CHANNEL_RESULT_OK. ]*/
                    result = CHANNEL_RESULT_OK;
                }
            }
            /*Codes_SRS_CHANNEL_INTERNAL_43_108: [ If the first operation in the list of pending operations contains a non-NULL on_data_consumed_cb: ]*/
            else
            {
                if (dequeue_operation(channel_internal, out_op_pull, correlation_id, on_data_available_cb, pull_context, NULL, NULL, NULL, NULL) != 0)
                {
                    /*Codes_SRS_CHANNEL_INTERNAL_43_023: [ If there are any failures, channel_internal_pull shall fail and return CHANNEL_RESULT_ERROR. ]*/
                    LogError("Failure in dequeue_operation(channel_internal=%p, out_op_pull=%p, correlation_id=%" PRI_RC_STRING ", on_data_available_cb=%p, pull_context=%p, NULL, NULL, NULL, NULL)", channel_internal, out_op_pull, RC_STRING_VALUE_OR_NULL(correlation_id), on_data_available_cb, pull_context);
                    result = CHANNEL_RESULT_ERROR;
                }
                else
                {
                    /* Codes_SRS_CHANNEL_INTERNAL_43_011: [ channel_internal_pull shall succeeds and return CHANNEL_RESULT_OK. ]*/
                    result = CHANNEL_RESULT_OK;
                }
            }
            /*Codes_SRS_CHANNEL_INTERNAL_43_115: [ channel_internal_pull shall call srw_lock_release_exclusive. ]*/
            srw_lock_release_exclusive(channel_internal_ptr->lock);
        }
        result == CHANNEL_RESULT_OK ? (void)0 : sm_exec_end(channel_internal_ptr->sm);
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_internal_push, THANDLE(CHANNEL_INTERNAL), channel_internal, THANDLE(RC_STRING), correlation_id, THANDLE(RC_PTR), data, ON_DATA_CONSUMED_CB, on_data_consumed_cb, void*, push_context, THANDLE(ASYNC_OP)*, out_op_push)
{
    CHANNEL_RESULT result;
    CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_internal);

    /*Codes_SRS_CHANNEL_INTERNAL_43_153: [ channel_internal_push shall call sm_exec_begin. ]*/
    SM_RESULT sm_result = sm_exec_begin(channel_internal_ptr->sm);
    if (sm_result != SM_EXEC_GRANTED)
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_041: [ If there are any failures, channel_internal_push shall fail and return CHANNEL_RESULT_ERROR. ]*/
        LogError("Failure in sm_exec_begin(channel_internal_ptr->sm=%p). SM_RESULT sm_result = %" PRI_MU_ENUM "", channel_internal_ptr->sm, MU_ENUM_VALUE(SM_RESULT, sm_result));
        result = CHANNEL_RESULT_ERROR;
    }
    else
    {

        /*Codes_SRS_CHANNEL_INTERNAL_43_116: [ channel_internal_push shall call srw_lock_acquire_exclusive. ]*/
        srw_lock_acquire_exclusive(channel_internal_ptr->lock);
        {
            /*Codes_SRS_CHANNEL_INTERNAL_43_117: [ If the list of pending operations is empty or the first operation in the list of pending operations contains a non-NULL on_data_consumed_cb: ]*/
            if (
                DList_IsListEmpty(&channel_internal_ptr->op_list) ||
                CONTAINING_RECORD(channel_internal_ptr->op_list.Flink, CHANNEL_OP, anchor)->on_data_consumed_cb != NULL
                )
            {
                if (enqueue_operation(channel_internal, out_op_push, NULL, NULL, NULL, correlation_id, on_data_consumed_cb, push_context, data) != 0)
                {
                    /*Codes_SRS_CHANNEL_INTERNAL_43_041: [ If there are any failures, channel_internal_push shall fail and return CHANNEL_RESULT_ERROR. ]*/
                    LogError("Failure in enqueue_operation(channel_internal=%p, out_op_push=%p, NULL, NULL, NULL, correlation_id=%" PRI_RC_STRING ", on_data_consumed_cb=%p, push_context=%p, data=%p)", channel_internal, out_op_push, RC_STRING_VALUE_OR_NULL(correlation_id), on_data_consumed_cb, push_context, data);
                    result = CHANNEL_RESULT_ERROR;
                }
                else
                {
                    /*Codes_SRS_CHANNEL_INTERNAL_43_132: [ channel_internal_push shall succeed and return CHANNEL_RESULT_OK. ]*/
                    result = CHANNEL_RESULT_OK;
                }
            }
            /*Codes_SRS_CHANNEL_INTERNAL_43_124: [ Otherwise (the first operation in the list of pending operations contains a non-NULL on_data_available_cb): ]*/
            else
            {
                if (dequeue_operation(channel_internal, out_op_push, NULL, NULL, NULL, correlation_id, on_data_consumed_cb, push_context, data) != 0)
                {
                    /*Codes_SRS_CHANNEL_INTERNAL_43_041: [ If there are any failures, channel_internal_push shall fail and return CHANNEL_RESULT_ERROR. ]*/
                    LogError("Failure in dequeue_operation(channel_internal=%p, out_op_push=%p, NULL, NULL, NULL, correlation_id=%" PRI_RC_STRING ", on_data_consumed_cb=%p, push_context=%p, data=%p)", channel_internal, out_op_push, RC_STRING_VALUE_OR_NULL(correlation_id), on_data_consumed_cb, push_context, data);
                    result = CHANNEL_RESULT_ERROR;
                }
                else
                {
                    /*Codes_SRS_CHANNEL_INTERNAL_43_132: [ channel_internal_push shall succeed and return CHANNEL_RESULT_OK. ]*/
                    result = CHANNEL_RESULT_OK;
                }
            }
            /*Codes_SRS_CHANNEL_INTERNAL_43_131: [ channel_internal_push shall call srw_lock_release_exclusive. ]*/
            srw_lock_release_exclusive(channel_internal_ptr->lock);
        }
        result == CHANNEL_RESULT_OK ? (void)0 : sm_exec_end(channel_internal_ptr->sm);
    }

    return result;
}
