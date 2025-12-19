`critical_section` requirements
================

## Overview

`critical_section` is module that provides a few helper functions that let us use a `volatile_atomic int32_t` as a thread synchronization object with `critical_section_enter` and `critical_section_leave` functionality.

It differs from c-pal's `srw_lock` in 2 important respects:
1. It does not require any `_create` or `_destroy` calls.  It only requires a `volatile_atomic int32_t` which is much simpler to allocate.
2. All access is exclusive.  There is no concept of shared access.

## Exposed API

```c
    MOCKABLE_FUNCTION(, int, critical_section_enter, volatile_atomic int32_t*, access_value);
    MOCKABLE_FUNCTION(, int, critical_section_leave, volatile_atomic int32_t*, access_value);
```

### critical_section_enter

```c
    MOCKABLE_FUNCTION(, int, critical_section_enter, volatile_atomic int32_t*, access_value);
```
`critical_section_enter` blocks until it is safe to enter the critical section.

**SRS_CRITICAL_SECTION_18_001: [** If `access_value` is NULL, `critical_section_enter` shall fail and return a non-zero value. **]**

**SRS_CRITICAL_SECTION_18_002: [** `critical_section_enter` shall call `interlocked_compare_exchange` to set `access_value` to 1 if it was previously 0. **]**

**SRS_CRITICAL_SECTION_18_003: [** If `interlocked_compare_exchange` indicates that `access_value` was changed from 0 to 1, `critical_section_enter` shall succeed and return 0. **]**

**SRS_CRITICAL_SECTION_18_004: [** Otherwise, `critical_section_enter` shall call `wait_on_address` passing `access_value`. **]**

**SRS_CRITICAL_SECTION_18_005: [** After `wait_on_address` returns, `critical_section_enter` shall loop around to the beginning of the function to call `interlocked_compare_exchange` again. **]**



### critical_section_leave

```c
    MOCKABLE_FUNCTION(, int, critical_section_leave, volatile_atomic int32_t*, access_value);
```
`critical_section_leave` releases the critical section.

**SRS_CRITICAL_SECTION_18_006: [** If `access_value` is NULL, `critical_section_leave` shall fail and return a non-zero value. **]**

**SRS_CRITICAL_SECTION_18_007: [** `critical_section_leave` shall call `interlocked_exchange` to set `access_value` to 0. **]**

**SRS_CRITICAL_SECTION_18_008: [** `critical_section_leave` shall call `wake_by_address_single` to wake any threads that may be waiting for `access_value` to change. **]**

**SRS_CRITICAL_SECTION_18_009: [** `critical_section_leave` shall succeed and return 0. **]**
