// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "c_logging/xlogging.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/singlylinkedlist.h"

#include "c_util/waiter_queue_ll.h"

MU_DEFINE_ENUM_STRINGS(WAITER_QUEUE_CALL_REASON, WAITER_QUEUE_CALL_REASON_VALUES);

typedef struct WAITER_QUEUE_LL_TAG
{
    SINGLYLINKEDLIST_HANDLE list;
} WAITER_QUEUE_LL;

typedef struct WAITER_QUEUE_LL_ITEM_TAG
{
    POP_CALLBACK pop_callback;
    void* pop_callback_context;
} WAITER_QUEUE_LL_ITEM;

WAITER_QUEUE_LL_HANDLE waiter_queue_ll_create(void)
{
    WAITER_QUEUE_LL_HANDLE result;
    WAITER_QUEUE_LL_HANDLE waiter_queue_ll = malloc(sizeof(WAITER_QUEUE_LL));
    if (waiter_queue_ll == NULL)
    {
        LogError("Failure in  malloc(sizeof(WAITER_QUEUE_LL))");
        result = NULL;
    }
    else
    {
        waiter_queue_ll->list = singlylinkedlist_create();
        if (waiter_queue_ll->list == NULL)
        {
            LogError("Failure in singlylinkedlist_create()");
            result = NULL;
        }
        else
        {
            result = waiter_queue_ll;
            goto all_ok;
        }
        free(waiter_queue_ll);
    }
all_ok:
    return result;
}

void waiter_queue_ll_destroy(WAITER_QUEUE_LL_HANDLE waiter_queue_ll)
{
    if (waiter_queue_ll == NULL)
    {
        LogError("Invalid argument: WAITER_QUEUE_LL_HANDLE waiter_queue_ll=%p", waiter_queue_ll);
    }
    else
    {
        singlylinkedlist_destroy(waiter_queue_ll->list);
        free(waiter_queue_ll);
    }
}

int waiter_queue_ll_push(WAITER_QUEUE_LL_HANDLE waiter_queue_ll, POP_CALLBACK pop_callback, void* pop_callback_context)
{
    int result;
    if (waiter_queue_ll == NULL || pop_callback == NULL)
    {
        LogError("Invalid arguments: WAITER_QUEUE_LL_HANDLE waiter_queue_ll=%p, POP_CALLBACK pop_callback=%p, void* pop_callback_context=%p", waiter_queue_ll, pop_callback, pop_callback_context);
        result = MU_FAILURE;
    }
    else
    {
        WAITER_QUEUE_LL_ITEM* waiter_queue_ll_item = malloc(sizeof(WAITER_QUEUE_LL_ITEM));
        if (waiter_queue_ll_item == NULL)
        {
            LogError("Failure in malloc(sizeof(WAITER_QUEUE_LL_ITEM))");
            result = MU_FAILURE;
        }
        else
        {
            waiter_queue_ll_item->pop_callback = pop_callback;
            waiter_queue_ll_item->pop_callback_context = pop_callback_context;
            if (singlylinkedlist_add(waiter_queue_ll->list, waiter_queue_ll_item) == NULL)
            {
                LogError("Failure in singlylinkedlist_add(waiter_queue_ll->list, waiter_queue_ll_item)");
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
                goto all_ok;
            }
            free(waiter_queue_ll_item);
        }
    }
all_ok:
    return result;
}

int waiter_queue_ll_pop(WAITER_QUEUE_LL_HANDLE waiter_queue_ll, void* data)
{
    int result;
    if (waiter_queue_ll == NULL)
    {
        LogError("Invalid arguments: WAITER_QUEUE_LL_HANDLE waiter_queue_ll=%p, void* data=%p", waiter_queue_ll, data);
        result = MU_FAILURE;
    }
    else
    {
        LIST_ITEM_HANDLE list_item_handle = singlylinkedlist_get_head_item(waiter_queue_ll->list);
        bool continue_processing = true;
        while (true)
        {
            if (list_item_handle == NULL || !continue_processing)
            {
                result = 0;
                break;
            }
            else
            {
                WAITER_QUEUE_LL_ITEM* waiter_queue_ll_item = (WAITER_QUEUE_LL_ITEM*)singlylinkedlist_item_get_value(list_item_handle);
                if (waiter_queue_ll_item == NULL)
                {
                    LogError("Failure in singlylinkedlist_item_get_value(list_item_handle)");
                    result = MU_FAILURE;
                    break;
                }
                else
                {
                    bool remove_item = waiter_queue_ll_item->pop_callback(waiter_queue_ll_item->pop_callback_context, data, &continue_processing, WAITER_QUEUE_CALL_REASON_POPPED);
                    LIST_ITEM_HANDLE old_list_item_handle = list_item_handle;
                    list_item_handle = singlylinkedlist_get_next_item(list_item_handle);
                    if (remove_item)
                    {
                        if (singlylinkedlist_remove(waiter_queue_ll->list, old_list_item_handle) != 0)
                        {
                            LogError("Failure in singlylinkedlist_remove(waiter_queue_ll->list, old_list_item_handle)");
                            result = MU_FAILURE;
                            break;
                        }
                        else
                        {
                            free(waiter_queue_ll_item);
                        }
                    }
                }
            }
        }
    }
    return result;
}

void waiter_queue_ll_abandon(WAITER_QUEUE_LL_HANDLE waiter_queue)
{
    if (waiter_queue == NULL)
    {
        LogError("Invalid argument: WAITER_QUEUE_LL_HANDLE waiter_queue=%p", waiter_queue);
    }
    else
    {
        LIST_ITEM_HANDLE list_item_handle = singlylinkedlist_get_head_item(waiter_queue->list);
        while (list_item_handle != NULL)
        {
            WAITER_QUEUE_LL_ITEM* waiter_queue_ll_item = (WAITER_QUEUE_LL_ITEM*)singlylinkedlist_item_get_value(list_item_handle);
            if (waiter_queue_ll_item == NULL)
            {
                LogError("Failure in singlylinkedlist_item_get_value(list_item_handle)");
            }
            else
            {
                bool continue_processing;
                (void)waiter_queue_ll_item->pop_callback(waiter_queue_ll_item->pop_callback_context, NULL, &continue_processing, WAITER_QUEUE_CALL_REASON_ABANDONED);
                LIST_ITEM_HANDLE old_list_item_item_handle = list_item_handle;
                list_item_handle = singlylinkedlist_get_next_item(list_item_handle);
                if (singlylinkedlist_remove(waiter_queue->list, old_list_item_item_handle) != 0)
                {
                    LogError("Failure in singlylinkedlist_remove(waiter_queue->list, old_list_item_item_handle)");
                }
                else
                {
                    free(waiter_queue_ll_item);
                }
            }
        }
    }
}
