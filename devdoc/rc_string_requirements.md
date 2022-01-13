# rc_string requirements
================

## Overview

`rc_string` is a module that encapsulates a reference counted string.

## Exposed API

```c
    typedef struct RC_STRING_TAG
    {
        const char* string;
    } RC_STRING;

    THANDLE_TYPE_DECLARE(RC_STRING);

    typedef void (*RC_STRING_FREE_FUNC)(void* context);

    #define PRI_RC_STRING "s"

    #define RC_STRING_VALUE(rc) ((rc)->string)
    #define RC_STRING_VALUE_OR_NULL(rc) (((rc) == NULL) ? "NULL" : (rc)->string)

    MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create, const char*, string);
    MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create_with_move_memory, const char*, string);
    MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create_with_custom_free, const char*, string, RC_STRING_FREE_FUNC, free_func, void*, free_func_context);
    MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_recreate, THANDLE(RC_STRING), self);
```

## RC_STRING_VALUE

```c
#define RC_STRING_VALUE(rc) ...
```

`RC_STRING_VALUE` can be used in `printf` in the values list to print a `RC_STRING`.

If `NULL` is used with `RC_STRING_VALUE`, the behavior is undefined.

**SRS_RC_STRING_01_021: [** `RC_STRING_VALUE` shall print the `string` field of `rc`. **]**

## RC_STRING_VALUE_OR_NULL

```c
#define RC_STRING_VALUE_OR_NULL(rc) ...
```

`RC_STRING_VALUE_OR_NULL` can be used in `printf` in the values list to print a `RC_STRING`, while supporting `NULL` values.

**SRS_RC_STRING_01_022: [** If `rc` is `NULL`, `RC_STRING_VALUE_OR_NULL` shall print `NULL`. **]**

**SRS_RC_STRING_01_023: [** If `rc` is non `NULL`, `RC_STRING_VALUE_OR_NULL` shall print the `string` field of `rc`. **]**

## rc_string_create

```c
MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create, const char*, string);
```

`rc_string_create` creates a new ref counted string by copying the ASCII string at `string`.

**SRS_RC_STRING_01_001: [** If `string` is `NULL`, `rc_string_create` shall fail and return `NULL`. **]**

**SRS_RC_STRING_01_002: [** Otherwise, `rc_string_create` shall determine the length of `string`. **]**

**SRS_RC_STRING_01_003: [** `rc_string_create` shall allocate memory for the `THANDLE(RC_STRING)`, ensuring all the bytes in `string` can be copied (including the zero terminator). **]**

**SRS_RC_STRING_01_004: [** `rc_string_create` shall copy the string memory (including the `NULL` terminator). **]**

**SRS_RC_STRING_01_005: [** `rc_string_create` shall succeed and return a non-`NULL` handle. **]**

**SRS_RC_STRING_01_006: [** If any error occurs, `rc_string_create` shall fail and return `NULL`. **]**

## rc_string_create_with_move_memory

```c
MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create_with_move_memory, char*, string);
```

`rc_string_create_with_move_memory` creates a new ref counted string while transferring the ownership of `string` to the handle (move semantics).

**SRS_RC_STRING_01_007: [** If `string` is `NULL`, `rc_string_create_with_move_memory` shall fail and return `NULL`. **]**

**SRS_RC_STRING_01_008: [** Otherwise, `rc_string_create_with_move_memory` shall allocate memory for the `THANDLE(RC_STRING)`. **]**

**SRS_RC_STRING_01_009: [** `rc_string_create_with_move_memory` shall associate `string` with the new handle. **]**

**SRS_RC_STRING_01_010: [** `rc_string_create_with_move_memory` shall succeed and return a non-`NULL` handle. **]**

**SRS_RC_STRING_01_020: [** When the `THANDLE(RC_STRING)` reference count reaches 0, `string` shall be free with `free`. **]**

**SRS_RC_STRING_01_011: [** If any error occurs, `rc_string_create_with_move_memory` shall fail and return `NULL`. **]**

## rc_string_create_with_custom_free

```c
MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create_with_custom_free, const char*, string, RC_STRING_FREE_FUNC, free_func, void*, free_func_context);
```

`rc_string_create_with_custom_free` creates a new ref counted string while transferring the ownership of `string` to the handle (`string` is to be freed with a custom function).

**SRS_RC_STRING_01_012: [** If `string` is `NULL`, `rc_string_create_with_custom_free` shall fail and return `NULL`. **]**

**SRS_RC_STRING_01_013: [** If `free_func` is `NULL`, `rc_string_create_with_custom_free` shall fail and return `NULL`. **]**

**SRS_RC_STRING_01_014: [** `free_func_context` shall be allowed to be `NULL`. **]**

**SRS_RC_STRING_01_015: [** `rc_string_create_with_custom_free` shall allocate memory for the `THANDLE(RC_STRING)`. **]**

**SRS_RC_STRING_01_016: [** `rc_string_create_with_custom_free` shall associate `string`, `free_func` and `free_func_context` with the new handle. **]**

**SRS_RC_STRING_01_017: [** `rc_string_create_with_custom_free` shall succeed and return a non-`NULL` handle. **]**

**SRS_RC_STRING_01_018: [** When the `THANDLE(RC_STRING)` reference count reaches 0, `free_func` shall be called with `free_func_context` to free the memory used by `string`. **]**

**SRS_RC_STRING_01_019: [** If any error occurs, `rc_string_create_with_custom_free` shall fail and return `NULL`. **]**

### rc_string_recreate
```c
MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_recreate, THANDLE(RC_STRING), source);
```

`rc_string_recreate` copies `self`'s string by content. This allows detaching the original `source`'s underlying storage, for example, if it was previously created by `rc_string_create_with_custom_free` from a `CONSTBUFFER_HANDLE`.

**SRS_RC_STRING_02_001: [** If `source` is `NULL` then `rc_string_recreate` shall return `NULL`. **]**

**SRS_RC_STRING_02_002: [** `rc_string_recreate` shall perform same steps as `rc_string_create` to return a `THANDLE(RC_STRING)` with the same content as `source`. **]**



