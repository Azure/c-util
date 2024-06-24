// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THREAD_SAFE_SET_H
#define THREAD_SAFE_SET_H

#ifdef __cplusplus
#include <cstdbool>
#else
#include <stdbool.h>
#endif


#include "macro_utils/macro_utils.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define THREAD_SAFE_SET_INSERT_RESULT_VALUES \
    THREAD_SAFE_SET_INSERT_OK, \
    THREAD_SAFE_SET_INSERT_KEY_ALREADY_EXISTS, \
    THREAD_SAFE_SET_INSERT_ERROR \

MU_DEFINE_ENUM(THREAD_SAFE_SET_INSERT_RESULT, THREAD_SAFE_SET_INSERT_RESULT_VALUES);

#define THREAD_SAFE_SET_REMOVE_RESULT_VALUES \
    THREAD_SAFE_SET_REMOVE_OK, \
    THREAD_SAFE_SET_REMOVE_NOT_FOUND, \
    THREAD_SAFE_SET_REMOVE_ERROR \

MU_DEFINE_ENUM(THREAD_SAFE_SET_REMOVE_RESULT, THREAD_SAFE_SET_REMOVE_RESULT_VALUES);

#define THREAD_SAFE_SET_CONTAINS_RESULT_VALUES \
    THREAD_SAFE_SET_CONTAINS_FOUND, \
    THREAD_SAFE_SET_CONTAINS_NOT_FOUND, \
    THREAD_SAFE_SET_CONTAINS_ERROR

MU_DEFINE_ENUM(THREAD_SAFE_SET_CONTAINS_RESULT, THREAD_SAFE_SET_CONTAINS_RESULT_VALUES);

/*
    * @brief Function to compare two elements in the set.
    *
    * @param lhs The left hand side of the comparison.
    * @param rhs The right hand side of the comparison.
    *
    * @return true if the elements are equal, false otherwise.
*/
typedef bool(*THREAD_SAFE_SET_ELEMENT_MATCH_FUNCTION)(void* lhs, void* rhs);

/*
    * @brief Function to perform an action on an element in the set.
    *
    * @param item The item to perform the action on.
    * @param action_context The context for the action.
    * @param continue_processing Indicates if the action should continue processing.
*/
typedef void (*THREAD_SAFE_SET_ACTION_FUNCTION)(const void* item, const void* action_context, bool* continue_processing);

/*
    * @brief Function to determine if an element should be removed from the set.
    *
    * @param item The item to evaluate.
    * @param match_context The context for the evaluation.
    * @param continue_processing Indicates if the evaluation should continue processing.
    *
    * @return true if the item should be removed, false otherwise.
*/
typedef bool(*THREAD_SAFE_SET_CONDITION_FUNCTION)(const void* item, const void* match_context, bool* continue_processing);

typedef struct THREAD_SAFE_SET_TAG* THREAD_SAFE_SET_HANDLE;

MOCKABLE_FUNCTION(, THREAD_SAFE_SET_HANDLE, thread_safe_set_create, THREAD_SAFE_SET_ELEMENT_MATCH_FUNCTION, match_function);
MOCKABLE_FUNCTION(, void, thread_safe_set_destroy, THREAD_SAFE_SET_HANDLE, thread_safe_set);

MOCKABLE_FUNCTION(, THREAD_SAFE_SET_INSERT_RESULT, thread_safe_set_insert, THREAD_SAFE_SET_HANDLE, thread_safe_set, void*, element);
MOCKABLE_FUNCTION(, THREAD_SAFE_SET_REMOVE_RESULT, thread_safe_set_remove, THREAD_SAFE_SET_HANDLE, thread_safe_set, void*, element);

MOCKABLE_FUNCTION(, THREAD_SAFE_SET_CONTAINS_RESULT, thread_safe_set_contains, THREAD_SAFE_SET_HANDLE, thread_safe_set, void*, element);
MOCKABLE_FUNCTION(, int, thread_safe_set_foreach, THREAD_SAFE_SET_HANDLE, thread_safe_set, THREAD_SAFE_SET_ACTION_FUNCTION, action_function, void*, action_context);

MOCKABLE_FUNCTION(, int, thread_safe_set_remove_if, THREAD_SAFE_SET_HANDLE, thread_safe_set, THREAD_SAFE_SET_CONDITION_FUNCTION, condition_function, const void*, match_context);


#ifdef __cplusplus
}
#endif // __cplusplus



#endif // THREAD_SAFE_SET_H
