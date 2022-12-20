# State manager requirements
================

## Overview

`State manager` (short:`sm`) is a module the manages the call state for the APIs of a module under the following semantics:

1. The module has 2 step initialization (that is, the module has a _create and a _open APIs). Note: _open can be a do-nothing operation for the case when the user module doesn't have a real _open.

2. The module's APIs are callable after _open has executed (_open being the exception - it can only be called after _create).

3. The module may go into a "faulted" state which disallows all calls, other than closing the module. The "faulted" state means that something catastrophic happened so the module must be closed and destroyed. It is a terminal state and _open will fail.

4. The module has a _close function that reverts the effects of _open and allows calling _open again. Calling _close when the module is "faulted" cleans up the module but leaves it in a "faulted" state.

5. the APIs that can be called in the _open state can be further divided into 3 categories

   a) APIs that can be called in parallel (for example a _read API).

   b) APIs that are barrier. (for example _set_content, _flush_all etc). Barrier APIs have the following semantics
      
       i) there can only be 1 barrier API at any given time

       ii) the barrier API can only execute when all the previous APIs have stopped executing

       iii) no other APIs (including other barriers) can start to execute after the current barrier (that means those APIs will not be granted execution)

    c) `sm_close_begin`. Close blocks the execution of all the other APIs and waits for them to drain.

6. there's no "parking", "queueing", "sleeping", "waiting", "buffering", "saving" or otherwise postponing or delaying the execution of an API: it either starts to execute (for barriers execution includes waiting for all the previous APIs to finish executing; for close it means draining everything - barriers or calls alike) or it fails. Retrying is a user-land mechanism.

## Design

Historic context: before `sm` the world had explicit states, such as 
- "OPEN" (where all APIs would be allowed to execute)
- "OPENING" (marks the transiton from "CREATED" to "OPEN")
- "CLOSING" (where APIs would be drained - it devolves into "CREATED" once that was done)
- all sorts of other substates of "OPEN" which would not allow other APIs to execute (and once work was done, the substate would revert back to "OPEN").

The world had for every module an explicit "count of API" - which was the total number of ongoing APIs. It would be incremented at the begin of every APIs. Then the API would check if it is executed in the "OPEN" state (substates would have different names). It would be decremented at the end of every API. 

A substate of "OPEN" would begin execution by switching the state variable to "SUBSTATE" (whatever it was, like "rewrite_history") Then a condition required for a substate of "OPEN" to start executing was to have a total number of executing API calls of "1" (it counted itself too). 

In a multithreaded environment reaching the condition of "let's have 1 executing API" was sometimes difficult, because other APIs would be insistent to execute too (and the first thing they did was to increment the number of executing APIs... thus blocking the execution of the substate). With enough luck, the number of pending API calls would eventually match 1 and the substate would go happily about its way.

`sm` aims at providing a better solution by:
- providing a consistent way of managing "state" across modules (no more individual "ongoingAPIcalls" and "pendingAPIcalls" and "what's the correct order? state before counting or counting before state?")
- providing progress in all cases for APIs that require their own sub-state undisturbed (that is, APIs that cannot execute in parallel with any other APIs), so no more a problem of "enough luck".

To achieve the above goals, `sm` maintains the following state:
1. the current granted state + 1 bit that is set to 1 when `sm_close_begin` is called and 1 bit that is set to 1 when `sm_fault` is called.
2. the number of APIs that did not finish executing (`n`). `n` is incremented at every `sm_exec_begin` and decremented when the user signaled finishing executing by calling `sm_exec_end`.

`n` is used to enable drains on the calls. The drains are signaled and they will unblock the execution of a `sm_close_begin` or a `sm_barrier_begin`.

Barriers - since they are exclusive - are realized by switching to a state called `SM_OPENED_BARRIER`. Prohibiting regular calls to _begin is achieved by switching temporarily the state from `SM_OPENED` to `SM_OPENED_DRAINING_TO_BARRIER`.

Close is realized by prohibiting all calls (including competing `sm_close_begin` calls) by setting a bit with `InterlockedOr`. `sm_close_begin` will wait for the state to reach `SM_OPENED` and the number of executing calls to be `0`. This allows an ongoing barrier to finish (and return to `SM_OPENED` state), or the executing APIs to finish.
`sm_close_begin_with_cb` will invoke a callback function between prohibiting all the calls and waiting for the number of executing calls to be `0`. The callback function is useful in various cases, such as, when we need to cancel the ongoing operations, before we wait for the outstanding calls to finish.

