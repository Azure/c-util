`bs_watchdog` requirements
================

## Overview

`bs_watchdog` is a module that can start a watchdog timer that will alert a callback when it expires if it has not been stopped by then.

This may be used to detect code that is stuck in a long-running operation.

Individual watchdog instances can be started with specific timeout times in milliseconds and a callback to call if the watchdog expires. The watchdog instances should be stopped by the caller when the task they are watching completes so that no callback is fired.

Each time a watchdog timer instances is started, it has the following state:
 - `RUNNING` : The timer has started and if it expires it will call the callback.
 - `STOP` : The explicit stop was called and if the timer expires then nothing will happen.

## Exposed API

```c
typedef void(*BS_WATCHDOG_EXPIRED_CALLBACK)(void* context, const char* message);

typedef struct BS_WATCHDOG_TAG* BS_WATCHDOG_HANDLE;

MOCKABLE_FUNCTION(, BS_WATCHDOG_HANDLE, bs_watchdog_start, THANDLE(THREADPOOL), threadpool, uint32_t, timeout_ms, THANDLE(RC_STRING), message, BS_WATCHDOG_EXPIRED_CALLBACK, callback, void*, context);
MOCKABLE_FUNCTION(, void, bs_watchdog_reset, BS_WATCHDOG_HANDLE, watchdog);
MOCKABLE_FUNCTION(, void, bs_watchdog_stop, BS_WATCHDOG_HANDLE, watchdog);
```

### bs_watchdog_start

```c
MOCKABLE_FUNCTION(, BS_WATCHDOG_HANDLE, bs_watchdog_start, THANDLE(THREADPOOL), threadpool, uint32_t, timeout_ms, THANDLE(RC_STRING), message, BS_WATCHDOG_EXPIRED_CALLBACK, callback, void*, context);
```

`bs_watchdog_start` starts a watchdog timer in the `threadpool` with the specified `timeout_ms`. When the timer expires this will call the `callback` with `context` and `message`.

**SRS_BS_WATCHDOG_42_029: [** If `threadpool` is `NULL` then `bs_watchdog_start` shall fail and return `NULL`. **]**

**SRS_BS_WATCHDOG_42_030: [** If `callback` is `NULL` then `bs_watchdog_start` shall fail and return `NULL`. **]**

**SRS_BS_WATCHDOG_42_016: [** `bs_watchdog_start` shall allocate memory for the `BS_WATCHDOG_HANDLE`. **]**

**SRS_BS_WATCHDOG_42_028: [** `bs_watchdog_start` shall store the `message`. **]**

**SRS_BS_WATCHDOG_42_017: [** `bs_watchdog_start` shall set the state of the watchdog to `RUNNING`. **]**

**SRS_BS_WATCHDOG_42_018: [** `bs_watchdog_start` shall create a timer that expires after `timeout_ms` by calling `threadpool_timer_start` with `bs_watchdog_expired_callback` as the callback. **]**

**SRS_BS_WATCHDOG_42_019: [** If there are any errors then `bs_watchdog_start` shall fail and return `NULL`. **]**

**SRS_BS_WATCHDOG_42_020: [** `bs_watchdog_start` shall succeed and return the allocated handle. **]**

### bs_watchdog_expired_callback

```c
static void bs_watchdog_expired_callback(void* context)
```

Callback for the timer.

**SRS_BS_WATCHDOG_42_027: [** If `context` is `NULL` then `bs_watchdog_expired_callback` shall terminate the process. **]**

**SRS_BS_WATCHDOG_42_021: [** If the state of the watchdog is `RUNNING` then `bs_watchdog_expired_callback` shall call `callback` with the `context` and `message` from `bs_watchdog_start`. **]**

### bs_watchdog_reset

```c
MOCKABLE_FUNCTION(, void, bs_watchdog_reset, BS_WATCHDOG_HANDLE, watchdog);
```

`bs_watchdog_reset` restarts the current watchdog timer as if `bs_watchdog_stop` was called followed by `bs_watchdog_start` with the original parameters.

**SRS_BS_WATCHDOG_42_031: [** If `watchdog` is `NULL` then `bs_watchdog_reset` shall return. **]**

**SRS_BS_WATCHDOG_42_032: [** `bs_watchdog_reset` shall set the state of the watchdog to `STOP`. **]**

**SRS_BS_WATCHDOG_42_033: [** `bs_watchdog_reset` shall cancel the current timer by calling `threadpool_timer_cancel`. **]**

**SRS_BS_WATCHDOG_42_034: [** `bs_watchdog_reset` shall set the state of the watchdog to `RUNNING`. **]**

**SRS_BS_WATCHDOG_42_035: [** `bs_watchdog_reset` shall restart the timer by calling `threadpool_timer_restart` with the original `timeout_ms` from the call to start. **]**

### bs_watchdog_stop

```c
MOCKABLE_FUNCTION(, void, bs_watchdog_stop, BS_WATCHDOG_INSTANCE_HANDLE, watchdog);
```

`bs_watchdog_stop` frees the resources from `bs_watchdog_start`. If the timer has not fired yet, then it will prevent the callback from being called.

**SRS_BS_WATCHDOG_42_022: [** If `watchdog` is `NULL` then `bs_watchdog_stop` shall return. **]**

**SRS_BS_WATCHDOG_42_023: [** `bs_watchdog_stop` shall set the state of the watchdog to `STOP`. **]**

**SRS_BS_WATCHDOG_42_024: [** `bs_watchdog_stop` shall stop and cleanup the timer by calling `threadpool_timer_destroy`. **]**

**SRS_BS_WATCHDOG_42_025: [** `bs_watchdog_stop` shall free the `watchdog`. **]**
