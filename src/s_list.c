// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_util/s_list.h"

MU_DEFINE_ENUM_STRINGS(S_LIST_IS_EMPTY_RESULT, S_LIST_IS_EMPTY_RESULT_VALUES);

int s_list_initialize(PS_LIST_ENTRY list_head)
{
    int result;
    if (list_head == NULL)
    {
        /* Codes_SRS_S_LIST_07_001: [ If list_head is NULL, s_list_initialize shall fail and return a non-zero value. ]*/
        LogError("Invalid argument (list_head=%p)", list_head);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_S_LIST_07_002: [ s_list_initialize shall initialize the next pointer in list_head points to NULL and return zero on success. ]*/
        list_head->next = NULL;
        result = 0;
    }
    return result;
}

S_LIST_IS_EMPTY_RESULT s_list_is_empty(const PS_LIST_ENTRY list_head)
{
    S_LIST_IS_EMPTY_RESULT result;
    if (list_head == NULL)
    {
        /* Codes_SRS_S_LIST_07_038: [ If list_head is NULL, s_list_is_empty shall fail and return INVALID_ARGS. ]*/
        LogError("Invalid argument (list_head=%p)", list_head);
        result = S_LIST_IS_EMPTY_RESULT_INVALID_ARGS;
    }
    else
    {
        if (list_head->next == NULL)
        {
            /* Codes_SRS_S_LIST_07_003: [ s_list_is_empty shall return EMPTY if there is no S_LIST_ENTRY in this list. ]*/
            result = S_LIST_IS_EMPTY_RESULT_EMPTY;
        }
        else
        {
            /* Codes_SRS_S_LIST_07_004: [ s_list_is_empty shall return NOT_EMPTY if there is one or more entris in the list. ]*/
            result = S_LIST_IS_EMPTY_RESULT_NOT_EMPTY;
        }
    }
    return result;
}

int s_list_add(PS_LIST_ENTRY list_head, PS_LIST_ENTRY list_entry)
{
    int result;
    if (
    /* Codes_SRS_S_LIST_07_005: [ If list_head is NULL, s_list_add shall fail and return a non-zero value. ]*/
    (list_head == NULL) ||
    /* Codes_SRS_S_LIST_07_006: [If list_entry is NULL, s_list_add shall fail and return a non-zero value.]*/
    (list_entry == NULL))
    {
        LogError("Invalid arguments (list_head=%p, list_entry=%p)", list_head, list_entry);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_S_LIST_07_007: [ s_list_add shall add one entry to the tail of the list and return zero on success. ]*/
        PS_LIST_ENTRY list_instance = list_head;

        while (list_instance->next != NULL)
        {
            list_instance = list_instance->next;
        }

        list_instance->next = list_entry;
        result = 0;
    }
    return result;
}

int s_list_add_head(PS_LIST_ENTRY list_head, PS_LIST_ENTRY list_entry)
{
    int result;
    if (
    /* Codes_SRS_S_LIST_07_008: [ If list_head is NULL, s_list_add_head shall fail and return a non-zero value. ]*/
    (list_head == NULL) ||
    /* Codes_SRS_S_LIST_07_009: [ If list_entry is NULL, s_list_add_head shall fail and return a non-zero value. ]*/
    (list_entry == NULL))
    {
        LogError("Invalid arguments (list_head=%p, list_entry=%p)", list_head, list_entry);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_S_LIST_07_010: [ s_list_add_head shall insert list_entry at head and return zero on success. ]*/
        list_entry->next = list_head->next;
        list_head->next = list_entry;
        result = 0;
    }
    return result;
}

