# State manager requirements
================

## Overview

`State manager` (short:`sm`) is a module the manages the call state for the APIs of a module under the following sematics:

1. the module has 2 step initialization (that is, the module has a _create and a _open APIs). Note: _open can be a do-nothing operation for the case when the user module doesn't have a real _open.

2. the module's APIs are callable after _open has executed (_open being the exception - it can only be called after _create)

3. the module has a _close function that reverts the effects of _open and allows calling _open again

4. the APIs that can be called in the _open state can be further divided into 2 categories

   a) APIs that can be called in parallel (for example a _read API).

   b) APIs that are barrier. (for example _set_content, _flush_all etc). Barrier APIs have the following semantics
      
       i) there can only be 1 barrier API at any given time

       ii) the barrier API can only execute when all the previous APIs have stopped executing

       iii) no other APIs (including other barriers) can start to execute after the current barrier (that means those APIs will fail)

5. there's no "parking", "queueing", "sleeping", "waiting", "buffering", "saving" or otherwise postponing or delaying the execution of an API: it either starts to execute (for barries execution includes waiting for all the previous APIs to finish executing) or it fails. Retrying is a user-land mechanism.

## Design

Historic context: before `sm` the world had explicit states, such as 
- "OPEN" (where all APIs would be allowed to execute)
- "OPENING" (marks the transiton from "CREATED" to "OPEN")
- "CLOSING" (where APIs would be drained - it devolves into "CREATED" once that was done)
- all sort of other substates of "OPEN" which would not allow other APIs to execute (and once work was done, the substte would revert back to "OPEN").

The world had for every module an explicit "count of API" - which was the total number of ongoing APIs. It would be incremented at the begin of every APIs. Then the API would check if it is executed in the "OPEN" state (substates would have different names). It would be decremented at the end of every API. 

A substate of "OPEN" would begin execution by switching the state variable to "SUBSTATE" (whatever it was, like "rewrite_history") Then a condition required for a substate of "OPEN" to start executing was to have a total number of executing API calls of "1" (it counted itself too). 

In a multithreaded environment reaching the condition of "let's have 1 executing API" was sometimes difficult, because other APIs would be insistent to execute too (and the first thing they did was to increment the number of executing APIs... thus blocking the execution of the substate). With enough luck, the number of pending API calls would eventually match 1 and the substate would go happily about its way.

`sm` aims at providing a better solution by:
- providing a consistent way of managing "state" across modules (no more individual "ongoingAPIcalls" and "pendingAPIcalls" and "what's the correct order? state before counting or counting before state?")
- providing progress in all cases for APIs that require their own sub-state undisturbed (that is, APIs that cannot execute in parallel with any other APIs), so no more a problem of "enough luck".

To achieve the above goals, `sm` assigns to each API call a sequence number (`n`). Sequence numbers are consecutive. The logic is simple: an API is granted execution when there's no barrier (it acts as invariant for the increment of `n`). Otherwise, the API is prohibited from execution.

Barriers follow their own ever increasing numbers called `b_now`. `b_now` start at 0. A barrier is taken when `b_now`'s least significant bit is 1. Example `b_now` == 0 means there's no ongoing barrier. `b_now` == 3 means there's an ongoing barrier.

Multiple barrier can attempt to set `b_now`'s least significant bit to 1. Only 1 of them will win the competition onver `InterlockedOr64` so only 1 will grant execution. The other barries will lose arbitration and will not be granted execution rights. When user calls `sm_barrier_end` to indicate that the barrier should be lifted, `b_now` is `InterlockedIncremented64`.

Once a barries wins the `b_now` competition it increments `n` and wait for the difference between started APIs (`n`)  and the number of executed APIs (`e`) to be 1. When that happens, barrier is known to have drained all the previous calls and is allowed to return to user land with execution granted.

`close` is in no way different than any other barrier with the exception that close reverts the state to `create`'s. As opposed to the "historical" implementation, `_close` might not grant execution because another barrier is executing. However, just like any other barrier, `_close` will wait for all preceeding APIs to finish executing before granting execution to the caller.

`sm` does not fully verify all sequences of calls. It is the user's responsibility to provide pair of calls. That is, `sm_open_begin` should be followed by a `sm_open_end` before let's say `sm_begin_close` is called. Some of these combinations are detected by the virtue of how barriers work but not all. For example, all calls to `sm_barrier_begin` or `sm_begin` immediately after `sm_open_begin` will block (this is expected to be discovered in user land at user code testing time).

