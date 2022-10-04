// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_util/slist.h"

MU_DEFINE_ENUM_STRINGS(SLIST_IS_EMPTY_RESULT, SLIST_IS_EMPTY_RESULT_VALUES);

int slist_initialize(PSINGLYLINKEDLIST_ENTRY list_head)
{
    int result;
    if (list_head == NULL)
    {
        /* Codes_SRS_SLIST_07_001: [ If list_head is NULL, slist_initialize shall fail and return a non-zero value. ]*/
        LogError("Invalid argument (list_head=%p)", list_head);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_SLIST_07_002: [ slist_initialize shall initialize the next pointer in list_head points to NULL and return zero on success. ]*/
        list_head->next = NULL;
        result = 0;
    }
    return result;
}

SLIST_IS_EMPTY_RESULT slist_is_empty(const PSINGLYLINKEDLIST_ENTRY list_head)
{
    SLIST_IS_EMPTY_RESULT result;
    if (list_head == NULL)
    {
        /* Codes_SRS_SLIST_07_038: [ If list_head is NULL, slist_is_empty shall fail and return INVALID_ARGS. ]*/
        LogError("Invalid argument (list_head=%p)", list_head);
        result = INVALID_ARGS;
    }
    else
    {
        if (list_head->next == NULL)
        {
            /* Codes_SRS_SLIST_07_003: [ slist_is_empty shall return EMPTY if there is no SINGLYLINKEDLIST_ENTRY in this list. ]*/
            result = EMPTY;
        }
        else
        {
            /* Codes_SRS_SLIST_07_004: [ slist_is_empty shall return NOT_EMPTY if there is one or more entris in the list. ]*/
            result = NOT_EMPTY;
        }
    }
    return result;
}

int slist_add(PSINGLYLINKEDLIST_ENTRY list_head, PSINGLYLINKEDLIST_ENTRY list_entry)
{
    int result;
    if (
    /* Codes_SRS_SLIST_07_005: [ If list_head is NULL, slist_add shall fail and return a non-zero value. ]*/
    (list_head == NULL) ||
    /* Codes_SRS_SLIST_07_006: [If list_entry is NULL, slist_add shall fail and return a non-zero value.]*/
    (list_entry == NULL))
    {
        LogError("Invalid argument (list_head=%p, list_entry=%p)", list_head, list_entry);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_SLIST_07_007: [ slist_add shall add one entry to the tail of the list and return zero on success. ]*/
        PSINGLYLINKEDLIST_ENTRY list_instance = list_head;

        while (list_instance->next != NULL)
        {
            list_instance = list_instance->next;
        }

        list_instance->next = list_entry;
        result = 0;
    }
    return result;
}

int slist_add_head(PSINGLYLINKEDLIST_ENTRY list_head, PSINGLYLINKEDLIST_ENTRY list_entry)
{
    int result;
    if (
    /* Codes_SRS_SLIST_07_008: [ If list_head is NULL, slist_add_head shall fail and return a non-zero value. ]*/
    (list_head == NULL) ||
    /* Codes_SRS_SLIST_07_009: [ If list_entry is NULL, slist_add_head shall fail and return a non-zero value. ]*/
    (list_entry == NULL))
    {
        LogError("Invalid argument (list_head=%p, list_entry=%p)", list_head, list_entry);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_SLIST_07_010: [ slist_add_head shall insert list_entry at head and return zero on success. ]*/
        list_entry->next = list_head->next;
        list_head->next = list_entry;
        result = 0;
    }
    return result;
}

