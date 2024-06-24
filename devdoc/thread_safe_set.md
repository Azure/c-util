# `thread_safe_set` requirements

## Overview

`thread_safe_set` is a module that implements a thread-safe set of `void*` elements. It is intended to be used in a multi-threaded environment where multiple threads can add and remove elements from the set.


## Threading model

`thread_safe_set` uses a single lock to protect the set from concurrent access. 

## Exposed API

```c

#define THREAD_SAFE_SET_INSERT_RESULT_VALUES \
    THREAD_SAFE_SET_INSERT_OK, \
    THREAD_SAFE_SET_INSERT_KEY_ALREADY_EXISTS, \
    THREAD_SAFE_SET_INSERT_ERROR \

MU_DEFINE_ENUM(THREAD_SAFE_SET_INSERT_RESULT, THREAD_SAFE_SET_INSERT_RESULT_VALUES);

#define THREAD_SAFE_SET_REMOVE_RESULT_VALUES \
    THREAD_SAFE_SET_REMOVE_OK, \
    THREAD_SAFE_SET_REMOVE_NOT_FOUND, \
    THREAD_SAFE_SET_REMOVE_ERROR \

MU_DEFINE_ENUM(THREAD_SAFE_SET_REMOVE_RESULT, THREAD_SAFE_SET_REMOVE_RESULT_VALUES);

#define THREAD_SAFE_SET_CONTAINS_RESULT_VALUES \
    THREAD_SAFE_SET_CONTAINS_FOUND, \
    THREAD_SAFE_SET_CONTAINS_NOT_FOUND, \
    THREAD_SAFE_SET_CONTAINS_ERROR

MU_DEFINE_ENUM(THREAD_SAFE_SET_CONTAINS_RESULT, THREAD_SAFE_SET_CONTAINS_RESULT_VALUES);

/*
    * @brief Function to compare two elements in the set.
    *
    * @param lhs The left hand side of the comparison.
    * @param rhs The right hand side of the comparison.
    *
    * @return true if the elements are equal, false otherwise.
*/
typedef bool(*THREAD_SAFE_SET_ELEMENT_MATCH_FUNCTION)(void* lhs, void* rhs);

/*
    * @brief Function to perform an action on an element in the set.
    *
    * @param item The item to perform the action on.
    * @param action_context The context for the action.
    * @param continue_processing Indicates if the action should continue processing.
*/
typedef void (*THREAD_SAFE_SET_ACTION_FUNCTION)(const void* item, const void* action_context, bool* continue_processing);

/*
    * @brief Function to determine if an element should be removed from the set.
    *
    * @param item The item to evaluate.
    * @param match_context The context for the evaluation.
    * @param continue_processing Indicates if the evaluation should continue processing.
    *
    * @return true if the item should be removed, false otherwise.
*/
typedef bool(*THREAD_SAFE_SET_CONDITION_FUNCTION)(const void* item, const void* match_context, bool* continue_processing);

typedef struct THREAD_SAFE_SET_TAG* THREAD_SAFE_SET_HANDLE;

MOCKABLE_FUNCTION(, THREAD_SAFE_SET_HANDLE, thread_safe_set_create, THREAD_SAFE_SET_ELEMENT_MATCH_FUNCTION, match_function);
MOCKABLE_FUNCTION(, void, thread_safe_set_destroy, THREAD_SAFE_SET_HANDLE, thread_safe_set);

MOCKABLE_FUNCTION(, THREAD_SAFE_SET_INSERT_RESULT, thread_safe_set_insert, THREAD_SAFE_SET_HANDLE, thread_safe_set, void*, element);
MOCKABLE_FUNCTION(, THREAD_SAFE_SET_REMOVE_RESULT, thread_safe_set_remove, THREAD_SAFE_SET_HANDLE, thread_safe_set, void*, element);

MOCKABLE_FUNCTION(, THREAD_SAFE_SET_CONTAINS_RESULT, thread_safe_set_contains, THREAD_SAFE_SET_HANDLE, thread_safe_set, void*, element);
MOCKABLE_FUNCTION(, int, thread_safe_set_foreach, THREAD_SAFE_SET_HANDLE, thread_safe_set, THREAD_SAFE_SET_ACTION_FUNCTION, action_function, void*, action_context);

MOCKABLE_FUNCTION(, int, thread_safe_set_remove_if, THREAD_SAFE_SET_HANDLE, thread_safe_set, THREAD_SAFE_SET_CONDITION_FUNCTION, condition_function, const void*, condition_context);

```

