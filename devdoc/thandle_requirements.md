# thandle_requirements
================

## Overview

`thandle` is a module that provides a list of macros that help with ref counted handles.

Given a type `T` , `thandle`'s macros will encapsulate the type, provide a HANDLE (that is) to it which other APIs can use. The handle type is `THANDLE(T)`. The type provided retain `T`'s mechanism. That is, if `T` is a struct type, then `THANDLE(T)->x` is the same as `T->x`.

`thandle` will provide macros for declaring all the needed constructs in a .h file and macros for defining the constructs in a .c file.

`thandle` can de directed to use a user-indicated functions for allocating/free memory by having the preprocessor tokens `THANDLE_MALLOC_FUNCTION`/`THANDLE_FREE_FUNCTION` defined at the time of `THANDLE_TYPE_DEFINE` macro expansion. 

If `THANDLE_MALLOC_FUNCTION`/`THANDLE_FREE_FUNCTION` are not defined then `thandle` uses `malloc`/`free` from <stdlib.h>.

## Exposed API

```c

/*to be used as the type of handle that wraps T*/
#define THANDLE(T)

/*to be used in a header file*/
#define THANDLE_TYPE_DECLARE(T)

/*to be used in a .c file*/
#define THANDLE_TYPE_DEFINE(T)

```

## THANDLE(T)

```c
#define THANDLE(T) 
```

`THANDLE(T)` introduces a new incomplete type that can be used with all subsequent APIs constructed with `THANDLE`. `T` is a type that was introduced by `MU_DEFINE_STRUCT` in a .c file and was wrapped using `THANDLE_TYPE_DEFINE(T)` in the .c file.

The type is const - so the following incorrect assignment fails at compile time:

```c
    THANDLE(T) one = t_create(...);
    THANDLE(T) two;
    two = one; /*compiler error*/
```

## THANDLE_TYPE_DECLARE(T)
```c
#define THANDLE_TYPE_DECLARE(T)
```

`THANDLE_TYPE_DECLARE` introduces several functions that can be used with `THANDLE(T)` type. These are `THANDLE_DEC_REF(T)`, `THANDLE_INC_REF(T)`, `THANDLE_ASSIGN(T)`, `THANDLE_INITIALIZE(T)`, `THANDLE_MOVE(T)`, `THANDLE_INITIALIZE_MOVE(T)`.

###  THANDLE_DEC_REF(T)
```c
MOCKABLE_FUNCTION(, void, THANDLE_DEC_REF(T), THANDLE(T), t);
```

`THANDLE_DEC_REF` decrements the reference count of `t` and frees memory if reference count reaches 0.

**SRS_THANDLE_02_001: [** If `t` is `NULL` then `THANDLE_DEC_REF` shall return. **]**

**SRS_THANDLE_02_002: [** `THANDLE_DEC_REF` shall decrement the ref count of `t`.  **]**

**SRS_THANDLE_02_003: [** If the ref count of `t` reaches 0 then `THANDLE_DEC_REF` shall call `dispose` (if not `NULL`) and free the used memory.  **]**

### THANDLE_INC_REF(T)
```c
MOCKABLE_FUNCTION(, void, THANDLE_INC_REF(T), THANDLE(T), t);
```

`THANDLE_INC_REF` increments the reference count of `t`.

**SRS_THANDLE_02_004: [** If `t` is `NULL` then `THANDLE_INC_REF` shall return. **]**

**SRS_THANDLE_02_005: [** `THANDLE_INC_REF` shall increment the reference count of `t`. **]**

### THANDLE_ASSIGN(T)
```c
MOCKABLE_FUNCTION(, void, THANDLE_ASSIGN(T), THANDLE(T) *, t1, THANDLE(T), t2 );
```

`THANDLE_ASSIGN` does t1=t2. Both `t1` and `t2` are existing constructed handles.


**SRS_THANDLE_02_006: [** If `t1` is `NULL` then `THANDLE_ASSIGN` shall return. **]**

**SRS_THANDLE_02_007: [** If `*t1` is `NULL` and `t2` is `NULL` then `THANDLE_ASSIGN` shall return. **]**

**SRS_THANDLE_02_008: [** If `*t1` is `NULL` and `t2` is not `NULL` then `THANDLE_ASSIGN` shall increment the reference count of `t2` and store `t2` in `*t1`. **]**

**SRS_THANDLE_02_009: [** If `*t1` is not `NULL` and `t2` is `NULL` then `THANDLE_ASSIGN` shall decrement the reference count of `*t1` and store `NULL` in `*t1`. **]**

**SRS_THANDLE_02_010: [** If `*t1` is not `NULL` and `t2` is not `NULL` then `THANDLE_ASSIGN` shall increment the reference count of `t2`, shall decrement the reference count of `*t1` and store `t2` in `*t1`. **]**

###  THANDLE_INITIALIZE(T)
```c
 MOCKABLE_FUNCTION(, void, THANDLE_INITIALIZE(T), THANDLE(T) *, lvalue, THANDLE(T), rvalue );
 ```

