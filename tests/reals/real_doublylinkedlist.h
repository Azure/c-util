// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_DOUBLYLINKEDLIST_H
#define REAL_DOUBLYLINKEDLIST_H

#include "macro_utils/macro_utils.h"
#include "c_util/doublylinkedlist.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_DOUBLYLINKEDLIST_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        DList_InitializeListHead, \
        DList_IsListEmpty, \
        DList_InsertTailList, \
        DList_InsertHeadList, \
        DList_AppendTailList, \
        DList_RemoveEntryList, \
        DList_RemoveHeadList, \
        DList_Find, \
        DList_RemoveIf, \
        DList_ForEach \
    )


#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void real_DList_InitializeListHead(PDLIST_ENTRY listHead);
int real_DList_IsListEmpty(const PDLIST_ENTRY listHead);
void real_DList_InsertTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY listEntry);
void real_DList_InsertHeadList(PDLIST_ENTRY listHead, PDLIST_ENTRY listEntry);
void real_DList_AppendTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY ListToAppend);
int real_DList_RemoveEntryList(PDLIST_ENTRY listEntry);
PDLIST_ENTRY real_DList_RemoveHeadList(PDLIST_ENTRY listHead);
int DList_Find(PDLIST_ENTRY listHead, DLIST_MATCH_FUNCTION matchFunction, const void* matchContext, PDLIST_ENTRY* foundEntry);
int DList_RemoveIf(PDLIST_ENTRY listHead, DLIST_CONDITION_FUNCTION conditionFunction, const void* conditionContext, DLIST_ENTRY_DESTROY_FUNCTION destroyFunction, const void* destroyContext, PDLIST_ENTRY* newHead);
int real_DList_ForEach(PDLIST_ENTRY listHead, DLIST_ACTION_FUNCTION actionFunction, const void* actionContext);

#ifdef __cplusplus
}
#endif

#endif // REAL_DOUBLYLINKEDLIST_H
