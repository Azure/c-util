singlylinkedlist2 requirements
================

## Overview

SinglyLinkedList is module that provides the functionality of a singly linked list, allowing its user to add, remove and iterate the list elements.

## Exposed API

```c
typedef struct SLIST_ENTRY_TAG
{
    struct SLIST_ENTRY_TAG *next;
} SLIST_ENTRY, *PSLIST_ENTRY;

typedef bool (*SLIST_MATCH_FUNCTION)(PSLIST_ENTRY listEntry, void* matchContext);
typedef bool (*SLIST_CONDITION_FUNCTION)(PSLIST_ENTRY listEntry, void* matchContext, bool* continueProcessing);
typedef int (*SLIST_ACTION_FUNCTION)(PSLIST_ENTRY listEntry, void* action_context, bool* continue_processing);

MOCKABLE_FUNCTION(, void, SList_InitializeListHead, PSLIST_ENTRY, listHead);
MOCKABLE_FUNCTION(, int, SList_IsListEmpty, const PSLIST_ENTRY, listHead);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, SList_Add, PSLIST_ENTRY, listHead, PSLIST_ENTRY, listEntry);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, SList_AddHead, PSLIST_ENTRY, listHead, PSLIST_ENTRY, listEntry);
MOCKABLE_FUNCTION(, int, SList_Remove, PSLIST_ENTRY, listHead, PSLIST_ENTRY, listEntry);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, SList_RemoveHead, PSLIST_ENTRY, listHead);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, SList_GetNextItem, PSLIST_ENTRY, listEntry);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, SList_Find, PSLIST_ENTRY, listHead, SLIST_MATCH_FUNCTION, matchFunction, void*, matchContext);
MOCKABLE_FUNCTION(, int, SList_RemoveIf, PSLIST_ENTRY, listHead, SLIST_CONDITION_FUNCTION, conditionFunction, void*, matchContext);
MOCKABLE_FUNCTION(, int, SList_ForEach, PSLIST_ENTRY, listHead, SLIST_ACTION_FUNCTION, actionFunction, void*, actionContext);

```
## Usage

`SLIST_ENTRY` does not store any data. Instead, `SLIST_ENTRY` is placed inside a struct:

```c
typedef struct DATA_TAG
{
    int num1;
    int num2;
    SLIST_ENTRY anchor;
} DATA;
```

The parent struct of a list entry can be obtained using `CONTAINING_RECORD`:

```
PSLIST_ENTRY listEntry = SList_RemoveHead(listHead);
DATA* data = CONTAINING_RECORD(listEntry, DATA, anchor);
```

### SList_InitializeListHead
```c
void SList_InitializeListHead(PSLIST_ENTRY listHead);
```
SList_InitializeListHead will initialize the listHead next pointer points to NULL. 

### SList_IsListEmpty
```c
int SList_IsListEmpty(const PSLIST_ENTRY listHead);
```

SList_IsListEmpty shall return a non-zero value if there are no SLIST_ENTRY's on this list other than the list head. 

SList_IsListEmpty shall return 0 if there is one or more items in the list. 

Notes:
1.	SList_IsListEmpty shall be undefined if the SLIST_ENTRY is not currently part of a list.
2.	SList_IsListEmpty shall be undefined if the SLIST_ENTRY has not been initialized as with SList_InitializeListHead.

### SList_Add
```c
PSLIST_ENTRY SList_Add(PSLIST_ENTRY listHead, PSLIST_ENTRY listEntry);
```

SList_Add shall add one item to the tail of the list and on success it shall return an entry to the added item. 

If any of the arguments is NULL, SList_Add shall not add the item to the list and return NULL. 

If allocating the new list node fails, SList_Add shall return NULL. 

### SList_AddHead
```
PSLIST_ENTRY SList_AddHead(PSLIST_ENTRY listHead, PSLIST_ENTRY listEntry);
```

`SList_AddHead` shall insert `item` at head on success it shall return an entry to the added item. 

If any of the arguments is NULL, SList_AddHead shall not add the item to the list and return NULL. 

If allocating the new list node fails, SList_AddHead shall return NULL.

### SList_Remove
```c
int SList_Remove(PSLIST_ENTRY listHead, PSLIST_ENTRY listEntry);
```

SList_Remove shall remove a list item from the list and on success it shall return 0. 

If any of the arguments list or item_handle is NULL, SList_Remove shall fail and return a non-zero value. 

If the item item_handle is not found in the list, then singlylinkedlist_remove shall fail and return a non-zero value. 

### SList_RemoveHead
```c
PDLIST_ENTRY SList_RemoveHead(PDLIST_ENTRY listHead);
```

SList_RemoveHead removes the oldest entry from the list defined by the listHead parameter and returns a pointer to that entry. 

SList_RemoveHead shall return listHead if that's the only item in the list. 

Note: The next field of the returned PDLIST_ENTRY shall be undefined.

### SList_GetNextItem
```c
PDLIST_ENTRY SList_GetNextItem(PSLIST_ENTRY, listEntry);
```

SList_GetNextItem shall return the next item in the list following the entry listEntry. 

If listEntry is NULL then SList_GetNextItem shall return NULL. 

If no more items exist in the list after the listEntry entry, SList_GetNextItem shall return NULL. 

### SList_Find
```c
PSLIST_ENTRY SList_Find(PSLIST_ENTRY listHead, SLIST_MATCH_FUNCTION matchFunction, void* matchContext);
```

SList_Find shall iterate through all items in a list and return the first one that satisfies a certain match function. 

If the list or the matchFunction argument is NULL, SList_Find shall return NULL. 

SList_Find shall determine whether an item satisfies the match criteria by invoking the match function for each item in the list until a matching item is found. 

The matchFunction shall get as arguments the list item being attempted to be matched and the matchContext as is. 

If the matchFunction returns false, SList_Find shall consider that item as not matching. 

If the matchFunction returns true, SList_Find shall consider that item as matching. 

If the list is empty, SList_Find shall return NULL. 

### SList_RemoveIf
```c
int SList_RemoveIf(PSLIST_ENTRY listHead, SLIST_CONDITION_FUNCTION conditionFunction, void* matchContext);
```

If the list or the conditionFunction argument is NULL, SList_RemoveIf shall return non-zero value. 

SList_RemoveIf shall iterate through all items in a list and remove all that satisfies a certain condition function. 

SList_RemoveIf shall determine whether an entry satisfies the condition criteria by invoking the condition function for that entry. 

If the conditionFunction returns true, SList_RemoveIf shall consider that item as to be removed. 

If the conditionFunction returns false, SList_RemoveIf shall consider that item as not to be removed. 

If the conditionFunction returns continue_processing as false, SList_RemoveIf shall stop iterating through the list and return. 

If no errors occur, SList_RemoveIf shall return zero. 

### SList_ForEach
```c
int SList_ForEach(PSLIST_ENTRY listHead, SLIST_ACTION_FUNCTION actionFunction, void* actionContext);
```

If the list or the actionFunction argument is NULL, SList_ForEach shall return non-zero value. 

SList_ForEach shall iterate through all items in a list and invoke actionFunction for each one of them. 

If the actionFunction returns continue_processing as false, SList_ForEach shall stop iterating through the list and return. 

If no errors occur, SList_ForEach shall return zero. 