## thread_safe_set_create

```c
MOCKABLE_FUNCTION(, THREAD_SAFE_SET_HANDLE, thread_safe_set_create, THREAD_SAFE_SET_ELEMENT_MATCH_FUNCTION, match_function);
```

Creates a new thread-safe set that uses the provided match function to compare elements in the set.

**SRS_THREAD_SAFE_SET_43_001: [** If `match_function` is NULL, `thread_safe_set_create` shall fail and return `NULL`. **]**

**SRS_THREAD_SAFE_SET_43_002: [** `thread_safe_set_create` shall allocate memory for the new set. **]**

**SRS_THREAD_SAFE_SET_43_003: [** `thread_safe_set_create` shall call `srw_lock_create`. **]**

**SRS_THREAD_SAFE_SET_43_004: [** `thread_safe_set_create` shall call `singlylinkedlist_create` to create a list of elements. **]**

**SRS_THREAD_SAFE_SET_43_005: [** `thread_safe_set_create` shall return a non-`NULL` handle on success. **]**

**SRS_THREAD_SAFE_SET_43_006: [** If there are any failures, `thread_safe_set_create` shall fail and return `NULL`. **]**

## thread_safe_set_destroy

```c

MOCKABLE_FUNCTION(, void, thread_safe_set_destroy, THREAD_SAFE_SET_HANDLE, thread_safe_set);

```

Destroys a thread-safe set.

**SRS_THREAD_SAFE_SET_43_007: [** If `thread_safe_set` is `NULL`, `thread_safe_set_destroy` shall return without doing anything. **]**

**SRS_THREAD_SAFE_SET_43_OO8: [** `thread_safe_set_destroy` shall call `singlylinkedlist_destroy` **]**

**SRS_THREAD_SAFE_SET_43_009: [** `thread_safe_set_destroy` shall call `srw_lock_destroy` **]**

**SRS_THREAD_SAFE_SET_43_010: [** `thread_safe_set_destroy` shall free the given `thread_safe_set` handle. **]**

## thread_safe_set_insert

```c

#define THREAD_SAFE_SET_INSERT_RESULT_VALUES \
    THREAD_SAFE_SET_INSERT_OK, \
    THREAD_SAFE_SET_INSERT_KEY_ALREADY_EXISTS, \
    THREAD_SAFE_SET_INSERT_ERROR \

MU_DEFINE_ENUM(THREAD_SAFE_SET_INSERT_RESULT, THREAD_SAFE_SET_INSERT_RESULT_VALUES);

MOCKABLE_FUNCTION(, THREAD_SAFE_SET_INSERT_RESULT, thread_safe_set_insert, THREAD_SAFE_SET_HANDLE, thread_safe_set, void*, element);

```

Inserts an element into the set.

**SRS_THREAD_SAFE_SET_43_011: [** If `thread_safe_set` is `NULL`, `thread_safe_set_insert` shall return `THREAD_SAFE_SET_INSERT_ERROR`. **]**

**SRS_THREAD_SAFE_SET_43_012: [** If `element` is `NULL`, `thread_safe_set_insert` shall return `THREAD_SAFE_SET_INSERT_ERROR`. **]**

**SRS_THREAD_SAFE_SET_43_013: [** `thread_safe_set_insert` shall call `srw_lock_acquire_exclusive`. **]**

**SRS_THREAD_SAFE_SET_43_014: [** `thread_safe_set_insert` shall call `singlylinkedlist_find` to find the given element. **]**

