`thandle_tuple_array` requirements
================

## Overview

`thandle_tuple_array` holds an array of structs with `THANDLE(<TYPE>)` members along with the count of array elements. That is, an array of tuples where each tuple field is a `THANDLE`.

**NOTE:** This array itself is not a `THANDLE` and is not reference counted. As an improvement, this array could be made to use `THANDLE` itself.

## Exposed API

```c
#define DECLARE_THANDLE_TUPLE_TYPE(name, ...) \
    ...

#define DECLARE_THANDLE_TUPLE_ARRAY_TYPE(name) \
    ...

#define THANDLE_TUPLE_ARRAY_CREATE(name) ...
#define THANDLE_TUPLE_ARRAY_DESTROY(name) ...

#define DECLARE_THANDLE_TUPLE_ARRAY(name, ...) \
    DECLARE_THANDLE_TUPLE_TYPE(name, __VAR_ARGS__) \
    DECLARE_THANDLE_TUPLE_ARRAY_TYPE(name) \
    MOCKABLE_FUNCTION(, THANDLE_TUPLE_ARRAY_TYPE(name)*, THANDLE_TUPLE_ARRAY_CREATE(name), uint32_t, count); \
    MOCKABLE_FUNCTION(, void, THANDLE_TUPLE_ARRAY_DESTROY(name), THANDLE_TUPLE_ARRAY_TYPE(name)*, tuple_array);

#define DEFINE_THANDLE_TUPLE_ARRAY(name, ...) \
    ...
```

### Example Usage

```c
// In the header
#define MY_TUPLE_FIELDS \
    RC_STRING, foo, \
    RC_STRING, bar, \
    RC_STRING, baz

DECLARE_THANDLE_TUPLE_ARRAY(MY_TUPLE, MY_TUPLE_FIELDS);

// In .c file:
DEFINE_THANDLE_TUPLE_ARRAY(MY_TUPLE, MY_TUPLE_FIELDS)

// Callers
THANDLE_TUPLE_ARRAY_TYPE(MY_TUPLE) tuple = THANDLE_TUPLE_ARRAY_CREATE(MY_TUPLE)(4);
// ...
THANDLE_ASSIGN(&tuple[0].foo, some_string_1);
THANDLE_ASSIGN(&tuple[0].bar, some_string_2);
THANDLE_ASSIGN(&tuple[0].baz, some_string_3);
// ...
THANDLE_TUPLE_ARRAY_DESTROY(name)(tuple);
```

### THANDLE_TUPLE_ARRAY_CREATE(name)

```c
MOCKABLE_FUNCTION(, THANDLE_TUPLE_ARRAY_TYPE(name)*, THANDLE_TUPLE_ARRAY_CREATE(name), uint32_t, count);
```

`THANDLE_TUPLE_ARRAY_CREATE(name)` allocates memory to hold the array of tuples. The array is modifiable by the caller after allocating.

**SRS_THANDLE_TUPLE_ARRAY_42_001: [** `THANDLE_TUPLE_ARRAY_CREATE(name)` shall allocate memory for the array. **]**

**SRS_THANDLE_TUPLE_ARRAY_42_003: [** `THANDLE_TUPLE_ARRAY_CREATE(name)` shall initialize the members of the tuples in the array to `NULL`. **]**

**SRS_THANDLE_TUPLE_ARRAY_42_004: [** If there are any errors then `THANDLE_TUPLE_ARRAY_CREATE(name)` shall fail and return `NULL`. **]**

**SRS_THANDLE_TUPLE_ARRAY_42_005: [** `THANDLE_TUPLE_ARRAY_CREATE(name)` shall succeed and return the allocated array. **]**

### THANDLE_TUPLE_ARRAY_DESTROY(name)

```c
MOCKABLE_FUNCTION(, void, THANDLE_TUPLE_ARRAY_DESTROY(name), THANDLE_TUPLE_ARRAY_TYPE(name)*, tuple_array);
```

`THANDLE_TUPLE_ARRAY_DESTROY(name)` cleans up the array.

**SRS_THANDLE_TUPLE_ARRAY_42_006: [** If `tuple_array` is `NULL` then `THANDLE_TUPLE_ARRAY_DESTROY(name)` shall fail and return. **]**

**SRS_THANDLE_TUPLE_ARRAY_42_007: [** `THANDLE_TUPLE_ARRAY_DESTROY(name)` shall iterate over all of the elements in `tuple_array` and call `THANDLE_ASSIGN(type)` with `NULL` for each field. **]**

**SRS_THANDLE_TUPLE_ARRAY_42_008: [** `THANDLE_TUPLE_ARRAY_DESTROY(name)` shall free the memory allocated in `tuple_array`. **]**
