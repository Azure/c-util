`watchdog_threadpool` requirements
================

## Overview

`watchdog_threadpool` is a module that manages a threadpool for [`watchdog`](watchdog_requirements.md) instances.

This can be used by callers of `watchdog` by passing `watchdog_threadpool_get` as the threadpool argument to `watchdog_start`.

## Exposed API

```c
MOCKABLE_FUNCTION(, int, watchdog_threadpool_init);
MOCKABLE_FUNCTION(, void, watchdog_threadpool_deinit);

MOCKABLE_FUNCTION(, THANDLE(THREADPOOL), watchdog_threadpool_get);
```

### watchdog_threadpool_init

```c
MOCKABLE_FUNCTION(, int, watchdog_threadpool_init);
```

Initialize the watchdog global threadpool.

**SRS_WATCHDOG_THREADPOOL_42_001: [** If the watchdog threadpool has already been initialized then `watchdog_threadpool_init` shall fail and return a non-zero value. **]**

**SRS_WATCHDOG_THREADPOOL_42_002: [** `watchdog_threadpool_init` shall create an execution engine by calling `execution_engine_create`. **]**

**SRS_WATCHDOG_THREADPOOL_42_003: [** `watchdog_threadpool_init` shall create a `threadpool` by calling `threadpool_create`. **]**

**SRS_WATCHDOG_THREADPOOL_42_006: [** `watchdog_threadpool_init` shall store the `threadpool`. **]**

**SRS_WATCHDOG_THREADPOOL_42_007: [** If there are any other errors then `watchdog_threadpool_init` shall fail and return a non-zero value. **]**

**SRS_WATCHDOG_THREADPOOL_42_008: [** `watchdog_threadpool_init` shall return 0. **]**

### watchdog_threadpool_deinit

```c
MOCKABLE_FUNCTION(, void, watchdog_threadpool_deinit);
```

De-initialize the watchdog.

**SRS_WATCHDOG_THREADPOOL_42_009: [** If the watchdog threadpool has not been initialized then `watchdog_deinit` shall return. **]**

**SRS_WATCHDOG_THREADPOOL_42_011: [** `watchdog_threadpool_deinit` shall destroy the `threadpool` by assign `threadpool` to NULL. **]**

**SRS_WATCHDOG_THREADPOOL_42_013: [** After `watchdog_threadpool_deinit` returns then `watchdog_threadpool_init` may be called again. **]**

### watchdog_threadpool_get

```c
MOCKABLE_FUNCTION(, THANDLE(THREADPOOL), watchdog_threadpool_get);
```

Access the global threadpool.

**SRS_WATCHDOG_THREADPOOL_42_014: [** `watchdog_threadpool_get` shall return the threadpool created by `watchdog_threadpool_init`. **]**

**SRS_WATCHDOG_THREADPOOL_42_015: [** If the watchdog threadpool has not been initialized then `watchdog_threadpool_get` shall return `NULL`. **]**
