# `channel` requirements

## Overview

`channel` is a module that allows the user to move data asynchronously from producer to consumer without having to block the thread.

Consumers of data call `channel_pull` to register a `ON_DATA_AVAILABLE_CB` function to be called when data becomes available.

Producers of data call `channel_push` to notify the channel that data is available and provide a `ON_DATA_CONSUMED_CB` to be called when the data has been delivered to the consumer.

`channel_pull` and `channel_push` can be called from different threads in any order. The callbacks are executed on threadpool threads.

```mermaid
stateDiagram-v2
    B: CREATED
    O: OPEN, OP_QUEUE=[]
    N: OPEN,OP_QUEUE=[push_op,...]
    R: OPEN,OP_QUEUE=[pull_op,...]
    E: OPEN,OP_QUEUE=[]
    C: CLOSED, OP_QUEUE=[]

    [*] --> B: channel_create
    B --> O: channel_open

    O --> N: channel_push
    O --> R: channel_pull

    N --> E: channel_pull

    R --> E: channel_push

    N --> E: cancel channel_push
    R --> E: cancel channel_pull

    R --> R: channel_pull
    N --> N: channel_push

    O --> C: channel_close
    E --> C: channel_close
    R --> C: channel_close
    N --> C: channel_close
    
    C --> O: channel_open
    C --> [*]: channel_destroy

```

## Operations

The first call to `channel_pull`/`channel_push` creates an `operation` and adds it to a `list of pending operations`. When `channel_push`/`channel_pull` is called subsequently, the least-recently added `operation` in the `list of pending operations` is scheduled to have its callbacks called on the threadpool. Each operation stores the `CHANNEL_CALLBACK_RESULT` that the callback should be called with.

`CHANNEL_CALLBACK_RESULT_ABANDONED`: The callbacks are called with this result when channel is closed.

`CHANNEL_CALLBACK_RESULT_CANCELLED`: The callbacks are called with this result when the `operation` has been cancelled. 

`CHANNEL_CALLBACK_RESULT_OK`: The callbacks are called with this result in the success case.

In the case when an `operation`'s callbacks are scheduled to be called with `CHANNEL_CALLBACK_RESULT_OK` but the operation is cancelled before they can execute, the callbacks are called with `CHANNEL_CALLBACK_RESULT_CANCELLED`.

Notes on operation state transitions:
- An incomplete operation (unpaired call to `channel_push`/`channel_pull`) can be cancelled.
- An `operation` that is scheduled to be called with `CHANNEL_CALLBACK_RESULT_OK` (but callbacks have not yet executed) can be cancelled.
- An `operation` that is abandoned cannot be cancelled.
- An `operation` that is cancelled cannot be complete with `CHANNEL_CALLBACK_RESULT_OK`.

## Reentrancy

`channel_pull` and `channel_push` can be called from callbacks of this module. Since the callbacks are executed on threadpool threads, there is no risk of stack overflow or deadlock.

## Need for `CHANNEL_INTERNAL`

The lifetime of the state of the `channel` module is linked to two things:
 - `THANDLE(CHANNEL)` references held by the producer and consumer.
 - `THANDLE(ASYNC_OP)` operations returned from `channel_push` and `channel_pull`. The `THANDLE(ASYNC_OP)`s need to hold references to the state of the `channel` because `async_op_cancel` needs to be able to lock the list of pending operations to remove the operation from the list.

Since the `THANDLE(ASYNC_OP)`s can outlive the `THANDLE(CHANNEL)` references and `THANDLE(ASYNC_OP)` must access the state of the `channel`, the state of the `channel` cannot be freed when the last `THANDLE(CHANNEL)` reference is released. 

To decouple the lifetime of the state from the `THANDLE(CHANNEL)` references, `THANDLE(CHANNEL_INTERNAL)` is used to store the state of the module. `THANDLE(CHANNEL)` holds a single reference to `THANDLE(CHANNEL_INTERNAL)` and each `THANDLE(ASYNC_OP)` also holds a reference to `THANDLE(CHANNEL_INTERNAL)`. When the last `THANDLE(CHANNEL)` reference is released, all pending operations are abandoned and `THANDLE(CHANNEL)` releases its reference to `THANDLE(CHANNEL_INTERNAL)`. However, `THANDLE(CHANNEL_INTERNAL)` is not freed yet because `THANDLE(ASYNC_OP)`s still hold references to it.

## Exposed API

`channel_common.h` (contains type definitions for types used in `channel.h` and `channel_internal.h`):
```c
#define CHANNEL_RESULT_VALUES \
    CHANNEL_RESULT_OK, \
    CHANNEL_RESULT_INVALID_ARGS, \
    CHANNEL_RESULT_ERROR

MU_DEFINE_ENUM(CHANNEL_RESULT, CHANNEL_RESULT_VALUES);

#define CHANNEL_CALLBACK_RESULT_VALUES \
    CHANNEL_CALLBACK_RESULT_OK, \
    CHANNEL_CALLBACK_RESULT_CANCELLED, \
    CHANNEL_CALLBACK_RESULT_ABANDONED

MU_DEFINE_ENUM(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_VALUES);

typedef void(*ON_DATA_AVAILABLE_CB)(void* pull_context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_PTR) data);
typedef void(*ON_DATA_CONSUMED_CB)(void* push_context, CHANNEL_CALLBACK_RESULT result);
```

