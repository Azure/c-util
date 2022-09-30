# slist requirements
================

## Overview

`slist` is module that provides the functionality of a singly linked list, allowing its user to add, remove and iterate the list elements.

## Exposed API

```c
typedef struct SINGLYLINKEDLIST_ENTRY_TAG
{
    struct SINGLYLINKEDLIST_ENTRY_TAG *next;
} SINGLYLINKEDLIST_ENTRY, *PSINGLYLINKEDLIST_ENTRY;

typedef bool (*SLIST_MATCH_FUNCTION)(PSINGLYLINKEDLIST_ENTRY list_entry, const void* match_context);
typedef bool (*SLIST_CONDITION_FUNCTION)(PSINGLYLINKEDLIST_ENTRY list_entry, const void* match_context, bool* continue_processing);
typedef int (*SLIST_ACTION_FUNCTION)(PSINGLYLINKEDLIST_ENTRY list_entry, const void* action_context, bool* continue_processing);

MOCKABLE_FUNCTION(, bool, slist_initialize, PSINGLYLINKEDLIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, bool, slist_is_empty, const PSINGLYLINKEDLIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, PSINGLYLINKEDLIST_ENTRY, slist_add, PSINGLYLINKEDLIST_ENTRY, list_head, PSINGLYLINKEDLIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, PSINGLYLINKEDLIST_ENTRY, slist_add_head, PSINGLYLINKEDLIST_ENTRY, list_head, PSINGLYLINKEDLIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, int, slist_remove, PSINGLYLINKEDLIST_ENTRY, list_head, PSINGLYLINKEDLIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, PSINGLYLINKEDLIST_ENTRY, slist_remove_head, PSINGLYLINKEDLIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, PSINGLYLINKEDLIST_ENTRY, slist_find, PSINGLYLINKEDLIST_ENTRY, list_head, SLIST_MATCH_FUNCTION, match_function, const void*, match_context);
MOCKABLE_FUNCTION(, int, slist_remove_if, PSINGLYLINKEDLIST_ENTRY, list_head, SLIST_CONDITION_FUNCTION, condition_function, const void*, match_context);
MOCKABLE_FUNCTION(, int, slist_for_each, PSINGLYLINKEDLIST_ENTRY, list_head, SLIST_ACTION_FUNCTION, action_function, const void*, action_context);

```
## Usage

`SINGLYLINKEDLIST_ENTRY` does not store any data. Instead, `SINGLYLINKEDLIST_ENTRY` is placed inside a struct:

```c
typedef struct DATA_TAG
{
    int num1;
    int num2;
    SINGLYLINKEDLIST_ENTRY anchor;
} DATA;
```

The parent struct of a list entry can be obtained using `CONTAINING_RECORD`:

```
PSINGLYLINKEDLIST_ENTRY list_entry = slist_remove_head(list_head);
DATA* data = CONTAINING_RECORD(list_entry, DATA, anchor);
```

### slist_initialize
```c
bool slist_initialize(PSINGLYLINKEDLIST_ENTRY list_head);
```

**SRS_SLIST_07_001: [** If `list_head` is `NULL`, `slist_initialize` shall fail and return `false`. **]**

**SRS_SLIST_07_002: [** `slist_initialize` shall initialize the `next` pointer in `list_head` points to `NULL`and return `true` on success.  **]**

### slist_is_empty
```c
bool slist_is_empty(const PSINGLYLINKEDLIST_ENTRY list_head);
```
**SRS_SLIST_07_038: [** If `list_head` is `NULL`, `slist_is_empty` shall fail and return `true`.  **]**

**SRS_SLIST_07_003: [** `slist_is_empty` shall return `true` if there is no `SINGLYLINKEDLIST_ENTRY` in this list. **]** 

**SRS_SLIST_07_004: [** `slist_is_empty` shall return `false` if there is one or more entris in the list. **]** 

Notes:
1. `slist_is_empty` shall be undefined if the `SINGLYLINKEDLIST_ENTRY` is not currently part of a list.
2. `slist_is_empty` shall be undefined if the `SINGLYLINKEDLIST_ENTRY` has not been initialized as with `slist_initialize`.

### slist_add
```c
PSINGLYLINKEDLIST_ENTRY slist_add(PSINGLYLINKEDLIST_ENTRY list_head, PSINGLYLINKEDLIST_ENTRY list_entry);
```

**SRS_SLIST_07_005: [** If `list_head` is `NULL`, `slist_add` shall fail and return `NULL`. **]**

**SRS_SLIST_07_006: [** If `list_entry` is `NULL`, `slist_add` shall fail and return `NULL`. **]**

**SRS_SLIST_07_007: [** `slist_add` shall add one entry to the tail of the list on success and return a pointer to the added entry. **]** 

### slist_add_head
```
PSINGLYLINKEDLIST_ENTRY slist_add_head(PSINGLYLINKEDLIST_ENTRY list_head, PSINGLYLINKEDLIST_ENTRY list_entry);
```

**SRS_SLIST_07_008: [** If `list_head` is `NULL`, `slist_add_head` shall fail and return `NULL`. **]** 