**SRS_THREAD_SAFE_SET_43_015: [** If the element is found, `thread_safe_set_insert` shall return `THREAD_SAFE_SET_INSERT_KEY_ALREADY_EXISTS`. **]**

**SRS_THREAD_SAFE_SET_43_016: [** If the element is not found, `thread_safe_set_insert` shall call `singlylinkedlist_add` to add the element to the list. **]**

**SRS_THREAD_SAFE_SET_43_017: [** `thread_safe_set_insert` shall call `srw_lock_release_exclusive`. **]**

**SRS_THREAD_SAFE_SET_43_018: [** `thread_safe_set_insert` shall succeed and return `THREAD_SAFE_SET_INSERT_OK`. **]**

**SRS_THREAD_SAFE_SET_43_019: [** If there are any failures, `thread_safe_set_insert` shall return `THREAD_SAFE_SET_INSERT_ERROR`. **]**

## thread_safe_set_remove

```c

#define THREAD_SAFE_SET_REMOVE_RESULT_VALUES \
    THREAD_SAFE_SET_REMOVE_OK, \
    THREAD_SAFE_SET_REMOVE_NOT_FOUND, \
    THREAD_SAFE_SET_REMOVE_ERROR, \

MU_DEFINE_ENUM(THREAD_SAFE_SET_REMOVE_RESULT, THREAD_SAFE_SET_REMOVE_RESULT_VALUES);

MOCKABLE_FUNCTION(, THREAD_SAFE_SET_REMOVE_RESULT, thread_safe_set_remove, THREAD_SAFE_SET_HANDLE, thread_safe_set, void*, element);

```

Removes an element from the set.

**SRS_THREAD_SAFE_SET_43_020: [** If `thread_safe_set` is `NULL`, `thread_safe_set_remove` shall return `THREAD_SAFE_SET_REMOVE_ERROR`. **]**

**SRS_THREAD_SAFE_SET_43_021: [** If `element` is `NULL`, `thread_safe_set_remove` shall return `THREAD_SAFE_SET_REMOVE_ERROR`. **]**

**SRS_THREAD_SAFE_SET_43_022: [** `thread_safe_set_remove` shall call `srw_lock_acquire_exclusive`. **]**

**SRS_THREAD_SAFE_SET_43_023: [** `thread_safe_set_remove` shall call `singlylinkedlist_find` to find the given element. **]**

**SRS_THREAD_SAFE_SET_43_024: [** If the element is not found, `thread_safe_set_remove` shall return `THREAD_SAFE_SET_REMOVE_NOT_FOUND`. **]**

**SRS_THREAD_SAFE_SET_43_025: [** If the element is found, `thread_safe_set_remove` shall call `singlylinkedlist_remove` to remove the element from the list. **]**

**SRS_THREAD_SAFE_SET_43_026: [** `thread_safe_set_remove` shall call `srw_lock_release_exclusive`. **]**

**SRS_THREAD_SAFE_SET_43_027: [** `thread_safe_set_remove` shall succeed and return `THREAD_SAFE_SET_REMOVE_OK`. **]**

**SRS_THREAD_SAFE_SET_43_028: [** If there are any failures, `thread_safe_set_remove` shall return `THREAD_SAFE_SET_REMOVE_ERROR`. **]**

## thread_safe_set_contains

```c

MOCKABLE_FUNCTION(, THREAD_SAFE_SET_CONTAINS_RESULT, thread_safe_set_contains, THREAD_SAFE_SET_HANDLE, thread_safe_set, void*, element);

```

Determines if the set contains the given element.

**SRS_THREAD_SAFE_SET_43_029: [** If `thread_safe_set` is `NULL`, `thread_safe_set_contains` shall return `THREAD_SAFE_SET_CONTAINS_ERROR`. **]**

**SRS_THREAD_SAFE_SET_43_030: [** If `element` is `NULL`, `thread_safe_set_contains` shall return `THREAD_SAFE_SET_CONTAINS_ERROR`. **]**

**SRS_THREAD_SAFE_SET_43_031: [** `thread_safe_set_contains` shall call `srw_lock_acquire_shared`. **]**

