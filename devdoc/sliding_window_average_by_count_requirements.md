# `sliding_window_average_by_count` Requirements

## Overview

`sliding_window_average_by_count` is a module which has keeps a sliding window average by count. A count in this case is a call to increment the window.

## Exposed API

```c
typedef struct SLIDING_WINDOW_AVERAGE_TAG SLIDING_WINDOW_AVERAGE;

THANDLE_TYPE_DECLARE(SLIDING_WINDOW_AVERAGE);

MOCKABLE_FUNCTION(, THANDLE(SLIDING_WINDOW_AVERAGE), sliding_window_average_by_count_create, int32_t, window_count);
MOCKABLE_FUNCTION(, int, sliding_window_average_by_count_add, THANDLE(SLIDING_WINDOW_AVERAGE), handle, int64_t, next_count);
MOCKABLE_FUNCTION(, int,  sliding_window_average_by_count_get, THANDLE(SLIDING_WINDOW_AVERAGE), handle, double*, average);
```

## static functions

```c
static void sliding_window_average_by_count_dispose(SLIDING_WINDOW_AVERAGE* content)
```

### sliding_window_average_by_count_create

```c
MOCKABLE_FUNCTION(, THANDLE(SLIDING_WINDOW_AVERAGE), sliding_window_average_by_count_create, int32_t, window_count);
```

`sliding_window_average_by_count_create` creates a refcounted handle to a data structure to track a sliding window average.

**SRS_SLIDING_AVERAGE_WINDOW_45_001: [** `sliding_window_average_by_count_create` shall return `NULL` is `window_count` is not >= 1. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_002: [** `sliding_window_average_by_count_create` shall call `THANDLE_MALLOC_FLEX` to allocate `SLIDING_WINDOW_AVERAGE` struct and an array of `int64_t` the size of `window_count`. **]**


**SRS_SLIDING_AVERAGE_WINDOW_45_026: [** `sliding_window_average_by_count_create` shall call `srw_lock_create` to create a lock for the `SLIDING_WINDOW_AVERAGE` struct. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_004: [** `sliding_window_average_by_count_create` shall initialize all counts in window to zero. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_005: [** `sliding_window_average_by_count_create` shall initialize the current sum, the current count, and the current average to zero. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_006: [** `sliding_window_average_by_count_create` shall initialize the `next_available_slot` to 0. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_003: [** `sliding_window_average_by_count_create` shall return `NULL` if there are any errors. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_025: [** `sliding_window_average_by_count_create` shall return a non-null `THANDLE(SLIDING_WINDOW_AVERAGE)` on success. **]**

### sliding_window_average_by_count_add

```c
MOCKABLE_FUNCTION(, int, sliding_window_average_by_count_add, THANDLE(SLIDING_WINDOW_AVERAGE), handle, int64_t, next_count);
```

`sliding_window_average_by_count_add` places the given value in the next element in window and recalculate the average across the window.

**SRS_SLIDING_AVERAGE_WINDOW_45_008: [** `sliding_window_average_by_count_add` shall return a non-zero value if handle is `NULL`. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_027: [** `sliding_window_average_by_count_add` shall call `srw_lock_acquire_exclusive` on the lock in the `SLIDING_WINDOW_AVERAGE` struct. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_010: [** If `next_available_slot` is \>= `window_count` **]**

- **SRS_SLIDING_AVERAGE_WINDOW_45_011: [** `sliding_window_average_by_count_add` shall subtract the value at `next_available_slot%window_count` from the current sum. **]**

- **SRS_SLIDING_AVERAGE_WINDOW_45_012: [** `sliding_window_average_by_count_add` shall add the `next_count` value to the current sum. **]**

- **SRS_SLIDING_AVERAGE_WINDOW_45_013: [** `sliding_window_average_by_count_add` shall assign the `next_count` value at `next_available_slot%window_count` index. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_014: [** If the `next_available_slot` \< `window_count` **]**

- **SRS_SLIDING_AVERAGE_WINDOW_45_015: [** `sliding_window_average_by_count_add` shall add the `next_count` value to the current sum **]**

- **SRS_SLIDING_AVERAGE_WINDOW_45_016: [** `sliding_window_average_by_count_add` shall assign the `next_slot` value at `next_available_slot` index **]**

- **SRS_SLIDING_AVERAGE_WINDOW_45_017: [** `sliding_window_average_by_count_add` shall increment the current count. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_009: [** `sliding_window_average_by_count_add` shall increment the `next_available_slot`. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_018: [** `sliding_window_average_by_count_add` shall assign the current average to the current sum/current count. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_028: [** `sliding_window_average_by_count_add` shall call `srw_lock_release_exclusive` on the lock in the `SLIDING_WINDOW_AVERAGE` struct. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_019: [** `sliding_window_average_by_count_add` shall return zero on success. **]**

### sliding_window_average_by_count_get

```c
MOCKABLE_FUNCTION(, int,  sliding_window_average_by_count_get, THANDLE(SLIDING_WINDOW_AVERAGE), handle, double*, average);
```

`sliding_window_average_by_count_get` gives the current sliding window average for this handle.

**SRS_SLIDING_AVERAGE_WINDOW_45_021: [** `sliding_window_average_by_count_get` shall return a non-zero value if `handle` is `NULL`. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_022: [** `sliding_window_average_by_count_get` shall return a non-zero value if `average` is `NULL`. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_029: [** `sliding_window_average_by_count_get` shall call `srw_lock_acquire_shared` on the lock in the `SLIDING_WINDOW_AVERAGE` struct. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_023: [** `sliding_window_average_by_count_get` shall copy the current average into `average`. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_030: [** `sliding_window_average_by_count_get` shall call `srw_lock_release_shared` on the lock in the `SLIDING_WINDOW_AVERAGE` struct. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_024: [** `sliding_window_average_by_count_get` shall return zero on success. **]**

### sliding_window_average_by_count_dispose

```c
static void sliding_window_average_by_count_dispose(SLIDING_WINDOW_AVERAGE* content)
```

`sliding_window_average_by_count_dispose` disposes of any held resources when the refcount goes to zero.

**SRS_SLIDING_AVERAGE_WINDOW_45_031: [** `sliding_window_average_by_count_dispose` shall return if content is `NULL`. **]**

**SRS_SLIDING_AVERAGE_WINDOW_45_033: [** `sliding_window_average_by_count_dispose` shall call `srw_lock_destroy` on the lock. **]**

