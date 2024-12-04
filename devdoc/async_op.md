# `async_op` requirements

## Overview

`async_op` is a module that manages the lifetime of an asynchronous operation. The asynchronous operation is embedded in a `THANDLE` and it uses 1 (one) memory allocation for itself AND provides within the same memory allocation space for the caller to deposit its own data (context).

`async_op` can:

1) provide space for the caller of `async_op` to store its data (context) with a size and alignment. The data is opaque to `async_op`.

2) call the user provided `cancel`.

3) be used just like any `THANDLE`. When the reference count of the `THANDLE` reaches 0, the `dispose` function is called to allow the user to clear its context.

4) given a context pointer `void* context` previously produced by `async_op_create` (the value of `THANDLE(ASYNC_OP)->context`) async_op can produce the THANDLE(ASYNC_OP) to which this context belongs.

## Exposed API

```c
#define ASYNC_OP_STATE_VALUES \
    ASYNC_RUNNING /*initial state*/, \
    ASYNC_CANCELLING /*set when cancel is called.*/, \
    ASYNC_INVALID_ARG /*returned when called with invalid arguments*/ \

MU_DEFINE_ENUM(ASYNC_OP_STATE, ASYNC_OP_STATE_VALUES);

typedef void(*ASYNC_OP_CANCEL_IMPL)(void* context); /*async_op calls this function when it needs to cancel. params */
typedef void(*ASYNC_OP_DISPOSE)(void* context);

typedef struct ASYNC_OP_TAG
{
    void* context; /*this is supposed to be used by the user*/
    struct /* anonymous structure of fields that the user should never use or care about*/
    {
        ASYNC_OP_CANCEL_IMPL cancel;

        ASYNC_OP_DISPOSE dispose;
        union
        {
            ASYNC_OP_STATE cancel_state_e; /*just for seeing the state as string instead of numbers in a debugger if needed*/
            volatile_atomic int32_t cancel_state;
        };
        unsigned char private_context[]; /*not for use. context is the only field in ASYNC_OP that is user accesible*/
    };
} ASYNC_OP;

THANDLE_TYPE_DECLARE(ASYNC_OP);

MOCKABLE_FUNCTION(, THANDLE(ASYNC_OP), async_op_create, ASYNC_OP_CANCEL_IMPL, cancel, uint32_t, context_size, uint32_t, context_align, ASYNC_OP_DISPOSE, dispose);
MOCKABLE_FUNCTION(, THANDLE(ASYNC_OP), async_op_from_context, void*, context);
MOCKABLE_FUNCTION(, ASYNC_OP_STATE, async_op_cancel, THANDLE(ASYNC_OP), async_op);
```


### Memory layout

Note: offsets below are "most likely" on a 64bit machine.

#### Before `async_op_from_context` (around Nov 2024)

One ASYNC_OP has the following memory layout (not including the THANDLE's specific additions)

|offset:+0  | offset: +8                        | offset: +32       |
|`context`  | cancel, dispose, cancel_state_e   | private_context[] |

`context` is pointing to a memory address inside `private_context` as below. The memory address has `context_align` alignment and it is followed by `context_size` bytes.

|offset:+0  | offset: +8                        | offset: +32       |
|`context`  | cancel, dispose, cancel_state_e   | private_context[] |
   |                                                      ^
   +----------------------------------------------------- |

`private_context` is allocated to have enough size to contains a `context_align` address followed by `context_size` bytes. Since `malloc` doesn't have support for such tricky allocations, it is extremely likely that `private_context` will be over-allocated and some bytes will not be used.


#### After `async_op_from_context`

Justification: only knowing `context` which points inside the `private_context` is impossible to find the initial `ASYNC_OP` as was allocated. This is because the original `context_align` is lost. For example, `context_align` of 1 and 64 will all produce some address inside `private_context` which (likely) looks the same in both cases: it is a 32 byte aligned address. Note: all 64 byte `context_align` are also 32 byte aligned.

Therefore it follows that additional things need to be stored in order to find the `THANDLE(ASYNC_OP)` value. The simplest proposal: "additional things" will contain the value of `THANDLE(ASYNC_OP)`.

The place to store the "additional things" can only be before (lower address) than `context`. Effectively this turns the memory map of `ASYNC_OP` into the following by splitting the `private_context` into several areas:

1) possibly unused area at the beginning
2) "additional things" (with the alignment and size required for such a structure)
3) maybe unused bytes following "additional things" until...
4) first `context_align` byte followed by at least `context_size` bytes

[...] in the picture below is "maybe unused bytes"

|offset:+0  | offset: +8                        | offset: +32                             |
|`context`  | cancel, dispose, cancel_state_e   | private_context[]                       |
   |                                            | [...] additional things [...] context   |
   |           `THANDLE(ASYNC_OP)`                      |                       ^
   |                ^                                   |                       |
   |                |                                   |                       |
   |                +-----------------------------------+                       |
   +----------------------------------------------------------------------------+

From `context` it can be derived the previous memory address which has the proper alignment for "additional things" and has sizeof(additional things). Here's example computations:

