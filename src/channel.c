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

#include "../inc/c_util/channel.h"

MU_DEFINE_ENUM_STRINGS(CHANNEL_RESULT, CHANNEL_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_VALUES);

typedef struct CHANNEL_TAG
{
    THANDLE(THREADPOOL) threadpool;
    SM_HANDLE sm;
    bool is_open;
    SRW_LOCK_HANDLE lock;
    DLIST_ENTRY op_list;
    THANDLE(PTR(LOG_CONTEXT_HANDLE)) log_context;
}CHANNEL;

THANDLE_TYPE_DEFINE(CHANNEL);

typedef struct CHANNEL_OP_TAG
{
    ON_DATA_CONSUMED_CB on_data_consumed_cb;
    ON_DATA_AVAILABLE_CB on_data_available_cb;
    void* on_data_consumed_context;
    void* on_data_available_context;
    THANDLE(RC_PTR) data;
    DLIST_ENTRY anchor;

    THANDLE(RC_STRING) push_correlation_id;
    THANDLE(RC_STRING) pull_correlation_id;

    THANDLE(CHANNEL) channel;
    THANDLE(ASYNC_OP) async_op; // self reference to keep op alive even after user disposes

    CHANNEL_CALLBACK_RESULT result;
}CHANNEL_OP;


static void execute_callbacks(void* context);

static void channel_dispose(CHANNEL* channel)
{
    /*Codes_SRS_CHANNEL_43_150: [ channel_dispose shall release the reference to the log_context ]*/
    THANDLE_ASSIGN(PTR(LOG_CONTEXT_HANDLE))(&channel->log_context, NULL);

    /*Codes_SRS_CHANNEL_43_091: [ channel_dispose shall release the reference to THANDLE(THREADPOOL). ]*/
    THANDLE_ASSIGN(THREADPOOL)(&channel->threadpool, NULL);

    /*Codes_SRS_CHANNEL_43_099: [ channel_dispose shall call srw_lock_destroy. ]*/
    srw_lock_destroy(channel->lock);

    /*Codes_SRS_CHANNEL_43_165: [ channel_dispose shall call sm_destroy. ]*/
    sm_destroy(channel->sm);
}


static int copy_entry(PDLIST_ENTRY listEntry, void* actionContext, bool* continueProcessing)
{
    DLIST_ENTRY* op_list = (DLIST_ENTRY*)actionContext;
    DList_InsertTailList(op_list, listEntry);
    *continueProcessing = true;
    return 0;
}

static void abandon_pending_operations(void* context)
{
    CHANNEL* channel_ptr = context;

    DLIST_ENTRY op_list;
    DList_InitializeListHead(&op_list);
    /*Codes_SRS_CHANNEL_43_167: [abandon_pending_operations shall call srw_lock_acquire_exclusive.]*/
    srw_lock_acquire_exclusive(channel_ptr->lock);
    {
        /*Codes_SRS_CHANNEL_43_168: [abandon_pending_operations shall set is_open to false.]*/
        channel_ptr->is_open = false;

        /*Codes_SRS_CHANNEL_43_174: [abandon_pending_operations shall make a local copy of the list of pending operations.]*/
        (void)DList_ForEach(&channel_ptr->op_list, copy_entry, &op_list);

        /*Codes_SRS_CHANNEL_43_175 : [abandon_pending_operations shall set the list of pending operations to an empty list by calling DList_InitializeListHead.]*/
        DList_InitializeListHead(&channel_ptr->op_list);

        /*Codes_SRS_CHANNEL_43_169: [ abandon_pending_operations shall call srw_lock_release_exclusive. ]*/
        srw_lock_release_exclusive(channel_ptr->lock);
    }

    /*Codes_SRS_CHANNEL_43_095: [ abandon_pending_operations shall iterate over the local copy and do the following: ]*/
    for (DLIST_ENTRY* entry = DList_RemoveHeadList(&op_list); entry != &op_list; entry = DList_RemoveHeadList(&op_list))
    {
        CHANNEL_OP* channel_op = CONTAINING_RECORD(entry, CHANNEL_OP, anchor);

        /*Codes_SRS_CHANNEL_43_096: [ set the result of the operation to CHANNEL_CALLBACK_RESULT_ABANDONED. ]*/
        channel_op->result = CHANNEL_CALLBACK_RESULT_ABANDONED;

        /*Codes_SRS_CHANNEL_43_097: [ call execute_callbacks with the operation as context.]*/
        execute_callbacks(channel_op);
    }
}

