# `rc_ptr` requirements

## Overview

`rc_ptr` is a module that encapsulates reference-counted pointers.

## Exposed API
```c
typedef void (*RC_PTR_FREE_FUNC)(void* context);

typedef struct RC_PTR_TAG
{
    const void* ptr;
    RC_PTR_FREE_FUNC free_func; // will be removed later
} RC_PTR;

THANDLE_TYPE_DECLARE(RC_PTR);

#define PRI_RC_PTR "p"

#define RC_PTR_VALUE(rc) ((rc)->ptr)

#define RC_PTR_AS_VOIDPTR(rc) (((rc) == NULL) ? NULL : (rc)->ptr)

MOCKABLE_FUNCTION(, THANDLE(RC_PTR), rc_ptr_create_with_move_memory, const void*, ptr, RC_PTR_FREE_FUNC, free_func);
```

### rc_ptr_create_with_move_memory
```c
MOCKABLE_FUNCTION(, THANDLE(RC_PTR), rc_ptr_create_with_move_memory, const void*, ptr, RC_PTR_FREE_FUNC, free_func);
```

`rc_ptr_create_with_move_memory` creates a `THANDLE(RC_PTR)` that encapsulates the given `ptr`.

- When the reference count for the returned `THANDLE(RC_PTR)` reaches `0`, the given `free_func` shall be called with the given `ptr`.