`THANDLE_INITIALIZE` achieves lvalue=rvalue. `lvalue` is NOT a constructed handle. Example: it is a field in a struct that has just been `malloc`'d.

**SRS_THANDLE_02_011: [** If `lvalue` is `NULL` then `THANDLE_INITIALIZE` shall return. **]**

**SRS_THANDLE_02_018: [** If `rvalue` is `NULL` then `THANDLE_INITIALIZE` shall store `NULL` in `*lvalue`. **]**

**SRS_THANDLE_02_012: [** `THANDLE_INITIALIZE` shall increment the reference count of `rvalue` and store it in `*lvalue`. **]**

## THANDLE_TYPE_DEFINE(T)
```
#define THANDLE_TYPE_DEFINE(T)
```

`THANDLE_TYPE_DEFINE` introduces the implementation for the functions in `THANDLE_TYPE_DECLARE` (`THANDLE_DEC_REF`, `THANDLE_INC_REF`, `THANDLE_ASSIGN`, `THANDLE_INITIALIZE`, `THANDLE_GET_T`) and three new memory management functions `THANDLE_MALLOC(T)`, `THANDLE_MALLOC_WITH_EXTRA_SIZE(T)` and `THANDLE_FREE(T)`.

### THANDLE_MALLOC(T)
```c
static T* THANDLE_MALLOC(T)(void(*dispose)(T*))
```

`THANDLE_MALLOC` return a pointer to `T`. `dispose` is a function that the `THANDLE_DEC_REF` calls when the reference count reaches 0 in order to free the resources allocated by the user in `T`. `dispose` can be `NULL` in which case there are no user resources to be de-allocated.

**SRS_THANDLE_02_013: [** `THANDLE_MALLOC` shall allocate memory. **]**

**SRS_THANDLE_02_014: [** `THANDLE_MALLOC` shall initialize the reference count to 1, store `dispose` and return a `T*` . **]**

**SRS_THANDLE_02_015: [** If `malloc` fails then `THANDLE_MALLOC` shall fail and return `NULL`. **]**

### THANDLE_MALLOC_WITH_EXTRA_SIZE
```c
static T* THANDLE_MALLOC_WITH_EXTRA_SIZE(T)(void(*dispose)(T*), size_t extra_size)
```

`THANDLE_MALLOC_WITH_EXTRA_SIZE` return a pointer to `T`. `dispose` is a function that the `THANDLE_DEC_REF` calls when the reference count reaches 0 in order to free the resources allocated by the user in `T`. `dispose` can be `NULL` in which case there are no user resources to be de-allocated. `T` is a type that has a flexible array. `extra_size` is the size in bytes of the flexible array.

**SRS_THANDLE_02_020: [** `THANDLE_MALLOC_WITH_EXTRA_SIZE` shall allocate memory enough to hold `T` and `extra_size`. **]**

**SRS_THANDLE_02_021: [** `THANDLE_MALLOC_WITH_EXTRA_SIZE` shall initialize the reference count to 1, store `dispose` and return a `T*`. **]**

**SRS_THANDLE_02_022: [** If `malloc` fails then `THANDLE_MALLOC_WITH_EXTRA_SIZE` shall fail and return `NULL`. **]**


### THANDLE_FREE(T)
```c
    THANDLE_FREE(T)(T* t)
```

`THANDLE_FREE` frees the allocated memory by `THANDLE_MALLOC` or `THANDLE_MALLOC_WITH_EXTRA_SIZE`. It is called from `THANDLE_DEC_REF` (when reference count reaches 0) after `dispose`, or it can be called from the user code to free the memory.

**SRS_THANDLE_02_016: [** If `t` is `NULL` then `THANDLE_FREE` shall return. **]**

**SRS_THANDLE_02_017: [** `THANDLE_FREE` shall free the allocated memory by `THANDLE_MALLOC`. **]**


### THANDLE_GET_T(T)
```c
static T* THANDLE_GET_T(T)(THANDLE(T) t)
```

Given a previously constructed `THANDLE(T)`, `THANDLE_GET_T(T)` reeturns a pointer to the underlying type of the handle. Useful for all the APIs that take a `THANDLE(T)` as parameter.

**SRS_THANDLE_02_023: [** If `t` is `NULL` then `THANDLE_GET_T(T)` shall return `NULL`. **]**

**SRS_THANDLE_02_024: [** `THANDLE_GET_T(T)` shall return the same pointer as `THANDLE_MALLOC`/`THANDLE_MALLOC_WITH_EXTRA_SIZE` returned at the handle creation time. **]**

### THANDLE_CREATE_FROM_CONTENT_FLEX_MACRO(T)
```c
THANDLE_CREATE_FROM_CONTENT_FLEX_MACRO(T)
THANDLE(T) THANDLE_CREATE_FROM_CONTENT_FLEX(T)(const T* source, void(*dispose)(T*), int(*copy)(T* destination, const T* source), size_t(*get_sizeof)(const T* source))
```

Given a previously existing T, `THANDLE_CREATE_FROM_CONTENT_FLEX` will copy `T`'s content and return a `THANDLE(T)`. The allocated memory's size for copy is given by `get_sizeof` return value.

