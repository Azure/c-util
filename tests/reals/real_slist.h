// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_SLIST_H
#define REAL_SLIST_H

#include "macro_utils/macro_utils.h"
#include "c_util/slist.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_SLIST_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        slist_initialize, \
        slist_is_empty, \
        slist_add, \
        slist_add_head, \
        slist_remove, \
        slist_remove_head, \
        slist_find, \
        slist_remove_if, \
        slist_for_each \
    )


#include <stddef.h>


bool real_slist_initialize(PSLIST_ENTRY list_head);
bool real_slist_is_empty(const PSLIST_ENTRY list_head);
PSLIST_ENTRY real_slist_add(PSLIST_ENTRY list_head, PSLIST_ENTRY list_entry);
PSLIST_ENTRY real_slist_add_head(PSLIST_ENTRY list_head, PSLIST_ENTRY list_entry);
PSLIST_ENTRY real_slist_remove(PSLIST_ENTRY list_head, PSLIST_ENTRY list_entry);
PSLIST_ENTRY real_slist_remove_head(PSLIST_ENTRY list_head);
PSLIST_ENTRY real_slist_find(PSLIST_ENTRY list_head, SLIST_MATCH_FUNCTION match_function, void* match_context);
PSLIST_ENTRY real_slist_remove_if(PSLIST_ENTRY list_head, SLIST_CONDITION_FUNCTION condition_function, void* match_context);
int real_slist_for_each(PSLIST_ENTRY list_head, SLIST_ACTION_FUNCTION action_function, void* action_context);


#endif // REAL_SLIST_H
