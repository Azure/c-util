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

#include "c_util/doublylinkedlist.h"
#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"

#include "../inc/c_util/channel_internal.h"

typedef struct CHANNEL_INTERNAL_TAG
{
    THANDLE(THREADPOOL) threadpool;
    SRW_LOCK_HANDLE lock;
    DLIST_ENTRY op_list;
}
CHANNEL_INTERNAL;
THANDLE_TYPE_DEFINE(CHANNEL_INTERNAL);

typedef struct CHANNEL_OP_TAG
{
    PUSH_CALLBACK push_callback;
    PULL_CALLBACK pull_callback;
    void* push_context;
    void* pull_context;
    THANDLE(RC_PTR) data;
    DLIST_ENTRY anchor;

    THANDLE(CHANNEL_INTERNAL) channel_internal;
    THANDLE(ASYNC_OP) async_op; // self reference to keep op alive even after user disposes (watch out for deadlocks)

    CHANNEL_CALLBACK_RESULT result;
}CHANNEL_OP;


static void execute_callbacks(void* context);

static void channel_internal_dispose(CHANNEL_INTERNAL* channel_internal)
{
    /*Codes_SRS_CHANNEL_INTERNAL_43_099: [ channel_internal_dispose shall call srw_lock_destroy. ]*/
    srw_lock_destroy(channel_internal->lock);

    /*Codes_SRS_CHANNEL_INTERNAL_43_091: [ channel_internal_dispose shall release the reference to THANDLE(THREADPOOL). ]*/
    THANDLE_ASSIGN(THREADPOOL)(&channel_internal->threadpool, NULL);
}


void channel_internal_close(THANDLE(CHANNEL_INTERNAL) channel_internal)
{
    CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_internal);

    /*Codes_SRS_CHANNEL_INTERNAL_43_094: [ channel_dispose shall call srw_lock_acquire_exclusive. ]*/
    srw_lock_acquire_exclusive(channel_internal_ptr->lock);

    /*Codes_SRS_CHANNEL_INTERNAL_43_095: [ channel_dispose shall iterate over the list of pending operations and do the following: ]*/

    for (DLIST_ENTRY* entry = DList_RemoveHeadList(&channel_internal_ptr->op_list); entry != &channel_internal_ptr->op_list; entry = DList_RemoveHeadList(&channel_internal_ptr->op_list))
    {
        CHANNEL_OP* channel_op = CONTAINING_RECORD(entry, CHANNEL_OP, anchor);

        /*Codes_SRS_CHANNEL_INTERNAL_43_096: [ set the result of the operation to CHANNEL_CALLBACK_RESULT_ABANDONED. ]*/
        channel_op->result = CHANNEL_CALLBACK_RESULT_ABANDONED;

        /*Codes_SRS_CHANNEL_INTERNAL_43_097: [ call threadpool_schedule_work with execute_callbacks as work_function. ]*/
        if (threadpool_schedule_work(channel_internal_ptr->threadpool, execute_callbacks, channel_op) != 0)
        {
            LogError("Failure in threadpool_schedule_work(execute_callbacks, channel_op)");
        }
    }

    /*Codes_SRS_CHANNEL_INTERNAL_43_100: [ channel_dispose shall call srw_lock_release_exclusive. ]*/
    srw_lock_release_exclusive(channel_internal_ptr->lock);
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CHANNEL_INTERNAL), channel_internal_create_and_open, THANDLE(THREADPOOL), threadpool)
{
    THANDLE(CHANNEL_INTERNAL) result = NULL;


    /*Codes_SRS_CHANNEL_INTERNAL_43_098: [ channel_create shall call srw_lock_create. ]*/
    SRW_LOCK_HANDLE lock = srw_lock_create(false, "channel");
    if (lock == NULL)
    {
        /*Codes_SRS_CHANNEL_43_002: [ If there are any failures, channel_create shall fail and return NULL. ]*/
        LogError("Failure in srw_lock_create(false, \"channel\")");
    }
    else
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_078: [ channel_create shall create a CHANNEL_INTERNAL object by calling THANDLE_MALLOC with channel_internal_dispose as dispose.]*/
        THANDLE(CHANNEL_INTERNAL) channel_internal = THANDLE_MALLOC(CHANNEL_INTERNAL)(channel_internal_dispose);
        if (channel_internal == NULL)
        {
            /*Codes_SRS_CHANNEL_43_002: [ If there are any failures, channel_create shall fail and return NULL. ]*/
            LogError("Failure in THANDLE_MALLOC(CHANNEL_INTERNAL)(channel_internal_dispose)");
        }
        else
        {
            CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_internal);

            channel_internal_ptr->lock = lock;

            /*Codes_SRS_CHANNEL_INTERNAL_43_080: [ channel_create shall store given threadpool in the created CHANNEL_INTERNAL. ]*/
            THANDLE_INITIALIZE(THREADPOOL)(&channel_internal_ptr->threadpool, threadpool);

            /*Codes_SRS_CHANNEL_INTERNAL_43_084: [ channel_create shall call DList_InitializeListHead. ]*/
            DList_InitializeListHead(&channel_internal_ptr->op_list);

            /*Codes_SRS_CHANNEL_INTERNAL_43_086: [ channel_create shall succeed and return the created THANDLE(CHANNEL). ]*/
            THANDLE_INITIALIZE_MOVE(CHANNEL_INTERNAL)(&result, &channel_internal);
            goto all_ok;
        }
        srw_lock_destroy(lock);
    }