Fault is realized by prohibiting all calls (except for `sm_close_begin` calls and all `_end` calls) by setting a bit with `InterlockedOr`. This means that any currently executing operations can complete, but the faulted state is terminal.

`sm` will verify all sequence of calls. When a _begin call is called in an unexpected state, `sm` will refuse to grant the execution. `sm_exec_end` calls do not have a return value, but `sm` does protect internally against mismatched such calls. For example, `n` is decremented by `sm_exec_end`, but `sm` does not allow `n` to reach negative values.

`n` is a 32-bit value. At the time of writing `sm` it is of no concern an overflow above `INT32_MAX` because that would mean there are more than 2 billion standing requests that have no yet been ended, and the assumption is that something will go wrong in other parts of the system before it goes bad in `sm`.

These are the internal state of `sm`:
- `SM_CREATED` - entered after a call to `sm_create` or `sm_open_end` is called with `success` set to `false`.
- `SM_OPENING` - entered after a call to `sm_open_begin`
- `SM_OPENED` - entered after `sm_open_end` is called with `success` set to `true`.
- `SM_OPENED_DRAINING_TO_BARRIER` - entered after a call to `sm_barrier_begin` is about to be granted. This state is active while `n` is greater than 0.
- `SM_OPENED_DRAINING_TO_CLOSE` - entered after a call to `sm_close_begin` is about to be granted. This state is active while `n` is greater than 0.
- `SM_OPENED_BARRIER` - entered when `sm_barrier_begin` is granted. 
- `SM_CLOSING` - entered when a _begin_close is granted.

In addition to the above states, part of the state is the `_close` bit that is set to `1` when `sm_close_begin` has been called. The bit stays `1` until the state is switched to `SM_CLOSING`.

The state is a 32-bit variable. It is made up of the following components:
- Least-significant 6 bits (values 0-63) represent a state enum value from above (`SM_CREATED`, `SM_OPENING`, `SM_OPENED`, `SM_OPENED_DRAINING_TO_BARRIER`, `SM_OPENED_DRAINING_TO_CLOSE`, `SM_OPENED_BARRIER`, `SM_CLOSING`).
- The 6th bit (decimal 64) set to 1 when `sm_fault` is called. The bit is never reset.
- The 7th bit (decimal 128) set to 1 when `sm_close_begin` is called. The bit is reset by `sm_close_end`.
- Remaining bits (most-significant 3 bytes, decimal >=256) representing an ever increasing counter of calls that is needed to avoid potential ABA problems. That is, a `SM_OPENED` state will be different from `SM_OPENED_STATE` after it went through a `sm_close_begin`/`sm_close_end`/`sm_open_begin`/`sm_open_end`.

## Exposed API

```c
typedef struct SM_HANDLE_DATA_TAG* SM_HANDLE;

#define SM_RESULT_VALUES    \
    SM_EXEC_GRANTED,        \
    SM_EXEC_REFUSED,        \
    SM_ERROR                \

MU_DEFINE_ENUM(SM_RESULT, SM_RESULT_VALUES);

typedef void(*ON_SM_CLOSING_COMPLETE_CALLBACK)(void* context);

MOCKABLE_FUNCTION(, SM_HANDLE, sm_create, const char*, name);
MOCKABLE_FUNCTION(, void, sm_destroy, SM_HANDLE, sm);

MOCKABLE_FUNCTION(, SM_RESULT, sm_open_begin, SM_HANDLE, sm);
MOCKABLE_FUNCTION(, void, sm_open_end, SM_HANDLE, sm, bool, success);

MOCKABLE_FUNCTION(, SM_RESULT, sm_close_begin, SM_HANDLE, sm);
MOCKABLE_FUNCTION(, void, sm_close_end, SM_HANDLE, sm);

MOCKABLE_FUNCTION(, SM_RESULT, sm_exec_begin, SM_HANDLE, sm);
MOCKABLE_FUNCTION(, void, sm_exec_end, SM_HANDLE, sm);

MOCKABLE_FUNCTION(, SM_RESULT, sm_barrier_begin, SM_HANDLE, sm);
MOCKABLE_FUNCTION(, void, sm_barrier_end, SM_HANDLE, sm);

MOCKABLE_FUNCTION(, void, sm_fault, SM_HANDLE, sm);
```

