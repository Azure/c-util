# slist requirements
================

## Overview

`slist` is module that provides the functionality of a singly linked list, allowing its user to add, remove and iterate the list elements.

## Exposed API

```c
typedef struct SLIST_ENTRY_TAG
{
    struct SLIST_ENTRY_TAG *next;
} SLIST_ENTRY, *PSLIST_ENTRY;

typedef bool (*SLIST_MATCH_FUNCTION)(PSLIST_ENTRY list_entry, const void* match_context);
typedef bool (*SLIST_CONDITION_FUNCTION)(PSLIST_ENTRY list_entry, const void* match_context, bool* continue_processing);
typedef int (*SLIST_ACTION_FUNCTION)(PSLIST_ENTRY list_entry, const void* action_context, bool* continue_processing);

MOCKABLE_FUNCTION(, bool, slist_initialize, PSLIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, bool, slist_is_empty, const PSLIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, slist_add, PSLIST_ENTRY, list_head, PSLIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, slist_add_head, PSLIST_ENTRY, list_head, PSLIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, slist_remove, PSLIST_ENTRY, list_head, PSLIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, slist_remove_head, PSLIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, PSLIST_ENTRY, slist_find, PSLIST_ENTRY, list_head, SLIST_MATCH_FUNCTION, match_function, void*, match_context);
MOCKABLE_FUNCTION(, int, slist_remove_if, PSLIST_ENTRY, list_head, SLIST_CONDITION_FUNCTION, condition_function, void*, match_context);
MOCKABLE_FUNCTION(, int, slist_for_each, PSLIST_ENTRY, list_head, SLIST_ACTION_FUNCTION, action_function, void*, action_context);

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
PSLIST_ENTRY list_entry = slist_remove_head(list_head);
DATA* data = CONTAINING_RECORD(list_entry, DATA, anchor);
```

### slist_initialize
```c
bool slist_initialize(PSLIST_ENTRY list_head);
```

If `list_head` is `NULL`, `slist_initialize` shall fail and return `NULL`. 

`slist_initialize` shall initialize the `next` pointer in `list_head` points to `NULL`and return true on success. 

### slist_is_empty
```c
bool slist_is_empty(const PSLIST_ENTRY list_head);
```

`slist_is_empty` shall return true if there is no SLIST_ENTRY in this list. 

`slist_is_empty` shall return false if there is one or more entris in the list. 

Notes:
1. `slist_is_empty` shall be undefined if the `SLIST_ENTRY` is not currently part of a list.
2. `slist_is_empty` shall be undefined if the `SLIST_ENTRY` has not been initialized as with `slist_initialize`.

### slist_add
```c
PSLIST_ENTRY slist_add(PSLIST_ENTRY list_head, PSLIST_ENTRY list_entry);
```

If `list_head` is `NULL`, `slist_add` shall fail and return `NULL`.

If `list_entry` is `NULL`, `slist_add` shall fail and return `NULL`.

`slist_add` shall add one entry to the tail of the list on success and return an entry to the added entry. 

### slist_add_head
```
PSLIST_ENTRY slist_add_head(PSLIST_ENTRY list_head, PSLIST_ENTRY list_entry);
```

If `list_head` is `NULL`, `slist_add_head` shall fail and return `NULL`. 

If `list_entry` is `NULL`, `slist_add_head` shall fail and return `NULL`.

`slist_add_head` shall insert `list_entry` at head on success and return an entry to the added entry. 

### slist_remove
```c
PSLIST_ENTRY slist_remove(PSLIST_ENTRY list_head, PSLIST_ENTRY list_entry);
```

If `list_head` is `NULL`, `slist_remove` shall fail and return `NULL`. 

If `list_entry` is `NULL`, `slist_remove` shall fail and return `NULL`.

`slist_remove` shall remove a list entry from the list on success and return a pointer to the new head entry. 

If the entry `list_entry` is not found in the list, then `slist_remove` shall fail and `NULL`. 

### slist_remove_head
```c
PSLIST_ENTRY slist_remove_head(PDLIST_ENTRY list_head);
```

`slist_remove_head` removes the head entry from the list defined by the `list_head` parameter on success and return a pointer to the new head entry. 

`slist_remove_head` shall return `NULL` if that's the only entry in the list. 

### slist_find
```c
PSLIST_ENTRY slist_find(PSLIST_ENTRY list_head, SLIST_MATCH_FUNCTION match_function, void* match_context);
```

If `list_head` is `NULL`, `slist_find` shall fail and return `NULL`.

If `match_function` is `NULL`, `slist_find` shall fail and return `NULL`.

`slist_find` shall iterate through all entris in the list and return the first entry that satisfies the `match_function` on success. 

`slist_find` shall determine whether an item satisfies the match criteria by invoking the `match_function` for each entry in the list until a matching entry is found. 

The `match_function` shall get as arguments the list entry being attempted to be matched and the `match_context` as is. 

If the `match_function` returns false, `slist_find` shall consider that item as not matching. 

If the `match_function` returns true, `slist_find` shall consider that item as matching. 

If the list is empty, `slist_find` shall return `NULL`. 

### slist_remove_if
```c
int slist_remove_if(PSLIST_ENTRY list_head, SLIST_CONDITION_FUNCTION condition_function, void* match_context);
```

If `list_head` is `NULL`, `slist_remove_if` shall fail and return a non-zero value.

If `condition_function` is `NULL`, `slist_remove_if` shall fail and return a non-zero value. 

`slist_remove_if` shall iterate through all entris in a list, remove all that satisfies the `condition_function` and return zero on success. 

`slist_remove_if` shall determine whether an entry satisfies the condition criteria by invoking the condition function for that entry. 

If the `condition_function` returns true, `slist_remove_if` shall consider that entry as to be removed. 

If the `condition_function` returns false, `slist_remove_if` shall consider that entry as not to be removed. 

If the `condition_function` returns `continue_processing` as false, `slist_remove_if` shall stop iterating through the list and return. 

### slist_for_each
```c
int slist_for_each(PSLIST_ENTRY list_head, SLIST_ACTION_FUNCTION action_function, void* action_context);
```
If `list_head` is `NULL`, `slist_for_each` shall fail and return a non-zero value.

If `action_function` is `NULL`, `slist_for_each` shall fail and return a non-zero value. 

`slist_for_each` shall iterate through all entris in the list, invoke `action_function` for each one of them and return zero on success. 

If the `action_function` fails, `slist_for_each` shall fail and return a non-zero value.

If the `action_function` returns `continue_processing` as false, `slist_for_each` shall stop iterating through the list and return.  

