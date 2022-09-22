// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SLIST_H
#define SLIST_H

#ifdef __cplusplus
#else
#include <stdbool.h>
#endif

#include "macro_utils/macro_utils.h"

typedef struct SLIST_ENTRY_TAG
{
    struct SLIST_ENTRY_TAG *next;
} SLIST_ENTRY, *PSLIST_ENTRY;

/**
* @brief                          Function passed to slist_find, which returns whichever first list entry that matches it.
* @param list_entry               Current list node being evaluated.
* @param match_context            Context passed to slist_find.
* @returns                        True to indicate that the current list node is the one to be returned, or false to continue traversing the list.
*/
typedef bool (*SLIST_MATCH_FUNCTION)(PSLIST_ENTRY list_entry, void* match_context);

/**
* @brief                          Function passed to slist_remove_if, which is used to define if an item of the list should be removed or not.
* @param list_entry               Value of the current list node being evaluated for removal.
* @param match_context            Context passed to slist_remove_if.
* @param continue_processing      Indicates if slist_remove_if shall continue iterating through the next nodes of the list or stop.
* @returns                        True to indicate that the current list node shall be removed, or false to not to.
*/
typedef bool (*SLIST_CONDITION_FUNCTION)(PSLIST_ENTRY list_entry, void* match_context, bool* continue_processing);

/**
* @brief                          Function passed to slist_for_each, which is called for the value of each node of the list.
* @param list_entry               Value of the current list node being processed.
* @param action_context           Context passed to slist_for_each.
* @param continue_processing      Indicates if slist_for_each shall continue iterating through the next nodes of the list or stop.
*/
typedef int (*SLIST_ACTION_FUNCTION)(PSLIST_ENTRY list_entry, void* action_context, bool* continue_processing);

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif
MOCKABLE_FUNCTION(, bool, slist_initialize, PSLIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, bool, slist_is_empty, const PSLIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, slist_add, PSLIST_ENTRY, list_head, PSLIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, slist_add_head, PSLIST_ENTRY, list_head, PSLIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, slist_remove, PSLIST_ENTRY, list_head, PSLIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, slist_remove_head, PSLIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, slist_find, PSLIST_ENTRY, list_head, SLIST_MATCH_FUNCTION, match_function, void*, match_context);
MOCKABLE_FUNCTION(, int, slist_remove_if, PSLIST_ENTRY, list_head, SLIST_CONDITION_FUNCTION, condition_function, void*, match_context);
MOCKABLE_FUNCTION(, int, slist_for_each, PSLIST_ENTRY, list_head, SLIST_ACTION_FUNCTION, action_function, void*, action_context);
#ifdef __cplusplus
}
#endif

#endif /* SLIST_H */
