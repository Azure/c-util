`bs_watchdog_threadpool` requirements
================

## Overview

`bs_watchdog_threadpool` is a module that manages a threadpool for [`bs_watchdog`](bs_watchdog_requirements.md) instances.

This can be used by callers of `bs_watchdog` by passing `bs_watchdog_threadpool_get` as the threadpool argument to `bs_watchdog_start`.

## Exposed API

```c
MOCKABLE_FUNCTION(, int, bs_watchdog_threadpool_init);
MOCKABLE_FUNCTION(, void, bs_watchdog_threadpool_deinit);

MOCKABLE_FUNCTION(, THANDLE(THREADPOOL), bs_watchdog_threadpool_get);
```

### bs_watchdog_threadpool_init

```c
MOCKABLE_FUNCTION(, int, bs_watchdog_threadpool_init);
```

Initialize the watchdog global threadpool.

**SRS_BS_WATCHDOG_THREADPOOL_42_001: [** If the watchdog threadpool has already been initialized then `bs_watchdog_threadpool_init` shall fail and return a non-zero value. **]**

**SRS_BS_WATCHDOG_THREADPOOL_42_002: [** `bs_watchdog_threadpool_init` shall create an execution engine by calling `execution_engine_create`. **]**

**SRS_BS_WATCHDOG_THREADPOOL_42_003: [** `bs_watchdog_threadpool_init` shall create a `threadpool` by calling `threadpool_create`. **]**

**SRS_BS_WATCHDOG_THREADPOOL_42_004: [** `bs_watchdog_threadpool_init` shall open the `threadpool` by calling `threadpool_open`. **]**

**SRS_BS_WATCHDOG_THREADPOOL_42_006: [** `bs_watchdog_threadpool_init` shall store the `threadpool`. **]**

**SRS_BS_WATCHDOG_THREADPOOL_42_007: [** If there are any other errors then `bs_watchdog_threadpool_init` shall fail and return a non-zero value. **]**

**SRS_BS_WATCHDOG_THREADPOOL_42_008: [** `bs_watchdog_threadpool_init` shall return 0. **]**

### bs_watchdog_threadpool_deinit

```c
MOCKABLE_FUNCTION(, void, bs_watchdog_threadpool_deinit);
```

De-initialize the watchdog.

**SRS_BS_WATCHDOG_THREADPOOL_42_009: [** If the watchdog threadpool has not been initialized then `bs_watchdog_deinit` shall return. **]**

**SRS_BS_WATCHDOG_THREADPOOL_42_010: [** `bs_watchdog_threadpool_deinit` shall close the `threadpool` by calling `threadpool_close`. **]**

**SRS_BS_WATCHDOG_THREADPOOL_42_011: [** `bs_watchdog_threadpool_deinit` shall destroy the `threadpool` by assign `threadpool` to NULL. **]**

**SRS_BS_WATCHDOG_THREADPOOL_42_016: [** `bs_watchdog_threadpool_deinit` shall destroy the `execution_engine` by calling `execution_engine_dec_ref`. **]**

**SRS_BS_WATCHDOG_THREADPOOL_42_013: [** After `bs_watchdog_threadpool_deinit` returns then `bs_watchdog_threadpool_init` may be called again. **]**

### bs_watchdog_threadpool_get

```c
MOCKABLE_FUNCTION(, THANDLE(THREADPOOL), bs_watchdog_threadpool_get);
```

Access the global threadpool.

**SRS_BS_WATCHDOG_THREADPOOL_42_014: [** `bs_watchdog_threadpool_get` shall return the threadpool created by `bs_watchdog_threadpool_init`. **]**

**SRS_BS_WATCHDOG_THREADPOOL_42_015: [** If the watchdog threadpool has not been initialized then `bs_watchdog_threadpool_get` shall return `NULL`. **]**
