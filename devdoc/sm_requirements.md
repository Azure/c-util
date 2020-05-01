# State manager requirements
================

## Overview

`State manager` (short:`sm`) is a module the manages the call state for the APIs of a module under the following sematics:

1. the module has 2 step initialization (that is, the module has a _create and a _open APIs)

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
- providing a consistent way of managing "state" across modules (no more individual "ongoingAPIcalls" and "pendingAPIcalls" and "what's the corrent order? state before counting or counting before state?")
- providing progress in all cases for APIs that require their own sub-state undisturbed (that is, APIs that cannot execute in parallel with any other APIs), so no more a problem of "enough luck".

To achieve the above goals, `sm` assigns to each API call a sequence number (`n`). Sequence numbers are consecutive. The logic is simple: if the current API's sequence number is less than the sequence number of the barrier (`bnow`), then the API is allowed to execute. Otherwise, the API is prohibited from execution, because it follows a barrier. 

When there's no barrier scheduled to be executed / executing, the `b_now` is set to the maximum representable number (`INT64_MAX`). When there's a barrier executing or scheduled to execute, the `b_now` is lowered to that barrier's `n`.

Multiple barrier can attempt to set `b_now` to their own `n`. `InterlockedCompareExchange` will only allow one to execute. The other barries (and other APIs obviously) will be rejected from executing.

`b_now` has a few special values. `b_now` == -1 means `sm` has been created. When `b_now` is -1 all APIs are prohibited from executing because their `n` is going to be greater than -1. `b_now` is set to `0` by the winning competing `open` (if there's multiple of them).

When `open` ends - `b_now` is set to `INT64_MAX` and `n` is set to 1, thus allowing other APIs to execute.

Once a barries wins the `b_now` competition it wait for `e` (the number of finished executed APIs) to reach 0. Once that happens, the barrier can proceed to the barrier code. When the barrier finishes executionm `b_now` is raise again to `INT64_MAX`.

`close` is in no way different than any other barrier with the exception that close reverts `b_now` to -1 ("create state") thus allowing another `open` to execute.

## Exposed API

```c
    MOCKABLE_FUNCTION(, SM_HANDLE, sm_create, const char*, name);
    MOCKABLE_FUNCTION(, void, sm_destroy, SM_HANDLE, sm);

    MOCKABLE_FUNCTION(, int, sm_open_begin, SM_HANDLE, sm);
    MOCKABLE_FUNCTION(, void, sm_open_end, SM_HANDLE, sm);

    MOCKABLE_FUNCTION(, void, sm_close_begin, SM_HANDLE, sm);
    MOCKABLE_FUNCTION(, void, sm_close_end, SM_HANDLE, sm);

    MOCKABLE_FUNCTION(, int, sm_begin, SM_HANDLE, sm);
    MOCKABLE_FUNCTION(, void, sm_end, SM_HANDLE, sm);

    MOCKABLE_FUNCTION(, int, sm_barrier_begin, SM_HANDLE, sm);
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

**SRS_SM_02_006: [** `sm shall free all used resources. **]**

### sm_open_begin
```c
MOCKABLE_FUNCTION(, int, sm_open_begin, SM_HANDLE, sm);
```

`sm_open_begin` signals from the user to `sm` that "open" is requested.

**SRS_SM_02_007: [** If `sm` is `NULL` then `sm_open_begin` shall fail and return a non-zero value. **]**

**SRS_SM_02_008: [** If `b_now` is not -1 then `sm_open_begin` shall fail and return a non-zero value. **]**

**SRS_SM_02_009: [** `sm_open_begin` shall set `b_now` to 0, succeed and return 0. **]**

### sm_open_end
```c
MOCKABLE_FUNCTION(, void, sm_open_end, SM_HANDLE, sm);
```

`sm_open_end` signals from the user to `sm` that "open" has completed.

**SRS_SM_02_010: [** If `sm` is `NULL` then `sm_open_end` shall return. **]**

**SRS_SM_02_012: [** If `sm_open_end` doesn't follow a call to `sm_open_begin` then `sm_open_end` shall return. **]**

**SRS_SM_02_011: [** `sm_open_end` shall set `b_now` to `INT64_MAX` and return. **]**

### sm_close_begin
```c
MOCKABLE_FUNCTION(, int, sm_close_begin, SM_HANDLE, sm);
```

`sm_close_begin` signals from the user than "close" is to begin.

**SRS_SM_02_013: [** If `sm` is `NULL` then `sm_close_begin` shall fail and return a non-zero value. **]**

**SRS_SM_02_020: [** If there was no `sm_open_begin`/`sm_open_end` called previously, `sm_close_begin` shall fail and return a non-zero value. **]**

**SRS_SM_02_014: [** `sm_close_begin` shall set `b_now` to its own `n`. **]**

**SRS_SM_02_015: [** If setting `b_now` to `n` fails then `sm_close_begin` shall fail and return a non-zero value. **]**

**SRS_SM_02_016: [** `sm_close_begin` shall wait for `e` to reach 0. **]**

**SRS_SM_02_017: [** `sm_close_begin` shall succeed and return 0. **]**

### sm_close_end
```c
MOCKABLE_FUNCTION(, void, sm_close_end, SM_HANDLE, sm);
```

`sm_close_end` signal from the user the termination of the started "close" activities.

**SRS_SM_02_018: [** If `sm` is `NULL` then `sm_close_end` shall return. **]**

**SRS_SM_02_019: [** `sm_close_end` shall switch `b_now` to `-1`. **]**