Note: there's an ever increasing `n`, `e` and `b_now`. These are 64 bit values. `LONG64` max value is 9223372036854775808. Assuming 1,000,000,000 increments for `n` per second, the maximum value of LONG64 will be reached in 9,223,372,036 seconds. This is more than 290 years. This rather simplistic computation shows that in the current state of computation power (year is 2020 today) there's no need to worry about `n` wrapping around to `INT64_MIN`.

## Exposed API

```c
typedef struct SM_HANDLE_DATA_TAG* SM_HANDLE;

#define SM_RESULT_VALUES    \
    SM_EXEC_GRANTED,        \
    SM_EXEC_REFUSED,        \
    SM_ERROR                \

MU_DEFINE_ENUM(SM_RESULT, SM_RESULT_VALUES);

MOCKABLE_FUNCTION(, SM_HANDLE, sm_create, const char*, name);
MOCKABLE_FUNCTION(, void, sm_destroy, SM_HANDLE, sm);

MOCKABLE_FUNCTION(, SM_RESULT, sm_open_begin, SM_HANDLE, sm);
MOCKABLE_FUNCTION(, void, sm_open_end, SM_HANDLE, sm);

MOCKABLE_FUNCTION(, SM_RESULT, sm_close_begin, SM_HANDLE, sm);
MOCKABLE_FUNCTION(, void, sm_close_end, SM_HANDLE, sm);

MOCKABLE_FUNCTION(, SM_RESULT, sm_begin, SM_HANDLE, sm);
MOCKABLE_FUNCTION(, void, sm_end, SM_HANDLE, sm);

MOCKABLE_FUNCTION(, SM_RESULT, sm_barrier_begin, SM_HANDLE, sm);
MOCKABLE_FUNCTION(, void, sm_barrier_end, SM_HANDLE, sm);
```

### sm_create
```c
MOCKABLE_FUNCTION(, SM_HANDLE, sm_create, const char*, name);
```
`sm_create` creates a new State manager handle. `name` is used with logging and bears no functional significance.

**SRS_SM_02_001: [** If `name` is `NULL` then `sm_create` shall behave as if `name` was "NO_NAME". **]**

**SRS_SM_02_002: [** `sm_create` shall allocate memory for the instance. **]**

**SRS_SM_02_003: [** `sm_create` shall set `b_now` to -1, `n` to 0, and `e` to 0 succeed and return a non-`NULL` value. **]**

**SRS_SM_02_004: [** If there are any failures then `sm_create` shall fail and return `NULL`. **]**

### sm_destroy
```c
MOCKABLE_FUNCTION(, void, sm_destroy, SM_HANDLE, sm);
```

`sm_destroy` frees all used resources. 

**SRS_SM_02_005: [** If `sm` is `NULL` then `sm_destroy` shall return. **]**

**SRS_SM_02_006: [** `sm_destroy` shall free all used resources. **]**

### sm_open_begin
```c
MOCKABLE_FUNCTION(, SM_RESULT, sm_open_begin, SM_HANDLE, sm);
```

`sm_open_begin` asks from `sm` permission to enter "open" state.

**SRS_SM_02_007: [** If `sm` is `NULL` then `sm_open_begin` shall fail and return `SM_ERROR`. **]**

**SRS_SM_02_008: [** If `b_now` is not -1 then `sm_open_begin` shall fail and return `SM_EXEC_REFUSED`. **]**

**SRS_SM_02_009: [** `sm_open_begin` shall set `b_now` to 0, succeed and return `SM_EXEC_GRANTED`. **]**

### sm_open_end
```c
MOCKABLE_FUNCTION(, void, sm_open_end, SM_HANDLE, sm);
```

`sm_open_end` informs `sm` that user's "open" state operations have completed.

**SRS_SM_02_010: [** If `sm` is `NULL` then `sm_open_end` shall return. **]**

