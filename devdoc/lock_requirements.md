# lock

## Overview

The **lock** adapter implements a synchronization primitive.

## Exposed API

```c
typedef void* LOCK_HANDLE;

#define LOCK_RESULT_VALUES \
    LOCK_OK, \
    LOCK_ERROR \

MU_DEFINE_ENUM(LOCK_RESULT, LOCK_RESULT_VALUES);

MOCKABLE_FUNCTION(, LOCK_HANDLE, Lock_Init);
MOCKABLE_FUNCTION(, LOCK_RESULT, Lock, LOCK_HANDLE, handle);
MOCKABLE_FUNCTION(, LOCK_RESULT, Unlock, LOCK_HANDLE, handle);
MOCKABLE_FUNCTION(, LOCK_RESULT, Lock_Deinit, LOCK_HANDLE, handle);
```

```c
HANDLE_LOCK Lock_Init (void);
```

**SRS_LOCK_10_002: [** `Lock_Init` on success shall return a valid lock handle which should be a non-`NULL` value **]**

**SRS_LOCK_10_003: [** `Lock_Init` on error shall return `NULL` **]**

```c
LOCK_RESULT Lock(HANDLE_LOCK handle);
```

**SRS_LOCK_10_004: [** `Lock` shall be implemented as a non-recursive lock **]**

**SRS_LOCK_10_005: [** `Lock` on success shall return `LOCK_OK` **]**

**SRS_LOCK_10_006: [** `Lock` on error shall return `LOCK_ERROR` **]**

**SRS_LOCK_10_007: [** `Lock` on `NULL` handle passed returns `LOCK_ERROR` **]**

```c
LOCK_RESULT Unlock(HANDLE_LOCK handle);
```

**SRS_LOCK_10_008: [** `Unlock` shall perform a platform dependant unlock **]**

**SRS_LOCK_10_009: [** `Unlock` on success shall return `LOCK_OK` **]**

**SRS_LOCK_10_010: [** `Unlock` on error shall return `LOCK_ERROR` **]**

**SRS_LOCK_10_011: [** `Unlock` on `NULL` handle passed returns `LOCK_ERROR` **]**

```c
LOCK_RESULT Lock_Deinit(HANDLE_LOCK handle);
```

**SRS_LOCK_10_012: [** `Lock_Deinit` frees all resources associated with `handle` **]**

**SRS_LOCK_10_013: [** `Lock_Deinit` on NULL `handle` passed returns `LOCK_ERROR` **]**