### sm_create
```c
MOCKABLE_FUNCTION(, SM_HANDLE, sm_create, const char*, name);
```
`sm_create` creates a new State manager handle. `name` is used with logging and bears no functional significance.

**SRS_SM_02_001: [** If `name` is `NULL` then `sm_create` shall behave as if `name` was "NO_NAME". **]**

**SRS_SM_02_002: [** `sm_create` shall allocate memory for the instance. **]**

**SRS_SM_02_037: [** `sm_create` shall set state to `SM_CREATED` and `n` to 0. **]**

**SRS_SM_02_004: [** If there are any failures then `sm_create` shall fail and return `NULL`. **]**

### sm_destroy
```c
MOCKABLE_FUNCTION(, void, sm_destroy, SM_HANDLE, sm);
```

`sm_destroy` frees all used resources. 

**SRS_SM_02_005: [** If `sm` is `NULL` then `sm_destroy` shall return. **]**

**SRS_SM_02_038: [** `sm_destroy` behave as if `sm_close_begin` would have been called. **]**

**SRS_SM_02_006: [** `sm_destroy` shall free all used resources. **]**

### sm_open_begin
```c
MOCKABLE_FUNCTION(, SM_RESULT, sm_open_begin, SM_HANDLE, sm);
```

`sm_open_begin` results in `sm` entering the "open" state.

**SRS_SM_02_007: [** If `sm` is `NULL` then `sm_open_begin` shall fail and return `SM_ERROR`. **]**

**SRS_SM_02_039: [** If the state is not `SM_CREATED` then `sm_open_begin` shall return `SM_EXEC_REFUSED`. **]**

**SRS_SM_42_011: [** If `SM_FAULTED_BIT` is 1 then `sm_open_begin` shall return `SM_EXEC_REFUSED`. **]**

**SRS_SM_02_040: [** `sm_open_begin` shall switch the state to `SM_OPENING`. **]**

**SRS_SM_02_009: [** `sm_open_begin` shall return `SM_EXEC_GRANTED`. **]**

### sm_open_end
```c
MOCKABLE_FUNCTION(, void, sm_open_end, SM_HANDLE, sm, bool, success);
```

`sm_open_end` informs `sm` that user's "open" state operations have completed and if they were successful.

**SRS_SM_02_010: [** If `sm` is `NULL` then `sm_open_end` shall return. **]**

**SRS_SM_02_041: [** If state is not `SM_OPENING` then `sm_open_end` shall return. **]**

**SRS_SM_02_074: [** If `success` is `true` then `sm_open_end` shall switch the state to `SM_OPENED`. **]**

**SRS_SM_02_075: [** If `success` is `false` then `sm_open_end` shall switch the state to `SM_CREATED`. **]**

### sm_close_begin_internal

```c
static SM_RESULT sm_close_begin_internal(SM_HANDLE sm, ON_SM_CLOSING_COMPLETE_CALLBACK callback, void* callback_context);
```

`sm_close_begin_internal` is the helper function for sm_close_begin and sm_close_begin_cb.

**SRS_SM_02_045: [** `sm_close_begin_internal` shall set `SM_CLOSE_BIT` to 1. **]**

**SRS_SM_02_046: [** If `SM_CLOSE_BIT` was already 1 then `sm_close_begin_internal` shall return `SM_EXEC_REFUSED`. **]**

**SRS_SM_02_047: [** If the state is `SM_OPENED` then `sm_close_begin_internal` shall switch it to `SM_OPENED_DRAINING_TO_CLOSE`. **]**

**SRS_SM_28_007: [** `callback` shall be allowed to be NULL. **]**

**SRS_SM_28_008: [** If `callback` is not `NULL`, `sm_close_begin_internal` shall invoke `callback` function with `callback_context` as argument. **]**

**SRS_SM_02_048: [** `sm_close_begin_internal` shall wait for `n` to reach 0. **]**

**SRS_SM_02_049: [** `sm_close_begin_internal` shall switch the state to `SM_CLOSING` and return `SM_EXEC_GRANTED`. **]**

