// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.See LICENSE file in the project root for full license information.

#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_util/thread_safe_set.h"

THREAD_SAFE_SET_HANDLE thread_safe_set_create(THREAD_SAFE_SET_ELEMENT_MATCH_FUNCTION match_function)
{
    (void)match_function;
    return NULL;
}

void thread_safe_set_destroy(THREAD_SAFE_SET_HANDLE thread_safe_set)
{
    (void)thread_safe_set;
}

THREAD_SAFE_SET_INSERT_RESULT thread_safe_set_insert(THREAD_SAFE_SET_HANDLE thread_safe_set, void* element)
{
    (void)thread_safe_set;
    (void)element;
    return THREAD_SAFE_SET_INSERT_OK;
}

THREAD_SAFE_SET_REMOVE_RESULT thread_safe_set_remove(THREAD_SAFE_SET_HANDLE thread_safe_set, void* element)
{
    (void)thread_safe_set;
    (void)element;
    return THREAD_SAFE_SET_REMOVE_OK;
}

THREAD_SAFE_SET_CONTAINS_RESULT thread_safe_set_contains(THREAD_SAFE_SET_HANDLE thread_safe_set, void* element)
{
    (void)thread_safe_set;
    (void)element;
    return THREAD_SAFE_SET_CONTAINS_FOUND;
}

int thread_safe_set_foreach(THREAD_SAFE_SET_HANDLE thread_safe_set, THREAD_SAFE_SET_ACTION_FUNCTION action_function, void* action_context)
{
    (void)thread_safe_set;
    (void)action_function;
    (void)action_context;
    return 0;
}

int thread_safe_set_remove_if(THREAD_SAFE_SET_HANDLE thread_safe_set, THREAD_SAFE_SET_CONDITION_FUNCTION condition_function, const void* match_context)
{
    (void)thread_safe_set;
    (void)condition_function;
    (void)match_context;
    return 0;
}
