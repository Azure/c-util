# s_list requirements
================

## Overview

`s_list` is module that provides the functionality of a singly linked list, allowing its user to add, remove and iterate the list elements.

## Exposed API

```c
typedef struct S_LIST_ENTRY_TAG
{
    struct S_LIST_ENTRY_TAG *next;
} S_LIST_ENTRY, *PS_LIST_ENTRY;

typedef enum
{
    S_LIST_IS_EMPTY_RESULT_EMPTY,
    S_LIST_IS_EMPTY_RESULT_NOT_EMPTY,
    S_LIST_IS_EMPTY_RESULT_INVALID_ARGS,
} S_LIST_IS_EMPTY_RESULT;

typedef bool (*S_LIST_MATCH_FUNCTION)(PS_LIST_ENTRY list_entry, const void* match_context);
typedef bool (*S_LIST_CONDITION_FUNCTION)(PS_LIST_ENTRY list_entry, const void* match_context, bool* continue_processing);
typedef int (*S_LIST_ACTION_FUNCTION)(PS_LIST_ENTRY list_entry, const void* action_context, bool* continue_processing);

MOCKABLE_FUNCTION(, int, s_list_initialize, PS_LIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, S_LIST_IS_EMPTY_RESULT, s_list_is_empty, const PS_LIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, PS_LIST_ENTRY, s_list_add, PS_LIST_ENTRY, list_head, PS_LIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, int, s_list_add_head, PS_LIST_ENTRY, list_head, PS_LIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, int, s_list_remove, PS_LIST_ENTRY, list_head, PS_LIST_ENTRY, list_entry);
MOCKABLE_FUNCTION(, PS_LIST_ENTRY, s_list_remove_head, PS_LIST_ENTRY, list_head);
MOCKABLE_FUNCTION(, PS_LIST_ENTRY, s_list_find, PS_LIST_ENTRY, list_head, S_LIST_MATCH_FUNCTION, match_function, const void*, match_context);
MOCKABLE_FUNCTION(, int, s_list_remove_if, PS_LIST_ENTRY, list_head, S_LIST_CONDITION_FUNCTION, condition_function, const void*, match_context);
MOCKABLE_FUNCTION(, int, s_list_for_each, PS_LIST_ENTRY, list_head, S_LIST_ACTION_FUNCTION, action_function, const void*, action_context);

```
## Usage

`S_LIST_ENTRY` does not store any data. Instead, `S_LIST_ENTRY` is placed inside a struct:

```c
typedef struct DATA_TAG
{
    int num1;
    int num2;
    S_LIST_ENTRY anchor;
} DATA;
```

The parent struct of a list entry can be obtained using `CONTAINING_RECORD`:

```
PS_LIST_ENTRY list_entry = s_list_remove_head(list_head);
DATA* data = CONTAINING_RECORD(list_entry, DATA, anchor);
```

### s_list_initialize
```c
int s_list_initialize(PS_LIST_ENTRY list_head);
```

**SRS_S_LIST_07_001: [** If `list_head` is `NULL`, `s_list_initialize` shall fail and return a non-zero value. **]**

**SRS_S_LIST_07_002: [** `s_list_initialize` shall initialize the `next` pointer in `list_head` points to `NULL`and return zero on success.  **]**

### s_list_is_empty
```c
S_LIST_IS_EMPTY_RESULT s_list_is_empty(const PS_LIST_ENTRY list_head);
```
**SRS_S_LIST_07_038: [** If `list_head` is `NULL`, `s_list_is_empty` shall fail and return `INVALID_ARGS`.  **]**

**SRS_S_LIST_07_003: [** `s_list_is_empty` shall return `EMPTY` if there is no `S_LIST_ENTRY` in this list. **]** 

**SRS_S_LIST_07_004: [** `s_list_is_empty` shall return `NOT_EMPTY` if there is one or more entris in the list. **]** 

Notes:
1. `s_list_is_empty` shall be undefined if the `S_LIST_ENTRY` is not currently part of a list.
2. `s_list_is_empty` shall be undefined if the `S_LIST_ENTRY` has not been initialized as with `s_list_initialize`.

### s_list_add
```c
int s_list_add(PS_LIST_ENTRY list_head, PS_LIST_ENTRY list_entry);
```

**SRS_S_LIST_07_005: [** If `list_head` is `NULL`, `s_list_add` shall fail and return a non-zero value. **]**

**SRS_S_LIST_07_006: [** If `list_entry` is `NULL`, `s_list_add` shall fail and return a non-zero value. **]**

**SRS_S_LIST_07_007: [** `s_list_add` shall add one entry to the tail of the list and return zero on success. **]** 

### s_list_add_head
```
int s_list_add_head(PS_LIST_ENTRY list_head, PS_LIST_ENTRY list_entry);
```

**SRS_S_LIST_07_008: [** If `list_head` is `NULL`, `s_list_add_head` shall fail and return a non-zero value. **]** 

**SRS_S_LIST_07_009: [** If `list_entry` is `NULL`, `s_list_add_head` shall fail and return a non-zero value. **]**