void channel_close(THANDLE(CHANNEL) channel)
{
    if (channel == NULL)
    {
        /*Codes_SRS_CHANNEL_18_001: [ If channel is NULL, channel_close shall return immediately. ]*/
        // do nothing
    }
    else
    {
        CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);

        /*Codes_SRS_CHANNEL_43_094: [ channel_close shall call sm_close_begin_with_cb with abandon_pending_operation as the callback. ]*/
        SM_RESULT sm_close_begin_result = sm_close_begin_with_cb(channel_ptr->sm, abandon_pending_operations, channel_ptr, NULL, NULL);
        if (sm_close_begin_result != SM_EXEC_GRANTED)
        {
            LogError("Failure in sm_close_begin_with_cb(channel_ptr->sm=%p, abandon_pending_operations=%p, channel_ptr=%p, NULL, NULL). SM_RESULT sm_close_begin_result = %" PRI_MU_ENUM "", channel_ptr->sm, abandon_pending_operations, channel_ptr, MU_ENUM_VALUE(SM_RESULT, sm_close_begin_result));
        }
        else
        {
            /*Codes_SRS_CHANNEL_43_100: [ channel_close shall call sm_close_end. ]*/
            sm_close_end(channel_ptr->sm);
        }
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CHANNEL), channel_create, THANDLE(PTR(LOG_CONTEXT_HANDLE)), log_context, THANDLE(THREADPOOL), threadpool)
{
    THANDLE(CHANNEL) result = NULL;

    /*Codes_SRS_CHANNEL_43_151: [ channel_create shall call sm_create. ]*/
    SM_HANDLE sm = sm_create("channel");
    if (sm == NULL)
    {
        /*Codes_SRS_CHANNEL_43_002: [ If there are any failures, channel_create shall fail and return NULL. ]*/
        LogError("Failure in sm_create(\"channel\")");
    }
    else
    {
        /*Codes_SRS_CHANNEL_43_098: [ channel_create shall call srw_lock_create. ]*/
        SRW_LOCK_HANDLE lock = srw_lock_create(false, "channel");
        if (lock == NULL)
        {
            /*Codes_SRS_CHANNEL_43_002: [ If there are any failures, channel_create shall fail and return NULL. ]*/
            LogError("Failure in srw_lock_create(false, \"channel\")");
        }
        else
        {
            /*Codes_SRS_CHANNEL_43_078: [ channel_create shall create a CHANNEL object by calling THANDLE_MALLOC with channel_dispose as dispose.]*/
            THANDLE(CHANNEL) channel = THANDLE_MALLOC(CHANNEL)(channel_dispose);
            if (channel == NULL)
            {
                /*Codes_SRS_CHANNEL_43_002: [ If there are any failures, channel_create shall fail and return NULL. ]*/
                LogError("Failure in THANDLE_MALLOC(CHANNEL)(channel_dispose=%p)", channel_dispose);
            }
            else
            {
                CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);

                channel_ptr->sm = sm;
                channel_ptr->lock = lock;

                /*Codes_SRS_CHANNEL_43_080: [ channel_create shall store given threadpool in the created CHANNEL. ]*/
                THANDLE_INITIALIZE(THREADPOOL)(&channel_ptr->threadpool, threadpool);

                /*Codes_SRS_CHANNEL_43_149: [ channel_create shall store the given log_context in the created CHANNEL. ]*/
                THANDLE_INITIALIZE(PTR(LOG_CONTEXT_HANDLE))(&channel_ptr->log_context, log_context);

                /*Codes_SRS_CHANNEL_43_084: [ channel_create shall call DList_InitializeListHead. ]*/
                DList_InitializeListHead(&channel_ptr->op_list);

                /*Codes_SRS_CHANNEL_43_086: [ channel_create shall succeed and return the created THANDLE(CHANNEL). ]*/
                THANDLE_INITIALIZE_MOVE(CHANNEL)(&result, &channel);
                goto all_ok;
            }
            srw_lock_destroy(lock);
        }
        sm_destroy(sm);
    }
