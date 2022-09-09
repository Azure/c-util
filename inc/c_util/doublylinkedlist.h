// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DOUBLYLINKEDLIST_H
#define DOUBLYLINKEDLIST_H

#ifdef __cplusplus
#else
#include <stdbool.h>
#endif

#include "macro_utils/macro_utils.h"

typedef struct DLIST_ENTRY_TAG
{
    struct DLIST_ENTRY_TAG *Flink;
    struct DLIST_ENTRY_TAG *Blink;
} DLIST_ENTRY, *PDLIST_ENTRY;

typedef int (*DLIST_ACTION_FUNCTION)(PDLIST_ENTRY listEntry, void* actionContext, bool* continueProcessing);

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif
MOCKABLE_FUNCTION(, void, DList_InitializeListHead, PDLIST_ENTRY, listHead);
MOCKABLE_FUNCTION(, int, DList_IsListEmpty, const PDLIST_ENTRY, listHead);
MOCKABLE_FUNCTION(, void, DList_InsertTailList, PDLIST_ENTRY, listHead, PDLIST_ENTRY, listEntry);
MOCKABLE_FUNCTION(, void, DList_InsertHeadList, PDLIST_ENTRY, listHead, PDLIST_ENTRY, listEntry);
MOCKABLE_FUNCTION(, void, DList_AppendTailList, PDLIST_ENTRY, listHead, PDLIST_ENTRY, ListToAppend);
MOCKABLE_FUNCTION(, int, DList_RemoveEntryList, PDLIST_ENTRY, listEntry);
MOCKABLE_FUNCTION(, PDLIST_ENTRY, DList_RemoveHeadList, PDLIST_ENTRY, listHead);
MOCKABLE_FUNCTION(, PDLIST_ENTRY, DList_RemoveTailList, PDLIST_ENTRY, listHead);
MOCKABLE_FUNCTION(, int, DList_ForEach, PDLIST_ENTRY, listHead, DLIST_ACTION_FUNCTION, actionFunction, void*, actionContext);

#ifdef __cplusplus
}
#endif

#endif /* DOUBLYLINKEDLIST_H */
