# `async_op` requirements

## Overview

`async_op` is a module that manages the lifetime of an asynchronous operation. The asynchronous operation is embeded in a `THANDLE` and it uses 1 (one) memory allocation for itself AND provides within the same memory allocation space for the caller to deposit its own data (context).

`async_op` can:

1) provide space for the caller of `async_op` to store its data (context) with a size and alignment. The data is opaque to `async_op`.

2) if provides a `cancel` facility that calls a user implementation that cancels the ongoing operation. 

3) be used just like any `THANDLE`. When the reference count of the `THANDLE` reaches 0, the `dispose` function is called to allow the user to clear its context.

## Exposed API

```c
#define ASYNC_OP_STATE_VALUES \
    ASYNC_RUNNING /*initial state*/, \
    ASYNC_CANCELLING /*set when cancel is called.*/ \

MU_DEFINE_ENUM(ASYNC_OP_STATE, ASYNC_OP_STATE_VALUES);

typedef void(*ASYNC_OP_CANCEL_IMPL)(void* context); /*async_op calls this function when it needs to cancel. params */
typedef void(*ASYNC_OP_DISPOSE)(void* context);

typedef struct ASYNC_OP_TAG
{
    ASYNC_OP_CANCEL_IMPL cancel;
    void* context; /*this is supposed to be used by the user*/
    ASYNC_OP_DISPOSE dispose;
    union
    {
        ASYNC_OP_STATE cancel_state_e; /*just for seeing the state as string instead of numbers*/
        volatile_atomic int32_t cancel_state;
    }u;
    unsigned char private_context[]; /*do not use*/
} ASYNC_OP;

THANDLE_TYPE_DECLARE(ASYNC_OP);

MOCKABLE_FUNCTION(, THANDLE(ASYNC_OP), async_op_create, ASYNC_OP_CANCEL_IMPL, cancel, uint32_t, context_size, uint32_t, context_align, ASYNC_OP_DISPOSE, dispose);
MOCKABLE_FUNCTION(, ASYNC_OP_STATE, async_op_cancel, THANDLE(ASYNC_OP), async_op);
```

### async_op_create
```c
MOCKABLE_FUNCTION(, THANDLE(ASYNC_OP), async_op_create, ASYNC_OP_CANCEL_IMPL, cancel, uint32_t, context_size, uint32_t, context_align, ASYNC_OP_DISPOSE, dispose);
```

`async_op_create` creates the wrapper over an asynchronous call. It will allocate enough bytes to hold `context_size` bytes with an alignment of `context_align`. This allocated memory can be retrieved at a later time by the user by calling `async_op_get_context`.

`cancel` is a pointer to a function that receives the same `context` as returned by `async_op_get_context` (see below). It can be `NULL` in which case `async_op_cancel` becomes no-op. 

`dispose` is a pointer to a function that receives the same `context` as returned by `async_op_get_context` (see below). It can be `NULL` in which case at the destruction of `THANDLE` it will not result in a call to user space to free the data that it stored in the context.

**SRS_ASYNC_OP_02_001: [** If `context_align` is not a power of 2 then `async_op_create` shall fail and return `NULL`. **]**

**SRS_ASYNC_OP_02_002: [** `async_op_create` shall call `THANDLE_MALLOC_WITH_EXTRA_SIZE` with the extra size set as `context_size` + `context_align` - 1.  **]**

Note: the above formula will always store an address with `context_align` alignment in `private_context`.

**SRS_ASYNC_OP_02_003: [** `async_op_create` shall compute `context` (that the user is supposed to use), record `cancel`, `dispose`, set state to `ASYNC_RUNNING` and return a non-`NULL` value. **]**

**SRS_ASYNC_OP_02_004: [** If there are any failures then `async_op_create` shall fail and return `NULL`. **]**

### async_op_cancel
```c
ASYNC_OP_STATE, async_op_cancel, THANDLE(ASYNC_OP), async_op);
```

`async_op_cancel` cancels an ongoing operation by calling `cancel`.

**SRS_ASYNC_OP_02_005: [** If `async_op` is `NULL` then `async_op_cancel` shall return `ASYNC_CANCELLING`. **]**

**SRS_ASYNC_OP_02_006: [** `async_op_cancel` shall atomically switch the state to `ASYNC_CANCELLING` if the current state is `ASYNC_RUNNING` by using `interlocked_compare_exchange`. **]**

  **SRS_ASYNC_OP_02_007: [** If `async_op`'s `cancel` is non-`NULL` then `async_op_cancel` shall call it with `async_op->context` as parameter. **]**

**SRS_ASYNC_OP_02_008: [** `async_op_cancel` shall return the state of the operation. **]**
