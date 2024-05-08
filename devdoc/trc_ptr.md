# `trc_ptr` requirements

## Overview

`trc_ptr` is a module that provides a list of macros (similar to `THANDLE`) that encapsulates reference-counted pointers.

The macro `TRC_PTR(T)` defines a typename that is a `THANDLE` wrapping a structure that encapsulates a pointer of type `T`. The user creates the pointer of type `T` and passes it to `TRC_PTR_CREATE_WITH_MOVE_POINTER(T)` function, which takes 2 arguments, the pointer and a dispose function. The dispose function is called when the reference count of the `THANDLE` reaches zero. The dispose function passes a single argument of type `T` and it should be the function that is used to free the pointer.

Users of this module will declare a `TRC_PTR(T)` type using `TRC_PTR_DECLARE(T)` and define it using `TRC_PTR_DEFINE(T)`.

Because `TRC_PTR(T)` is kind of `THANDLE`, all of `THANDLE`'s APIs apply to `TRC_PTR(T)`. For convenience the following macros are provided out of the box with the same semantics as those of `THANDLE`'s:
`TRC_PTR_INITIALIZE(T)`
`TRC_PTR_ASSIGN(T)`
`TRC_PTR_MOVE(T)`
`TRC_PTR_INITIALIZE_MOVE(T)`


## Exposed API
```c

#define TRC_PTR(T) THANDLE(TRC_PTR_STRUCT(T))
#define TRC_PTR_VALUE(rc) ((rc)->ptr)
#define TRC_PTR_OR_NULL(rc) (((rc) == NULL) ? NULL : (rc)->ptr)

#define TRC_PTR_CREATE_WITH_MOVE_POINTER(T) TRC_PTR_LL_CREATE_WITH_MOVE_POINTER(T)
#define TRC_PTR_ASSIGN(T) THANDLE_ASSIGN(TRC_PTR_STRUCT(T))
#define TRC_PTR_INITIALIZE(T) THANDLE_INITIALIZE(TRC_PTR_STRUCT(T))
#define TRC_PTR_MOVE(T) THANDLE_MOVE(TRC_PTR_STRUCT(T))
#define TRC_PTR_INITIALIZE_MOVE(T) THANDLE_INITIALIZE_MOVE(TRC_PTR_STRUCT(T))

#define TRC_PTR_DECLARE(T) \
    TYPED_RC_PTR_LL_DECLARE(T, T)

#define TRC_PTR_DEFINE(T) \
    TYPED_RC_PTR_LL_DEFINE(T, T)

```

## static functions

```c
#define DEFINE_TRC_PTR_LL_DISPOSE_FUNC(C, T) \
    static void MU_C2(T, _rc_ptr_dispose)( TRC_PTR_STRUCT(T)* context)
```

### TRC_PTR_CREATE_WITH_MOVE_POINTER(T)
```c
MOCKABLE_FUNCTION(, THANDLE(MU_C2(T, _RC_PTR)), TRC_PTR_LL_CREATE_WITH_MOVE_POINTER(C), T, ptr, MU_C2(C, _RC_PTR_FREE_FUNC), free_func);
```

`TRC_PTR_CREATE_WITH_MOVE_POINTER(T)` creates a `TRC_PTR(T)` type that is a `THANDLE` which encapsulates the given `ptr`.

**SRS_TRC_PTR_45_001: [** If `ptr` is `NULL`, `TRC_PTR_CREATE_WITH_MOVE_POINTER(T)` shall fail and return `NULL`. **]**

**SRS_TRC_PTR_45_002: [** `TRC_PTR_CREATE_WITH_MOVE_POINTER(T)` shall create a `TRC_PTR(T)` by calling `THANDLE_MALLOC(T)` with `MU_C2(C, _rc_ptr_dispose)` as the dispose function. **]**

**SRS_TRC_PTR_45_003: [** `TRC_PTR_CREATE_WITH_MOVE_POINTER(T)` shall store the given `ptr` and `free_func` in the created `THANDLE(TRC_PTR_STRUCT(T))`. **]**

**SRS_TRC_PTR_45_004: [** If there are any failures, `TRC_PTR_CREATE_WITH_MOVE_POINTER(T)` shall fail and return `NULL`.  **]**

**SRS_TRC_PTR_45_005: [** `TRC_PTR_CREATE_WITH_MOVE_POINTER(T)` shall succeed and return a non-`NULL` value. **]**

### rc_ptr_dispose
```c
static void MU_C2(T, _rc_ptr_dispose)( TRC_PTR_STRUCT(T)* context)
```

`T_rc_ptr_dispose` is called when all references to a `TRC_PTR(T)` are released.

**SRS_TRC_PTR_45_006: [** If `free_func` is not `NULL`, `T_rc_ptr_dispose` shall call `free_func` with the `context->ptr`. **]**
