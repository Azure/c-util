# `waiter_queue` requirements

## Overview

`waiter_queue` is a module that allows a consumers to park calls and for producers to execute them when data becomes available. It has two parts: `waiter_queue_ll` that contains the logic for parking and executing calls and `waiter_queue_hl` that contains logic for making `waiter_queue` thread-safe.


## Exposed API

### waiter_queue_ll
```
#define WAITER_QUEUE_CALL_REASON_VALUES \
    WAITER_QUEUE_CALL_REASON_POPPED, \
    WAITER_QUEUE_CALL_REASON_ABANDONED

MU_DEFINE_ENUM(WAITER_QUEUE_CALL_REASON, WAITER_QUEUE_CALL_REASON_VALUES);

typedef struct WAITER_QUEUE_LL_TAG* WAITER_QUEUE_LL_HANDLE;
typedef bool(*POP_CALLBACK)(void* context, void* data, bool* continue_processing, WAITER_QUEUE_CALL_REASON reason);

#ifdef __cplusplus
extern "C" {
#endif

    MOCKABLE_FUNCTION(, WAITER_QUEUE_LL_HANDLE, waiter_queue_ll_create);
    MOCKABLE_FUNCTION(, void, waiter_queue_ll_destroy, WAITER_QUEUE_LL_HANDLE, waiter_queue_ll);

    MOCKABLE_FUNCTION(, int, waiter_queue_ll_push, WAITER_QUEUE_LL_HANDLE, waiter_queue_ll, POP_CALLBACK, pop_callback, void*, pop_callback_context);
    MOCKABLE_FUNCTION(, int, waiter_queue_ll_pop, WAITER_QUEUE_LL_HANDLE, waiter_queue_ll, void*, data);
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

    MOCKABLE_FUNCTION(, int, waiter_queue_hl_push, WAITER_QUEUE_HL_HANDLE, waiter_queue_hl, POP_CALLBACK, pop_callback, void*, pop_callback_context);
    MOCKABLE_FUNCTION(, int, waiter_queue_hl_pop, WAITER_QUEUE_HL_HANDLE, waiter_queue_hl, void*, data);

#ifdef __cplusplus
}
#endif
```

Consumers of data call `waiter_queue_hl_push` with a callback function and context to park a call. The call is parked in a queue and will be executed when the producers indicates that data is available by means of calling `waiter_queue_hl_pop`.

Producers of data call `waiter_queue_hl_pop` with the data they would like to provide to the parked calls. If any calls had been parked by consumers by means of calling `waiter_queue_hl_push`, the parked calls are executed with the given data and `WAITER_QUEUE_CALL_REASON` as `WAITER_QUEUE_CALL_REASON_POPPED`.

Calls are executed in the FIFO order in which they were parked. If the `POP_CALLBACK` for a parked call returns `true`, that call is removed from the queue. Otherwise, the parked call stays in the queue and may be executed again when more data becomes available.

A single call to `waiter_queue_hl_pop` may result in executing multiple parked calls. `waiter_queue` executes each `POP_CALLBACK` in FIFO order until a `POP_CALLBACK` sets the `continue_processing` out argument to `false`.

If there are any parked calls when `waiter_queue_hl_destroy` is called, their `POP_CALLBACKS` will be executed with `NULL` as data and `WAITER_QUEUE_CALL_REASON_ABONDONED` as reason.