**SRS_THREAD_SAFE_SET_43_032: [** `thread_safe_set_contains` shall call `singlylinkedlist_find` to find the given element. **]**

**SRS_THREAD_SAFE_SET_43_033: [** If the element is not found, `thread_safe_set_contains` shall return `THREAD_SAFE_SET_CONTAINS_NOT_FOUND`. **]**

**SRS_THREAD_SAFE_SET_43_034: [** If the element is found, `thread_safe_set_contains` shall return `THREAD_SAFE_SET_CONTAINS_FOUND`. **]**

**SRS_THREAD_SAFE_SET_43_035: [** `thread_safe_set_contains` shall call `srw_lock_release_shared`. **]**

## thread_safe_set_foreach

```c
typedef void (*THREAD_SAFE_SET_ACTION_FUNCTION)(const void* item, const void* action_context, bool* continue_processing);

MOCKABLE_FUNCTION(, int, thread_safe_set_foreach, THREAD_SAFE_SET_HANDLE, thread_safe_set, THREAD_SAFE_SET_ACTION_FUNCTION, action_function, void*, action_context);

```

Performs an action on each element in the set.

**SRS_THREAD_SAFE_SET_43_036: [** If `thread_safe_set` is `NULL`, `thread_safe_set_foreach` shall return a non-zero value. **]**

**SRS_THREAD_SAFE_SET_43_037: [** If `action_function` is `NULL`, `thread_safe_set_foreach` shall return a non-zero value. **]**

**SRS_THREAD_SAFE_SET_43_038: [** `thread_safe_set_foreach` shall call `srw_lock_acquire_shared`. **]**

**SRS_THREAD_SAFE_SET_43_039: [** `thread_safe_set_foreach` shall call `singlylinkedlist_foreach` to iterate over the list of elements.  **]**

**SRS_THREAD_SAFE_SET_43_040: [** `thread_safe_set_foreach` shall call `action_function` for each element in the list. **]**

**SRS_THREAD_SAFE_SET_43_041: [** `thread_safe_set_foreach` shall call `srw_lock_release_shared`. **]**

**SRS_THREAD_SAFE_SET_43_042: [** `thread_safe_set_foreach` shall succeed and return 0. **]**

**SRS_THREAD_SAFE_SET_43_043: [** If there are any failures, `thread_safe_set_foreach` shall fail and return a non-zero value. **]**

## thread_safe_set_remove_if

```c

typedef bool(*THREAD_SAFE_SET_CONDITION_FUNCTION)(const void* item, const void* match_context, bool* continue_processing);

MOCKABLE_FUNCTION(, int, thread_safe_set_remove_if, THREAD_SAFE_SET_HANDLE, thread_safe_set, THREAD_SAFE_SET_CONDITION_FUNCTION, condition_function, const void*, condition_context);

```

Removes elements from the set that meet a given condition.

**SRS_THREAD_SAFE_SET_43_044: [** If `thread_safe_set` is `NULL`, `thread_safe_set_remove_if` shall return a non-zero value. **]**

**SRS_THREAD_SAFE_SET_43_045: [** If `condition_function` is `NULL`, `thread_safe_set_remove_if` shall return a non-zero value. **]**

**SRS_THREAD_SAFE_SET_43_046: [** `thread_safe_set_remove_if` shall call `srw_lock_acquire_exclusive`. **]**

**SRS_THREAD_SAFE_SET_43_047: [** `thread_safe_set_remove_if` shall call `singlylinkedlist_remove_if` to remove elements that meet the condition. **]**

**SRS_THREAD_SAFE_SET_43_048: [** `thread_safe_set_remove_if` shall call `srw_lock_release_exclusive`. **]**

**SRS_THREAD_SAFE_SET_43_049: [** `thread_safe_set_remove_if` shall succeed and return 0. **]**

**SRS_THREAD_SAFE_SET_43_050: [** If there are any failures, `thread_safe_set_remove_if` shall fail and return a non-zero value. **]**
