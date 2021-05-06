`ps_util` requirements
================

## Overview

`ps_util` is a module that contains process utility functions (like terminating a process).

## Exposed API

```c
MOCKABLE_FUNCTION(, void, ps_util_terminate_process);
MOCKABLE_FUNCTION(, void, ps_util_exit_process, int, exit_code);
```

### ps_util_terminate_process

```c
MOCKABLE_FUNCTION(, void, ps_util_terminate_process);
    ...
```

`ps_util_terminate_process` terminates the process.

**SRS_PS_UTIL_01_001: [** `ps_util_terminate_process` shall call `abort`. **]**

### ps_util_exit_process

```c
MOCKABLE_FUNCTION(, void, ps_util_exit_process, int, exit_code);
    ...
```

`ps_util_exit_process` exits a process with a given exit code.

**SRS_PS_UTIL_01_002: [** `ps_util_exit_process` shall call `exit`, passing `exit_code` as argument. **]**