**SRS_SM_02_050: [** If the state is `SM_OPENED_BARRIER` then `sm_close_begin_internal` shall re-evaluate the state. **]**

**SRS_SM_02_051: [** If the state is `SM_OPENED_DRAINING_TO_BARRIER` then `sm_close_begin_internal` shall re-evaluate the state. **]**

**SRS_SM_02_052: [** If the state is any other value then `sm_close_begin_internal` shall return `SM_EXEC_REFUSED`. **]**

**SRS_SM_02_053: [** `sm_close_begin_internal` shall set `SM_CLOSE_BIT` to 0. **]**

**SRS_SM_02_071: [** If there are any failures then `sm_close_begin_internal` shall fail and return `SM_ERROR`. **]**

### sm_close_begin
```c
MOCKABLE_FUNCTION(, SM_RESULT, sm_close_begin, SM_HANDLE, sm);
```

`sm_close_begin` results in `sm` exiting the `SM_OPENED` state (or one of its derived state) and returning to `SM_CREATED`. `sm_close_begin` waits for pending calls to become 0.

**SRS_SM_02_013: [** If `sm` is `NULL` then `sm_close_begin` shall fail and return `SM_ERROR`. **]**

**SRS_SM_28_005: [** `sm_close_begin` shall call `sm_close_begin_internal` with `callback` as `NULL` and `callback_context` as `NULL`. **]**

**SRS_SM_28_006: [** `sm_close_begin` shall return the returned `SM_RESULT` from `sm_close_begin_internal`. **]**

### sm_close_begin_with_cb
```c
MOCKABLE_FUNCTION(, SM_RESULT, sm_close_begin_with_cb, SM_HANDLE, sm, ON_SM_CLOSING_COMPLETE_CALLBACK, callback, void*, callback_context);
```

`sm_close_begin_with_cb` results in `sm` exiting the `SM_OPENED` state (or one of its derived state) and returning to `SM_CREATED`. `sm_close_begin_with_cb` invokes the `callback` function with `callback_context` before waiting for pending calls to become 0.

**SRS_SM_28_001: [** If `sm` is `NULL` then `sm_close_begin_with_cb` shall fail and return `SM_ERROR`. **]**

**SRS_SM_28_002: [** If `callback` is `NULL` then `sm_close_begin_with_cb` shall fail and return `SM_ERROR`. **]**

**SRS_SM_28_003: [** `sm_close_begin_with_cb` shall call `sm_close_begin_internal` with `callback` and `callback_context` as arguments. **]**

**SRS_SM_28_004: [** `sm_close_begin_with_cb` shall return the returned `SM_RESULT` from `sm_close_begin_internal`. **]**

### sm_close_end
```c
MOCKABLE_FUNCTION(, void, sm_close_end, SM_HANDLE, sm);
```

`sm_close_end` informs `sm` that user's "close" state operations have completed.

**SRS_SM_02_018: [** If `sm` is `NULL` then `sm_close_end` shall return. **]**

**SRS_SM_02_043: [** If the state is not `SM_CLOSING` then `sm_close_end` shall return. **]**

**SRS_SM_02_044: [** `sm_close_end` shall switch the state to `SM_CREATED`. **]**

**SRS_SM_42_012: [** `sm_close_end` shall not reset the `SM_FAULTED_BIT`. **]**

### sm_exec_begin
```c
MOCKABLE_FUNCTION(, int, sm_exec_begin, SM_HANDLE, sm);
```

`sm_exec_begin` asks from `sm` permission to execute a non-barrier operation.

**SRS_SM_02_021: [** If `sm` is `NULL` then `sm_exec_begin` shall fail and return `SM_ERROR`. **]**

**SRS_SM_02_054: [** If state is not `SM_OPENED` then `sm_exec_begin` shall return `SM_EXEC_REFUSED`. **]**

**SRS_SM_02_055: [** If `SM_CLOSE_BIT` is 1 then `sm_exec_begin` shall return `SM_EXEC_REFUSED`. **]**

**SRS_SM_42_002: [** If `SM_FAULTED_BIT` is 1 then `sm_exec_begin` shall return `SM_EXEC_REFUSED`. **]**