int slist_remove(PSINGLYLINKEDLIST_ENTRY list_head, PSINGLYLINKEDLIST_ENTRY list_entry)
{
    int result;
    if (
    /* Codes_SRS_SLIST_07_011: [ If list_head is NULL, slist_remove shall fail and a non-zero value. ]*/
    (list_head == NULL) ||
    /* Codes_SRS_SLIST_07_012: [ If list_entry is NULL, slist_remove shall fail and a non-zero value. ]*/
    (list_entry == NULL))
    {
        LogError("Invalid argument (list_head=%p, list_entry=%p)", list_head, list_entry);
        result = MU_FAILURE;
    }
    else
    {
        PSINGLYLINKEDLIST_ENTRY prev_item = NULL;
        PSINGLYLINKEDLIST_ENTRY current_item = list_head->next;
        while (current_item != NULL)
        {
            if (current_item == list_entry)
            {
                break;
            }
            prev_item = current_item;
            current_item = current_item->next;
        }
        if (current_item == NULL)
        {
            /* Codes_SRS_SLIST_07_014: [ If the entry list_entry is not found in the list, then slist_remove shall fail and a non-zero value. ]*/
            result = MU_FAILURE;
        }
        else 
        {
            /* Codes_SRS_SLIST_07_013: [ slist_remove shall remove a list entry from the list and return zero on success. ]*/
            if (prev_item != NULL)
            {
                prev_item->next = current_item->next;
            }
            else
            {
                list_head->next = list_head->next->next;
            }
            result = 0;
        }
    }
    return result;
}

PSINGLYLINKEDLIST_ENTRY slist_remove_head(PSINGLYLINKEDLIST_ENTRY list_head)
{
    PSINGLYLINKEDLIST_ENTRY result;
    if (list_head == NULL)
    {
        /* Codes_SRS_SLIST_07_015: [ If list_head is NULL, slist_remove_head shall fail and return NULL. ]*/
        LogError("Invalid argument (list_head=%p)", list_head);
        result = NULL;
    }
    else
    {
        if (list_head->next == NULL)
        {
            /* Codes_SRS_SLIST_07_017: [ slist_remove_head shall return list_head if that's the only entry in the list. ]*/
            result = list_head;
        }
        else
        {
            /* Codes_SRS_SLIST_07_016: [ slist_remove_head removes the head entry from the list defined by the list_head parameter on success and return a pointer to that entry. ]*/
            result = list_head->next;
            list_head->next = list_head->next->next;
        }
    }
    return result;
}

PSINGLYLINKEDLIST_ENTRY slist_find(PSINGLYLINKEDLIST_ENTRY list_head, SLIST_MATCH_FUNCTION match_function, const void* match_context)
{
    PSINGLYLINKEDLIST_ENTRY result;
    if (
    /* Codes_SRS_SLIST_07_018: [ If list_head is NULL, slist_find shall fail and return NULL. ]*/
    (list_head == NULL) ||
    /* Codes_SRS_SLIST_07_019: [ If match_function is NULL, slist_find shall fail and return NULL. ]*/
    (match_function == NULL))
    {
        LogError("Invalid argument (list_head=%p, match_function=%p)", list_head, match_function);
        result = NULL;
    }
    else
    {
        PSINGLYLINKEDLIST_ENTRY current_item = list_head->next;

        if (current_item == NULL)
        {
            /* Codes_SRS_SLIST_07_025: [ If the list is empty, slist_find shall return NULL. ]*/
            result = NULL;
        }
        else
        {
            /* Codes_SRS_SLIST_07_020: [ slist_find shall iterate through all entris in the list and return the first entry that satisfies the match_function on success. ]*/
            while (current_item != NULL)
            {
                /* Codes_SRS_SLIST_07_021: [ slist_find shall determine whether an item satisfies the match criteria by invoking the match_function for each entry in the list until a matching entry is found. ]*/
                /* Codes_SRS_SLIST_07_022: [ The match_function shall get as arguments the list entry being attempted to be matched and the match_context as is. ]*/
                if (match_function(current_item, match_context) == true)
                {
                    /* Codes_SRS_SLIST_07_024: [ If the match_function returns true, slist_find shall consider that item as matching. ]*/
                    break;
                }

                /* Codes_SRS_SLIST_07_023: [ If the match_function returns false, slist_find shall consider that item as not matching. ]*/
                current_item = current_item->next;
            }
            if (current_item == NULL)
            {
                /* Codes_SRS_SLIST_07_039: [ If the item is not found, slist_find shall return NULL. ]*/
                result = NULL;
            }
            else
            {
                result = current_item;
            }
        }

    }
    return result;
}