**SRS_SM_02_012: [** If `sm_open_end` doesn't follow a call to `sm_open_begin` then `sm_open_end` shall return. **]**

**SRS_SM_02_011: [** `sm_open_end` shall increment `e`, `b_now` to 0 and return. **]**

### sm_close_begin
```c
MOCKABLE_FUNCTION(, int, sm_close_begin, SM_HANDLE, sm);
```

`sm_open_begin` asks from `sm` permission to exit "open" state.

**SRS_SM_02_013: [** If `sm` is `NULL` then `sm_close_begin` shall fail and return `SM_ERROR`. **]**

**SRS_SM_02_020: [** If there was no `sm_open_begin`/`sm_open_end` called previously, `sm_close_begin` shall fail and `SM_EXEC_REFUSED`. **]**

**SRS_SM_02_014: [** `sm_close_begin` shall set lowest bit of `b_now` to 1. **]**

**SRS_SM_02_015: [** If setting the lowest bit `b_now` to 1 fails then `sm_close_begin` shall  return `SM_EXEC_REFUSED`. **]**

**SRS_SM_02_016: [** `sm_close_begin` shall wait for all previous operations to end. **]**

**SRS_SM_02_017: [** `sm_close_begin` shall succeed and return `SM_EXEC_GRANTED`. **]**

**SRS_SM_02_034: [** If there are any failures then `sm_close_begin` shall fail and return `SM_ERROR`. **]**

### sm_close_end
```c
MOCKABLE_FUNCTION(, void, sm_close_end, SM_HANDLE, sm);
```

`sm_close_end` informs `sm` that user's "close" state operations have completed.

**SRS_SM_02_018: [** If `sm` is `NULL` then `sm_close_end` shall return. **]**

**SRS_SM_02_019: [** `sm_close_end` shall switch `b_now` to `-1`, `n` to 0 and `e` to 0. **]**

### sm_begin
```c
MOCKABLE_FUNCTION(, int, sm_begin, SM_HANDLE, sm);
```

`sm_begin` asks from `sm` permission to execute a non-barrier operation.

**SRS_SM_02_021: [** If `sm` is `NULL` then `sm_begin` shall fail and return `SM_ERROR`. **]**

**SRS_SM_02_022: [** If there's a barrier set then `sm_begin` shall return `SM_EXEC_REFUSED`. **]**

**SRS_SM_02_035: [** `sm_begin` shall increment `n`. **]**

**SRS_SM_02_036: [** If the barrier changed after incrementing `n` then `sm_begin` shall increment `e`, signal a potential drain, and return `SM_EXEC_REFUSED`.
 **]**

**SRS_SM_02_023: [** `sm_begin` shall succeed and return `SM_EXEC_GRANTED`. **]**

### sm_end
```c
MOCKABLE_FUNCTION(, void, sm_end, SM_HANDLE, sm);
```

`sm_end` informs `sm` that user's execution of a non-barrier operations has completed.

**SRS_SM_02_024: [** If `sm` is `NULL` then `sm_end` shall return. **]**

**SRS_SM_02_025: [** `sm_end` shall increment the number of executed APIs (`e`). **]**

**SRS_SM_02_026: [** If `n`-`e` is 1 then `sm_end` shall wake up the waiting barrier. **]**

### sm_barrier_begin
```c
MOCKABLE_FUNCTION(, int, sm_barrier_begin, SM_HANDLE, sm);
```

`sm_barrier_begin` asks from `sm` permission to execute a barrier operation.

**SRS_SM_02_027: [** If `sm` is `NULL` then `sm_barrier_begin` shall fail and return `SM_ERROR`. **]**

**SRS_SM_02_028: [** If `b_now` has least significand bit set to 1 then `sm_barrier_begin` shall fail and `SM_EXEC_REFUSED`. **]**

**SRS_SM_02_029: [** `sm_barrier_begin` shall wait for the completion of all the previous operations. **]**

**SRS_SM_02_030: [** `sm_barrier_begin` shall succeed and return `SM_EXEC_GRANTED`. **]**

**SRS_SM_02_031: [** If there are any failures then `sm_barrier_begin` shall fail and return `SM_ERROR`. **]**

### sm_barrier_end
```c
MOCKABLE_FUNCTION(, void, sm_barrier_end, SM_HANDLE, sm);
```

`sm_barrier_end` informs `sm` that user's execution of a barrier operations has completed.

**SRS_SM_02_032: [** If `sm` is `NULL` then `sm_barrier_end` shall return. **]**

**SRS_SM_02_033: [** `sm_barrier_end` shall increment the number of executed operations (`e`), increment `b_now` and return. **]**