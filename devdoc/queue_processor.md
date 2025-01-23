# `queue_processor` requirements

## Overview

`queue_processor` is a module that queues work items to be executed in order.
Adding items to the queue can happen from multiple threads.
Executing items only happens on one thread.

## Design

`queue_processor` does not create a dedicated thread, but instead uses a `threadpool` instance to schedule work.
`queue_processor` uses a multi producer single consumer queue to store the work items.

In order to make sure that only one threadpool work item processes items at a given moment, a 3 states state machine is used.
The states represent a poor man's lock + a flag indicating new data:

- IDLE - Currently, no item is being executed.
- EXECUTING - An item that was scheduled through `queue_processor_schedule_item` is being executed.
- NEW_DATA - An item that was scheduled through `queue_processor_schedule_item` is being executed and at least one extra item has been queued behind the currently executing item.

Transitions done in `queue_processor_schedule_item`:

- If state is IDLE then it is set to EXECUTING and a new threadpool work item is started.
- If state is EXECUTING then it is set to NEW_DATA in order to signal the threadpool work item that more data is in the queue.

Transitions done in the threadpool work item callback function `on_threadpool_work` after processing all items in the queue and the queue is (allegedly) empty:

- If state is EXECUTING then it is set to IDLE and the work item exits.
- If state is NEW_DATA then it is set to EXECUTING and the work item executes again all the items in the queue.

## Exposed API

```c
typedef struct QUEUE_PROCESSOR_TAG* QUEUE_PROCESSOR_HANDLE;

typedef void(*ON_QUEUE_PROCESSOR_ERROR)(void* context);
typedef void(*QUEUE_PROCESSOR_PROCESS_ITEM)(void* context);

MOCKABLE_FUNCTION(, QUEUE_PROCESSOR_HANDLE, queue_processor_create, THANDLE(THREADPOOL), threadpool);
MOCKABLE_FUNCTION(, void, queue_processor_destroy, QUEUE_PROCESSOR_HANDLE, queue_processor);
MOCKABLE_FUNCTION(, int, queue_processor_open, QUEUE_PROCESSOR_HANDLE, queue_processor, ON_QUEUE_PROCESSOR_ERROR, on_error, void*, on_error_context);
MOCKABLE_FUNCTION(, void, queue_processor_close, QUEUE_PROCESSOR_HANDLE, queue_processor);

MOCKABLE_FUNCTION(, int, queue_processor_schedule_item, QUEUE_PROCESSOR_HANDLE, queue_processor, QUEUE_PROCESSOR_PROCESS_ITEM, process_item_func, void*, process_item_context);
```

### queue_processor_create

```c
MOCKABLE_FUNCTION(, QUEUE_PROCESSOR_HANDLE, queue_processor_create, THANDLE(THREADPOOL), threadpool);
```

`queue_processor_create` creates a new queue processor instance.

**SRS_QUEUE_PROCESSOR_11_001: [** If `threadpool` is `NULL`, `queue_processor_create` shall fail and return `NULL`. **]**

**SRS_QUEUE_PROCESSOR_01_002: [** Otherwise, `queue_processor_create` shall create a new queue processor and on success return a non-NULL handle to it. **]**

**S_R_S_QUEUE_PROCESSOR_05_002: [** `queue_processor_create` shall create a threadpool work item context by calling `threadpool_create_work_item` with callback `on_threadpool_work` and `queue_processor` as input to the context. **]

- **S_R_S_QUEUE_PROCESSOR_05_003: [** If `threadpool_create_work_item` fails then `queue_processor_create` shall fail and return a `NULL` **]**

**SRS_QUEUE_PROCESSOR_01_003: [** `queue_processor_create` shall initialize its threadpool object by calling `THANDLE_INITIALIZE(THREADPOOL)`. **]**

**SRS_QUEUE_PROCESSOR_01_045: [** `queue_processor_create` shall create a multi producer single consumer queue. **]**

**SRS_QUEUE_PROCESSOR_01_004: [** If any error occurs, `queue_processor_create` shall fail and return NULL. **]**

### queue_processor_destroy

```c
MOCKABLE_FUNCTION(, void, queue_processor_destroy, QUEUE_PROCESSOR_HANDLE, queue_processor);
```