**SRS_S_LIST_07_010: [** `s_list_add_head` shall insert `list_entry` at head and return zero on success. **]** 

### s_list_remove
```c
int s_list_remove(PS_LIST_ENTRY list_head, PS_LIST_ENTRY list_entry);
```

**SRS_S_LIST_07_011: [** If `list_head` is `NULL`, `s_list_remove` shall fail and return a non-zero value.  **]**

**SRS_S_LIST_07_012: [** If `list_entry` is `NULL`, `s_list_remove` shall fail and return a non-zero value. **]**

**SRS_S_LIST_07_013: [** `s_list_remove` shall remove a list entry from the list and return zero on success. **]** 

**SRS_S_LIST_07_014: [** If the entry `list_entry` is not found in the list, then `s_list_remove` shall fail and a non-zero value. **]** 

### s_list_remove_head
```c
PS_LIST_ENTRY s_list_remove_head(PS_LIST_ENTRY list_head);
```

**SRS_S_LIST_07_015: [** If `list_head` is `NULL`, `s_list_remove_head` shall fail and return `NULL`.  **]**

**SRS_S_LIST_07_016: [** `s_list_remove_head` removes the head entry from the list defined by the `list_head` parameter on success and return a pointer to that entry. **]** 

**SRS_S_LIST_07_017: [** `s_list_remove_head` shall return `list_head` if that's the only entry in the list. **]** 

### s_list_find
```c
PS_LIST_ENTRY s_list_find(PS_LIST_ENTRY list_head, S_LIST_MATCH_FUNCTION match_function, const void* match_context);
```

**SRS_S_LIST_07_018: [** If `list_head` is `NULL`, `s_list_find` shall fail and return `NULL`. **]**

**SRS_S_LIST_07_019: [** If `match_function` is `NULL`, `s_list_find` shall fail and return `NULL`. **]**

**SRS_S_LIST_07_020: [** `s_list_find` shall iterate through all entris in the list and return the first entry that satisfies the `match_function` on success. **]** 

**SRS_S_LIST_07_021: [** `s_list_find` shall determine whether an item satisfies the match criteria by invoking the `match_function` for each entry in the list until a matching entry is found. **]** 

**SRS_S_LIST_07_022: [** The `match_function` shall get as arguments the list entry being attempted to be matched and the `match_context` as is.  **]**

**SRS_S_LIST_07_023: [** If the `match_function` returns `false`, `s_list_find` shall consider that item as not matching. **]** 

**SRS_S_LIST_07_024: [** If the `match_function` returns `true`, `s_list_find` shall consider that item as matching. **]** 

**SRS_S_LIST_07_025: [** If the list is empty, `s_list_find` shall return `NULL`. **]** 

**SRS_S_LIST_07_039: [** If the item is not found, `s_list_find` shall return `NULL`. **]**

### s_list_remove_if
```c
int s_list_remove_if(PS_LIST_ENTRY list_head, S_LIST_CONDITION_FUNCTION condition_function, const void* match_context);
```

**SRS_S_LIST_07_026: [** If `list_head` is `NULL`, `s_list_remove_if` shall fail and return a non-zero value. **]**

**SRS_S_LIST_07_027: [** If `condition_function` is `NULL`, `s_list_remove_if` shall fail and return a non-zero value.  **]**

**SRS_S_LIST_07_040: [** If the list is empty, `s_list_find` shall return a non-zero value. **]**

**SRS_S_LIST_07_028: [** `s_list_remove_if` shall iterate through all entris in a list, remove all that satisfies the `condition_function` and return zero. **]** 

**SRS_S_LIST_07_029: [** `s_list_remove_if` shall determine whether an entry satisfies the condition criteria by invoking the condition function for that entry. **]** 

**SRS_S_LIST_07_030: [** If the `condition_function` returns `true`, `s_list_remove_if` shall consider that entry as to be removed. **]** 

**SRS_S_LIST_07_031: [** If the `condition_function` returns `false`, `s_list_remove_if` shall consider that entry as not to be removed. **]** 

**SRS_S_LIST_07_032: [** If the `condition_function` returns `continue_processing` as `false`, `s_list_remove_if` shall stop iterating through the list and return. **]** 

### s_list_for_each
```c
int s_list_for_each(PS_LIST_ENTRY list_head, S_LIST_ACTION_FUNCTION action_function, void* action_context);
```

**SRS_S_LIST_07_033: [** If `list_head` is `NULL`, `s_list_for_each` shall fail and return a non-zero value. **]**

**SRS_S_LIST_07_034: [** If `action_function` is `NULL`, `s_list_for_each` shall fail and return a non-zero value. **]** 

**SRS_S_LIST_07_035: [** `s_list_for_each` shall iterate through all entries in the list, invoke `action_function` for each one of them and return zero on success.  **]**

**SRS_S_LIST_07_036: [** If the `action_function` fails, `s_list_for_each` shall fail and return a non-zero value. **]**

**SRS_S_LIST_07_037: [** If the `action_function` returns `continue_processing` as `false`, `s_list_for_each` shall stop iterating through the list and return. **]**  

