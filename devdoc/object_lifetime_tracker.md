 `object_lifetime_tracker` requirements

## Overview

`object_lifetime_tracker` is a module that handles the lifetime of objects. When objects are created, they can be registered with the `object_lifetime_tracker`. When an object does not need to be tracked any longer (because it is being destroyed or simply because the using module does not need the object to be tracked any longer), the objects can be unregistered so they are no longer tracked. `object_lifetime_tracker` allows objects to be grouped by a user-provided `key`.
It stores a DList of `keys`. For every `key`, it stores a DList of `objects`. It allows for all `objects` that were registered with the same `key` to be destroyed together.


```c

#define KEY_MATCH_FUNCTION_RESULT_VALUES \
    KEY_MATCH_FUNCTION_RESULT_MATCHING, \
    KEY_MATCH_FUNCTION_RESULT_NOT_MATCHING, \
    KEY_MATCH_FUNCTION_RESULT_ERROR

MU_DEFINE_ENUM(KEY_MATCH_FUNCTION_RESULT, KEY_MATCH_FUNCTION_RESULT_VALUES);

#define OBJECT_MATCH_FUNCTION_RESULT_VALUES \
    OBJECT_MATCH_FUNCTION_RESULT_MATCHING, \
    OBJECT_MATCH_FUNCTION_RESULT_NOT_MATCHING, \
    OBJECT_MATCH_FUNCTION_RESULT_ERROR

MU_DEFINE_ENUM(OBJECT_MATCH_FUNCTION_RESULT, OBJECT_MATCH_FUNCTION_RESULT_VALUES);

#define OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT_VALUES \
    OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, \
    OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_OBJECT, \
    OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_EXISTS, \
    OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR

MU_DEFINE_ENUM(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT_VALUES);

#define OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT_VALUES \
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, \
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR, \
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_NOT_FOUND, \
    OBJECT_LIFETIME_TRACKER_UNREGISTER_KEY_NOT_FOUND

MU_DEFINE_ENUM(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT_VALUES);

typedef struct OBJECT_LIFETIME_TRACKER_TAG* OBJECT_LIFETIME_TRACKER_HANDLE;
typedef void(*DESTROY_OBJECT)(void* object, void* context);
typedef KEY_MATCH_FUNCTION_RESULT(*KEY_MATCH_FUNCTION)(const void* lhs, const void* rhs);
typedef OBJECT_MATCH_FUNCTION_RESULT(*OBJECT_MATCH_FUNCTION)(const void* lhs, const void* rhs);

MOCKABLE_FUNCTION(, OBJECT_LIFETIME_TRACKER_HANDLE, object_lifetime_tracker_create, KEY_MATCH_FUNCTION, key_match_function, OBJECT_MATCH_FUNCTION, object_match_function);
MOCKABLE_FUNCTION(, void, object_lifetime_tracker_destroy, OBJECT_LIFETIME_TRACKER_HANDLE, object_lifetime_tracker);

MOCKABLE_FUNCTION(, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, object_lifetime_tracker_register_object, OBJECT_LIFETIME_TRACKER_HANDLE, object_lifetime_tracker, const void*, key, void*, object, DESTROY_OBJECT, destroy_object, void*, destroy_context);
MOCKABLE_FUNCTION(, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, object_lifetime_tracker_unregister_object, OBJECT_LIFETIME_TRACKER_HANDLE, object_lifetime_tracker, const void*, key, void*, object);
MOCKABLE_FUNCTION(, void, object_lifetime_tracker_destroy_all_objects_for_key, OBJECT_LIFETIME_TRACKER_HANDLE, object_lifetime_tracker, const void*, key);
```

### object_lifetime_tracker_create

```c
MOCKABLE_FUNCTION(, OBJECT_LIFETIME_TRACKER_HANDLE, object_lifetime_tracker_create, KEY_MATCH_FUNCTION, key_match_function, OBJECT_MATCH_FUNCTION, object_match_function);
```

`object_lifetime_tracker_create` creates a object lifetime tracker. `key_match_function` is a user-provided function that compares two keys. It returns `KEY_MATCH_FUNCTION_RESULT_MATCHING` if they match, `KEY_MATCH_FUNCTION_RESULT_NOT_MATCHING` if they do not match, and `KEY_MATCH_FUNCTION_RESULT_ERROR` if there was an error.