**SRS_THANDLE_02_025: [** If `source` is `NULL` then `THANDLE_CREATE_FROM_CONTENT_FLEX` shall fail and return `NULL`. **]**

**SRS_THANDLE_02_031: [** `THANDLE_CREATE_FROM_CONTENT_FLEX` shall call `get_sizeof` to get the needed size to store `T`. **]**

**SRS_THANDLE_02_026: [** `THANDLE_CREATE_FROM_CONTENT_FLEX` shall allocate memory. **]**

**SRS_THANDLE_02_027: [** If `copy` is `NULL` then `THANDLE_CREATE_FROM_CONTENT_FLEX` shall memcpy the content of `source` in allocated memory. **]**

**SRS_THANDLE_02_028: [** If `copy` is not `NULL` then `THANDLE_CREATE_FROM_CONTENT_FLEX` shall call `copy` to copy `source` into allocated memory. **]**

**SRS_THANDLE_02_029: [** `THANDLE_CREATE_FROM_CONTENT_FLEX` shall initialize the ref count to 1, succeed and return a non-`NULL` value. **]**

**SRS_THANDLE_02_030: [** If there are any failures then `THANDLE_CREATE_FROM_CONTENT_FLEX` shall fail and return `NULL`. **]**

### THANDLE_GET_SIZEOF(T)
```c
static size_t THANDLE_GET_SIZEOF(T)(const T* t)
```

`THANDLE_GET_SIZEOF(T)` is a helper macro for `THANDLE_CREATE_FROM_CONTENT_MACRO(T)` (below) that returns `sizeof(T)`.


###  THANDLE_CREATE_FROM_CONTENT_MACRO(T)
```c
THANDLE_CREATE_FROM_CONTENT_MACRO(T)
THANDLE(T) THANDLE_CREATE_FROM_CONTENT(T)(const T* source, void(*dispose)(T*), int(*copy)(T* destination, const T* source))
```

`THANDLE_CREATE_FROM_CONTENT_MACRO` is the same as `THANDLE_CREATE_FROM_CONTENT_FLEX_MACRO` where `get_sizeof` function returns what the C's sizeof operator evaluates to.

**SRS_THANDLE_02_032: [** `THANDLE_CREATE_FROM_CONTENT` returns what `THANDLE_CREATE_FROM_CONTENT_FLEX(T)(source, dispose, copy, THANDLE_GET_SIZEOF(T));` returns. **]**

###  THANDLE_MOVE_MACRO(T)
```c
THANDLE_MOVE_MACRO(T)
void THANDLE_MOVE(T)(THANDLE(T) * t1, THANDLE(T) * t2)
```

`THANDLE_MOVE` moves `*t2` under `*t1` and NULLs *t2. 

**SRS_THANDLE_02_033: [** If `t1` is `NULL` then `THANDLE_MOVE` shall return. **]**

**SRS_THANDLE_02_034: [** If `t2` is `NULL` then `THANDLE_MOVE` shall return. **]**

**SRS_THANDLE_02_035: [** If `*t1` is `NULL` and `*t2` is `NULL` then `THANDLE_MOVE` shall return. **]**

**SRS_THANDLE_02_036: [** If `*t1` is `NULL` and `*t2` is not `NULL` then `THANDLE_MOVE` shall move `*t2` under `t1`, set `*t2` to `NULL` and return. **]**

**SRS_THANDLE_02_037: [** If `*t1` is not `NULL` and `*t2` is `NULL` then `THANDLE_MOVE` shall `THANDLE_DEC_REF` `*t1`, set `*t1` to `NULL` and return. **]**

**SRS_THANDLE_02_038: [** If `*t1` is not `NULL` and `*t2` is not `NULL` then `THANDLE_MOVE` shall `THANDLE_DEC_REF` `*t1`, set `*t1` to `*t2`, set `*t2` to `NULL` and return. **]**

###  THANDLE_INITIALIZE_MOVE_MACRO(T)
```c
THANDLE_INITIALIZE_MOVE_MACRO(T)
void THANDLE_INITIALIZE_MOVE(T)(THANDLE(T) * t1, THANDLE(T) * t2)
```

`THANDLE_INITIALIZE_MOVE` moves `*t2` under `*t1` and NULLs *t2, assuming that `t1` is not initialized. 

**SRS_THANDLE_01_001: [** If `t1` is `NULL` then `THANDLE_INITIALIZE_MOVE` shall return. **]**

**SRS_THANDLE_01_002: [** If `t2` is `NULL` then `THANDLE_INITIALIZE_MOVE` shall return. **]**

**SRS_THANDLE_01_003: [** If `*t2` is `NULL` then `THANDLE_INITIALIZE_MOVE` shall set `*t1` to `NULL` and return. **]**

**SRS_THANDLE_01_004: [** If `*t2` is not `NULL` then `THANDLE_INITIALIZE_MOVE` shall set `*t1` to `*t2`, set `*t2` to `NULL` and return. **]**