all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, channel_open, THANDLE(CHANNEL), channel)
{
    int result;
    /*Codes_SRS_CHANNEL_43_159: [channel_open shall call sm_open_begin.]*/
    SM_RESULT sm_result = sm_open_begin(channel->sm);
    if (sm_result != SM_EXEC_GRANTED)
    {
        /*Codes_SRS_CHANNEL_43_161 : [If there are any failures, channel_open shall fail and return a non-zero value.]*/
        LogError("Failure in sm_open_begin(channel->sm=%p). SM_RESULT sm_result = %" PRI_MU_ENUM "", channel->sm, MU_ENUM_VALUE(SM_RESULT, sm_result));
        result = MU_FAILURE;
    }
    else
    {
        CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);

        /*Codes_SRS_CHANNEL_43_172: [ channel_open shall call srw_lock_acquire_exclusive. ]*/
        srw_lock_acquire_exclusive(channel_ptr->lock);

        /*Codes_SRS_CHANNEL_43_166: [ channel_open shall set is_open to true. ]*/
        channel_ptr->is_open = true;

        /*Codes_SRS_CHANNEL_43_173: [ channel_open shall call srw_lock_release_exclusive. ]*/
        srw_lock_release_exclusive(channel_ptr->lock);

        /*Codes_SRS_CHANNEL_43_160 : [channel_open shall call sm_open_end.]*/
        sm_open_end(channel->sm, true);
        /*Codes_SRS_CHANNEL_43_162 : [channel_open shall succeed and return 0.]*/
        result = 0;
    }
    return result;
}

static void execute_callbacks(void* context)
{
    /*Codes_SRS_CHANNEL_43_148: [ If channel_op_context is NULL, execute_callbacks shall terminate the process. ]*/
    if (context == NULL)
    {
        /*Codes_SRS_CHANNEL_43_148: [ If channel_op_context is NULL, execute_callbacks shall terminate the process. ]*/
        LogCriticalAndTerminate("Invalid arguments: void* context=%p", context);
    }
    else
    {
        CHANNEL_OP* channel_op = (CHANNEL_OP*)context;
        CHANNEL_CALLBACK_RESULT result = channel_op->result; // local copy to make sure both callbacks are called with the same result

        LOGGER_LOG(LOG_LEVEL_VERBOSE, T_PTR_VALUE_OR_NULL(channel_op->channel->log_context), "Executing callbacks for pull_correlation_id=%" PRI_RC_STRING ", push_correlation_id=%" PRI_RC_STRING ", callback_result=%" PRI_MU_ENUM "", RC_STRING_VALUE_OR_NULL(channel_op->pull_correlation_id), RC_STRING_VALUE_OR_NULL(channel_op->push_correlation_id), MU_ENUM_VALUE(CHANNEL_CALLBACK_RESULT, result));

        // A channel_op might have 2 callbacks, so it might have 2 sm_exec_begin ref's outstanding.
        // We have to call sm_exec_end for both of them before we call our first callback.
        // Otherwise, if one of the callbacks calls channel_close, it might deadlock waiting for an sm_exec_end.

        if (channel_op->on_data_available_cb != NULL)
        {
            sm_exec_end(channel_op->channel->sm);
        }
        if (channel_op->on_data_consumed_cb != NULL)
        {
            sm_exec_end(channel_op->channel->sm);
        }

        /*Codes_SRS_CHANNEL_43_145: [ execute_callbacks shall call the stored callback(s) with the result of the operation. ]*/
        if (channel_op->on_data_available_cb != NULL)
        {
            /*Codes_SRS_CHANNEL_43_157: [ execute_callbacks shall call sm_exec_end for each callback that is called. ]*/
            channel_op->on_data_available_cb(channel_op->on_data_available_context, result, channel_op->pull_correlation_id, channel_op->push_correlation_id, channel_op->data);
        }
        if (channel_op->on_data_consumed_cb != NULL)
        {
            /*Codes_SRS_CHANNEL_43_157: [ execute_callbacks shall call sm_exec_end for each callback that is called. ]*/
            channel_op->on_data_consumed_cb(channel_op->on_data_consumed_context, result, channel_op->pull_correlation_id, channel_op->push_correlation_id);
        }

        /*Codes_SRS_CHANNEL_43_147: [ execute_callbacks shall perform cleanup of the operation. ]*/
        THANDLE_ASSIGN(RC_STRING)(&channel_op->pull_correlation_id, NULL);
        THANDLE_ASSIGN(RC_STRING)(&channel_op->push_correlation_id, NULL);
        THANDLE_ASSIGN(RC_PTR)(&channel_op->data, NULL);
        THANDLE_ASSIGN(CHANNEL)(&channel_op->channel, NULL);

        // copy to local reference to avoid cutting the branch you are sitting on
        THANDLE(ASYNC_OP) temp = NULL;
        THANDLE_INITIALIZE_MOVE(ASYNC_OP)(&temp, &channel_op->async_op);
        THANDLE_ASSIGN(ASYNC_OP)(&temp, NULL);
    }
}