int slist_remove_if(PSINGLYLINKEDLIST_ENTRY list_head, SLIST_CONDITION_FUNCTION condition_function, const void* match_context)
{
    int result;
    if (
    /* Codes_SRS_SLIST_07_026: [ If list_head is NULL, slist_remove_if shall fail and return a non-zero value. ]*/
    (list_head == NULL) ||
    /* Codes_SRS_SLIST_07_027: [ If condition_function is NULL, slist_remove_if shall fail and return a non-zero value. ]*/
        (condition_function == NULL))
    {
        LogError("Invalid argument (list_head=%p, condition_function=%p)", list_head, condition_function);
        result = MU_FAILURE;
    }
    else
    {
        PSINGLYLINKEDLIST_ENTRY prev_item = list_head;
        PSINGLYLINKEDLIST_ENTRY current_item = list_head->next;

        if (current_item == NULL)
        {
            /* Codes_SRS_SLIST_07_040: [ If the list is empty, slist_find shall return a non-zero value. ]*/
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_SLIST_07_028: [ slist_remove_if shall iterate through all entris in a list, remove all that satisfies the condition_function and return zero. ]*/
            while (current_item != NULL)
            {
                bool continue_processing = false;

                /* Codes_SRS_SLIST_07_029: [ slist_remove_if shall determine whether an entry satisfies the condition criteria by invoking the condition function for that entry. ]*/
                if (condition_function(current_item, match_context, &continue_processing) == true)
                {
                    /* Codes_SRS_SLIST_07_030: [ If the condition_function returns true, slist_remove_if shall consider that entry as to be removed. ]*/			
                    prev_item->next = current_item->next;
                }
                else
                {
                    /* Codes_SRS_SLIST_07_031: [ If the condition_function returns false, slist_remove_if shall consider that entry as not to be removed. ]*/
                    prev_item = current_item;
                }

                /* Codes_SRS_SLIST_07_032: [ If the condition_function returns continue_processing as false, slist_remove_if shall stop iterating through the list and return. ]*/
                if (continue_processing == false)
                {
                    break;
                }

                current_item = current_item->next;
            }
            result = 0;
        }
    }

    return result;
}

int slist_for_each(PSINGLYLINKEDLIST_ENTRY list_head, SLIST_ACTION_FUNCTION action_function, const void* action_context)
{
    int result;

    if (
    /* Codes_SRS_SLIST_07_033: [ If list_head is NULL, slist_for_each shall fail and return a non - zero value. ]*/
    (list_head == NULL) ||
    /* Codes_SRS_SLIST_07_034: [ If action_function is NULL, slist_for_each shall fail and return a non - zero value. ]*/
    (action_function == NULL))
    {
        LogError("Invalid argument (list_head=%p, action_function=%p)", list_head, action_function);
        result = MU_FAILURE;
    }
    else
    {
        PSINGLYLINKEDLIST_ENTRY current_item = list_head->next;

        /* Codes_SRS_SLIST_07_035: [ slist_for_each shall iterate through all entries in the list, invoke action_function for each one of themand return zero on success. ]*/
        while (current_item != NULL)
        {
            bool continue_processing = false;

            if (action_function(current_item, action_context, &continue_processing) != 0)
            {
                /* Codes_SRS_SLIST_07_036: [ If the action_function fails, slist_for_each shall fail and return a non - zero value. ]*/
                LogError("failure in actionFunction(current_item = %p, action_context = %p, &continue_processing = %p)", current_item, action_context, &continue_processing);
                result = MU_FAILURE;
                break;
            }
            else
            {
                if (continue_processing == false)
                {
                    /* Codes_SRS_SLIST_07_037: [ If the action_function returns continue_processing as false, slist_for_each shall stop iterating through the list and return. ]*/
                    result = 0;
                    break;
                }
            }
            current_item = current_item->next;
        }
        result = 0;
    }

    return result;
}