all_ok:
    return result;
}

static void execute_callbacks(void* context)
{
    if (context == NULL)
    {
        LogError("Invalid arguments: void* context=%p", context);
    }
    else
    {
        CHANNEL_OP* channel_op = (CHANNEL_OP*)context;

        /*Codes_SRS_CHANNEL_INTERNAL_43_145: [ execute_callbacks shall call the stored callback(s) with the result of the operation. ]*/
        if (channel_op->pull_callback)
        {
            channel_op->pull_callback(channel_op->pull_context, channel_op->result, channel_op->data);
        }
        if (channel_op->push_callback)
        {
            channel_op->push_callback(channel_op->push_context, channel_op->result);
        }

        /*Codes_SRS_CHANNEL_INTERNAL_43_147: [ execute_callbacks shall perform cleanup of the operation. ]*/
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

    /*Codes_SRS_CHANNEL_INTERNAL_43_134: [ cancel_op shall call srw_lock_acquire_exclusive. ]*/
    srw_lock_acquire_exclusive(channel_internal_ptr->lock);
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_135: [ If the operation is in the list of pending operations: ]*/
        if (channel_op->anchor.Flink != &channel_op->anchor && &channel_op->anchor == channel_op->anchor.Flink->Blink)
        {
            /*Codes_SRS_CHANNEL_INTERNAL_43_137: [ cancel_op shall call DList_RemoveEntryList to remove the operation from the list of pending operations. ]*/
            (void)DList_RemoveEntryList(&channel_op->anchor);

            /*Codes_SRS_CHANNEL_INTERNAL_43_136: [ cancel_op shall set the result of the operation to CHANNEL_CALLBACK_RESULT_CANCELLED. ]*/
            channel_op->result = CHANNEL_CALLBACK_RESULT_CANCELLED;

            /*Codes_SRS_CHANNEL_INTERNAL_43_138: [ cancel_op shall call threadpool_schedule_work with execute_callbacks as work_function and the operation as work_function_context. ]*/
            if (threadpool_schedule_work(channel_internal_ptr->threadpool, execute_callbacks, channel_op) != 0)
            {
                LogError("Failure in threadpool_schedule_work(execute_callbacks, channel_op)");
            }
        }
        /*Codes_SRS_CHANNEL_INTERNAL_43_146: [ If theoperation is not in the list of pending operations and the result of the operation is CHANNEL_CALLBACK_RESULT_OK, cancel_op shall set the result of the operation to CHANNEL_CALLBACK_RESULT_CANCELLED. ]*/
        else if (channel_op->result == CHANNEL_CALLBACK_RESULT_OK)
        {
            channel_op->result = CHANNEL_CALLBACK_RESULT_CANCELLED;
        }
    }
    /*Codes_SRS_CHANNEL_INTERNAL_43_139: [ cancel_op shall call srw_lock_release_exclusive. ]*/
    srw_lock_release_exclusive(channel_internal_ptr->lock);
}

static void dispose_channel_op(void* context)
{
    /* don't need to do anything here because actual cleanup happens in execute_callbacks */
    (void)context;
}