**SRS_SM_02_056: [** `sm_exec_begin` shall increment `n`. **]**

**SRS_SM_02_057: [** If the state changed after incrementing `n` then `sm_exec_begin` shall return `SM_EXEC_REFUSED`. **]**

**SRS_SM_02_058: [** `sm_exec_begin` shall return `SM_EXEC_GRANTED`. **]**

### sm_exec_end
```c
MOCKABLE_FUNCTION(, void, sm_exec_end, SM_HANDLE, sm);
```

`sm_exec_end` informs `sm` that user's execution of a non-barrier operations has completed.

**SRS_SM_02_024: [** If `sm` is `NULL` then `sm_exec_end` shall return. **]**

**SRS_SM_02_059: [** If state is not `SM_OPENED` then `sm_exec_end` shall return. **]**

**SRS_SM_02_060: [** If state is not `SM_OPENED_DRAINING_TO_BARRIER` then `sm_exec_end` shall return. **]**

**SRS_SM_02_061: [** If state is not `SM_OPENED_DRAINING_TO_CLOSE` then `sm_exec_end` shall return. **]**

**SRS_SM_42_013: [** `sm_exec_end` may be called when `SM_FAULTED_BIT` is 1. **]**

**SRS_SM_02_062: [** `sm_exec_end` shall decrement `n` with saturation at 0. **]**

**SRS_SM_02_063: [** If `n` reaches 0 then `sm_exec_end` shall signal that. **]**

**SRS_SM_42_010: [** If `n` would decrement below 0, then `sm_exec_end` shall terminate the process. **]**

### sm_barrier_begin
```c
MOCKABLE_FUNCTION(, int, sm_barrier_begin, SM_HANDLE, sm);
```

`sm_barrier_begin` asks from `sm` permission to execute a barrier operation.

**SRS_SM_02_027: [** If `sm` is `NULL` then `sm_barrier_begin` shall fail and return `SM_ERROR`. **]**

**SRS_SM_02_064: [** If state is not `SM_OPENED` then `sm_barrier_begin` shall return `SM_EXEC_REFUSED`. **]**

**SRS_SM_02_065: [** If `SM_CLOSE_BIT` is set to 1 then `sm_barrier_begin` shall return `SM_EXEC_REFUSED`. **]**

**SRS_SM_42_003: [** If `SM_FAULTED_BIT` is set to 1 then `sm_barrier_begin` shall return `SM_EXEC_REFUSED`. **]**

**SRS_SM_02_066: [** `sm_barrier_begin` shall switch the state to `SM_OPENED_DRAINING_TO_BARRIER`. **]**

**SRS_SM_02_067: [** If the state changed meanwhile then `sm_barrier_begin` shall return `SM_EXEC_REFUSED`. **]**

**SRS_SM_02_068: [** `sm_barrier_begin` shall wait for `n` to reach 0. **]**

**SRS_SM_02_069: [** `sm_barrier_begin` shall switch the state to `SM_OPENED_BARRIER` and return `SM_EXEC_GRANTED`. **]**

**SRS_SM_02_070: [** If there are any failures then `sm_barrier_begin` shall return `SM_ERROR`. **]**

### sm_barrier_end
```c
MOCKABLE_FUNCTION(, void, sm_barrier_end, SM_HANDLE, sm);
```

`sm_barrier_end` informs `sm` that user's execution of a barrier operations has completed.

**SRS_SM_02_032: [** If `sm` is `NULL` then `sm_barrier_end` shall return. **]**

**SRS_SM_02_072: [** If state is not `SM_OPENED_BARRIER` then `sm_barrier_end` shall return. **]**

**SRS_SM_42_014: [** `sm_barrier_end` may be called when `SM_FAULTED_BIT` is 1. **]**

**SRS_SM_02_073: [** `sm_barrier_end` shall switch the state to `SM_OPENED`. **]**

### sm_fault
```c
MOCKABLE_FUNCTION(, void, sm_fault, SM_HANDLE, sm);
```

`sm_fault` puts `sm` in a faulted state that cannot be cleared.

**SRS_SM_42_004: [** If `sm` is `NULL` then `sm_fault` shall return. **]**

**SRS_SM_42_007: [** `sm_fault` shall set `SM_FAULTED_BIT` to 1. **]**