`channel.h`:
```c

THANDLE_TYPE_DECLARE(CHANNEL);

MOCKABLE_FUNCTION(, THANDLE(CHANNEL), channel_create, THANDLE(PTR(LOG_CONTEXT_HANDLE), log_context, THANDLE(THREADPOOL), threadpool);
MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_pull, THANDLE(CHANNEL), channel, THANDLE(RC_STRING), correlation_id, ON_DATA_AVAILABLE_CB, on_data_available_cb, void*, pull_context, THANDLE(ASYNC_OP)*, out_op_pull);
MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_push, THANDLE(CHANNEL), channel, THANDLE(RC_STRING), correlation_id, THANDLE(RC_PTR), data, ON_DATA_CONSUMED_CB, on_data_consumed_cb, void*, push_context, THANDLE(ASYNC_OP)*, out_op_push);

```

### channel_create
```c
    MOCKABLE_FUNCTION(, THANDLE(CHANNEL), channel_create, THANDLE(PTR(LOG_CONTEXT_HANDLE), log_context, THANDLE(THREADPOOL), threadpool);
```

`channel_create` creates the channel and returns it.

**SRS_CHANNEL_43_077: [** If `threadpool` is `NULL`, `channel_create` shall fail and return `NULL`. **]**

**SRS_CHANNEL_43_001: [** `channel_create` shall create a `CHANNEL` object by calling `THANDLE_MALLOC` with `channel_dispose` as `dispose`. **]**

**SRS_CHANNEL_43_078: [** `channel_create` shall call create a `THANDLE(CHANNEL_INTERNAL)` by calling `channel_internal_create`.**]**

**SRS_CHANNEL_43_079: [** `channel_create` shall store the created `THANDLE(CHANNEL_INTERNAL)` in the `THANDLE(CHANNEL)`. **]**

**SRS_CHANNEL_43_086: [** `channel_create` shall succeed and return the created `THANDLE(CHANNEL)`. **]**

**SRS_CHANNEL_43_002: [** If there are any failures, `channel_create` shall fail and return `NULL`. **]**


### channel_dispose
```c
    static void channel_dispose(CHANNEL* channel);
```

`channel_dispose` disposes the given `channel` and schedules all pending operations to be abandoned.

**SRS_CHANNEL_43_092: [** `channel_dispose` shall release the reference to `THANDLE(CHANNEL_INTERNAL)`. **]**

### channel_open
```c
    MOCKABLE_FUNCTION(, int, channel_open, THANDLE(CHANNEL), channel);
```

`channel_open` opens the given `channel`.

**SRS_CHANNEL_43_095: [** If `channel` is `NULL`, `channel_open` shall fail and return a non-zero value. **]**

**SRS_CHANNEL_43_096: [** `channel_open` shall call `channel_internal_open`. **]**

**SRS_CHANNEL_43_099: [** If there are any failures, `channel_open` shall fail and return a non-zero value. **]**

### channel_close
```c
    MOCKABLE_FUNCTION(, void, channel_close, THANDLE(CHANNEL), channel);
```

`channel_close` closes the given `channel`.

**SRS_CHANNEL_43_097: [** If `channel` is `NULL`, `channel_close` shall return immediately. **]**

**SRS_CHANNEL_43_098: [** `channel_close` shall call `channel_internal_close`. **]**


### channel_pull
```c
    MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_pull, THANDLE(CHANNEL), channel, THANDLE(RC_STRING), correlation_id, ON_DATA_AVAILABLE_CB, on_data_available_cb, void*, pull_context, THANDLE(ASYNC_OP)*, out_op_pull);
```

`channel_pull` registers the given `on_data_available_cb` to be called when there is data to be consumed.

**SRS_CHANNEL_43_007: [** If `channel` is `NULL`, `channel_pull` shall fail and return `CHANNEL_RESULT_INVALID_ARGS`. **]**

**SRS_CHANNEL_43_008: [** If `on_data_available_cb` is `NULL`, `channel_pull` shall fail and return `CHANNEL_RESULT_INVALID_ARGS`. **]**

**SRS_CHANNEL_43_009: [** If `out_op_pull` is `NULL`, `channel_pull` shall fail and return `CHANNEL_RESULT_INVALID_ARGS`. **]**

**SRS_CHANNEL_43_011: [** `channel_pull` shall call `channel_internal_pull` and return as it returns. **]**

### channel_push
```c
    MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_push, THANDLE(CHANNEL), channel, THANDLE(RC_STRING), correlation_id, THANDLE(RC_PTR), data, ON_DATA_CONSUMED_CB, on_data_consumed_cb, void*, push_context, THANDLE(ASYNC_OP)*, out_op_push);
```

`channel_push` notifies the channel that there is data available and registers the given `on_data_consumed_cb` to be called when the given `data` has been consumed.

**SRS_CHANNEL_43_024: [** If `channel` is `NULL`, `channel_push` shall fail and return `CHANNEL_RESULT_INVALID_ARGS`. **]**

**SRS_CHANNEL_43_025: [** If `on_data_consumed_cb` is `NULL`, `channel_push` shall fail and return `CHANNEL_RESULT_INVALID_ARGS`. **]**

**SRS_CHANNEL_43_026: [** If `out_op_push` is `NULL`, `channel_push` shall fail and return `CHANNEL_RESULT_INVALID_ARGS`. **]**

**SRS_CHANNEL_43_041: [** `channel_push` shall call `channel_internal_push` and return as it returns. **]**
