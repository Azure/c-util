# `waiter` requirements

## Overview

`waiter` is a module that allows the user to move data asynchronously from producer to consumer without having to block the thread. 

## Exposed API
```c
#define WAITER_RESULT_VALUES \
    WAITER_RESULT_OK, \
    WAITER_RESULT_CANCELLED, \
    WAITER_RESULT_ABANDONED

MU_DEFINE_ENUM(WAITER_RESULT, WAITER_RESULT_VALUES);

typedef struct WAITER_TAG* WAITER_HANDLE;
typedef void(*NOTIFICATION_CALLBACK)(void* context, THANDLE(RC_PTR) data, WAITER_RESULT result);
typedef void(*NOTIFY_COMPLETE_CALLBACK)(void* context, WAITER_RESULT result);

    MOCKABLE_FUNCTION(, WAITER_HANDLE, waiter_create)
    MOCKABLE_FUNCTION(, void, waiter_destroy, WAITER_HANDLE, waiter)
    MOCKABLE_FUNCTION(, THANDLE(ASYNC_OP), waiter_register_notification, WAITER_HANDLE, waiter, NOTIFICATION_CALLBACK, notification_callback, void*, context);
    MOCKABLE_FUNCTION(, THANDLE(ASYNC_OP), waiter_notify, WAITER_HANDLE, waiter, THANDLE(RC_PTR), data, NOTIFY_COMPLETE_CALLBACK, notify_complete_callback, void*, context);

```

### waiter_create
```c
    MOCKABLE_FUNCTION(, WAITER_HANDLE, waiter_create)
```

- `waiter_create` creates the waiter and returns it.

### waiter_destroy
```c
    MOCKABLE_FUNCTION(, void, waiter_destroy, WAITER_HANDLE, waiter)
```

- `waiter_destroy` destroys the waiter.
- If `waiter_register_notification` has been called previously but `waiter_notify` has not been called, `waiter_destroy` shall call the `notification_callback` given to `waiter_register_notification` with `context` as the `context` given to `waiter_register_notification`, `data` as `NULL` and `result` as `WAITER_RESULT_ABANDONED`.
- If `waiter_notify` has been called previously but `waiter_register_notification` has not been called, `waiter_destroy` shall call the `notify_complete_callback` given to `waiter_notify` with `context` as the `context` given to `waiter_notify`, and `result` as `WAITER_RESULT_ABANDONED`.

### waiter_register_notification
```c
    MOCKABLE_FUNCTION(, THANDLE(ASYNC_OP), waiter_register_notification, WAITER_HANDLE, waiter, NOTIFICATION_CALLBACK, notification_callback, void*, context);
```
- `waiter_register_notification` registers the given `notification_callback` to be called when `waiter_notify` is called.
- If `waiter_notify` has been called previously, `waiter_register_notification` shall  call:
    - the given `notification_callback` with `context` as the given `context`, `data` as the `data` that was given to `waiter_notify` and `result` as `WAITER_RESULT_OK`.
    - the `notify_callback` given to `waiter_notify` with `context` as the `context` given to `waiter_notify` and `result` as `WAITER_RESULT_OK`.
- `waiter_register_notification` shall return a `THANDLE(ASYNC_OP)` that the user can use to cancel the operation.
- If the user calls `async_op_cancel` on the returned `THANDLE(ASYNC_OP)` before `waiter_notify` is called, the given `notification_callback` is called with `context` as the given `context`, `data` as `NULL` and `result` as `WAITER_RESULT_CANCELLED`.
- If `waiter_register_notification` is called more than once, it shall fail and return `NULL`.

### waiter_notify
```c
    MOCKABLE_FUNCTION(, THANDLE(ASYNC_OP), waiter_notify, WAITER_HANDLE, waiter, THANDLE(RC_PTR), data, NOTIFY_COMPLETE_CALLBACK, notify_complete_callback, void*, context);
```
- `waiter_notify` notifies the waiter that there is data available and registers the given `notify_complete_callback` to be called when the given `data` has been consumed.
- If `waiter_register_notification` has been called previously, `waiter_notify` shall call:
    - the given `notify_complete_callback` with `context` as the given `context` and `result` as `WAITER_RESULT_OK`.
    - the `notification_callback` given to `waiter_register_notification` with `context` as the `context` given to `waiter_register_notification`, `data` as the given `data` and `result` as `WAITER_RESULT_OK`.
- `waiter_notify` shall return a `THANDLE(ASYNC_OP)` that the user can use to cancel the operation.
- If the user calls `async_op_cancel` on the returned `THANDLE(ASYNC_OP)` before `waiter_register_notification` is called, the given `notify_complete_callback` is called with `context` as the given `context` and `result` as `WAITER_RESULT_CANCELLED`.
- If `waiter_notify` is called more than once, it shall fail and return `NULL`.