**SRS_SLIST_07_009: [** If `list_entry` is `NULL`, `slist_add_head` shall fail and return `NULL`. **]**

**SRS_SLIST_07_010: [** `slist_add_head` shall insert `list_entry` at head on success and return an entry to the added entry. **]** 

### slist_remove
```c
int slist_remove(PSINGLYLINKEDLIST_ENTRY list_head, PSINGLYLINKEDLIST_ENTRY list_entry);
```

**SRS_SLIST_07_011: [** If `list_head` is `NULL`, `slist_remove` shall fail and return a non-zero value.  **]**

**SRS_SLIST_07_012: [** If `list_entry` is `NULL`, `slist_remove` shall fail and return a non-zero value. **]**

**SRS_SLIST_07_013: [** `slist_remove` shall remove a list entry from the list and return zero on success. **]** 

**SRS_SLIST_07_014: [** If the entry `list_entry` is not found in the list, then `slist_remove` shall fail and a non-zero value. **]** 

### slist_remove_head
```c
PSINGLYLINKEDLIST_ENTRY slist_remove_head(PSINGLYLINKEDLIST_ENTRY list_head);
```

**SRS_SLIST_07_015: [** If `list_head` is `NULL`, `slist_remove_head` shall fail and return `NULL`.  **]**

**SRS_SLIST_07_016: [** `slist_remove_head` removes the head entry from the list defined by the `list_head` parameter on success and return a pointer to that entry. **]** 

**SRS_SLIST_07_017: [** `slist_remove_head` shall return `list_head` if that's the only entry in the list. **]** 

### slist_find
```c
PSINGLYLINKEDLIST_ENTRY slist_find(PSINGLYLINKEDLIST_ENTRY list_head, SLIST_MATCH_FUNCTION match_function, const void* match_context);
```

**SRS_SLIST_07_018: [** If `list_head` is `NULL`, `slist_find` shall fail and return `NULL`. **]**

**SRS_SLIST_07_019: [** If `match_function` is `NULL`, `slist_find` shall fail and return `NULL`. **]**

**SRS_SLIST_07_020: [** `slist_find` shall iterate through all entris in the list and return the first entry that satisfies the `match_function` on success. **]** 

**SRS_SLIST_07_021: [** `slist_find` shall determine whether an item satisfies the match criteria by invoking the `match_function` for each entry in the list until a matching entry is found. **]** 

**SRS_SLIST_07_022: [** The `match_function` shall get as arguments the list entry being attempted to be matched and the `match_context` as is.  **]**

**SRS_SLIST_07_023: [** If the `match_function` returns `false`, `slist_find` shall consider that item as not matching. **]** 

**SRS_SLIST_07_024: [** If the `match_function` returns `true`, `slist_find` shall consider that item as matching. **]** 

**SRS_SLIST_07_025: [** If the list is empty, `slist_find` shall return `NULL`. **]** 

**SRS_SLIST_07_039: [** If the item is not found, `slist_find` shall return `NULL`. **]**

### slist_remove_if
```c
int slist_remove_if(PSINGLYLINKEDLIST_ENTRY list_head, SLIST_CONDITION_FUNCTION condition_function, const void* match_context);
```

**SRS_SLIST_07_026: [** If `list_head` is `NULL`, `slist_remove_if` shall fail and return a non-zero value. **]**

**SRS_SLIST_07_027: [** If `condition_function` is `NULL`, `slist_remove_if` shall fail and return a non-zero value.  **]**

**SRS_SLIST_07_040: [** If the list is empty, `slist_find` shall return a non-zero value. **]**

**SRS_SLIST_07_028: [** `slist_remove_if` shall iterate through all entris in a list, remove all that satisfies the `condition_function` and return zero. **]** 

**SRS_SLIST_07_029: [** `slist_remove_if` shall determine whether an entry satisfies the condition criteria by invoking the condition function for that entry. **]** 

**SRS_SLIST_07_030: [** If the `condition_function` returns `true`, `slist_remove_if` shall consider that entry as to be removed. **]** 

**SRS_SLIST_07_031: [** If the `condition_function` returns `false`, `slist_remove_if` shall consider that entry as not to be removed. **]** 

**SRS_SLIST_07_032: [** If the `condition_function` returns `continue_processing` as `false`, `slist_remove_if` shall stop iterating through the list and return. **]** 

### slist_for_each
```c
int slist_for_each(PSINGLYLINKEDLIST_ENTRY list_head, SLIST_ACTION_FUNCTION action_function, void* action_context);
```

**SRS_SLIST_07_033: [** If `list_head` is `NULL`, `slist_for_each` shall fail and return a non-zero value. **]**

**SRS_SLIST_07_034: [** If `action_function` is `NULL`, `slist_for_each` shall fail and return a non-zero value. **]** 

**SRS_SLIST_07_035: [** `slist_for_each` shall iterate through all entries in the list, invoke `action_function` for each one of them and return zero on success.  **]**

**SRS_SLIST_07_036: [** If the `action_function` fails, `slist_for_each` shall fail and return a non-zero value. **]**

**SRS_SLIST_07_037: [** If the `action_function` returns `continue_processing` as `false`, `slist_for_each` shall stop iterating through the list and return. **]**  