`queue_processor_destroy` frees all resources associated with a queue processor instance.

**SRS_QUEUE_PROCESSOR_01_005: [** If `queue_processor` is NULL, `queue_processor_destroy` shall return. **]**

**SRS_QUEUE_PROCESSOR_01_006: [** Otherwise `queue_processor_destroy` shall wait until the state is either CLOSED or OPEN. **]**

**SRS_QUEUE_PROCESSOR_01_007: [** `queue_processor_destroy` shall perform a close in case the `queue_processor` is not CLOSED. **]**

**SRS_QUEUE_PROCESSOR_01_008: [** `queue_processor_destroy` shall  assign `NULL` the threadpool initialized in `queue_processor_create`. **]**

**SRS_QUEUE_PROCESSOR_01_046: [** `queue_processor_destroy` shall destroy the multi producer single consumer queue created in `queue_processor_create`. **]**

**SRS_QUEUE_PROCESSOR_01_009: [** `queue_processor_destroy` shall free the memory associated with the worker. **]**

### queue_processor_open

```c
MOCKABLE_FUNCTION(, int, queue_processor_open, QUEUE_PROCESSOR_HANDLE, queue_processor, ON_QUEUE_PROCESSOR_ERROR, on_error, void*, on_error_context);
```
Note: on_error and on_error_context are no longer used and will be removed in a subsequent PR.

`queue_processor_open` opens the queue processor.

**SRS_QUEUE_PROCESSOR_01_010: [** If `queue_processor` is NULL, `queue_processor_open` shall fail and return a non-zero value. **]**

[Will be Deprecated]:
**SRS_QUEUE_PROCESSOR_01_048: [** If `on_error` is NULL, `queue_processor_open` shall fail and return a non-zero value. **]**

**S_R_S_QUEUE_PROCESSOR_05_001: [** `on_error` shall be allowed to be NULL. **]**

**SRS_QUEUE_PROCESSOR_01_049: [** `on_error_context` shall be allowed to be NULL. **]**

**SRS_QUEUE_PROCESSOR_01_013: [** Otherwise, `queue_processor_open` shall switch the state to `QUEUE_PROCESSOR_STATE_OPENING`. **]**

**SRS_QUEUE_PROCESSOR_01_014: [** If the queue_processor state was not `QUEUE_PROCESSOR_STATE_CLOSED`, `queue_processor_open` shall fail and return non-zero value. **]**

**S_R_S_QUEUE_PROCESSING_05_008: [** `queue_processor_open` shall set the queue_processor state to `QUEUE_PROCESSOR_STATE_OPENED`. **]

**SRS_QUEUE_PROCESSOR_01_016: [** On success, `queue_processor_open` shall return 0. **]**

### queue_processor_close

```c
MOCKABLE_FUNCTION(, void, queue_processor_close, QUEUE_PROCESSOR_HANDLE, queue_processor);
```

`queue_processor_close` closes the queue processor.

**SRS_QUEUE_PROCESSOR_01_024: [** If `queue_processor` is NULL, `queue_processor_close` shall return. **]**

**SRS_QUEUE_PROCESSOR_01_025: [** If the state of `queue_processor` is not OPEN, `queue_processor_close` shall return. **]**

**SRS_QUEUE_PROCESSOR_01_026: [** Otherwise, `queue_processor_close` shall switch the state to `QUEUE_PROCESSOR_STATE_CLOSING`. **]**

**SRS_QUEUE_PROCESSOR_01_027: [** `queue_processor_close` shall wait for any ongoing `queue_processor_schedule_item` API calls to complete. **]**

**S_R_S_QUEUE_PROCESSOR_05_004: [** `queue_processor_close` shall destroy the threadpool work item context by calling `threadpool_destroy_work_item`. **]**

**SRS_QUEUE_PROCESSOR_11_003: [** If the process state is `IDLE` then `queue_processor_close` shall change it to `PROCESSING_STATE_EXECUTING` and call the `on_threadpool_work` function. **]**

**SRS_QUEUE_PROCESSOR_01_051: [** Any queued items that were not processed shall be processed by `queue_processor_close`. **]**

