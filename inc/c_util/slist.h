// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SLIST_H
#define SLIST_H

#ifdef __cplusplus
#else
#include <stdbool.h>
#endif

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct SINGLYLINKEDLIST_ENTRY_TAG
{
    struct SINGLYLINKEDLIST_ENTRY_TAG* next;
} SINGLYLINKEDLIST_ENTRY, * PSINGLYLINKEDLIST_ENTRY;

#define SLIST_IS_EMPTY_RESULT_VALUES \
    EMPTY,               \
    NOT_EMPTY,      \
    INVALID_ARGS
MU_DEFINE_ENUM(SLIST_IS_EMPTY_RESULT, SLIST_IS_EMPTY_RESULT_VALUES);

/**
* @brief                          Function passed to slist_find, which returns whichever first list entry that matches it.
* @param list_entry               Current list node being evaluated.
* @param match_context            Context passed to slist_find.
* @returns                        True to indicate that the current list node is the one to be returned, or false to continue traversing the list.
*/
typedef bool (*SLIST_MATCH_FUNCTION)(PSINGLYLINKEDLIST_ENTRY list_entry, const void* match_context);

/**
* @brief                          Function passed to slist_remove_if, which is used to define if an item of the list should be removed or not.
* @param list_entry               Value of the current list node being evaluated for removal.
* @param match_context            Context passed to slist_remove_if.
* @param continue_processing      Indicates if slist_remove_if shall continue iterating through the next nodes of the list or stop.
* @returns                        True to indicate that the current list node shall be removed, or false to not to.
*/
typedef bool (*SLIST_CONDITION_FUNCTION)(PSINGLYLINKEDLIST_ENTRY list_entry, const void* match_context, bool* continue_processing);

/**
* @brief                          Function passed to slist_for_each, which is called for the value of each node of the list.
* @param list_entry               Value of the current list node being processed.
* @param action_context           Context passed to slist_for_each.
* @param continue_processing      Indicates if slist_for_each shall continue iterating through the next nodes of the list or stop.
*/
typedef int (*SLIST_ACTION_FUNCTION)(PSINGLYLINKEDLIST_ENTRY list_entry, const void* action_context, bool* continue_processing);

MOCKABLE_FUNCTION(, int, slist_initialize, PSINGLYLINKEDLIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, SLIST_IS_EMPTY_RESULT, slist_is_empty, const PSINGLYLINKEDLIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, int, slist_add, PSINGLYLINKEDLIST_ENTRY, list_head, PSINGLYLINKEDLIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, int, slist_add_head, PSINGLYLINKEDLIST_ENTRY, list_head, PSINGLYLINKEDLIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, int, slist_remove, PSINGLYLINKEDLIST_ENTRY, list_head, PSINGLYLINKEDLIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, PSINGLYLINKEDLIST_ENTRY, slist_remove_head, PSINGLYLINKEDLIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, PSINGLYLINKEDLIST_ENTRY, slist_find, PSINGLYLINKEDLIST_ENTRY, list_head, SLIST_MATCH_FUNCTION, match_function, const void*, match_context);
MOCKABLE_FUNCTION(, int, slist_remove_if, PSINGLYLINKEDLIST_ENTRY, list_head, SLIST_CONDITION_FUNCTION, condition_function, const void*, match_context);
MOCKABLE_FUNCTION(, int, slist_for_each, PSINGLYLINKEDLIST_ENTRY, list_head, SLIST_ACTION_FUNCTION, action_function, const void*, action_context);

#ifdef __cplusplus
}
#endif

#endif /* SLIST_H */
