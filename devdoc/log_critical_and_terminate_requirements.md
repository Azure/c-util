`log_critical_and_terminate` requirements
================

## Overview

`log_critical_and_terminate` is a module that emits a critical log and terminates the process.

## Exposed API

```c
#define LogCriticalAndTerminate(FORMAT, ...) \
    ...
```

### LogCriticalAndTerminate

```c
#define LogCriticalAndTerminate(FORMAT, ...) \
    ...
```

`LogCriticalAndTerminate` emits a critical log and terminates the process.

**SRS_LOG_CRITICAL_AND_TERMINATE_01_001: [** `LogCriticalAndTerminate` shall call `ps_util_terminate_process`. **]**