**SRS_QUEUE_PROCESSOR_01_029: [** `queue_processor_close` shall set the state to CLOSED. **]**

**SRS_QUEUE_PROCESSOR_01_030: [** After a close, a succesfull call to `queue_processor_open` shall be possible. **]**

### queue_processor_schedule_item

```c
MOCKABLE_FUNCTION(, int, queue_processor_schedule_item, QUEUE_PROCESSOR_HANDLE, queue_processor, QUEUE_PROCESSOR_PROCESS_ITEM, process_item_func, void*, process_item_context);
```

`queue_processor_schedule_item` queues an item to be executed.

**SRS_QUEUE_PROCESSOR_01_031: [** If `queue_processor` is NULL, `queue_processor_schedule_item` shall fail and return non-zero value. **]**

**SRS_QUEUE_PROCESSOR_01_047: [** If `process_item_func` is NULL, `queue_processor_schedule_item` shall fail and and return non-zero value. **]**

**SRS_QUEUE_PROCESSOR_01_052: [** `process_item_context` shall be allowed to be NULL. **]**

**SRS_QUEUE_PROCESSOR_01_032: [** If the `queue_processor` state is not OPEN, `queue_processor_schedule_item` shall fail and return non-zero value. **]**

**SRS_QUEUE_PROCESSOR_01_033: [** Otherwise `queue_processor_schedule_item` shall allocate a context where `process_item_func` and `process_item_context` shall be stored. **]**

**SRS_QUEUE_PROCESSOR_01_034: [** `queue_processor_schedule_item` shall enqueue the context in the multi producer single consumer queue created in `queue_processor_create`. **]**

**SRS_QUEUE_PROCESSOR_01_036: [** If the processing state is IDLE, `queue_processor_schedule_item` shall switch the processing state to EXECUTING, `queue_processor_schedule_item` shall call `threadpool_schedule_work` to schedule a work item and pass as callback `on_threadpool_work` and `queue_processor` via `threadpool_work_item_context`. **]**

**SRS_QUEUE_PROCESSOR_01_053: [** If the processing state is EXECUTING, `queue_processor_schedule_item` shall switch the processing state to NEW_DATA to signal `on_threadpool_work` that new items need to be processed. **]**

**SRS_QUEUE_PROCESSOR_01_037: [** On success, `queue_processor_schedule_item` returns 0. **]**

**SRS_QUEUE_PROCESSOR_01_038: [** If any error occurs, `queue_processor_schedule_item` shall fail and return non-zero value. **]**

Note: on_error and on_error_context are no longer used and will be removed in a subsequent PR.

[Will be Deprecated]:
**SRS_QUEUE_PROCESSOR_01_050: [** If scheduling the threadpool work item fails, an error shall be indicated by calling the `on_error` callback passed to `queue_processor_open`... **]**

[Will be Deprecated]:
**SRS_QUEUE_PROCESSOR_11_002: [** ... and shall set the state to `QUEUE_PROCESSOR_STATE_FAULTED`. **]**

### on_threadpool_work

```c
static void on_threadpool_work(void* context);
```

**SRS_QUEUE_PROCESSOR_01_039: [** If `context` is NULL, `on_threadpool_work` shall return. **]**

**SRS_QUEUE_PROCESSOR_01_040: [** Otherwise `on_threadpool_work` shall use `context` as the `QUEUE_PROCESSOR_HANDLE` passed in `queue_processor_schedule_item`. **]**

**SRS_QUEUE_PROCESSOR_01_041: [** While there are items in the queue: **]**

**SRS_QUEUE_PROCESSOR_01_042: [** `on_threadpool_work` shall dequeue an item from the queue. **]**

**SRS_QUEUE_PROCESSOR_01_043: [** `on_threadpool_work` shall call the `process_item_func` with the context set to `process_item_context`. **]**

**SRS_QUEUE_PROCESSOR_01_055: [** If the processing state is EXECUTING, it shall be set to IDLE. **]**

**SRS_QUEUE_PROCESSOR_01_054: [** If the processing state is NEW_DATA, it shall be set to EXECUTING and the queue shall be re-examined and all new items in it processed. **]**