Example 1. `context`=67, assume "additional things" needs 8 bytes with alignment of 8. The first memory address which matches both the requirement of size and alignment is 56. Note: 64 would have matched the alignment requirement, but doesn't provide enough bytes for size (8).

Example 2. `context`=64, assume "additional things" needs 8 bytes with alignment of 8. The first memory address which matches both the requirement of size and alignment is also 56 and leaves no gap between "additional things" and `context` begin.

The next question is "how to allocate private_context?". When talking about memory allocation, there are always 2 separate problems to solve:
1) size (considering `context_size` and sizeof("additional things"))
2) alignment (considering both `context_align` and alignof("additional things"))

To determine the minimal size to allocate that "no matter what alignment malloc returns" can still contain "additional things" with its alignment and `context_size` with `context_align` let's consider the simple case when only 1 pair of "align" and "sizeof" exist.

We try to answer the question: given a pair of "align" and "sizeof" what's the minimal size to give to malloc as parameter?

An address returned by `malloc` can always be expressed as: address = M * align + P, where 0 <= P < align (M, P are integers).

If the address returned by malloc is M * align + 0, then it can immediately be used as far as alignment purposes go.

If the address returned by malloc is M * align + 1 then only the next multiple of align can be returned, in this case that would be (M + 1) * align. The difference between (M + 1) * align and M * align +1 is (align - 1) and this is the maximum padding in the worst case of malloc unfriendly return.

Therefore the answer to the question "what's the parameter to malloc that satisfies and alignment and a size?" is "size + align - 1".

When 2 aligns and 2 sizeofs are concerned, a similar logic follows.

If the first address following the pair (align1, sizeof1) is multiple of align2 then only sizeof2 bytes would be required.

If the first address following the pair (align1, sizeof1) is multiple of align2 + 1 then additional align2 - 1 bytes would be required to be allocated in order to satisfy the align2.

So when 2 align/sizeof are required then malloc should be asked to allocate (align1 + sizeof1 - 1) + (align2 + sizeof2 - 1).

### async_op_create
```c
MOCKABLE_FUNCTION(, THANDLE(ASYNC_OP), async_op_create, ASYNC_OP_CANCEL_IMPL, cancel, uint32_t, context_size, uint32_t, context_align, ASYNC_OP_DISPOSE, dispose);
```

`async_op_create` creates the wrapper over an asynchronous call. It will allocate enough bytes to hold `context_size` bytes with an alignment of `context_align`. This allocated memory can be retrieved at a later time by the user by calling `async_op_get_context`.

`cancel` is a pointer to a function that receives the same `context` as returned by `async_op_get_context` (see below). It can be `NULL` in which case `async_op_cancel` becomes no-op.

`dispose` is a pointer to a function that receives the same `context` as returned by `async_op_get_context` (see below). It can be `NULL` in which case at the destruction of `THANDLE` it will not result in a call to user space to free the data that it stored in the context.

**SRS_ASYNC_OP_02_001: [** If `context_align` is not a power of 2 then `async_op_create` shall fail and return `NULL`. **]**

**SRS_ASYNC_OP_02_002: [** `async_op_create` shall call `THANDLE_MALLOC_FLEX` with the extra size set to at least (`context_size` + `context_align` - 1).**]**

Note: the above formula will always store an address with `context_align` alignment in `private_context`.

**SRS_ASYNC_OP_02_003: [** `async_op_create` shall compute `context` (that the user is supposed to use), record `cancel`, `dispose`, set state to `ASYNC_RUNNING` and return a non-`NULL` value. **]**

**SRS_ASYNC_OP_02_004: [** If there are any failures then `async_op_create` shall fail and return `NULL`. **]**

### async_op_cancel
```c
ASYNC_OP_STATE, async_op_cancel, THANDLE(ASYNC_OP), async_op);
```

`async_op_cancel` cancels an ongoing operation by calling `cancel`.

**SRS_ASYNC_OP_02_005: [** If `async_op` is `NULL` then `async_op_cancel` shall return `ASYNC_INVALID_ARG`. **]**

**SRS_ASYNC_OP_02_006: [** `async_op_cancel` shall atomically switch the state to `ASYNC_CANCELLING` if the current state is `ASYNC_RUNNING` by using `interlocked_compare_exchange`. **]**

  **SRS_ASYNC_OP_02_007: [** If `async_op`'s `cancel` is non-`NULL` then `async_op_cancel` shall call it with `async_op->context` as parameter. **]**

**SRS_ASYNC_OP_02_008: [** `async_op_cancel` shall return the state of the operation. **]**

### async_op_from_context
```
MOCKABLE_FUNCTION(, THANDLE(ASYNC_OP), async_op_from_context, void*, context);
```

`async_op_from_context` produces a THANDLE(ASYNC_OP) **without** incref'ing it from a previously returned to the user `context`.

The convention states that the THANDLE(ASYNC_OP) should have been incref'd previously by other means.

**SRS_ASYNC_OP_02_009: [** If `context` is `NULL` then `async_op_from_context` shall fail and return `NULL`. **]**

**SRS_ASYNC_OP_02_010: [** `async_op_from_context` shall return a non-`NULL` return. **]**