static int enqueue_operation(CHANNEL_INTERNAL* channel_internal, THANDLE(ASYNC_OP)* out_op, PULL_CALLBACK pull_callback, void* pull_context, PUSH_CALLBACK push_callback, void* push_context, THANDLE(RC_PTR) data)
{
    int result;
    /*Codes_SRS_CHANNEL_INTERNAL_43_103: [ channel_internal_pull shall create a THANDLE(ASYNC_OP) by calling async_op_create with cancel_op as cancel. ]*/
    /*Codes_SRS_CHANNEL_INTERNAL_43_119: [ channel_internal_push shall create a THANDLE(ASYNC_OP) by calling async_op_create with cancel_op as cancel. ]*/
    THANDLE(ASYNC_OP) async_op = async_op_create(cancel_op, sizeof(CHANNEL_OP), alignof(CHANNEL_OP), dispose_channel_op);
    if (async_op == NULL)
    {
        LogError("Failure in async_op_create(cancel_op, sizeof(CHANNEL_OP), alignof(CHANNEL_OP), dispose_channel_op)");
        result = MU_FAILURE;
    }
    else
    {
        CHANNEL_OP* channel_op = async_op->context;

        /*Codes_SRS_CHANNEL_INTERNAL_43_104: [ channel_internal_pull shall store the pull_callback and pull_context in the THANDLE(ASYNC_OP). ]*/
        channel_op->pull_callback = pull_callback;
        channel_op->pull_context = pull_context;


        /*Codes_SRS_CHANNEL_INTERNAL_43_120: [ channel_internal_push shall store the push_callback, push_context and data in the THANDLE(ASYNC_OP). ]*/
        channel_op->push_callback = push_callback;
        channel_op->push_context = push_context;

        THANDLE_INITIALIZE(ASYNC_OP)(&channel_op->async_op, async_op);
        THANDLE_INITIALIZE(CHANNEL_INTERNAL)(&channel_op->channel_internal, channel_internal);
        THANDLE_INITIALIZE(RC_PTR)(&channel_op->data, data);

        /*Codes_SRS_CHANNEL_INTERNAL_43_111: [ channel_internal_pull shall set the result of the created operation to CHANNEL_CALLBACK_RESULT_OK. ]*/
        /*Codes_SRS_CHANNEL_INTERNAL_43_127: [ channel_internal_push shall set the result of the created operation to CHANNEL_CALLBACK_RESULT_OK. ]*/
        channel_op->result = CHANNEL_CALLBACK_RESULT_OK;

        /*Codes_SRS_CHANNEL_INTERNAL_43_105: [ channel_internal_pull shall insert the created THANDLE(ASYNC_OP) in the list of pending operations by calling DList_InsertTailList. ]*/
        /*Codes_SRS_CHANNEL_INTERNAL_43_121: [ channel_internal_push shall insert the created THANDLE(ASYNC_OP) in the list of pending operations by calling DList_InsertTailList. ]*/
        DList_InsertTailList(&channel_internal->op_list, &channel_op->anchor);

        /*Codes_SRS_CHANNEL_INTERNAL_43_107: [ channel_internal_pull shall set *out_op_pull to the created THANDLE(ASYNC_OP). ]*/
        /*Codes_SRS_CHANNEL_INTERNAL_43_123: [ channel_internal_push shall set *out_op_push to the created THANDLE(ASYNC_OP). ]*/
        THANDLE_INITIALIZE_MOVE(ASYNC_OP)(out_op, &async_op);
        result = 0;
    }
    return result;
}

