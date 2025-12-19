// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_SINGLYLINKEDLIST_H
#define REAL_SINGLYLINKEDLIST_H


#include <stddef.h>


#include "macro_utils/macro_utils.h"
#include "c_util/singlylinkedlist.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_SINGLYLINKEDLIST_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        singlylinkedlist_create, \
        singlylinkedlist_destroy, \
        singlylinkedlist_add, \
        singlylinkedlist_add_head, \
        singlylinkedlist_remove, \
        singlylinkedlist_get_head_item, \
        singlylinkedlist_get_next_item, \
        singlylinkedlist_find, \
        singlylinkedlist_item_get_value, \
        singlylinkedlist_remove_if, \
        singlylinkedlist_foreach \
    )



SINGLYLINKEDLIST_HANDLE real_singlylinkedlist_create(void);
void real_singlylinkedlist_destroy(SINGLYLINKEDLIST_HANDLE list);
LIST_ITEM_HANDLE real_singlylinkedlist_add(SINGLYLINKEDLIST_HANDLE list, const void* item);
LIST_ITEM_HANDLE real_singlylinkedlist_add_head(SINGLYLINKEDLIST_HANDLE list, const void* item);
int real_singlylinkedlist_remove(SINGLYLINKEDLIST_HANDLE list, LIST_ITEM_HANDLE item_handle);
LIST_ITEM_HANDLE real_singlylinkedlist_get_head_item(SINGLYLINKEDLIST_HANDLE list);
LIST_ITEM_HANDLE real_singlylinkedlist_get_next_item(LIST_ITEM_HANDLE item_handle);
LIST_ITEM_HANDLE real_singlylinkedlist_find(SINGLYLINKEDLIST_HANDLE list, LIST_MATCH_FUNCTION match_function, const void* match_context);
const void* real_singlylinkedlist_item_get_value(LIST_ITEM_HANDLE item_handle);
int real_singlylinkedlist_remove_if(SINGLYLINKEDLIST_HANDLE list, LIST_CONDITION_FUNCTION condition_function, const void* match_context);
int real_singlylinkedlist_foreach(SINGLYLINKEDLIST_HANDLE list, LIST_ACTION_FUNCTION action_function, const void* action_context);




#endif // REAL_SINGLYLINKEDLIST_H