int s_list_remove(PS_LIST_ENTRY list_head, PS_LIST_ENTRY list_entry)
{
    int result;
    if (
    /* Codes_SRS_S_LIST_07_011: [ If list_head is NULL, s_list_remove shall fail and a non-zero value. ]*/
    (list_head == NULL) ||
    /* Codes_SRS_S_LIST_07_012: [ If list_entry is NULL, s_list_remove shall fail and a non-zero value. ]*/
    (list_entry == NULL))
    {
        LogError("Invalid arguments (list_head=%p, list_entry=%p)", list_head, list_entry);
        result = MU_FAILURE;
    }
    else
    {
        PS_LIST_ENTRY prev_item = NULL;
        PS_LIST_ENTRY current_item = list_head->next;
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
            /* Codes_SRS_S_LIST_07_014: [ If the entry list_entry is not found in the list, then s_list_remove shall fail and a non-zero value. ]*/
            result = MU_FAILURE;
        }
        else 
        {
            /* Codes_SRS_S_LIST_07_013: [ s_list_remove shall remove a list entry from the list and return zero on success. ]*/
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

PS_LIST_ENTRY s_list_remove_head(PS_LIST_ENTRY list_head)
{
    PS_LIST_ENTRY result;
    if (list_head == NULL)
    {
        /* Codes_SRS_S_LIST_07_015: [ If list_head is NULL, s_list_remove_head shall fail and return NULL. ]*/
        LogError("Invalid argument (list_head=%p)", list_head);
        result = NULL;
    }
    else
    {
        if (list_head->next == NULL)
        {
            /* Codes_SRS_S_LIST_07_017: [ s_list_remove_head shall return list_head if that's the only entry in the list. ]*/
            result = list_head;
        }
        else
        {
            /* Codes_SRS_S_LIST_07_016: [ s_list_remove_head removes the head entry from the list defined by the list_head parameter on success and return a pointer to that entry. ]*/
            result = list_head->next;
            list_head->next = list_head->next->next;
        }
    }
    return result;
}

PS_LIST_ENTRY s_list_find(PS_LIST_ENTRY list_head, S_LIST_MATCH_FUNCTION match_function, const void* match_context)
{
    PS_LIST_ENTRY result;
    if (
    /* Codes_SRS_S_LIST_07_018: [ If list_head is NULL, s_list_find shall fail and return NULL. ]*/
    (list_head == NULL) ||
    /* Codes_SRS_S_LIST_07_019: [ If match_function is NULL, s_list_find shall fail and return NULL. ]*/
    (match_function == NULL))
    {
        LogError("Invalid arguments (list_head=%p, match_function=%p)", list_head, match_function);
        result = NULL;
    }
    else
    {
        PS_LIST_ENTRY current_item = list_head->next;

        if (current_item == NULL)
        {
            /* Codes_SRS_S_LIST_07_025: [ If the list is empty, s_list_find shall return NULL. ]*/
            result = NULL;
        }
        else
        {
            /* Codes_SRS_S_LIST_07_020: [ s_list_find shall iterate through all entris in the list and return the first entry that satisfies the match_function on success. ]*/
            while (current_item != NULL)
            {
                /* Codes_SRS_S_LIST_07_021: [ s_list_find shall determine whether an item satisfies the match criteria by invoking the match_function for each entry in the list until a matching entry is found. ]*/
                /* Codes_SRS_S_LIST_07_022: [ The match_function shall get as arguments the list entry being attempted to be matched and the match_context as is. ]*/
                if (match_function(current_item, match_context) == true)
                {
                    /* Codes_SRS_S_LIST_07_024: [ If the match_function returns true, s_list_find shall consider that item as matching. ]*/
                    break;
                }

                /* Codes_SRS_S_LIST_07_023: [ If the match_function returns false, s_list_find shall consider that item as not matching. ]*/
                current_item = current_item->next;
            }
            if (current_item == NULL)
            {
                /* Codes_SRS_S_LIST_07_039: [ If the item is not found, s_list_find shall return NULL. ]*/
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

int s_list_remove_if(PS_LIST_ENTRY list_head, S_LIST_CONDITION_FUNCTION condition_function, const void* match_context)
{
    int result;
    if (
    /* Codes_SRS_S_LIST_07_026: [ If list_head is NULL, s_list_remove_if shall fail and return a non-zero value. ]*/
    (list_head == NULL) ||
    /* Codes_SRS_S_LIST_07_027: [ If condition_function is NULL, s_list_remove_if shall fail and return a non-zero value. ]*/
        (condition_function == NULL))
    {
        LogError("Invalid arguments (list_head=%p, condition_function=%p)", list_head, condition_function);
        result = MU_FAILURE;
    }
    else
    {
        PS_LIST_ENTRY prev_item = list_head;
        PS_LIST_ENTRY current_item = list_head->next;

        if (current_item == NULL)
        {
            /* Codes_SRS_S_LIST_07_040: [ If the list is empty, s_list_find shall return a non-zero value. ]*/
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_S_LIST_07_028: [ s_list_remove_if shall iterate through all entris in a list, remove all that satisfies the condition_function and return zero. ]*/
            while (current_item != NULL)
            {
                bool continue_processing = false;

                /* Codes_SRS_S_LIST_07_029: [ s_list_remove_if shall determine whether an entry satisfies the condition criteria by invoking the condition function for that entry. ]*/
                if (condition_function(current_item, match_context, &continue_processing) == true)
                {
                    /* Codes_SRS_S_LIST_07_030: [ If the condition_function returns true, s_list_remove_if shall consider that entry as to be removed. ]*/
                    prev_item->next = current_item->next;
                }
                else
                {
                    /* Codes_SRS_S_LIST_07_031: [ If the condition_function returns false, s_list_remove_if shall consider that entry as not to be removed. ]*/
                    prev_item = current_item;
                }

                /* Codes_SRS_S_LIST_07_032: [ If the condition_function returns continue_processing as false, s_list_remove_if shall stop iterating through the list and return. ]*/
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

int s_list_for_each(PS_LIST_ENTRY list_head, S_LIST_ACTION_FUNCTION action_function, const void* action_context)
{
    int result;

    if (
    /* Codes_SRS_S_LIST_07_033: [ If list_head is NULL, s_list_for_each shall fail and return a non - zero value. ]*/
    (list_head == NULL) ||
    /* Codes_SRS_S_LIST_07_034: [ If action_function is NULL, s_list_for_each shall fail and return a non - zero value. ]*/
    (action_function == NULL))
    {
        LogError("Invalid arguments (list_head=%p, action_function=%p)", list_head, action_function);
        result = MU_FAILURE;
    }
    else
    {
        PS_LIST_ENTRY current_item = list_head->next;

        /* Codes_SRS_S_LIST_07_035: [ s_list_for_each shall iterate through all entries in the list, invoke action_function for each one of themand return zero on success. ]*/
        while (current_item != NULL)
        {
            bool continue_processing = false;

            if (action_function(current_item, action_context, &continue_processing) != 0)
            {
                /* Codes_SRS_S_LIST_07_036: [ If the action_function fails, s_list_for_each shall fail and return a non - zero value. ]*/
                LogError("failure in actionFunction(current_item = %p, action_context = %p, &continue_processing = %p)", current_item, action_context, &continue_processing);
                result = MU_FAILURE;
                break;
            }
            else
            {
                if (continue_processing == false)
                {
                    /* Codes_SRS_S_LIST_07_037: [ If the action_function returns continue_processing as false, s_list_for_each shall stop iterating through the list and return. ]*/
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
