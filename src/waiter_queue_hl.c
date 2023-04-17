// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "c_logging/xlogging.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/sm.h"
#include "c_pal/srw_lock.h"
#include "c_util/waiter_queue_ll.h"

#include "c_util/waiter_queue_hl.h"

typedef struct WAITER_QUEUE_HL_TAG
{
    WAITER_QUEUE_LL_HANDLE waiter_queue_ll;
    SRW_LOCK_HANDLE srw_lock;
    SM_HANDLE sm;
} WAITER_QUEUE_HL;

typedef struct WAITER_QUEUE_HL_ITEM_TAG
{
    WAITER_QUEUE_HL_HANDLE waiter_queue_hl;
    POP_CALLBACK pop_callback;
    void* pop_callback_context;
} WAITER_QUEUE_HL_ITEM;

WAITER_QUEUE_HL_HANDLE waiter_queue_hl_create(void)
{
    WAITER_QUEUE_HL_HANDLE result;
    WAITER_QUEUE_HL_HANDLE waiter_queue_hl = malloc(sizeof(WAITER_QUEUE_HL));
    if (waiter_queue_hl == NULL)
    {
        LogError("Failure in malloc(sizeof(WAITER_QUEUE_HL))");
        result = NULL;
    }
    else
    {
        waiter_queue_hl->waiter_queue_ll = waiter_queue_ll_create();
        if (waiter_queue_hl->waiter_queue_ll == NULL)
        {
            LogError("Failure in waiter_queue_ll_create()");
            result = NULL;
        }
        else
        {
            waiter_queue_hl->srw_lock = srw_lock_create(false, "waiter_queue");
            if (waiter_queue_hl->srw_lock == NULL)
            {
                LogError("Failure in srw_lock_create(false, \"waiter_queue\")");
                result = NULL;
            }
            else
            {
                waiter_queue_hl->sm = sm_create("waiter_queue");
                if (waiter_queue_hl->sm == NULL)
                {
                    LogError("Failure in sm_create(\"waiter_queue\")");
                    result = NULL;
                }
                else
                {
                    SM_RESULT sm_open_begin_result = sm_open_begin(waiter_queue_hl->sm);
                    if (sm_open_begin_result != SM_EXEC_GRANTED)
                    {
                        LogError("Failure in sm_open_begin(waiter_queue_hl->sm");
                        result = NULL;
                    }
                    else
                    {
                        sm_open_end(waiter_queue_hl->sm, true);
                        result = waiter_queue_hl;
                        goto all_ok;
                    }
                    sm_destroy(waiter_queue_hl->sm);
                }
                srw_lock_destroy(waiter_queue_hl->srw_lock);
            }
            waiter_queue_ll_destroy(waiter_queue_hl->waiter_queue_ll);
        }
        free(waiter_queue_hl);
    }
all_ok:
    return result;
}

void waiter_queue_hl_destroy(WAITER_QUEUE_HL_HANDLE waiter_queue_hl)
{
    if (waiter_queue_hl == NULL)
    {
        LogError("Invalid argument: WAITER_QUEUE_HL_HANDLE waiter_queue_hl=%p", waiter_queue_hl);
    }
    else
    {
        srw_lock_acquire_exclusive(waiter_queue_hl->srw_lock);
        waiter_queue_ll_abandon(waiter_queue_hl->waiter_queue_ll);
        srw_lock_release_exclusive(waiter_queue_hl->srw_lock);

        SM_RESULT sm_close_begin_result = sm_close_begin(waiter_queue_hl->sm);
        if (sm_close_begin_result != SM_EXEC_GRANTED)
        {
            LogError("Failure in sm_close_begin(waiter_queue_hl->sm)");
        }
        else
        {
            sm_close_end(waiter_queue_hl->sm);
            sm_destroy(waiter_queue_hl->sm);
            srw_lock_destroy(waiter_queue_hl->srw_lock);
            waiter_queue_ll_destroy(waiter_queue_hl->waiter_queue_ll);
            free(waiter_queue_hl);
        }
    }
}

