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

**SRS_RC_PTR_43_002: [** `rc_ptr_create_with_move_pointer` shall allocate memory by calling `THANDLE_MALLOC`. **]**

**SRS_RC_PTR_43_003: [** If there are any failures, `rc_ptr_create_with_move_pointer` shall fail and return `NULL`. **]**

**SRS_RC_PTR_43_004: [** `rc_ptr_create_with_move_pointer` shall succeed and return a non-`NULL` value. **]**