static void cancel_op(void* context)
{
    CHANNEL_OP* channel_op = context;
    CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel_op->channel);

    bool call_callback = false;

    /*Codes_SRS_CHANNEL_43_154: [ cancel_op shall call sm_exec_begin. ]*/
    SM_RESULT sm_result = sm_exec_begin(channel_ptr->sm);
    if (sm_result != SM_EXEC_GRANTED)
    {
        /*Codes_SRS_CHANNEL_43_155: [ If there are any failures, cancel_op shall fail. ]*/
        LogError("Failure in sm_exec_begin(channel_ptr->sm=%p). SM_RESULT sm_result = %" PRI_MU_ENUM "", channel_ptr->sm, MU_ENUM_VALUE(SM_RESULT, sm_result));
    }
    else
    {
        /*Codes_SRS_CHANNEL_43_134: [ cancel_op shall call srw_lock_acquire_exclusive. ]*/
        srw_lock_acquire_exclusive(channel_ptr->lock);
        {
            /*Codes_SRS_CHANNEL_43_135: [ If the operation is in the list of pending operations, cancel_op shall call DList_RemoveEntryList to remove it. ]*/
            if (
                (channel_op->anchor.Flink != &channel_op->anchor) &&
                (&channel_op->anchor == channel_op->anchor.Flink->Blink)
                )
            {
                (void)DList_RemoveEntryList(&channel_op->anchor);
                call_callback = true;
            }
            /*Codes_SRS_CHANNEL_43_139: [ cancel_op shall call srw_lock_release_exclusive. ]*/
            srw_lock_release_exclusive(channel_ptr->lock);
        }

        /*Codes_SRS_CHANNEL_43_136: [ If the result of the operation is CHANNEL_CALLBACK_RESULT_OK, cancel_op shall set it to CHANNEL_CALLBACK_RESULT_CANCELLED. ]*/
        if (channel_op->result == CHANNEL_CALLBACK_RESULT_OK)
        {
            channel_op->result = CHANNEL_CALLBACK_RESULT_CANCELLED;
        }

        /*Codes_SRS_CHANNEL_43_138: [ If the operation had been found in the list of pending operations, cancel_op shall call threadpool_schedule_work with execute_callbacks as work_function and the operation as work_function_context. ]*/
        if (call_callback)
        {
            if (threadpool_schedule_work(channel_ptr->threadpool, execute_callbacks, channel_op) != 0)
            {
                LogError("Failure in threadpool_schedule_work(channel_ptr->threadpool=%p, execute_callbacks=%p, channel_op=%p)", channel_ptr->threadpool, execute_callbacks, channel_op);
            }
            else
            {
                /* all ok */
            }
        }

        /*Codes_SRS_CHANNEL_43_156: [ cancel_op shall call sm_exec_end. ]*/
        sm_exec_end(channel_ptr->sm);
    }
}

static void dispose_channel_op(void* context)
{
    /* don't need to do anything here because actual cleanup happens in execute_callbacks */
    (void)context;
}