static bool on_pop(void* context, void* data, bool* continue_processing, WAITER_QUEUE_CALL_REASON reason)
{
    bool result;
    if (context == NULL || continue_processing == NULL)
    {
        LogError("Invalid argument: void* context=%p, void* data=%p, bool* continue_processing=%p, WAITER_QUEUE_CALL_REASON reason=%" PRI_MU_ENUM "", context, data, continue_processing, MU_ENUM_VALUE(WAITER_QUEUE_CALL_REASON, reason));
        result = false;
    }
    else
    {
        WAITER_QUEUE_HL_ITEM* waiter_queue_hl_item = (WAITER_QUEUE_HL_ITEM*)context;
        result = waiter_queue_hl_item->pop_callback(waiter_queue_hl_item->pop_callback_context, data, continue_processing, reason);
        if (result || reason == WAITER_QUEUE_CALL_REASON_ABANDONED)
        {
            sm_exec_end(waiter_queue_hl_item->waiter_queue_hl->sm);
            free(waiter_queue_hl_item);
        }
    }
    return result;
}

int waiter_queue_hl_push(WAITER_QUEUE_HL_HANDLE waiter_queue_hl, POP_CALLBACK pop_callback, void* pop_callback_context)
{
    int result;
    if (waiter_queue_hl == NULL)
    {
        LogError("Invalid argument: WAITER_QUEUE_HL_HANDLE waiter_queue_hl=%p, POP_CALLBACK pop_callback=%p, void* pop_callback_context=%p", waiter_queue_hl, pop_callback, pop_callback_context);
        result = MU_FAILURE;
    }
    else
    {
        WAITER_QUEUE_HL_ITEM* waiter_queue_hl_item = malloc(sizeof(WAITER_QUEUE_HL_ITEM));
        if (waiter_queue_hl_item == NULL)
        {
            LogError("Failure in malloc(sizeof(WAITER_QUEUE_HL_ITEM))");
            result = MU_FAILURE;
        }
        else
        {
            waiter_queue_hl_item->pop_callback = pop_callback;
            waiter_queue_hl_item->pop_callback_context = pop_callback_context;
            waiter_queue_hl_item->waiter_queue_hl = waiter_queue_hl;
            SM_RESULT sm_exec_begin_result = sm_exec_begin(waiter_queue_hl->sm);
            if (sm_exec_begin_result != SM_EXEC_GRANTED)
            {
                LogError("Failure in sm_exec_begin(waiter_queue_hl->sm)");
                result = MU_FAILURE;
            }
            else
            {
                srw_lock_acquire_exclusive(waiter_queue_hl->srw_lock);
                if (waiter_queue_ll_push(waiter_queue_hl->waiter_queue_ll, on_pop, waiter_queue_hl_item) != 0)
                {
                    LogError("Failure in waiter_queue_ll_push(waiter_queue_hl->waiter_queue_ll, on_pop, waiter_queue_hl_item)");
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                }
                srw_lock_release_exclusive(waiter_queue_hl->srw_lock);
                if (result == 0)
                {
                    goto all_ok;
                }
                sm_exec_end(waiter_queue_hl->sm);
            }
            free(waiter_queue_hl_item);
        }
    }
all_ok:
    return result;
}

int waiter_queue_hl_pop(WAITER_QUEUE_HL_HANDLE waiter_queue_hl, void* data)
{
    int result;
    if (waiter_queue_hl == NULL)
    {
        LogError("Invalid argument: WAITER_QUEUE_HL_HANDLE waiter_queue_hl=%p, void* data=%p", waiter_queue_hl, data);
        result = MU_FAILURE;
    }
    else
    {
        SM_RESULT sm_exec_begin_result = sm_exec_begin(waiter_queue_hl->sm);
        if (sm_exec_begin_result != SM_EXEC_GRANTED)
        {
            LogError("Failure in sm_exec_begin(waiter_queue_hl->sm)");
            result = MU_FAILURE;
        }
        else
        {
            srw_lock_acquire_exclusive(waiter_queue_hl->srw_lock);
            if (waiter_queue_ll_pop(waiter_queue_hl->waiter_queue_ll, data) != 0)
            {
                LogError("Failure in waiter_queue_ll_pop(waiter_queue_hl->waiter_queue_ll, data)");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
            srw_lock_release_exclusive(waiter_queue_hl->srw_lock);
            sm_exec_end(waiter_queue_hl->sm);
        }
    }
    return result;
}
