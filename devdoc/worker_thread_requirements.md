# `worker_thread` requirements

## Overview

`worker_thread` is a helper module that implements execution of a work item on a single separate thread. It can be used in batching operations, sequencing completions, etc.
The user instantiates a worker thread instance and then signals when the work shall be executed by calling `worker_thread_schedule_process`.

## Thread Concerns

A calls to `worker_thread_schedule_process` must wait for the resulting `WORKER_FUNC` call or the call will not be delivered.  For this reason `worker_thread_schedule_process` function should be called in a single threaded manner.

### Reentrancy

Users can not call worker_thread_close or worker_thread_destroy functions from within `WORKER_FUNC` call.  This action will result in a deadlock waiting for the `WORKER_FUNC` to end.  The user is allowed to call `worker_thread_schedule_process` from within `WORKER_FUNC`.

## Exposed API

```c
    /* this is the callback that is to be called each time processing (worker executing) is scheduled */
    /* Note that if worker_thread_schedule_process is called multiple time before WORKER_FUNC executes (worker executing), a new WORKER_FUNC process will not be scheduled 
    (the number of executed WORKER_FUNC calls is <= number of worker_thread_schedule_process calls made ) */
    typedef void(*WORKER_FUNC)(void* worker_func_context);

    typedef struct WORKER_THREAD_TAG* WORKER_THREAD_HANDLE;

    #define WORKER_THREAD_SCHEDULE_PROCESS_RESULT_VALUES \
        WORKER_THREAD_SCHEDULE_PROCESS_OK, \
        WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE, \
        WORKER_THREAD_SCHEDULE_PROCESS_ERROR \

    MU_DEFINE_ENUM(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_RESULT_VALUES)

    MOCKABLE_FUNCTION(, WORKER_THREAD_HANDLE, worker_thread_create, WORKER_FUNC, worker_func, void*, worker_func_context);
    MOCKABLE_FUNCTION(, void, worker_thread_destroy, WORKER_THREAD_HANDLE, worker_thread);

    MOCKABLE_FUNCTION(, int, worker_thread_open, WORKER_THREAD_HANDLE, worker_thread);
    MOCKABLE_FUNCTION(, void, worker_thread_close, WORKER_THREAD_HANDLE, worker_thread);

    MOCKABLE_FUNCTION(, WORKER_THREAD_SCHEDULE_PROCESS_RESULT, worker_thread_schedule_process, WORKER_THREAD_HANDLE, worker_thread);
```

### worker_thread_create

```c
MOCKABLE_FUNCTION(, WORKER_THREAD_HANDLE, worker_thread_create, WORKER_FUNC, worker_func, void*, worker_func_context);
```

**SRS_WORKER_THREAD_01_001: [** `worker_thread_create` shall allocate memory for a new worker thread object and on success return a non-NULL handle to it. **]**

**SRS_WORKER_THREAD_01_003: [** If `worker_func` is NULL, `worker_thread_create` shall fail and return NULL. **]**

**SRS_WORKER_THREAD_01_004: [** `worker_func_context` shall be allowed to be NULL. **]**

**SRS_WORKER_THREAD_01_022: [** `worker_thread_create` shall perform the following actions in order: **]**

- **SRS_WORKER_THREAD_01_037: [** `worker_thread_create` shall create a state manager object by calling `sm_create` with the name `worker_thread`. **]**

- **SRS_WORKER_THREAD_01_006: [** `worker_thread_create` shall initialize the state object. **]**

**SRS_WORKER_THREAD_01_008: [** If any error occurs, `worker_thread_create` shall fail and return NULL. **]**

### worker_thread_destroy

```c
MOCKABLE_FUNCTION(, void, worker_thread_destroy, WORKER_THREAD_HANDLE, worker_thread);
```

**SRS_WORKER_THREAD_01_009: [** `worker_thread_destroy` shall free the resources associated with the worker thread handle. **]**

**SRS_WORKER_THREAD_01_010: [** If `worker_thread` is NULL, `worker_thread_destroy` shall return. **]**

**SRS_WORKER_THREAD_01_039: [** If the worker thread is open, `worker_thread_destroy` shall perform a close. **]**

**SRS_WORKER_THREAD_01_038: [** `worker_thread_destroy` shall destroy the state manager object created in `worker_thread_create`. **]**

### worker_thread_open

```c
MOCKABLE_FUNCTION(, int, worker_thread_open, WORKER_THREAD_HANDLE, worker_thread);
```

`worker_thread_open` opens the worker thread object (starts the underlying thread).