static int enqueue_operation(THANDLE(CHANNEL) channel, THANDLE(ASYNC_OP)* out_op, THANDLE(RC_STRING) pull_correlation_id, ON_DATA_AVAILABLE_CB on_data_available_cb, void* on_data_available_context, THANDLE(RC_STRING) push_correlation_id, ON_DATA_CONSUMED_CB on_data_consumed_cb, void* on_data_consumed_context, THANDLE(RC_PTR) data)
{
    int result;

    /*Codes_SRS_CHANNEL_43_103: [ channel_pull shall create a THANDLE(ASYNC_OP) by calling async_op_create with cancel_op as cancel. ]*/
    /*Codes_SRS_CHANNEL_43_119: [ channel_push shall create a THANDLE(ASYNC_OP) by calling async_op_create with cancel_op as cancel. ]*/
    THANDLE(ASYNC_OP) async_op = async_op_create(cancel_op, sizeof(CHANNEL_OP), alignof(CHANNEL_OP), dispose_channel_op);
    if (async_op == NULL)
    {
        LogError("Failure in async_op_create(cancel_op=%p, sizeof(CHANNEL_OP)=%zu, alignof(CHANNEL_OP)=%zu, dispose_channel_op=%p)", cancel_op, sizeof(CHANNEL_OP), alignof(CHANNEL_OP), dispose_channel_op);
        result = MU_FAILURE;
    }
    else
    {
        CHANNEL_OP* channel_op = async_op->context;

        /*Codes_SRS_CHANNEL_43_104: [ channel_pull shall store the correlation_id, on_data_available_cb and on_data_available_context in the THANDLE(ASYNC_OP). ]*/
        THANDLE_INITIALIZE(RC_STRING)(&channel_op->pull_correlation_id, pull_correlation_id);
        channel_op->on_data_available_cb = on_data_available_cb;
        channel_op->on_data_available_context = on_data_available_context;

        /*Codes_SRS_CHANNEL_43_120: [ channel_push shall store the correlation_id, on_data_consumed_cb, on_data_consumed_context and data in the THANDLE(ASYNC_OP). ]*/
        THANDLE_INITIALIZE(RC_STRING)(&channel_op->push_correlation_id, push_correlation_id);
        channel_op->on_data_consumed_cb = on_data_consumed_cb;
        channel_op->on_data_consumed_context = on_data_consumed_context;
        THANDLE_INITIALIZE(RC_PTR)(&channel_op->data, data);

        THANDLE_INITIALIZE(ASYNC_OP)(&channel_op->async_op, async_op);
        THANDLE_INITIALIZE(CHANNEL)(&channel_op->channel, channel);

        /*Codes_SRS_CHANNEL_43_111: [ channel_pull shall set the result of the created operation to CHANNEL_CALLBACK_RESULT_OK. ]*/
        /*Codes_SRS_CHANNEL_43_127: [ channel_push shall set the result of the created operation to CHANNEL_CALLBACK_RESULT_OK. ]*/
        channel_op->result = CHANNEL_CALLBACK_RESULT_OK;

        CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);
        /*Codes_SRS_CHANNEL_43_105: [ channel_pull shall insert the created THANDLE(ASYNC_OP) in the list of pending operations by calling DList_InsertTailList. ]*/
        /*Codes_SRS_CHANNEL_43_121: [ channel_push shall insert the created THANDLE(ASYNC_OP) in the list of pending operations by calling DList_InsertTailList. ]*/
        DList_InsertTailList(&channel_ptr->op_list, &channel_op->anchor);

        /*Codes_SRS_CHANNEL_43_107: [ channel_pull shall set *out_op_pull to the created THANDLE(ASYNC_OP). ]*/
        /*Codes_SRS_CHANNEL_43_123: [ channel_push shall set *out_op_push to the created THANDLE(ASYNC_OP). ]*/
        THANDLE_INITIALIZE_MOVE(ASYNC_OP)(out_op, &async_op);
        result = 0;
    }
    return result;
}