**SRS_OBJECT_LIFETIME_TRACKER_43_001: [** If `key_match_function` is `NULL`, `object_lifetime_tracker_create` shall fail and return `NULL`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_067: [** If `object_match_function` is `NULL`, `object_lifetime_create` shall fail and return `NULL` **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_002: [** `object_lifetime_tracker_create` shall allocate memory for the object lifetime tracker. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_003: [** `object_lifetime_tracker_create` shall call `srw_lock_create`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_004: [** `object_lifetime_tracker_create` shall call `DList_InitializeListHead` to initialize a DList for storing keys. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_005: [** `object_lifetime_tracker_create` shall succeed and return the created object lifetime tracker. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_006: [** If there are any failures, `object_lifetime_tracker_create` shall fail and return `NULL`. **]**

### object_lifetime_tracker_destroy

```c
MOCKABLE_FUNCTION(, void, object_lifetime_tracker_destroy, OBJECT_LIFETIME_TRACKER_HANDLE, object_lifetime_tracker);
```

`object_lifetime_tracker_create` destroys a given object lifetime tracker. It does not free the objects that were registered with it.

**SRS_OBJECT_LIFETIME_TRACKER_43_007: [** If `object_lifetime_tracker` is `NULL`, `object_lifetime_tracker_destroy` shall return. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_010: [** `object_lifetime_tracker_destroy` shall call `srw_lock_destroy`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_011: [** `object_lifetime_tracker_destroy` shall free the memory for the object lifetime tracker. **]**

### is_same_key

```c
static int is_same_key(PDLIST_ENTRY listEntry, const void* key_match_context, bool* continueProcessing);
```

`is_same_key` is a helper function used by `DList_ForEach` to compare keys using the user-provided `key_match_function`. `listEntry` points to the `key` already in the DList that is being checked. `key_match_context` contains the `key` the user is trying to find, the user-provided `key_match_function`, and a field to store the matched `PDLIST_ENTRY`.

**SRS_OBJECT_LIFETIME_TRACKER_43_050: [** `is_same_key` shall call `key_match_function` on the obtained key from `listEntry` and the key in `key_match_context`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_054: [** If `key_match_function` returns `KEY_MATCH_FUNCTION_RESULT_MATCHING`: **]**

 - **SRS_OBJECT_LIFETIME_TRACKER_43_055: [** `is_same_key` shall set `continueProcessing` to `false`. **]**

 - **SRS_OBJECT_LIFETIME_TRACKER_43_056: [** `is_same_key` shall store `listEntry` in `key_match_context`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_057: [** If `key_match_function` returns `KEY_MATCH_FUNCTION_RESULT_NOT_MATCHING`, `is_same_key` shall set `continueProcessing` to `true`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_058: [** `is_same_key` shall succeed and return zero. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_059: [** If there are any failures, `is_same_key` shall fail and return a non-zero value. **]**


### is_same_object

```c
static int is_same_object(PDLIST_ENTRY listEntry, const void* object_match_context, bool* continueProcessing);
```

`is_same_object` is a helper function used by `DList_ForEach` to compare objects using the user-provided `object_match_function`. `listEntry` points to the `object` already in the DList that is being checked. `object_match_context` contains the `object` the user is trying to find, the user-provided `object_match_function`, and a field to store the matched `PDLIST_ENTRY`.

**SRS_OBJECT_LIFETIME_TRACKER_43_068: [** `is_same_object` shall call `object_match_function` on the obtained object from `listEntry` and the object in `object_match_context`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_069: [** If `object_match_function` returns `OBJECT_MATCH_FUNCTION_RESULT_MATCHING`: **]**

 - **SRS_OBJECT_LIFETIME_TRACKER_43_070: [** `is_same_object` shall set `continueProcessing` to `false`.  **]**

 - **SRS_OBJECT_LIFETIME_TRACKER_43_071: [** `is_same_object` shall store `listEntry` in `object_match_context`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_072: [** If `object_match_function` returns `OBJECT_MATCH_FUNCTION_RESULT_NOT_MATCHING`, `is_same_object` shall set `continueProcessing` to `true`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_073: [** `is_same_object` shall succeed and return zero. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_074: [** If there are any failures, `is_same_object` shall fail and return a non-zero value. **]**

### object_lifetime_tracker_register_object

```c
MOCKABLE_FUNCTION(, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, object_lifetime_tracker_register_object, OBJECT_LIFETIME_TRACKER_HANDLE, object_lifetime_tracker, const void*, key, void*, object, DESTROY_OBJECT, destroy_object, void*, destroy_context);
```
`object_lifetime_tracker_register_object` registers an object belonging to a key with the object lifetime tracker. Once an object is registered with a given `key`, it can be destroyed using the given `destroy_object` destructor when `object_lifetime_tracker_destroy_all_objects_for_key` is called. `object_lifetime_tracker_register_object` does not allow an `object` to be registered with the same `key` multiple times.

**SRS_OBJECT_LIFETIME_TRACKER_43_012 : [** If `object_lifetime_tracker` is `NULL`, `object_lifetime_tracker_register_object` shall fail and return `OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_014: [** If `object` is `NULL`, `object_lifetime_tracker_register_object` shall fail and return `OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_015: [** If `destroy_object` is `NULL`, `object_lifetime_tracker_register_object` shall fail and return `OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_075: [** `object_lifetime_tracker_register_object` shall allow `destroy_context` to be `NULL`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_016: [** `object_lifetime_tracker_register_object` shall acquire the lock in exclusive mode. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_017: [** `object_lifetime_tracker_register_object` shall find the list entry for the given `key` in the DList of keys by calling `DList_ForEach` with `is_same_key`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_045: [** If the `key` is not found in the DList of keys: **]**

  - **SRS_OBJECT_LIFETIME_TRACKER_43_043: [** `object_lifetime_tracker_register_object` shall allocate memory to store data associated with the `key`. **]**

  - **SRS_OBJECT_LIFETIME_TRACKER_43_044: [** `object_lifetime_tracker_register_object` shall initialize a DList to store objects associated with the `key` by calling `DList_InitializeListHead`. **]**

  - **SRS_OBJECT_LIFETIME_TRACKER_43_018: [** `object_lifetime_tracker_register_object` shall add the given `key` to the DList of keys by calling `DList_InsertHeadList`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_076: [** `object_lifetime_tracker_register_object` shall find the list entry for the given `object` in the DList of objects for the given `key` by calling `DList_ForEach` with `is_same_object`.
 **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_077: [** If the `object` is not found in the DList of objects:
 **]**
  - **SRS_OBJECT_LIFETIME_TRACKER_43_060: [** `object_lifetime_tracker_register_object` shall allocate memory to store data associated with the `object`. **]**
  - **SRS_OBJECT_LIFETIME_TRACKER_43_019: [** `object_lifetime_tracker_register_object` shall store the given `object` with the given `destroy_object` and `destroy_context` in the DList of objects for given `key` by calling `DList_InsertHeadList`. **]**
  - **SRS_OBJECT_LIFETIME_TRACKER_43_078: [** If the given `key` had not been found, `object_lifetime_tracker_registeer_object` shall return `OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY`. **]**
  - **SRS_OBJECT_LIFETIME_TRACKER_43_079: [** `object_lifetime_tracker_register_object` shall return `OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_OBJECT`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_080: [** If the `object` is found in the DList of objects, `object_lifetime_tracker_register_object` shall return `OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_EXISTS`.
 **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_051: [** `object_lifetime_tracker_register_object` shall release the lock. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_021: [** If there are any failures, `object_lifetime_tracker_register_object` shall fail and return `OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR`. **]**

### object_lifetime_tracker_unregister_object

```c
MOCKABLE_FUNCTION(, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, object_lifetime_tracker_unregister_object, OBJECT_LIFETIME_TRACKER_HANDLE, object_lifetime_tracker, const void*, key, void*, object);
```
`object_lifetime_tracker_unregister_object` unregisters an object that was already registered with the object lifetime tracker. Once an object is unregistered, it shall not be destroyed when `object_lifetime_tracker_destroy_all_objects_for_key` is called.

**SRS_OBJECT_LIFETIME_TRACKER_43_022: [** If `object_lifetime_tracker` is `NULL`, `object_lifetime_tracker_unregister_object` shall fail and return `OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_024: [** If `object` is `NULL`, `object_lifetime_tracker_unregister_object` shall fail and return `OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_025: [** `object_lifetime_tracker_unregister_object` shall acquire the lock in exclusive mode. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_026: [** `object_lifetime_tracker_unregister_object` shall find the list entry for the given `key` in the DList of keys by calling `DList_ForEach` with `is_same_key`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_046: [** If the given `key` is not found, `object_lifetime_tracker_unregister_object` shall return `OBJECT_LIFETIME_TRACKER_UNREGISTER_KEY_NOT_FOUND`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_027: [** `object_lifetime_tracker_unregister_object` shall find the list entry for the given `object` in the DList of objects for the given `key` by calling `DList_ForEach` with `is_same_object`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_047: [** If the given `object` is not found, `object_lifetime_tracker_unregister_object` shall return `OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_NOT_FOUND`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_029: [** `object_lifetime_tracker_unregister_object` shall remove the list entry for the given `object` from the DList of objects for the given `key` by calling `DList_RemoveEntryList`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_061: [** `object_lifetime_tracker_unregister_object` shall free the memory associated with the given `object`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_030: [** If the DList of objects for the given `key` is empty: **]**

 - **SRS_OBJECT_LIFETIME_TRACKER_43_062: [** `object_lifetime_tracker_unregister_object` shall remove the list entry for the given `key` from the DList of keys by calling `DList_RemoveEntryList`. **]**

 - **SRS_OBJECT_LIFETIME_TRACKER_43_063: [** `object_lifetime_tracker_unregister_object` shall free the memory associated with the given `key`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_052: [** `object_lifetime_tracker_unregister_object` shall release the lock.
 **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_031: [** `object_lifetime_tracker_unregister_object` shall succeed and return `OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_032: [** If there are any failures, `object_lifetime_tracker_unregister_object` shall fail and return `OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR`. **]**

### object_lifetime_tracker_destroy_all_objects_for_key

```c
MOCKABLE_FUNCTION(, void, object_lifetime_tracker_destroy_all_objects_for_key, OBJECT_LIFETIME_TRACKER_HANDLE, object_lifetime_tracker, const void*, key);
```
`object_lifetime_tracker_destroy_all_objects_for_key` destroys all the objects belonging to the given key by calling the respective `destroy_object` function that was given when each object was registered.

**SRS_OBJECT_LIFETIME_TRACKER_43_033: [** If `object_lifetime_tracker` is `NULL`, `object_lifetime_tracker_destroy_all_objects_for_key` shall return. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_035: [** `object_lifetime_tracker_destroy_all_objects_for_key` shall acquire the lock in exclusive mode. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_036: [** `object_lifetime_tracker_destroy_all_objects_for_key` shall find the list entry for the given `key` in the DList of keys by calling `DList_ForEach` with `is_same_key`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_048: [** If the given `key` is not found, `object_lifetime_tracker_destroy_all_objects_for_key` shall return. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_038: [** `object_lifetime_tracker_destroy_all_objects_for_key` shall remove the list entries for all the objects in the DList of objects for the given `key` by calling `DList_RemoveHeadList` for each list entry. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_037: [** `object_lifetime_tracker_destroy_all_objects_for_key` shall destroy all the objects in the DList of objects for the given `key` in the reverse order in which they were registered by calling `destroy_object` with `destroy_context` as `context`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_066: [** `object_lifetime_tracker_destroy_all_objects_for_key` shall free the memory associated with all the objects. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_039: [** `object_lifetime_tracker_destroy_all_objects_for_key` shall remove the list entry for the given `key` from the DList of keys by calling `DList_RemoveEntryList`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_065: [** `object_lifetime_tracker_destroy_all_objects_for_key` shall free the memory associated with the given `key`. **]**

**SRS_OBJECT_LIFETIME_TRACKER_43_053: [** `object_lifetime_tracker_destroy_all_objects_for_key` shall release the lock. **]**
