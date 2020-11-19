`ps_util` requirements
================

## Overview

`ps_util` is a module that contains process utility functions (like terminating a process).

## Exposed API

```c
MOCKABLE_FUNCTION(, void, ps_util_terminate_process);
```

### ps_util_terminate_process

```c
MOCKABLE_FUNCTION(, void, ps_util_terminate_process);
    ...
```

`ps_util_terminate_process` terminates the process.

**SRS_PS_UTIL_01_001: [** `ps_util_terminate_process` shall call `abort`. **]**