static int dequeue_operation(THANDLE(CHANNEL) channel, THANDLE(ASYNC_OP)* out_op, THANDLE(RC_STRING) pull_correlation_id, ON_DATA_AVAILABLE_CB on_data_available_cb, void* on_data_available_context, THANDLE(RC_STRING) push_correlation_id, ON_DATA_CONSUMED_CB on_data_consumed_cb, void* on_data_consumed_context, THANDLE(RC_PTR) data)
{
    int result;

    CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);

    /*Codes_SRS_CHANNEL_43_109: [ channel_pull shall call DList_RemoveHeadList on the list of pending operations to obtain the operation. ]*/
    /*Codes_SRS_CHANNEL_43_125: [ channel_push shall call DList_RemoveHeadList on the list of pending operations to obtain the operation. ]*/
    PDLIST_ENTRY op_entry = DList_RemoveHeadList(&channel_ptr->op_list);

    CHANNEL_OP* channel_op = CONTAINING_RECORD(op_entry, CHANNEL_OP, anchor);
    if (on_data_available_cb != NULL)
    {
        /*Codes_SRS_CHANNEL_43_112: [ channel_pull shall store the correlation_id, on_data_available_cb and on_data_available_context in the obtained operation. ]*/
        THANDLE_INITIALIZE(RC_STRING)(&channel_op->pull_correlation_id, pull_correlation_id);
        channel_op->on_data_available_cb = on_data_available_cb;
        channel_op->on_data_available_context = on_data_available_context;
    }
    else if (on_data_consumed_cb != NULL)
    {
        /*Codes_SRS_CHANNEL_43_128: [ channel_push shall store the correlation_id, on_data_consumed_cb, on_data_consumed_context and data in the obtained operation. ]*/
        THANDLE_INITIALIZE(RC_STRING)(&channel_op->push_correlation_id, push_correlation_id);
        channel_op->on_data_consumed_cb = on_data_consumed_cb;
        channel_op->on_data_consumed_context = on_data_consumed_context;
        THANDLE_ASSIGN(RC_PTR)(&channel_op->data, data);
    }
    THANDLE(ASYNC_OP) temp = NULL;
    THANDLE_INITIALIZE(ASYNC_OP)(&temp, channel_op->async_op);
    /*Codes_SRS_CHANNEL_43_113: [ channel_pull shall call threadpool_schedule_work with execute_callbacks as work_function and the obtained operation as work_function_context. ]*/
    /*Codes_SRS_CHANNEL_43_129: [ channel_push shall call threadpool_schedule_work with execute_callbacks as work_function and the obtained operation as work_function_context. ]*/
    if (threadpool_schedule_work(channel_ptr->threadpool, execute_callbacks, channel_op) != 0)
    {
        LogError("Failure in threadpool_schedule_work(channel_ptr->threadpool=%p, execute_callbacks=%p, channel_op=%p)", channel_ptr->threadpool, execute_callbacks, channel_op);
        result = MU_FAILURE;
        // execute_callbacks calls sm_exec_end once for each non-NULL callback.
        // In the case where threadpool_schedule_work fails, sm_exec_end is called immediately.Therefore the callback must be unset so that sm_exec_end is not called twice.
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
        /*Codes_SRS_CHANNEL_43_114: [ channel_pull shall set *out_op_pull to the THANDLE(ASYNC_OP) of the obtained operation. ]*/
        /*Codes_SRS_CHANNEL_43_130: [ channel_push shall set *out_op_push to the THANDLE(ASYNC_OP) of the obtained operation. ]*/
        THANDLE_INITIALIZE(ASYNC_OP)(out_op, temp);
        result = 0;
    }
    THANDLE_ASSIGN(ASYNC_OP)(&temp, NULL);
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_pull, THANDLE(CHANNEL), channel, THANDLE(RC_STRING), correlation_id, ON_DATA_AVAILABLE_CB, on_data_available_cb, void*, on_data_available_context, THANDLE(ASYNC_OP)*, out_op_pull)
{
    CHANNEL_RESULT result;
    CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);

    /*Codes_SRS_CHANNEL_43_152: [ channel_pull shall call sm_exec_begin. ]*/
    SM_RESULT sm_result = sm_exec_begin(channel_ptr->sm);
    if (sm_result != SM_EXEC_GRANTED)
    {
        /*Codes_SRS_CHANNEL_43_023: [ If there are any failures, channel_pull shall fail and return CHANNEL_RESULT_ERROR. ]*/
        LogError("Failure in sm_exec_begin(channel_ptr->sm=%p). SM_RESULT sm_result = %" PRI_MU_ENUM "", channel_ptr->sm, MU_ENUM_VALUE(SM_RESULT, sm_result));
        result = CHANNEL_RESULT_ERROR;
    }
    else
    {
        /*Codes_SRS_CHANNEL_43_010: [ channel_pull shall call srw_lock_acquire_exclusive. ]*/
        srw_lock_acquire_exclusive(channel_ptr->lock);
        {
            /*Codes_SRS_CHANNEL_43_170: [ channel_pull shall check if is_open is true. ]*/
            if (!channel_ptr->is_open)
            {
                /*Codes_SRS_CHANNEL_43_023: [ If there are any failures, channel_pull shall fail and return CHANNEL_RESULT_ERROR. ]*/
                LogError("channel is closing, cannot start a new channel_pull operation");
                result = CHANNEL_RESULT_ERROR;
            }
            else
            {
                /*Codes_SRS_CHANNEL_43_101: [ If the list of pending operations is empty or the first operation in the list of pending operations contains a non-NULL on_data_available_cb: ]*/
                if (
                    DList_IsListEmpty(&channel_ptr->op_list) ||
                    CONTAINING_RECORD(channel_ptr->op_list.Flink, CHANNEL_OP, anchor)->on_data_available_cb != NULL
                    )
                {
                    if (enqueue_operation(channel, out_op_pull, correlation_id, on_data_available_cb, on_data_available_context, NULL, NULL, NULL, NULL) != 0)
                    {
                        /*Codes_SRS_CHANNEL_43_023: [ If there are any failures, channel_pull shall fail and return CHANNEL_RESULT_ERROR. ]*/
                        LogError("Failure in enqueue_operation(channel=%p, out_op_pull=%p, correlation_id=%" PRI_RC_STRING ", on_data_available_cb=%p, on_data_available_context=%p, NULL, NULL, NULL, NULL)", channel, out_op_pull, RC_STRING_VALUE_OR_NULL(correlation_id), on_data_available_cb, on_data_available_context);
                        result = CHANNEL_RESULT_ERROR;
                    }
                    else
                    {
                        /*Codes_SRS_CHANNEL_43_011: [ channel_pull shall succeeds and return CHANNEL_RESULT_OK. ]*/
                        result = CHANNEL_RESULT_OK;
                    }
                }
                /*Codes_SRS_CHANNEL_43_108: [ If the first operation in the list of pending operations contains a non-NULL on_data_consumed_cb: ]*/
                else
                {
                    if (dequeue_operation(channel, out_op_pull, correlation_id, on_data_available_cb, on_data_available_context, NULL, NULL, NULL, NULL) != 0)
                    {
                        /*Codes_SRS_CHANNEL_43_023: [ If there are any failures, channel_pull shall fail and return CHANNEL_RESULT_ERROR. ]*/
                        LogError("Failure in dequeue_operation(channel=%p, out_op_pull=%p, correlation_id=%" PRI_RC_STRING ", on_data_available_cb=%p, on_data_available_context=%p, NULL, NULL, NULL, NULL)", channel, out_op_pull, RC_STRING_VALUE_OR_NULL(correlation_id), on_data_available_cb, on_data_available_context);
                        result = CHANNEL_RESULT_ERROR;
                    }
                    else
                    {
                        /* Codes_SRS_CHANNEL_43_011: [ channel_pull shall succeeds and return CHANNEL_RESULT_OK. ]*/
                        result = CHANNEL_RESULT_OK;
                    }
                }
            }
            /*Codes_SRS_CHANNEL_43_115: [ channel_pull shall call srw_lock_release_exclusive. ]*/
            srw_lock_release_exclusive(channel_ptr->lock);
        }
        result == CHANNEL_RESULT_OK ? (void)0 : sm_exec_end(channel_ptr->sm);
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_push, THANDLE(CHANNEL), channel, THANDLE(RC_STRING), correlation_id, THANDLE(RC_PTR), data, ON_DATA_CONSUMED_CB, on_data_consumed_cb, void*, on_data_consumed_context, THANDLE(ASYNC_OP)*, out_op_push)
{
    CHANNEL_RESULT result;
    CHANNEL* channel_ptr = THANDLE_GET_T(CHANNEL)(channel);

    /*Codes_SRS_CHANNEL_43_153: [ channel_push shall call sm_exec_begin. ]*/
    SM_RESULT sm_result = sm_exec_begin(channel_ptr->sm);
    if (sm_result != SM_EXEC_GRANTED)
    {
        /*Codes_SRS_CHANNEL_43_041: [ If there are any failures, channel_push shall fail and return CHANNEL_RESULT_ERROR. ]*/
        LogError("Failure in sm_exec_begin(channel_ptr->sm=%p). SM_RESULT sm_result = %" PRI_MU_ENUM "", channel_ptr->sm, MU_ENUM_VALUE(SM_RESULT, sm_result));
        result = CHANNEL_RESULT_ERROR;
    }
    else
    {

        /*Codes_SRS_CHANNEL_43_116: [ channel_push shall call srw_lock_acquire_exclusive. ]*/
        srw_lock_acquire_exclusive(channel_ptr->lock);
        {
            /*Codes_SRS_CHANNEL_43_171: [ channel_push shall check if is_open is true. ]*/
            if (!channel_ptr->is_open)
            {
                /*Codes_SRS_CHANNEL_43_041: [ If there are any failures, channel_push shall fail and return CHANNEL_RESULT_ERROR. ]*/
                LogError("channel is closing, cannot start a new channel_push operation");
                result = CHANNEL_RESULT_ERROR;
            }
            else
            {
                /*Codes_SRS_CHANNEL_43_117: [ If the list of pending operations is empty or the first operation in the list of pending operations contains a non-NULL on_data_consumed_cb: ]*/
                if (
                    DList_IsListEmpty(&channel_ptr->op_list) ||
                    CONTAINING_RECORD(channel_ptr->op_list.Flink, CHANNEL_OP, anchor)->on_data_consumed_cb != NULL
                    )
                {
                    if (enqueue_operation(channel, out_op_push, NULL, NULL, NULL, correlation_id, on_data_consumed_cb, on_data_consumed_context, data) != 0)
                    {
                        /*Codes_SRS_CHANNEL_43_041: [ If there are any failures, channel_push shall fail and return CHANNEL_RESULT_ERROR. ]*/
                        LogError("Failure in enqueue_operation(channel=%p, out_op_push=%p, NULL, NULL, NULL, correlation_id=%" PRI_RC_STRING ", on_data_consumed_cb=%p, on_data_consumed_context=%p, data=%p)", channel, out_op_push, RC_STRING_VALUE_OR_NULL(correlation_id), on_data_consumed_cb, on_data_consumed_context, data);
                        result = CHANNEL_RESULT_ERROR;
                    }
                    else
                    {
                        /*Codes_SRS_CHANNEL_43_132: [ channel_push shall succeed and return CHANNEL_RESULT_OK. ]*/
                        result = CHANNEL_RESULT_OK;
                    }
                }
                /*Codes_SRS_CHANNEL_43_124: [ Otherwise (the first operation in the list of pending operations contains a non-NULL on_data_available_cb): ]*/
                else
                {
                    if (dequeue_operation(channel, out_op_push, NULL, NULL, NULL, correlation_id, on_data_consumed_cb, on_data_consumed_context, data) != 0)
                    {
                        /*Codes_SRS_CHANNEL_43_041: [ If there are any failures, channel_push shall fail and return CHANNEL_RESULT_ERROR. ]*/
                        LogError("Failure in dequeue_operation(channel=%p, out_op_push=%p, NULL, NULL, NULL, correlation_id=%" PRI_RC_STRING ", on_data_consumed_cb=%p, on_data_consumed_context=%p, data=%p)", channel, out_op_push, RC_STRING_VALUE_OR_NULL(correlation_id), on_data_consumed_cb, on_data_consumed_context, data);
                        result = CHANNEL_RESULT_ERROR;
                    }
                    else
                    {
                        /*Codes_SRS_CHANNEL_43_132: [ channel_push shall succeed and return CHANNEL_RESULT_OK. ]*/
                        result = CHANNEL_RESULT_OK;
                    }
                }
            }
            /*Codes_SRS_CHANNEL_43_131: [ channel_push shall call srw_lock_release_exclusive. ]*/
            srw_lock_release_exclusive(channel_ptr->lock);
        }
        result == CHANNEL_RESULT_OK ? (void)0 : sm_exec_end(channel_ptr->sm);
    }

    return result;
}
