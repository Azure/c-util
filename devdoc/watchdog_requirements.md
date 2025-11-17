`watchdog` requirements
================

## Overview

`watchdog` is a module that can start a watchdog timer that will alert a callback when it expires if it has not been stopped by then.

This may be used to detect code that is stuck in a long-running operation.

Individual watchdog instances can be started with specific timeout times in milliseconds and a callback to call if the watchdog expires. The watchdog instances should be stopped by the caller when the task they are watching completes so that no callback is fired.

The watchdog timer uses state management to operate:
- On start, it will move to the open state on success.
- Expiration callbacks will only be called in the open state. The callback will call `sm_exec_begin` to ensure timer is ready and will call `sm_exec_end` upon completion.
- When the timer is reset the state is transitioned to closed first to ensure any executing callback is complete and block incoming callbacks. Then it is reopened to allow callbacks again.
- When the timer is stopped, the state is transistioned to closed first to ensure any executing callback is complete, then the timer and state management are destroyed.

## Exposed API

```c
typedef void(*WATCHDOG_EXPIRED_CALLBACK)(void* context, const char* message);

typedef struct WATCHDOG_TAG* WATCHDOG_HANDLE;

MOCKABLE_FUNCTION(, WATCHDOG_HANDLE, watchdog_start, THANDLE(THREADPOOL), threadpool, uint32_t, timeout_ms, THANDLE(RC_STRING), message, WATCHDOG_EXPIRED_CALLBACK, callback, void*, context);
MOCKABLE_FUNCTION(, void, watchdog_reset, WATCHDOG_HANDLE, watchdog);
MOCKABLE_FUNCTION(, void, watchdog_stop, WATCHDOG_HANDLE, watchdog);
```

### watchdog_start

```c
MOCKABLE_FUNCTION(, WATCHDOG_HANDLE, watchdog_start, THANDLE(THREADPOOL), threadpool, uint32_t, timeout_ms, THANDLE(RC_STRING), message, WATCHDOG_EXPIRED_CALLBACK, callback, void*, context);
```

`watchdog_start` starts a watchdog timer in the `threadpool` with the specified `timeout_ms`. When the timer expires this will call the `callback` with `context` and `message`.

**SRS_WATCHDOG_42_029: [** If `threadpool` is `NULL` then `watchdog_start` shall fail and return `NULL`. **]**

**SRS_WATCHDOG_42_030: [** If `callback` is `NULL` then `watchdog_start` shall fail and return `NULL`. **]**

**SRS_WATCHDOG_42_016: [** `watchdog_start` shall allocate memory for the `WATCHDOG_HANDLE`. **]**

**SRS_WATCHDOG_45_006: [** `watchdog_start` shall call `sm_create` to create an `SM_HANDLE` handle state. **]**

**SRS_WATCHDOG_45_007: [** `watchdog_start` shall call `sm_open_begin` to move timer to the open state. **]**

**SRS_WATCHDOG_45_008: [** `watchdog_start` shall call `sm_open_end` to move timer to the open state. **]**

**SRS_WATCHDOG_42_028: [** `watchdog_start` shall store the `message`. **]**


**SRS_WATCHDOG_42_018: [** `watchdog_start` shall create a timer that expires after `timeout_ms` by calling `threadpool_timer_start` with `watchdog_expired_callback` as the callback. **]**

**SRS_WATCHDOG_42_019: [** If there are any errors then `watchdog_start` shall fail and return `NULL`. **]**

**SRS_WATCHDOG_42_020: [** `watchdog_start` shall succeed and return the allocated handle. **]**

### watchdog_expired_callback

```c
static void watchdog_expired_callback(void* context)
```

Callback for the timer.

**SRS_WATCHDOG_42_027: [** If `context` is `NULL` then `watchdog_expired_callback` shall terminate the process. **]**

**SRS_WATCHDOG_45_009: [** `watchdog_expired_callback` shall call `sm_exec_begin`. **]**

**SRS_WATCHDOG_45_010: [** if `sm_exec_begin` returns `SM_EXEC_GRANTED`, **]**

- **SRS_WATCHDOG_42_021: [** `watchdog_expired_callback` shall call `callback` with the `context` and `message` from `watchdog_start`. **]**

- **SRS_WATCHDOG_45_002: [** `watchdog_expired_callback` shall `sm_exec_end` **]**

### watchdog_reset

```c
MOCKABLE_FUNCTION(, void, watchdog_reset, WATCHDOG_HANDLE, watchdog);
```

`watchdog_reset` restarts the current watchdog timer as if `watchdog_stop` was called followed by `watchdog_start` with the original parameters.

**SRS_WATCHDOG_42_031: [** If `watchdog` is `NULL` then `watchdog_reset` shall return. **]**

**SRS_WATCHDOG_45_011: [** `watchdog_reset` shall call `sm_close_begin`. **]**

**SRS_WATCHDOG_45_018: [** If `sm_close_begin` returns `SM_EXEC_GRANTED`, **]**

- **SRS_WATCHDOG_42_033: [** `watchdog_reset` shall cancel the current timer by calling `threadpool_timer_cancel`. **]**

- **SRS_WATCHDOG_45_012: [** `watchdog_reset` shall call `sm_close_end`. **]**

**SRS_WATCHDOG_45_013: [** `watchdog_reset` shall call `sm_open_begin`. **]**

**SRS_WATCHDOG_45_014: [** `watchdog_reset` shall call `sm_open_end` if `sm_open_begin` succeeds. **]**

**SRS_WATCHDOG_42_035: [** `watchdog_reset` shall restart the timer by calling `threadpool_timer_restart` with the original `timeout_ms` from the call to start. **]**

### watchdog_stop

```c
MOCKABLE_FUNCTION(, void, watchdog_stop, WATCHDOG_INSTANCE_HANDLE, watchdog);
```

`watchdog_stop` frees the resources from `watchdog_start`. If the timer has not fired yet, then it will prevent the callback from being called.

**SRS_WATCHDOG_42_022: [** If `watchdog` is `NULL` then `watchdog_stop` shall return. **]**

**SRS_WATCHDOG_45_015: [** `watchdog_stop` shall call `sm_close_begin`. **]**

**SRS_WATCHDOG_45_016: [** `watchdog_stop` shall call `sm_close_end` if `sm_close_begin` succeeds. **]**

**SRS_WATCHDOG_42_024: [** `watchdog_stop` shall stop and cleanup the timer by decrementing timer reference count. **]**

**SRS_WATCHDOG_45_017: [** `watchdog_stop` shall call `sm_destroy`. **]**

**SRS_WATCHDOG_42_025: [** `watchdog_stop` shall free the `watchdog`. **]**
