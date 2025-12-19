`rc_string_array` requirements
================

## Overview

`rc_string_array` holds an array of `THANDLE(RC_STRING)` along with the count of array elements.

## Exposed API

```c
typedef struct RC_STRING_ARRAY_TAG
{
    const uint32_t count;
    THANDLE(RC_STRING)* string_array;
} RC_STRING_ARRAY;

MOCKABLE_FUNCTION(, RC_STRING_ARRAY*, rc_string_array_create, uint32_t, count);
MOCKABLE_FUNCTION(, void, rc_string_array_destroy, RC_STRING_ARRAY*, rc_string_array);
```

### rc_string_array_create

```c
MOCKABLE_FUNCTION(, RC_STRING_ARRAY*, rc_string_array_create, uint32_t, count);
```

`rc_string_array_create` allocates memory to hold the array of `RC_STRING`. The array is modifiable by the caller after allocating.

**SRS_RC_STRING_ARRAY_42_001: [** `rc_string_array_create` shall allocate memory for `RC_STRING_ARRAY`. **]**

**SRS_RC_STRING_ARRAY_42_008: [** `rc_string_array_create` shall allocate memory for `count` strings. **]**

**SRS_RC_STRING_ARRAY_42_007: [** `rc_string_array_create` shall initialize the strings in the array to `NULL`. **]**

**SRS_RC_STRING_ARRAY_42_002: [** If there are any errors then `rc_string_array_create` shall fail and return `NULL`. **]**

**SRS_RC_STRING_ARRAY_42_003: [** `rc_string_array_create` shall succeed and return the allocated `RC_STRING_ARRAY`. **]**

### rc_string_array_destroy

```c
MOCKABLE_FUNCTION(, void, rc_string_array_destroy, RC_STRING_ARRAY*, rc_string_array);
```

`rc_string_array_destroy` cleans up the array;

**SRS_RC_STRING_ARRAY_42_004: [** If `rc_string_array` is `NULL` then `rc_string_array_destroy` shall fail and return. **]**

**SRS_RC_STRING_ARRAY_42_005: [** `rc_string_array_destroy` shall iterate over all of the elements in `string_array` and call `THANDLE_ASSIGN(RC_STRING)` with `NULL`. **]**

**SRS_RC_STRING_ARRAY_42_006: [** `rc_string_array_destroy` shall free the memory allocated in `rc_string_array`. **]**
