// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SINGLYLINKEDLIST2_H
#define SINGLYLINKEDLIST2_H

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
* @brief                          Function passed to SList_Find, which returns whichever first list item that matches it.
* @param listEntry                Current list node being evaluated.
* @param matchContext             Context passed to SList_Find.
* @returns                        True to indicate that the current list node is the one to be returned, or false to continue traversing the list.
*/
typedef bool (*SLIST_MATCH_FUNCTION)(PSLIST_ENTRY listEntry, void* matchContext);

/**
* @brief                          Function passed to SList_RemoveIf, which is used to define if an item of the list should be removed or not.
* @param listEntry                Value of the current list node being evaluated for removal.
* @param matchContext             Context passed to SList_RemoveIf.
* @param continueProcessing       Indicates if SList_RemoveIf shall continue iterating through the next nodes of the list or stop.
* @returns                        True to indicate that the current list node shall be removed, or false to not to.
*/
typedef bool (*SLIST_CONDITION_FUNCTION)(PSLIST_ENTRY listEntry, void* matchContext, bool* continueProcessing);

/**
* @brief                          Function passed to SList_ForEach, which is called for the value of each node of the list.
* @param listEntry                Value of the current list node being processed.
* @param actionContext            Context passed to SList_ForEach.
* @param continueProcessing       Indicates if SList_ForEach shall continue iterating through the next nodes of the list or stop.
*/
typedef int (*SLIST_ACTION_FUNCTION)(PSLIST_ENTRY listEntry, void* actionContext, bool* continueProcessing);

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif
MOCKABLE_FUNCTION(, void, SList_InitializeListHead, PSLIST_ENTRY, listHead);
MOCKABLE_FUNCTION(, int, SList_IsListEmpty, const PSLIST_ENTRY, listHead);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, SList_Add, PSLIST_ENTRY, listHead, PSLIST_ENTRY, listEntry);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, SList_AddHead, PSLIST_ENTRY, listHead, PSLIST_ENTRY, listEntry);
MOCKABLE_FUNCTION(, int, SList_Remove, PSLIST_ENTRY, listHead, PSLIST_ENTRY, listEntry);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, SList_RemoveHeadList, PSLIST_ENTRY, listHead);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, SList_GetNextItem, PSLIST_ENTRY, listEntry);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, SList_Find, PSLIST_ENTRY, listHead, SLIST_MATCH_FUNCTION, matchFunction, void*, matchContext);
MOCKABLE_FUNCTION(, int, SList_RemoveIf, PSLIST_ENTRY, listHead, SLIST_CONDITION_FUNCTION, conditionFunction, void*, matchContext);
MOCKABLE_FUNCTION(, int, SList_ForEach, PSLIST_ENTRY, listHead, SLIST_ACTION_FUNCTION, actionFunction, void*, actionContext);

#ifdef __cplusplus
}
#endif

#endif /* SINGLYLINKEDLIST2_H */
