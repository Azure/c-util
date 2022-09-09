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

typedef int (*DLIST_ACTION_FUNCTION)(PDLIST_ENTRY listEntry, void* actionContext, bool* continueProcessing);

MOCKABLE_FUNCTION(, void, DList_InitializeListHead, PDLIST_ENTRY, listHead);
MOCKABLE_FUNCTION(, int, DList_IsListEmpty, const PDLIST_ENTRY, listHead);
MOCKABLE_FUNCTION(, void, DList_InsertTailList, PDLIST_ENTRY, listHead, PDLIST_ENTRY, listEntry);
MOCKABLE_FUNCTION(, void, DList_InsertHeadList, PDLIST_ENTRY, listHead, PDLIST_ENTRY, listEntry);
MOCKABLE_FUNCTION(, void, DList_AppendTailList, PDLIST_ENTRY, listHead, PDLIST_ENTRY, ListToAppend);
MOCKABLE_FUNCTION(, int, DList_RemoveEntryList, PDLIST_ENTRY, listEntry);
MOCKABLE_FUNCTION(, PDLIST_ENTRY, DList_RemoveHeadList, PDLIST_ENTRY, listHead);
MOCKABLE_FUNCTION(, int, DList_ForEach, PDLIST_ENTRY, listHead, DLIST_ACTION_FUNCTION, actionFunction, void*, actionContext);
```

## Usage

`DLIST_ENTRY` does not store any data. Instead, `DLIST_ENTRY` is placed inside a struct:

```c
typedef struct DATA_TAG
{
    int num1;
    int num2;
    DLIST_ENTRY anchor;
} DATA;
```

The parent struct of a list entry can be obtained using `CONTAINING_RECORD`:

```
PDLIST_ENTRY listEntry = DList_RemoveHeadList(listHead);
DATA* data = CONTAINING_RECORD(listEntry, DATA, anchor);
```

### DList_InitializeListHead
```c
void DList_InitializeListHead(PDLIST_ENTRY listHead);
```
**SRS_DLIST_06_005: [** DList_InitializeListHead will initialize the Flink & Blink to the address of the DLIST_ENTRY. **]**

### DList_IsListEmpty
```c
int DList_IsListEmpty(const PDLIST_ENTRY listHead);
```

**SRS_DLIST_06_003: [** DList_IsListEmpty shall return a non-zero value if there are no DLIST_ENTRY's on this list other than the list head. **]**

**SRS_DLIST_06_004: [** DList_IsListEmpty shall return 0 if there is one or more items in the list. **]**

Notes:
1.	DList_IsListEmpty shall be undefined if the DLIST_ENTRY is not currently part of a list.
2.	DList_IsListEmpty shall be undefined if the DLIST_ENTRY has not been initialized as with DList_InitializeListHead.

### DList_InsertTailList
```c
void DList_InsertTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY listEntry);
```

**SRS_DLIST_06_006: [** DList_InsertTailList shall place the DLIST_ENTRY at the end of the list defined by the listHead parameter. **]**

### DList_InsertHeadList
```c
void DList_InsertHeadList(PDLIST_ENTRY listHead, PDLIST_ENTRY entry);
```

**SRS_DLIST_02_002: [** DList_InsertHeadList inserts a singular entry in the list having as head listHead after "head". **]** (see diagram below)

DList_InsertHeadList shall assume listHead and listEntry non-NULL and pointing to existing DLIST_ENTRY structures. Calling DList_InsertHeadList with NULL parameters is undefined behavior.

### DList_AppendTailList
```c
void DList_AppendTailList(PDLIST_ENTRY listHead, PDLIST_ENTRY ListToAppend);
```

**SRS_DLIST_06_007: [** DList_AppendTailList shall place the list defined by listToAppend at the end of the list defined by the listHead parameter. **]**

### DList_RemoveEntryList
```c
int DList_RemoveEntryList(PDLIST_ENTRY listEntry);
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
PDLIST_ENTRY DList_RemoveHeadList(PDLIST_ENTRY listHead);
```

**SRS_DLIST_06_012: [** DList_RemoveHeadList removes the oldest entry from the list defined by the listHead parameter and returns a pointer to that entry. **]**

**SRS_DLIST_06_013: [** DList_RemoveHeadList shall return listHead if that's the only item in the list. **]**

Note: The Flink & Blink of the returned PDLIST_ENTRY shall be undefined.

### DList_RemoveTailList
```c
PDLIST_ENTRY DList_RemoveTailList(PDLIST_ENTRY listHead);
```

**SRS_DLIST_02_003: [** `DList_RemoveTailList` removes the newest entry inserted at the tail of the list defined by the `listHead` parameter and returns a pointer to that entry. **]**

Note: The pseudocode sequence: `DList_InsertTailList(1), DList_InsertTailList(2), DList_InsertTailList(3)` + `DList_RemoveTailList()` => {1,2}

**SRS_DLIST_02_004: [** `DList_RemoveTailList` shall return `listHead` if that's the only item in the list. **]**

Note: The `Flink` & `Blink` of the returned `PDLIST_ENTRY` shall be undefined.

### DList_ForEach
```c
MOCKABLE_FUNCTION(, int, DList_ForEach, PDLIST_ENTRY, listHead, DLIST_ACTION_FUNCTION, actionFunction, void*, actionContext);
```
`DList_ForEach` can be used to perform some action on each entry in a list. `actionFunction` is a user-provided function that returns zero on success and a non-zero value on error.

**SRS_DLIST_43_001: [** If `listHead` is `NULL`, `DList_ForEach` shall fail and return a non-zero value. **]**

**SRS_DLIST_43_002: [** If `actionFunction` is `NULL`, `DList_ForEach` shall fail and return a non-zero value. **]**

**SRS_DLIST_43_009: [** `DList_ForEach` shall call `actionFunction` on each entry in the list defined by `listHead` along with `actionContext`. **]**

**SRS_DLIST_43_010: [** If `continueProcessing` is `false`, `DList_ForEach` shall stop iterating over the list. **]**

**SRS_DLIST_43_011: [** `DList_ForEach` shall succeed and return zero. **]**

**SRS_DLIST_43_012: [** If there are any failures, `DList_ForEach` shall fail and return a non-zero value. **]**
