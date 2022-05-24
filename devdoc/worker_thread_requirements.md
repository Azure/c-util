`worker_thread` requirements
================

## Overview

`worker_thread` is a helper module that implements execution of a work item on a separate thread. It can be used in batching operations, sequencing completions, etc.
The user instantiates a worker thread instance and then signals when the work shall be executed by calling `worker_thread_schedule_process`.

Note: The worker thread module was created in order to ease testing of the user components.

## Exposed API

```c
    /* this is the callback that is to be called each time a batch process is scheduled */
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

- **SRS_WORKER_THREAD_01_006: [** `worker_thread_create` shall create an auto reset unnamed event used for signaling the thread to shutdown by calling `CreateEvent`. **]**

- **SRS_WORKER_THREAD_01_007: [** `worker_thread_create` shall create an auto reset unnamed event used for signaling that a new execution of the worker function should be done by calling `CreateEvent`. **]**

**SRS_WORKER_THREAD_01_008: [** If any error occurs, `worker_thread_create` shall fail and return NULL. **]**

### worker_thread_destroy

```c
MOCKABLE_FUNCTION(, void, worker_thread_destroy, WORKER_THREAD_HANDLE, worker_thread);
```

**SRS_WORKER_THREAD_01_009: [** `worker_thread_destroy` shall free the resources associated with the worker thread handle. **]**

**SRS_WORKER_THREAD_01_010: [** If `worker_thread` is NULL, `worker_thread_destroy` shall return. **]**

**SRS_WORKER_THREAD_01_039: [** If the worker thread is open, `worker_thread_destroy` shall perform a close. **]**

**SRS_WORKER_THREAD_01_038: [** `worker_thread_destroy` shall destroy the state manager object created in `worker_thread_create`. **]**

**SRS_WORKER_THREAD_01_013: [** `worker_thread_destroy` shall free the events created in `worker_thread_create` by calling `CloseHandle` on them. **]**

**SRS_WORKER_THREAD_01_023: [** Any errors in `worker_thread_destroy` shall be ignored. **]**

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

**SRS_WORKER_THREAD_01_034: [** `worker_thread_close` shall signal the thread shutdown event in order to indicate that the thread shall shutdown. **]**

**SRS_WORKER_THREAD_01_035: [** `worker_thread_close` shall wait for the thread to join by using `ThreadAPI_Join`. **]**

**SRS_WORKER_THREAD_01_036: [** `worker_thread_close` shall call `sm_close_end`. **]**

### worker_thread_schedule_process

```c
MOCKABLE_FUNCTION(, WORKER_THREAD_SCHEDULE_PROCESS_RESULT, worker_thread_schedule_process, WORKER_THREAD_HANDLE, worker_thread);
```

`worker_thread_schedule_process` signals that a new execution of the worker function should happen.

**SRS_WORKER_THREAD_01_016: [** If `worker_thread` is NULL, `worker_thread_schedule_process` shall fail and return `WORKER_THREAD_SCHEDULE_PROCESS_ERROR`. **]**

**SRS_WORKER_THREAD_01_041: [** Otherwise, `worker_thread_schedule_process` shall call `sm_exec_begin`. **]**

**SRS_WORKER_THREAD_01_042: [** If `sm_exec_begin` does not grant the execution, `worker_thread_schedule_process` shall fail and return `WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE`. **]**

**SRS_WORKER_THREAD_01_017: [** `worker_thread_schedule_process` shall set the process event created in `worker_thread_create`. **]**

**SRS_WORKER_THREAD_01_043: [** `worker_thread_schedule_process` shall call `sm_exec_end`. **]**

**SRS_WORKER_THREAD_01_015: [** On success `worker_thread_schedule_process` shall return `WORKER_THREAD_SCHEDULE_PROCESS_OK`. **]**

**SRS_WORKER_THREAD_01_018: [** If any other error occurs, `worker_thread_schedule_process` shall fail and return `WORKER_THREAD_SCHEDULE_PROCESS_ERROR`. **]**

### worker_thread

**SRS_WORKER_THREAD_01_019: [** The worker thread started by `worker_thread_create` shall wait for the 2 events: thread shutdown event and execute worker function event. **]**

**SRS_WORKER_THREAD_01_020: [** When the shutdown event is signaled, the worker thread shall exit. **]**

**SRS_WORKER_THREAD_01_021: [** When the execute worker function event is signaled, the worker thread shall call the `worker_func` function passed to `worker_thread_create` and it shall pass `worker_func_context` as argument. **]**

**SRS_WORKER_THREAD_01_025: [** In case of any error, the worker thread shall exit. **]**

