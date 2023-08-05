# `rc_ptr` requirements

## Overview

`rc_ptr` is a module that encapsulates reference-counted pointers.

## Exposed API
```c
typedef void (*RC_PTR_FREE_FUNC)(void* context);

typedef struct RC_PTR_TAG
{
    void* ptr;
    RC_PTR_FREE_FUNC free_func;
} RC_PTR;

THANDLE_TYPE_DECLARE(RC_PTR);

#define PRI_RC_PTR "p"

#define RC_PTR_VALUE(rc) ((rc)->ptr)

#define RC_PTR_OR_NULL(rc) (((rc) == NULL) ? NULL : (rc)->ptr)

MOCKABLE_FUNCTION(, THANDLE(RC_PTR), rc_ptr_create_with_move_pointer, void*, ptr, RC_PTR_FREE_FUNC, free_func);
```

### rc_ptr_create_with_move_pointer
```c
MOCKABLE_FUNCTION(, THANDLE(RC_PTR), rc_ptr_create_with_move_pointer, void*, ptr, RC_PTR_FREE_FUNC, free_func);
```
`rc_ptr_create_with_move_pointer` creates a `THANDLE(RC_PTR)` that encapsulates the given `ptr`.

**SRS_RC_PTR_43_001: [** If `ptr` is `NULL`, `rc_ptr_create_with_move_pointer` shall fail and return `NULL`. **]**

**SRS_RC_PTR_43_002: [** `rc_ptr_create_with_move_pointer` create a `THANDLE(RC_PTR)` by calling `THANDLE_MALLOC` with `rc_ptr_dispose` as the dispose function. **]**

**SRS_RC_PTR_43_003: [** `rc_ptr_create_with_move_pointer` shall store the given `ptr` and `free_func` in the created `THANDLE(RC_PTR)`. **]**

**SRS_RC_PTR_43_004: [** If there are any failures, `rc_ptr_create_with_move_pointer` shall fail and return `NULL`. **]**

**SRS_RC_PTR_43_005: [** `rc_ptr_create_with_move_pointer` shall succeed and return a non-`NULL` value. **]**

### rc_ptr_dispose
```c
static void rc_ptr_dispose(RC_PTR* rc_ptr)
```
`rc_ptr_dispose` is called when all references to a `THANDLE(RC_PTR)` are released.

**SRS_RC_PTR_43_006: [** If `free_func` is not `NULL`, `rc_ptr_dispose` shall call `free_func` with the `ptr`. **]**
