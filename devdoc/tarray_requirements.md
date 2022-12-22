# array_requirements
================

## Overview

`TARRAY` is a module that provides a list of macros (similar to `THANDLE`) that help manage an ever growing array of elements

Given a type `T`, `TARRAY(T)` is a `THANDLE`'d encapsulated type that contains `capacity` and `arr` as fields. `capacity` is the current capacity of `arr`, that is arr[0..capacity] are all valid indexes and they are of type `T`.

`TARRAY` only manages the growth of the array. It does not manage other aspects such as: 
1) keep track of which array elements are used / unused / not used anymore;
2) dispose of the array elements by any means;
3) last written array index (colloquially known as `size` in other implementations).

The above 3 concerns can be addressed by the user or maybe by an implementation of `TARRAY_HL`.

Because TARRAY is kind of THANDLE, all of `THANDLE`'s APIs apply to `TARRAY`. For convenience the following macros are provided out of the box with the same semantics as those of `THANDLE`'s:
`TARRAY_INITIALIZE(T)`
`TARRAY_ASSIGN(T)`
`TARRAY_MOVE(T)`
`TARRAY_INITIALIZE_MOVE(T)`

## Exposed API

```c

/*to be used as the type of handle that wraps T - it has "capacity" and "arr"*/
#define TARRAY(T)

/*to be used in a header file*/
#define TARRAY_TYPE_DECLARE(T)

/*to be used in a .c file*/
#define TARRAY_TYPE_DEFINE(T)

```

### TARRAY(T)

```c
#define TARRAY(T) 
```
`TARRAY(T)` is a `THANDLE`(`TARRAY_STRUCT_T`), where `TARRAY_STRUCT_T` is a structure introduced like below:
```c
typedef struct TARRAY_STRUCT_T_TAG
{
    uint32_t capacity;
    TARRAY_LL_CLEANUP_FUNCTION_TYPE_NAME(T) cleanup;
    T* arr;
} TARRAY_STRUCT_T;
```

### TARRAY_TYPE_DECLARE(T)
```c
#define TARRAY_TYPE_DECLARE(T)
```

`TARRAY_TYPE_DECLARE(T)` is a macro to be used in a header declaration. It introduces 3 APIs (as MOCKABLE_FUNCTIONS):

### TARRAY_CREATE_WITH_CAPACITY_AND_CLEANUP(T)
```c
TARRAY(T) TARRAY_CREATE_WITH_CAPACITY_AND_CLEANUP(T)(uint32_t capacity, TARRAY_LL_CREATE_WITH_CAPACITY_AND_CLEANUP_DECLARE(T) cleanup);
```

`TARRAY_CREATE_WITH_CAPACITY_AND_CLEANUP(T)` creates a new `TARRAY(T)` with initial capacity of `capacity` and `cleanup` function.

**SRS_TARRAY_01_001: [** If `capacity` is 0, `TARRAY_CREATE_WITH_CAPACITY_AND_CLEANUP(T)` shall fail and return `NULL`. **]**

**SRS_TARRAY_01_002: [** `TARRAY_CREATE_WITH_CAPACITY_AND_CLEANUP(T)` shall call `THANDLE_MALLOC` to allocate the result. **]**

**SRS_TARRAY_01_003: [** `TARRAY_CREATE_WITH_CAPACITY(T)` shall call `malloc_2` to allocate `capacity` entries for `result->arr`. **]**

**SRS_TARRAY_01_004: [** `TARRAY_CREATE_WITH_CAPACITY(T)` shall succeed and return a non-`NULL` value. **]**

**SRS_TARRAY_01_005: [** If there are any failures then `TARRAY_CREATE_WITH_CAPACITY(T)` shall fail and return `NULL`. **]**

### TARRAY_CREATE_WITH_CAPACITY(T)
```c
TARRAY(T) TARRAY_CREATE_WITH_CAPACITY(T)(uint32_t capacity);
```

`TARRAY_CREATE_WITH_CAPACITY(T)` creates a new `TARRAY(T)` with initial capacity of `capacity` and no cleanup function.

**SRS_TARRAY_02_012: [** `TARRAY_CREATE_WITH_CAPACITY(T)` shall return what `TARRAY_CREATE_WITH_CAPACITY_AND_CLEANUP(T)(capacity, NULL)` returns. **]**

### TARRAY_CREATE(T)
```c
TARRAY(T) TARRAY_CREATE(T)(void);
```

`TARRAY_CREATE(T)` creates a new `TARRAY(T)` with initial capacity of 1 and no cleanup function.

**SRS_TARRAY_02_011: [** `TARRAY_CREATE(T)` shall return what `TARRAY_CREATE_WITH_CAPACITY(T)`(1) returns. **]**



### TARRAY_ENSURE_CAPACITY(T)
```c
int TARRAY_ENSURE_CAPACITY(T)(TARRAY(T) tarray, uint32_t capacity)
```

`TARRAY_ENSURE_CAPACITY(T)` ensures that `tarray` has enough elements in `arr` to support addressing 0..capacity-1 indexes.

**SRS_TARRAY_02_005: [** If `tarray` is `NULL` then `TARRAY_ENSURE_CAPACITY(T)` shall fail and return a non-zero value. **]**

**SRS_TARRAY_02_006: [** If `tarray->capacity` is greater than or equal to `capacity` then `TARRAY_ENSURE_CAPACITY(T)` shall succeed and return 0. **]**

**SRS_TARRAY_02_010: [** If `capacity` is greater than 2147483648 then `TARRAY_ENSURE_CAPACITY(T)` shall fail and return a non-zero value. **]**

**SRS_TARRAY_02_007: [** `TARRAY_ENSURE_CAPACITY(T)` shall call realloc_2 to resize `arr` to the next multiple of 2 greater than or equal to `capacity`. **]**

**SRS_TARRAY_02_008: [** `TARRAY_ENSURE_CAPACITY(T)` shall succeed, record the new computed capacity, and return 0. **]**

**SRS_TARRAY_02_009: [** If there are any failures then `TARRAY_ENSURE_CAPACITY(T)` shall fail and return a non-zero value. **]**