static int dequeue_operation(CHANNEL_INTERNAL* channel_internal, THANDLE(ASYNC_OP)* out_op, PULL_CALLBACK pull_callback, void* pull_context, PUSH_CALLBACK push_callback, void* push_context, THANDLE(RC_PTR) data)
{
    int result;

    /*Codes_SRS_CHANNEL_INTERNAL_43_109: [ channel_internal_pull shall call DList_RemoveHeadList on the list of pending operations to obtain the operation. ]*/
    /*Codes_SRS_CHANNEL_INTERNAL_43_125: [ channel_internal_push shall call DList_RemoveHeadList on the list of pending operations to obtain the operation. ]*/
    PDLIST_ENTRY op_entry = DList_RemoveHeadList(&channel_internal->op_list);

    CHANNEL_OP* channel_op = CONTAINING_RECORD(op_entry, CHANNEL_OP, anchor);
    if (pull_callback)
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_112: [ channel_internal_pull shall store the pull_callback and pull_context in the obtained operation. ]*/
        channel_op->pull_callback = pull_callback;
        channel_op->pull_context = pull_context;
    }
    else if (push_callback)
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_128: [ channel_internal_push shall store the push_callback, push_context and data in the obtained operation. ]*/
        channel_op->push_callback = push_callback;
        channel_op->push_context = push_context;
        THANDLE_ASSIGN(RC_PTR)(&channel_op->data, data);
    }

    /*Codes_SRS_CHANNEL_INTERNAL_43_113: [ channel_internal_pull shall call threadpool_schedule_work with execute_callbacks as work_function and the obtained operation as work_function_context. ]*/
    /*Codes_SRS_CHANNEL_INTERNAL_43_129: [ channel_internal_push shall call threadpool_schedule_work with execute_callbacks as work_function and the obtained operation as work_function_context. ]*/
    if (threadpool_schedule_work(channel_internal->threadpool, execute_callbacks, channel_op) != 0)
    {
        LogError("Failure in threadpool_schedule_work(execute_callbacks, channel_op)");
        result = MU_FAILURE;
    }
    else
    {
        /*SRS_CHANNEL_43_114: [ channel_internal_pull shall set *out_op_pull to the THANDLE(ASYNC_OP) of the obtained operation. ]*/
        /*SRS_CHANNEL_43_130: [ channel_internal_push shall set *out_op_push to the THANDLE(ASYNC_OP) of the obtained operation. ]*/
        THANDLE_INITIALIZE(ASYNC_OP)(out_op, channel_op->async_op);
        result = 0;
    }
    return result;
}


IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_internal_pull, THANDLE(CHANNEL_INTERNAL), channel_internal, PULL_CALLBACK, pull_callback, void*, pull_context, THANDLE(ASYNC_OP)*, out_op_pull)
{
    CHANNEL_RESULT result;
    CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_internal);

    /*Codes_SRS_CHANNEL_INTERNAL_43_010: [ channel_internal_pull shall call srw_lock_acquire_exclusive. ]*/
    srw_lock_acquire_exclusive(channel_internal_ptr->lock);
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_101: [ If the list of pending operations is empty or the first operation in the list of pending operations contains a non-NULL pull_callback: ]*/
        if (
            DList_IsListEmpty(&channel_internal_ptr->op_list) ||
            CONTAINING_RECORD(channel_internal_ptr->op_list.Flink, CHANNEL_OP, anchor)->pull_callback != NULL
            )
        {
            if (enqueue_operation(channel_internal_ptr, out_op_pull, pull_callback, pull_context, NULL, NULL, NULL) != 0)
            {
                /*Codes_SRS_CHANNEL_INTERNAL_43_023: [ If there are any failures, channel_internal_pull shall fail and return CHANNEL_RESULT_ERROR. ]*/
                LogError("Failure in enqueue_operation(channel_internal_ptr, out_op_pull, pull_callback, pull_context, NULL, NULL, NULL)");
                result = CHANNEL_RESULT_ERROR;
            }
            else
            {
                /*Codes_SRS_CHANNEL_INTERNAL_43_011: [ channel_internal_pull shall succeeds and return CHANNEL_RESULT_OK. ]*/
                result = CHANNEL_RESULT_OK;
            }
        }
        /*Codes_SRS_CHANNEL_INTERNAL_43_108: [ If the first operation in the list of pending operations contains a non-NULL push_callback: ]*/
        else if (CONTAINING_RECORD(channel_internal_ptr->op_list.Flink, CHANNEL_OP, anchor)->push_callback != NULL)
        {
            if (dequeue_operation(channel_internal_ptr, out_op_pull, pull_callback, pull_context, NULL, NULL, NULL) != 0)
            {
                /*Codes_SRS_CHANNEL_INTERNAL_43_023: [ If there are any failures, channel_internal_pull shall fail and return CHANNEL_RESULT_ERROR. ]*/
                LogError("Failure in dequeue_operation(channel_internal_ptr, out_op_pull, pull_callback, pull_context, NULL, NULL, NULL)");
                result = CHANNEL_RESULT_ERROR;
            }
            else
            {
                /* Codes_SRS_CHANNEL_INTERNAL_43_011: [ channel_internal_pull shall succeeds and return CHANNEL_RESULT_OK. ]*/
                result = CHANNEL_RESULT_OK;
            }
        }
        else
        {
            /*Codes_SRS_CHANNEL_INTERNAL_43_023: [ If there are any failures, channel_internal_pull shall fail and return CHANNEL_RESULT_ERROR. ]*/
            LogError("THANDLE(CHANNEL) channel = %p is in unexpected state", channel_internal);
            result = CHANNEL_RESULT_ERROR;
        }
    }
    /*Codes_SRS_CHANNEL_INTERNAL_43_115: [ channel_internal_pull shall call srw_lock_release_exclusive. ]*/
    srw_lock_release_exclusive(channel_internal_ptr->lock);

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_internal_push, THANDLE(CHANNEL_INTERNAL), channel_internal, THANDLE(RC_PTR), data, PUSH_CALLBACK, push_callback, void*, push_context, THANDLE(ASYNC_OP)*, out_op_push)
{
    CHANNEL_RESULT result;

    CHANNEL_INTERNAL* channel_internal_ptr = THANDLE_GET_T(CHANNEL_INTERNAL)(channel_internal);

    /*Codes_SRS_CHANNEL_INTERNAL_43_116: [ channel_internal_push shall call srw_lock_acquire_exclusive. ]*/
    srw_lock_acquire_exclusive(channel_internal_ptr->lock);
    {
        /*Codes_SRS_CHANNEL_INTERNAL_43_117: [ If the list of pending operations is empty or the first operation in the list of pending operations contains a non-NULL push_callback: ]*/
        if (
            DList_IsListEmpty(&channel_internal_ptr->op_list) ||
            CONTAINING_RECORD(channel_internal_ptr->op_list.Flink, CHANNEL_OP, anchor)->push_callback != NULL
            )
        {
            if (enqueue_operation(channel_internal_ptr, out_op_push, NULL, NULL, push_callback, push_context, data) != 0)
            {
                /*Codes_SRS_CHANNEL_INTERNAL_43_041: [ If there are any failures, channel_internal_push shall fail and return CHANNEL_RESULT_ERROR. ]*/
                LogError("Failure in enqueue_operation(channel_internal_ptr, out_op_push, NULL, NULL, push_callback, push_context, data)");
                result = CHANNEL_RESULT_ERROR;
            }
            else
            {
                /*Codes_SRS_CHANNEL_INTERNAL_43_132: [ channel_internal_push shall succeed and return CHANNEL_RESULT_OK. ]*/
                result = CHANNEL_RESULT_OK;
            }
        }
        /*Codes_SRS_CHANNEL_INTERNAL_43_124: [ If the first operation in the list of pending operations contains a non-NULL pull_callback: ]*/
        else if (CONTAINING_RECORD(channel_internal_ptr->op_list.Flink, CHANNEL_OP, anchor)->pull_callback != NULL)
        {
            if (dequeue_operation(channel_internal_ptr, out_op_push, NULL, NULL, push_callback, push_context, data) != 0)
            {
                /*Codes_SRS_CHANNEL_INTERNAL_43_041: [ If there are any failures, channel_internal_push shall fail and return CHANNEL_RESULT_ERROR. ]*/
                LogError("Failure in dequeue_operation(channel_internal_ptr, out_op_push, NULL, NULL, push_callback, push_context, data)");
                result = CHANNEL_RESULT_ERROR;
            }
            else
            {
                /*Codes_SRS_CHANNEL_INTERNAL_43_132: [ channel_internal_push shall succeed and return CHANNEL_RESULT_OK. ]*/
                result = CHANNEL_RESULT_OK;
            }
        }
        else
        {
            /*Codes_SRS_CHANNEL_INTERNAL_43_041: [ If there are any failures, channel_internal_push shall fail and return CHANNEL_RESULT_ERROR. ]*/
            LogError("THANDLE(CHANNEL) channel = %p is in unexpected state", channel_internal);
            result = CHANNEL_RESULT_ERROR;
        }
    }
    /*Codes_SRS_CHANNEL_INTERNAL_43_131: [ channel_internal_push shall call srw_lock_release_exclusive. ]*/
    srw_lock_release_exclusive(channel_internal_ptr->lock);

    return result;
}
