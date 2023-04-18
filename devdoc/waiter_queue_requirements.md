# `waiter_queue` requirements

## Overview

`waiter_queue` is a module that allows consumers to park calls and for producers to signal that the parked calls be executed. A "parked call" consists of a callback function and a context. The consumer provides the callback function and context when it parks a call. The consumer expects the parked call's callback function to be executed with the provided context when the producer signals.

`waiter_queue` has two parts: `waiter_queue_ll` that contains the logic for parking and unblocking calls and `waiter_queue_hl` that contains logic for making `waiter_queue` thread-safe.

Consumers call `waiter_queue_hl_add_waiter` with a `UNBLOCK_CALLBACK` callback function and context to park a call. The parked call is added to a queue and will be executed when the producers signals by calling `waiter_queue_ll_unblock_waiters`.

Producers call `waiter_queue_ll_unblock_waiters` with the data they would like to provide to the parked calls. When a producer calls `waiter_queue_ll_unblock_waiters` and there are parked calls in the queue, the parked calls are executed with the given data and `WAITER_QUEUE_CALL_REASON_UNBLOCKED` as `reason`.

Calls are executed in the FIFO order in which they were parked. If the `UNBLOCK_CALLBACK` for a parked call sets the `remove` out argument to `true`, that call is removed from the queue. Otherwise, the parked call stays in the queue and may be executed again when the producer signals again.

A single call to `waiter_queue_ll_unblock_waiters` may result in executing multiple parked calls. `waiter_queue` executes each `UNBLOCK_CALLBACK` in FIFO order until a `UNBLOCK_CALLBACK` sets the `continue_processing` out argument to `false`.

`waiter_queue_hl_destroy` calls `waiter_queue_ll_abandon` to abandon all parked calls. Each parked call that is in the queue when `waiter_queue_hl_destroy` is called will have its `UNBLOCK_CALLBACK` executed with `NULL` as `data` and `WAITER_QUEUE_CALL_REASON_ABONDONED` as `reason`. 

## Example:

`waiter_queue_hl_create` is called.

Queue state: `head||tail`

Consumer calls `waiter_queue_hl_add_waiter(cb1, ctx1)`

Queue state: `head|(cb1, ctx1)|tail`

Consumer calls `waiter_queue_hl_add_waiter(cb2, ctx2)`

Queue state: `head|(cb1, ctx1)|(cb2, ctx2)|tail`

Producer calls `waiter_queue_hl_unblock_waiters(data1)`

`cb1` is called with `ctx1`, `data1` and `WAITER_QUEUE_CALL_REASON_UNBLOCKED`. `cb1` sets `remove` to `true` and `continue_processing` to `true`.

Queue state: `head|(cb2, ctx2)|tail`

`cb2` is called with `ctx2`, `data1` and `WAITER_QUEUE_CALL_REASON_UNBLOCKED`. `cb2` sets `remove` to `false` and `continue_processing` to `true`.

Queue state: `head|(cb2, ctx2)|tail`

Consumer calls `waiter_queue_hl_add_waiter(cb3, ctx3)`.

Queue state: `head|(cb2, ctx2)|(cb3, ctx3)|tail`

Producer calls `waiter_queue_hl_unblock_waiters(data2)`

`cb2` is called with `ctx2`, `data2` and `WAITER_QUEUE_CALL_REASON_UNBLOCKED`. `cb2` sets `remove` to `true` and `continue_processing` to `false`.

Queue state: `head|(cb3, ctx3)|tail`

`waiter_queue_hl_destroy` is called. `waiter_queue_hl_destroy` calls `waiter_queue_ll_abandon`.

`cb3` is called with `ctx3` as `context`, `NULL` as `data` and `WAITER_QUEUE_CALL_REASON_ABANDONED` as `reason`.

## Exposed API

### waiter_queue_ll
```
#define WAITER_QUEUE_CALL_REASON_VALUES \
    WAITER_QUEUE_CALL_REASON_UNBLOCKED, \
    WAITER_QUEUE_CALL_REASON_ABANDONED

MU_DEFINE_ENUM(WAITER_QUEUE_CALL_REASON, WAITER_QUEUE_CALL_REASON_VALUES);

typedef struct WAITER_QUEUE_LL_TAG* WAITER_QUEUE_LL_HANDLE;
typedef void(*UNBLOCK_CALLBACK)(void* context, void* data, bool* remove, bool* continue_processing, WAITER_QUEUE_CALL_REASON reason);

#ifdef __cplusplus
extern "C" {
#endif

    MOCKABLE_FUNCTION(, WAITER_QUEUE_LL_HANDLE, waiter_queue_ll_create);
    MOCKABLE_FUNCTION(, void, waiter_queue_ll_destroy, WAITER_QUEUE_LL_HANDLE, waiter_queue_ll);

    MOCKABLE_FUNCTION(, int, waiter_queue_ll_add_waiter, WAITER_QUEUE_LL_HANDLE, waiter_queue_ll, UNBLOCK_CALLBACK, unblock_callback, void*, unblock_callback_context);
    MOCKABLE_FUNCTION(, int, waiter_queue_ll_unblock_waiters, WAITER_QUEUE_LL_HANDLE, waiter_queue_ll, void*, data);
    MOCKABLE_FUNCTION(, void, waiter_queue_ll_abandon, WAITER_QUEUE_LL_HANDLE, waiter_queue_ll);

#ifdef __cplusplus
}
#endif
```

### waiter_queue_hl
```
typedef struct WAITER_QUEUE_HL_TAG* WAITER_QUEUE_HL_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

    MOCKABLE_FUNCTION(, WAITER_QUEUE_HL_HANDLE, waiter_queue_hl_create);
    MOCKABLE_FUNCTION(, void, waiter_queue_hl_destroy, WAITER_QUEUE_HL_HANDLE, waiter_queue_hl);

    MOCKABLE_FUNCTION(, int, waiter_queue_hl_add_waiter, WAITER_QUEUE_HL_HANDLE, waiter_queue_hl, UNBLOCK_CALLBACK, unblock_callback, void*, unblock_callback_context);
    MOCKABLE_FUNCTION(, int, waiter_queue_hl_unblock_waiters, WAITER_QUEUE_HL_HANDLE, waiter_queue_hl, void*, data);

#ifdef __cplusplus
}
#endif
```
