`critical_section` requirements
================

## Overview

`critical_section` is module that provides a few helper functions that let us use a `volatile_atomic int32_t` as a thread synchronization object with `enter_crit_section` and `leave_crit_section` functionality.

It differs from c-pal's `srw_lock` in 2 important respects:
1. It does not require any `_create` or `_destroy` calls.  It only requires a `volatile_atomic int32_t` which is much simpler to allocate.
2. All access is exclusive.  There is no concept of shared access.

## Exposed API

```c
    MOCKABLE_FUNCTION(, void, enter_crit_section, volatile_atomic int32_t*, access_value);
    MOCKABLE_FUNCTION(, void, leave_crit_section, volatile_atomic int32_t*, access_value);
```

### enter_crit_section

```c
    MOCKABLE_FUNCTION(, void, enter_crit_section, volatile_atomic int32_t*, access_value);
```
`enter_crit_section` blocks until it is safe to enter the critical section.

**SRS_CRITICAL_SECTION_18_001: [** If `access_value` is NULL, `enter_crit_section` shall terminate the process. **]**

**SRS_CRITICAL_SECTION_18_002: [** `enter_crit_section` shall call `interlocked_compare_exchange` to set `access_value` to 1 if it was previously 0. **]**

**SRS_CRITICAL_SECTION_18_003: [** If `interlocked_compare_exchange` indicates that `access_value` was changed from 0 to 1, `enter_crit_section` returns. **]**

**SRS_CRITICAL_SECTION_18_004: [** Otherwise, `enter_crit_section` shall call `wait_on_address` passing `access_value`. **]**

**SRS_CRITICAL_SECTION_18_005: [** After `wait_on_address` returns, `enter_crit_section` shall loop around to the beginning of the function to call `interlocked_compare_exchange` again. **]**

### leave_crit_section

```c
    MOCKABLE_FUNCTION(, void, leave_crit_section, volatile_atomic int32_t*, access_value);
```
`leave_crit_section` releases the critical section.

**SRS_CRITICAL_SECTION_18_006: [** If `access_value` is NULL, `leave_crit_section` shall terminate the process. **]**

**SRS_CRITICAL_SECTION_18_007: [** `leave_crit_section` shall call `interlocked_exchange` to set `access_value` to 0. **]**

**SRS_CRITICAL_SECTION_18_008: [** `leave_crit_section` shall call `wake_by_address_single` to wake any threads that may be waiting for `access_value` to change. **]**
