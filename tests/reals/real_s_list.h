// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_S_LIST_H
#define REAL_S_LIST_H

#include "macro_utils/macro_utils.h"
#include "c_util/s_list.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_S_LIST_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        s_list_initialize, \
        s_list_is_empty, \
        s_list_add, \
        s_list_remove, \
        s_list_remove_head, \
        s_list_find, \
        s_list_remove_if, \
        s_list_for_each \
    )


#include <stddef.h>


int real_s_list_initialize(PS_LIST_ENTRY list_head);
S_LIST_IS_EMPTY_RESULT real_s_list_is_empty(const PS_LIST_ENTRY list_head);
int real_s_list_add(PS_LIST_ENTRY list_head, PS_LIST_ENTRY list_entry);
int real_s_list_remove(PS_LIST_ENTRY list_head, PS_LIST_ENTRY list_entry);
PS_LIST_ENTRY real_s_list_remove_head(PS_LIST_ENTRY list_head);
PS_LIST_ENTRY real_s_list_find(PS_LIST_ENTRY list_head, S_LIST_MATCH_FUNCTION match_function, const void* match_context);
int real_s_list_remove_if(PS_LIST_ENTRY list_head, S_LIST_CONDITION_FUNCTION condition_function, const void* match_context);
int real_s_list_for_each(PS_LIST_ENTRY list_head, S_LIST_ACTION_FUNCTION action_function, const void* action_context);


#endif // REAL_S_LIST_H
