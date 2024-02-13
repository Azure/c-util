`watchdog` requirements
================

## Overview

`watchdog` is a module that can start a watchdog timer that will alert a callback when it expires if it has not been stopped by then.

This may be used to detect code that is stuck in a long-running operation.

Individual watchdog instances can be started with specific timeout times in milliseconds and a callback to call if the watchdog expires. The watchdog instances should be stopped by the caller when the task they are watching completes so that no callback is fired.

Each time a watchdog timer instances is started, it has the following state:
 - `RUNNING` : The timer has started and if it expires it will call the callback.
 - `EXPIRING` : The timer has expired, and the callback is being executed.
 - `STOP` : The explicit stop was called and if the timer expires then nothing will happen.

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

**SRS_WATCHDOG_42_028: [** `watchdog_start` shall store the `message`. **]**

**SRS_WATCHDOG_42_017: [** `watchdog_start` shall set the state of the watchdog to `RUNNING`. **]**

**SRS_WATCHDOG_42_018: [** `watchdog_start` shall create a timer that expires after `timeout_ms` by calling `threadpool_timer_start` with `watchdog_expired_callback` as the callback. **]**

**SRS_WATCHDOG_42_019: [** If there are any errors then `watchdog_start` shall fail and return `NULL`. **]**

**SRS_WATCHDOG_42_020: [** `watchdog_start` shall succeed and return the allocated handle. **]**

### watchdog_expired_callback

```c
static void watchdog_expired_callback(void* context)
```

Callback for the timer.

**SRS_WATCHDOG_42_027: [** If `context` is `NULL` then `watchdog_expired_callback` shall terminate the process. **]**

**SRS_WATCHDOG_45_005: [** If the state of the watchdog is `RUNNING` then **]**
- **SRS_WATCHDOG_45_001: [** `watchdog_expired_callback` shall set the state to `EXPIRING` **]**
- **SRS_WATCHDOG_42_021: [** `watchdog_expired_callback` shall call `callback` with the `context` and `message` from `watchdog_start`. **]**
- **SRS_WATCHDOG_45_002: [** `watchdog_expired_callback` shall return the state to `RUNNING`. **]**

### watchdog_reset

```c
MOCKABLE_FUNCTION(, void, watchdog_reset, WATCHDOG_HANDLE, watchdog);
```

`watchdog_reset` restarts the current watchdog timer as if `watchdog_stop` was called followed by `watchdog_start` with the original parameters.

**SRS_WATCHDOG_42_031: [** If `watchdog` is `NULL` then `watchdog_reset` shall return. **]**

**SRS_WATCHDOG_45_003: [** `watchdog_reset` shall wait until state is not `EXPIRING`. **]**

**SRS_WATCHDOG_42_032: [** `watchdog_reset` shall set the state of the watchdog to `STOP`. **]**

**SRS_WATCHDOG_42_033: [** `watchdog_reset` shall cancel the current timer by calling `threadpool_timer_cancel`. **]**

**SRS_WATCHDOG_42_034: [** `watchdog_reset` shall set the state of the watchdog to `RUNNING`. **]**

**SRS_WATCHDOG_42_035: [** `watchdog_reset` shall restart the timer by calling `threadpool_timer_restart` with the original `timeout_ms` from the call to start. **]**

### watchdog_stop

```c
MOCKABLE_FUNCTION(, void, watchdog_stop, WATCHDOG_INSTANCE_HANDLE, watchdog);
```

`watchdog_stop` frees the resources from `watchdog_start`. If the timer has not fired yet, then it will prevent the callback from being called.

**SRS_WATCHDOG_42_022: [** If `watchdog` is `NULL` then `watchdog_stop` shall return. **]**

**SRS_WATCHDOG_45_004: [** `watchdog_stop` shall wait until state is not `EXPIRING`. **]**

**SRS_WATCHDOG_42_023: [** `watchdog_stop` shall set the state of the watchdog to `STOP`. **]**

**SRS_WATCHDOG_42_024: [** `watchdog_stop` shall stop and cleanup the timer by calling `threadpool_timer_destroy`. **]**

**SRS_WATCHDOG_42_025: [** `watchdog_stop` shall free the `watchdog`. **]**
