# thandle_requirements
================

## Overview

`thandle` is a module that provides a list of macros that help with ref counted handles.

Given a type `T` previously introduced by `MU_DEFINE_STRUCT`, `thandle`'s macros will encapsulate the type, provide a HANDLE (that is, opaque type) to it which other APIs can use. The handle type is `THANDLE(T)`.

`thandle` will provide macros for declaring all the needed constructs in a .h file and macros for defining the constructs in a .c file.

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

`THANDLE_TYPE_DECLARE` introduces several functions that can be used with `THANDLE(T)` type. These are `THANDLE_DEC_REF(T)`, `THANDLE_INC_REF(T)`, `THANDLE_ASSIGN(T)`, `THANDLE_INITIALIZE(T)`.

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

**SRS_THANDLE_02_012: [** `THANDLE_INITIALIZE` shall increment the reference count of `rvalue` and store it in `*lvalue`. **]**

## THANDLE_TYPE_DEFINE(T)
```
#define THANDLE_TYPE_DEFINE(T)
```

`THANDLE_TYPE_DEFINE` introduces the implementation for the functions in `THANDLE_TYPE_DECLARE` (`THANDLE_DEC_REF`, `THANDLE_INC_REF`, `THANDLE_ASSIGN`, `THANDLE_INITIALIZE`) and two new memory management functions `THANDLE_MALLOC(T)` and `THANDLE_FREE(T)`.

### THANDLE_MALLOC(T)
```c
static T* THANDLE_MALLOC(T)(void(*dispose)(T*))
```

`THANDLE_MALLOC` return a pointer to `T`. `dispose` is a function that the `THANDLE_DEC_REF` calls when the reference count reaches 0 in order to free the resources allocated by the user in `T`. `dispose` can be `NULL` in which case there are no user resources to be de-allocated.

**SRS_THANDLE_02_013: [** `THANDLE_MALLOC` shall allocate memory. **]**

**SRS_THANDLE_02_014: [** `THANDLE_MALLOC` shall initialize the reference count to 1, store `dispose` and return a `T*` . **]**

**SRS_THANDLE_02_015: [** If `malloc` fails then `THANDLE_MALLOC` shall fail and return `NULL`. **]**


### `THANDLE_FREE(T)`
```c
    THANDLE_FREE(T)(T* t)
```

`THANDLE_FREE` frees the allocated memory by `THANDLE_MALLOC`. It is called from `THANDLE_DEC_REF` (when reference count reaches 0) after `dispose`, or it can be called from the user code to free the memory.

**SRS_THANDLE_02_016: [** If `t` is `NULL` then `THANDLE_FREE` shall return. **]**

**SRS_THANDLE_02_017: [** `THANDLE_FREE` shall free the allocated memory by `THANDLE_MALLOC`. **]**