**SRS_WORKER_THREAD_01_026: [** If `worker_thread` is `NULL`, `worker_thread_open` shall fail and return a non-zero value. **]**

**SRS_WORKER_THREAD_01_027: [** Otherwise, `worker_thread_open` shall call `sm_open_begin`. **]**

**SRS_WORKER_THREAD_01_028: [** `worker_thread_open` shall start a `worker_thread` thread that will call `worker_func` by calling `ThreadAPI_Create`. **]**

**SRS_WORKER_THREAD_01_029: [** `worker_thread_open` shall call `sm_open_end` with success reflecting whether any error has occured during the open. **]**

**SRS_WORKER_THREAD_01_030: [** On success, `worker_thread_open` shall return 0. **]**

**SRS_WORKER_THREAD_01_031: [** If any error occurs, `worker_thread_open` shall fail and return a non-zero value. **]**

### worker_thread_close

```c
MOCKABLE_FUNCTION(, void, worker_thread_close, WORKER_THREAD_HANDLE, worker_thread);
```

`worker_thread_close` closes the worker thread object (stops the underlying thread).

**SRS_WORKER_THREAD_01_032: [** If `worker_thread` is `NULL`, `worker_thread_close` shall return. **]**

**SRS_WORKER_THREAD_01_033: [** Otherwise, `worker_thread_close` shall call `sm_close_begin`. **]**

**SRS_WORKER_THREAD_01_040: [** If `sm_close_begin` does not return `SM_EXEC_GRANTED`, `worker_thread_close` shall return. **]**

**SRS_WORKER_THREAD_01_034: [** `worker_thread_close` shall set the worker thread state to close in order to indicate that the thread shall shutdown. **]**

**SRS_WORKER_THREAD_01_035: [** `worker_thread_close` shall wait for the thread to join by using `ThreadAPI_Join`. **]**

**SRS_WORKER_THREAD_01_036: [** `worker_thread_close` shall call `sm_close_end`. **]**

### worker_thread_schedule_process

```c
MOCKABLE_FUNCTION(, WORKER_THREAD_SCHEDULE_PROCESS_RESULT, worker_thread_schedule_process, WORKER_THREAD_HANDLE, worker_thread);
```

`worker_thread_schedule_process` signals that a new execution of the worker function should happen.  Multiple calls to `worker_thread_schedule_process` before a `WORKER_FUNC` happens will result in only 1 call to the `WORKER_FUNC`

**SRS_WORKER_THREAD_01_016: [** If `worker_thread` is NULL, `worker_thread_schedule_process` shall fail and return `WORKER_THREAD_SCHEDULE_PROCESS_ERROR`. **]**

**SRS_WORKER_THREAD_01_041: [** Otherwise, `worker_thread_schedule_process` shall call `sm_exec_begin`. **]**

**SRS_WORKER_THREAD_01_042: [** If `sm_exec_begin` does not grant the execution, `worker_thread_schedule_process` shall fail and return `WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE`. **]**

**SRS_WORKER_THREAD_01_017: [** `worker_thread_schedule_process` shall set the thread state to `WORKER_THREAD_STATE_PROCESS_ITEM`. **]**

**SRS_WORKER_THREAD_11_003: [** If the thread state is not `WORKER_THREAD_STATE_IDLE` or `WORKER_THREAD_STATE_PROCESS_ITEM`, `worker_thread_schedule_process` shall fail and return `WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE`. **]**

**SRS_WORKER_THREAD_01_043: [** `worker_thread_schedule_process` shall call `sm_exec_end`. **]**

**SRS_WORKER_THREAD_01_015: [** On success `worker_thread_schedule_process` shall return `WORKER_THREAD_SCHEDULE_PROCESS_OK`. **]**

### worker_thread

**SRS_WORKER_THREAD_01_019: [** The worker thread started by `worker_thread_create` shall get the thread state. **]**

**SRS_WORKER_THREAD_01_020: [** If the thread state is `WORKER_THREAD_STATE_CLOSE`, the worker thread shall exit. **]**

**SRS_WORKER_THREAD_01_021: [** If the thread state is `WORKER_THREAD_STATE_PROCESS_ITEM`, the worker thread shall call the `worker_func` function passed to `worker_thread_create` and it shall pass `worker_func_context` as argument... **]**

**SRS_WORKER_THREAD_11_001: [** ... and set the thread state to `WORKER_THREAD_STATE_IDLE` if it has not been changed. **]**

**SRS_WORKER_THREAD_11_002: [** If the thread state is `WORKER_THREAD_STATE_IDLE`, the worker thread shall wait for the state to transition to something else. **]**
