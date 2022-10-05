// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef S_LIST_H
#define S_LIST_H

#ifdef __cplusplus
#else
#include <stdbool.h>
#endif

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct S_LIST_ENTRY_TAG
{
    struct S_LIST_ENTRY_TAG* next;
} S_LIST_ENTRY, * PS_LIST_ENTRY;

#define S_LIST_IS_EMPTY_RESULT_VALUES \
    S_LIST_IS_EMPTY_RESULT_EMPTY,               \
    S_LIST_IS_EMPTY_RESULT_NOT_EMPTY,      \
    S_LIST_IS_EMPTY_RESULT_INVALID_ARGS

MU_DEFINE_ENUM(S_LIST_IS_EMPTY_RESULT, S_LIST_IS_EMPTY_RESULT_VALUES);

/**
* @brief                          Function passed to s_list_find, which returns whichever first list entry that matches it.
* @param list_entry               Current list node being evaluated.
* @param match_context            Context passed to s_list_find.
* @returns                        True to indicate that the current list node is the one to be returned, or false to continue traversing the list.
*/
typedef bool (*S_LIST_MATCH_FUNCTION)(PS_LIST_ENTRY list_entry, const void* match_context);

/**
* @brief                          Function passed to s_list_remove_if, which is used to define if an item of the list should be removed or not.
* @param list_entry               Value of the current list node being evaluated for removal.
* @param match_context            Context passed to s_list_remove_if.
* @param continue_processing      Indicates if s_list_remove_if shall continue iterating through the next nodes of the list or stop.
* @returns                        True to indicate that the current list node shall be removed, or false to not to.
*/
typedef bool (*S_LIST_CONDITION_FUNCTION)(PS_LIST_ENTRY list_entry, const void* match_context, bool* continue_processing);

/**
* @brief                          Function passed to s_list_for_each, which is called for the value of each node of the list.
* @param list_entry               Value of the current list node being processed.
* @param action_context           Context passed to s_list_for_each.
* @param continue_processing      Indicates if s_list_for_each shall continue iterating through the next nodes of the list or stop.
*/
typedef int (*S_LIST_ACTION_FUNCTION)(PS_LIST_ENTRY list_entry, const void* action_context, bool* continue_processing);

MOCKABLE_FUNCTION(, int, s_list_initialize, PS_LIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, S_LIST_IS_EMPTY_RESULT, s_list_is_empty, const PS_LIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, int, s_list_add, PS_LIST_ENTRY, list_head, PS_LIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, int, s_list_add_head, PS_LIST_ENTRY, list_head, PS_LIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, int, s_list_remove, PS_LIST_ENTRY, list_head, PS_LIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, PS_LIST_ENTRY, s_list_remove_head, PS_LIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, PS_LIST_ENTRY, s_list_find, PS_LIST_ENTRY, list_head, S_LIST_MATCH_FUNCTION, match_function, const void*, match_context);
MOCKABLE_FUNCTION(, int, s_list_remove_if, PS_LIST_ENTRY, list_head, S_LIST_CONDITION_FUNCTION, condition_function, const void*, match_context);
MOCKABLE_FUNCTION(, int, s_list_for_each, PS_LIST_ENTRY, list_head, S_LIST_ACTION_FUNCTION, action_function, const void*, action_context);

#ifdef __cplusplus
}
#endif

#endif /* S_LIST_H */
