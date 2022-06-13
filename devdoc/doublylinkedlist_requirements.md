DoublyLinkedList Requirements
================

## Overview

These functions are the basic linking structures used throughout NT.
They have been publicly available since 1992.
Note that the lists do NOT store any data, by design.
They simply provide a consistent mechanism for linking homogenous items. Also note that no input error checking is provided for this set of APIs.

## References

[http://www.dcl.hpi.uni-potsdam.de/research/WRK/2009/07/single_list_entry-containing_record/](http://www.dcl.hpi.uni-potsdam.de/research/WRK/2009/07/single_list_entry-containing_record/)

## Exposed API
```c
typedef struct DLIST_ENTRY_TAG
{
    struct DLIST_ENTRY_TAG *Flink;
    struct DLIST_ENTRY_TAG *Blink;
} DLIST_ENTRY, *PDLIST_ENTRY;

#define DLIST_MATCH_FUNCTION_RESULT_VALUES \
    DLIST_MATCH_FUNCTION_MATCHING, \
    DLIST_MATCH_FUNCTION_NOT_MATCHING, \
    DLIST_MATCH_FUNCTION_ERROR

MU_DEFINE_ENUM(DLIST_CONDITION_FUNCTION_RESULT, DLIST_CONDITION_FUNCTION_VALUES);

#define DLIST_CONDITION_FUNCTION_RESULT_VALUES \
    DLIST_CONDITION_FUNCTION_SATISFIED, \
    DLIST_CONDITION_FUNCTION_NOT_SATISFIED, \
    DLIST_CONDITION_FUNCTION_ERROR

MU_DEFINE_ENUM(DLIST_CONDITION_FUNCTION_RESULT, DLIST_CONDITION_FUNCTION_RESULT_VALUES);

typedef DLIST_CONDITION_FUNCTION_RESULT (*DLIST_MATCH_FUNCTION)(PDLIST_ENTRY listEntry, const void* matchContext);
typedef DLIST_CONDITION_FUNCTION_RESULT (*DLIST_CONDITION_FUNCTION)(PDLIST_ENTRY listEntry, const void* conditionContext, bool* continueProcessing);
typedef int (*DLIST_ENTRY_DESTROY_FUNCTION)(PDLIST_ENTRY listEntry, const void* destroyContext);
typedef int (*DLIST_ACTION_FUNCTION)(PDLIST_ENTRY listEntry, const void* actionContext, bool* continueProcessing);

extern void DList_InitializeListHead(PDLIST_ENTRY listHead);
extern int DList_IsListEmpty(const PDLIST_ENTRY listHead);
extern void DList_InsertTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY listEntry);
extern void DList_InsertHeadList(PDLIST_ENTRY listHead, PDLIST_ENTRY entry);
extern void DList_AppendTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY ListToAppend);
extern int DList_RemoveEntryList(PDLIST_ENTRY listEntry);
extern PDLIST_ENTRY DList_RemoveHeadList(PDLIST_ENTRY listHead);
extern PDLIST_ENTRY DList_Find(PDLIST_ENTRY listHead, DLIST_MATCH_FUNCTION matchFunction, const void* matchContext);
extern PDLIST_ENTRY DList_RemoveIf(PDLIST_ENTRY listHead, DLIST_CONDITION_FUNCTION conditionFunction, const void* conditionContext, DLIST_ENTRY_DESTROY_FUNCTION destroyFunction, const void* destroyContext);
extern int DList_ForEach(PDLIST_ENTRY listHead, DLIST_ACTION_FUNCTION actionFunction, const void* actionContext);

//
// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure.
//

#define CONTAINING_RECORD(address, type, field) ((type *)((char *)(address) - offsetof(type,field)))
```

### DList_InitializeListHead
```c
extern void DList_InitializeListHead(PDLIST_ENTRY listHead);
```
**SRS_DLIST_06_005: [** DList_InitializeListHead will initialize the Flink & Blink to the address of the DLIST_ENTRY. **]**

### DList_IsListEmpty
```c
extern int DList_IsListEmpty(const PDLIST_ENTRY listHead);
```

**SRS_DLIST_06_003: [** DList_IsListEmpty shall return a non-zero value if there are no DLIST_ENTRY's on this list other than the list head. **]**

**SRS_DLIST_06_004: [** DList_IsListEmpty shall return 0 if there is one or more items in the list. **]**

Notes:
1.	DList_IsListEmpty shall be undefined if the DLIST_ENTRY is not currently part of a list.
2.	DList_IsListEmpty shall be undefined if the DLIST_ENTRY has not been initialized as with DList_InitializeListHead.

### DList_InsertTailList
```c
extern void DList_InsertTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY listEntry);
```

**SRS_DLIST_06_006: [** DList_InsertTailList shall place the DLIST_ENTRY at the end of the list defined by the listHead parameter. **]**

### DList_InsertHeadList
```c
extern void DList_InsertHeadList(PDLIST_ENTRY listHead, PDLIST_ENTRY entry);
```

**SRS_DLIST_02_002: [** DList_InsertHeadList inserts a singular entry in the list having as head listHead after "head". **]** (see diagram below)

DList_InsertHeadList shall assume listHead and listEntry non-NULL and pointing to existing DLIST_ENTRY structures. Calling DList_InsertHeadList with NULL parameters is undefined behavior.

### DList_AppendTailList
```c
extern void DList_AppendTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY ListToAppend);
```

**SRS_DLIST_06_007: [** DList_AppendTailList shall place the list defined by listToAppend at the end of the list defined by the listHead parameter. **]**

### DList_RemoveEntryList
```c
extern int DList_RemoveEntryList(PDLIST_ENTRY listEntry);
```

**SRS_DLIST_06_008: [** DList_RemoveEntryList shall remove a listEntry from whatever list it is properly part of. **]**

**SRS_DLIST_06_009: [** The remaining list is properly formed. **]**

**SRS_DLIST_06_010: [** DList_RemoveEntryList shall return non-zero if the remaining list is empty. **]**

**SRS_DLIST_06_011: [** DList_RemoveEntryList shall return zero if the remaining list is NOT empty. **]**

Notes:
1.	If listEntry is NOT part of a list then the result of this function is undefined. Therefore, calling DList_RemoveEntryList twice on the same listEntry is undefined.
2.	The Flink & Blink of the listEntry parameter shall be undefined after calling this function

### DList_RemoveHeadList
```c
extern PDLIST_ENTRY DList_RemoveHeadList(PDLIST_ENTRY listHead);
```

**SRS_DLIST_06_012: [** DList_RemoveHeadList removes the oldest entry from the list defined by the listHead parameter and returns a pointer to that entry. **]**

**SRS_DLIST_06_013: [** DList_RemoveHeadList shall return listHead if that's the only item in the list. **]**

Note: The Flink & Blink of the returned PDLIST_ENTRY shall be undefined.

### DList_Find
```c
MOCKABLE_FUNCTION(, PDLIST_ENTRY, DList_Find, PDLIST_ENTRY, listHead, DLIST_MATCH_FUNCTION, matchFunction, const void*, matchContext);
```

**SRS_DLIST_43_001: [** `DList_Find` shall call `matchFunction` on each entry in the list defined by `listHead` along with `matchContext`. **]**

**SRS_DLIST_43_002: [** If the call to `matchFunction` for an entry returns `DLIST_MATCH_FUNCTION_MATCHING`, `DList_Find` shall return that entry. **]**

**SRS_DLIST_43_003: [** If no calls to `matchFunction` return `DLIST_MATCH_FUNCTION_MATCHING`, `DList_Find` shall return `NULL`. **]**

### DList_Remove_If
```c
MOCKABLE_FUNCTION(, PDLIST_ENTRY, DList_RemoveIf, PDLIST_ENTRY, listHead, DLIST_CONDITION_FUNCTION, conditionFunction, const void*, conditionContext, DLIST_ENTRY_DESTROY_FUNCTION, destroyFunction, const void*, destroyContext);
```

**SRS_DLIST_43_004: [** `DList_RemoveIf` shall call `conditionFunction` on each entry in the list defined by `listHead` along with `conditionContext`. **]**

**SRS_DLIST_43_005: [** If the call to `conditionFunction` for an entry returns `DLIST_CONDITION_FUNCTION_SATISFIED`:**]**

 - **SRS_DLIST_43_013: [** `DList_RemoveIf` shall remove the entry from the list. **]**

 - **SRS_DLIST_43_014: [** `DList_RemoveIf` shall call `destroyFunction` on the entry. **]**

**SRS_DLIST_43_006: [** If `continueProcessing` is `false`, `DList_RemoveIf` shall stop iterating over the list. **]**

**SRS_DLIST_43_007: [** `DList_RemoveIf` shall return the head of the list after entries have been removed. **]**

**SRS_DLIST_43_008: [** If all entries were removed, `DList_RemoveIf` shall return `NULL`. **]**

### DList_ForEach
```c
MOCKABLE_FUNCTION(, int, DList_ForEach, PDLIST_ENTRY, listHead, DLIST_ACTION_FUNCTION, actionFunction, const void*, actionContext);
```

**SRS_DLIST_43_009: [** `DList_ForEach` shall call `actionFunction` on each entry in the list defined by `listHead` along with `actionContext`. **]**

**SRS_DLIST_43_010: [** If `continueProcessing` is `false`, `DList_ForEach` shall stop iterating over the list. **]**

**SRS_DLIST_43_011: [** If any call to `actionFunction` returned a non-zero value, `DList_ForEach` shall fail and return a non-zero value. **]**

**SRS_DLIST_43_012: [** If all calls to `actionFunction` returned zero, DList_ForEach` shall succeed and return zero. **]**
