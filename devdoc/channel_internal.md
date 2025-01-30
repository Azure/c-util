# `channel_internal` requirements

## Overview

`channel_internal` is a module that contains the implementation for the `channel` module.

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

typedef void(*PULL_CALLBACK)(void* pull_context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_PTR) data);
typedef void(*PUSH_CALLBACK)(void* push_context, CHANNEL_CALLBACK_RESULT result);
```

`channel_internal.h`

```
THANDLE_TYPE_DECLARE(CHANNEL_INTERNAL);

MOCKABLE_FUNCTION(, THANDLE(CHANNEL_INTERNAL), channel_internal_create, THANDLE(PTR(LOG_CONTEXT_HANDLE)), log_context, THANDLE(THREADPOOL), threadpool);
MOCKABLE_FUNCTION(, int, channel_internal_open, THANDLE(CHANNEL_INTERNAL), channel_internal);
MOCKABLE_FUNCTION(, void, channel_internal_close, THANDLE(CHANNEL_INTERNAL), channel_internal);
MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_internal_pull, THANDLE(CHANNEL_INTERNAL), channel_internal, THANDLE(RC_STRING), correlation_id, PULL_CALLBACK, pull_callback, void*, pull_context, THANDLE(ASYNC_OP)*, out_op_pull);
MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_internal_push, THANDLE(CHANNEL_INTERNAL), channel_internal, THANDLE(RC_STRING), correlation_id, THANDLE(RC_PTR), data, PUSH_CALLBACK, push_callback, void*, push_context, THANDLE(ASYNC_OP)*, out_op_push);
```

### channel_internal_create
```c
    MOCKABLE_FUNCTION(, THANDLE(CHANNEL_INTERNAL), channel_internal_create, THANDLE(PTR(LOG_CONTEXT_HANDLE)), log_context, THANDLE(THREADPOOL), threadpool);
```

`channel_internal_create` creates the channel_internal and returns it.

**SRS_CHANNEL_INTERNAL_43_151: [** `channel_internal_create` shall call `sm_create`. **]**

**SRS_CHANNEL_INTERNAL_43_098: [** `channel_internal_create` shall call `srw_lock_create`. **]**

**SRS_CHANNEL_INTERNAL_43_078: [** `channel_internal_create` shall create a `CHANNEL_INTERNAL` object by calling `THANDLE_MALLOC` with `channel_internal_dispose` as `dispose`.**]**

**SRS_CHANNEL_INTERNAL_43_080: [** `channel_internal_create` shall store given `threadpool` in the created `CHANNEL_INTERNAL`. **]**

**SRS_CHANNEL_INTERNAL_43_149: [** `channel_internal_create` shall store the given `log_context` in the created `CHANNEL_INTERNAL`. **]**

**SRS_CHANNEL_INTERNAL_43_084: [** `channel_internal_create` shall call `DList_InitializeListHead`. **]**

**SRS_CHANNEL_INTERNAL_43_086: [** `channel_internal_create` shall succeed and return the created `THANDLE(CHANNEL_INTERNAL)`. **]**

**SRS_CHANNEL_INTERNAL_43_002: [** If there are any failures, `channel_internal_create` shall fail and return `NULL`. **]**


### channel_internal_open
```c
    MOCKABLE_FUNCTION(, int, channel_internal_open, THANDLE(CHANNEL_INTERNAL), channel_internal);
```

channel_internal_open` opens the given `channel_internal`.

**SRS_CHANNEL_INTERNAL_43_159: [** `channel_internal_open` shall call `sm_open_begin`. **]**

**SRS_CHANNEL_INTERNAL_43_160: [** `channel_internal_open` shall call `sm_open_end`. **]**

**SRS_CHANNEL_INTERNAL_43_161: [** If there are any failures, `channel_internal_open` shall fail and return a non-zero value. **]**

**SRS_CHANNEL_INTERNAL_43_162: [** `channel_internal_open` shall succeed and return 0. **]**


### channel_internal_close
```c
    static void channel_internal_close(THANDLE(CHANNEL_INTERNAL) channel_internal);
```

`channel_internal_close` schedules all pending operations to be abandoned.

**SRS_CHANNEL_INTERNAL_43_094: [** `channel_internal_close` shall call `sm_close_begin_with_cb` with `abandon_pending_operations` as the callback. **]**

**SRS_CHANNEL_INTERNAL_43_095: [** `abandon_pending_operations` shall iterate over the list of pending operations and do the following: **]**

 - **SRS_CHANNEL_INTERNAL_43_096: [** set the `result` of the `operation` to `CHANNEL_CALLBACK_RESULT_ABANDONED`. **]**

 - **SRS_CHANNEL_INTERNAL_43_097: [** call `threadpool_schedule_work` with `execute_callbacks` as `work_function`. **]**

**SRS_CHANNEL_INTERNAL_43_100: [** `channel_internal_close` shall call `sm_close_end`. **]**


### channel_internal_dispose
```c
    static void channel_internal_dispose(CHANNEL_INTERNAL* channel_internal);
```

`channel_internal_dispose` disposes the given `channel_internal`.


**SRS_CHANNEL_INTERNAL_43_150: [** `channel_internal_dispose` shall release the reference to the `log_context` **]**

**SRS_CHANNEL_INTERNAL_43_091: [** `channel_internal_dispose` shall release the reference to `THANDLE(THREADPOOL)`. **]**

**SRS_CHANNEL_INTERNAL_43_099: [** `channel_internal_dispose` shall call `srw_lock_destroy`. **]**

**SRS_CHANNEL_INTERNAL_43_165: [** `channel_internal_dispose` shall call `sm_destroy`. **]**

### channel_internal_pull
```c
    MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_internal_pull, THANDLE(CHANNEL_INTERNAL), channel_internal, THANDLE(RC_STRING), correlation_id, PULL_CALLBACK, pull_callback, void*, pull_context, THANDLE(ASYNC_OP)*, out_op_pull);
```

`channel_internal_pull` registers the given `pull_callback` to be called when there is data to be consumed.

**SRS_CHANNEL_INTERNAL_43_152: [** `channel_internal_pull` shall call `sm_exec_begin`. **]**

**SRS_CHANNEL_INTERNAL_43_010: [** `channel_internal_pull` shall call `srw_lock_acquire_exclusive`. **]**

**SRS_CHANNEL_INTERNAL_43_101: [** If the list of pending operations is empty or the first operation in the list of pending operations contains a `non-NULL` `pull_callback`: **]**

 - **SRS_CHANNEL_INTERNAL_43_103: [** `channel_internal_pull` shall create a `THANDLE(ASYNC_OP)` by calling `async_op_create` with `cancel_op` as `cancel`. **]**

 - **SRS_CHANNEL_INTERNAL_43_104: [** `channel_internal_pull` shall store the `correlation_id`, `pull_callback` and `pull_context` in the `THANDLE(ASYNC_OP)`. **]**

 - **SRS_CHANNEL_INTERNAL_43_111: [** `channel_internal_pull` shall set the `result` of the created `operation` to `CHANNEL_CALLBACK_RESULT_OK`. **]**

 - **SRS_CHANNEL_INTERNAL_43_105: [** `channel_internal_pull` shall insert the created `THANDLE(ASYNC_OP)` in the list of pending operations by calling `DList_InsertTailList`. **]**

 - **SRS_CHANNEL_INTERNAL_43_107: [** `channel_internal_pull` shall set `*out_op_pull` to the created `THANDLE(ASYNC_OP)`. **]**

**SRS_CHANNEL_INTERNAL_43_108: [** If the first operation in the list of pending operations contains a `non-NULL` `push_callback`: **]**

 - **SRS_CHANNEL_INTERNAL_43_109: [** `channel_internal_pull` shall call `DList_RemoveHeadList` on the list of pending operations to obtain the `operation`. **]**

 - **SRS_CHANNEL_INTERNAL_43_112: [** `channel_internal_pull` shall store the `correlation_id`, `pull_callback` and `pull_context` in the obtained `operation`. **]**

 - **SRS_CHANNEL_INTERNAL_43_113: [** `channel_internal_pull` shall call `threadpool_schedule_work` with `execute_callbacks` as `work_function` and the obtained `operation` as `work_function_context`. **]**

 - **SRS_CHANNEL_INTERNAL_43_114: [** `channel_internal_pull` shall set `*out_op_pull` to the `THANDLE(ASYNC_OP)` of the obtained `operation`. **]**

**SRS_CHANNEL_INTERNAL_43_115: [** `channel_internal_pull` shall call `srw_lock_release_exclusive`. **]**

**SRS_CHANNEL_INTERNAL_43_011: [** `channel_internal_pull` shall succeeds and return `CHANNEL_RESULT_OK`. **]**

**SRS_CHANNEL_INTERNAL_43_023: [** If there are any failures, `channel_internal_pull` shall fail and return `CHANNEL_RESULT_ERROR`. **]**


### channel_internal_push
```c
    MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_internal_push, THANDLE(CHANNEL_INTERNAL), channel_internal, THANDLE(RC_STRING), correlation_id, THANDLE(RC_PTR), data, PUSH_CALLBACK, push_callback, void*, push_context, THANDLE(ASYNC_OP)*, out_op_push);
```

`channel_internal_push` notifies the channel_internal that there is data available and registers the given `push_callback` to be called when the given `data` has been consumed.

**SRS_CHANNEL_INTERNAL_43_153: [** `channel_internal_push` shall call `sm_exec_begin`. **]**

**SRS_CHANNEL_INTERNAL_43_116: [** `channel_internal_push` shall call `srw_lock_acquire_exclusive`. **]**

**SRS_CHANNEL_INTERNAL_43_117: [** If the list of pending operations is empty or the first operation in the list of pending operations contains a `non-NULL` `push_callback`: **]**

 - **SRS_CHANNEL_INTERNAL_43_119: [** `channel_internal_push` shall create a `THANDLE(ASYNC_OP)` by calling `async_op_create` with `cancel_op` as `cancel`. **]**

 - **SRS_CHANNEL_INTERNAL_43_120: [** `channel_internal_push` shall store the `correlation_id`, `push_callback`, `push_context` and `data` in the `THANDLE(ASYNC_OP)`. **]**

 - **SRS_CHANNEL_INTERNAL_43_127: [** `channel_internal_push` shall set the `result` of the created `operation` to `CHANNEL_CALLBACK_RESULT_OK`. **]**

 - **SRS_CHANNEL_INTERNAL_43_121: [** `channel_internal_push` shall insert the created `THANDLE(ASYNC_OP)` in the list of pending operations by calling `DList_InsertTailList`. **]**

 - **SRS_CHANNEL_INTERNAL_43_123: [** `channel_internal_push` shall set `*out_op_push` to the created `THANDLE(ASYNC_OP)`. **]**

**SRS_CHANNEL_INTERNAL_43_124: [** Otherwise (the first operation in the list of pending operations contains a `non-NULL` `pull_callback`): **]**

 - **SRS_CHANNEL_INTERNAL_43_125: [** `channel_internal_push` shall call `DList_RemoveHeadList` on the list of pending operations to obtain the `operation`. **]**

 - **SRS_CHANNEL_INTERNAL_43_128: [** `channel_internal_push` shall store the `correlation_id`, `push_callback`, `push_context` and `data` in the obtained `operation`. **]**

 - **SRS_CHANNEL_INTERNAL_43_129: [** `channel_internal_push` shall call `threadpool_schedule_work` with `execute_callbacks` as `work_function` and the obtained `operation` as `work_function_context`. **]**

 - **SRS_CHANNEL_INTERNAL_43_130: [** `channel_internal_push` shall set `*out_op_push` to the `THANDLE(ASYNC_OP)` of the obtained `operation`. **]**

**SRS_CHANNEL_INTERNAL_43_131: [** `channel_internal_push` shall call `srw_lock_release_exclusive`. **]**

**SRS_CHANNEL_INTERNAL_43_132: [** `channel_internal_push` shall succeed and return `CHANNEL_RESULT_OK`. **]**

**SRS_CHANNEL_INTERNAL_43_041: [** If there are any failures, `channel_internal_push` shall fail and return `CHANNEL_RESULT_ERROR`. **]**


### cancel_op
```c
    static void cancel_op(void* channel_internal_op_context);
```

`cancel_op` is the cancel callback that is passed to `async_op_create` when creating a `THANDLE(ASYNC_OP)` for a `channel_internal_push` or `channel_internal_pull` operation.

**SRS_CHANNEL_INTERNAL_43_154: [** `cancel_op` shall call `sm_exec_begin`. **]**

**SRS_CHANNEL_INTERNAL_43_134: [** `cancel_op` shall call `srw_lock_acquire_exclusive`. **]**

**SRS_CHANNEL_INTERNAL_43_135: [** If the `operation` is in the list of pending `operations`, `cancel_op` shall call `DList_RemoveEntryList` to remove it. **]**

**SRS_CHANNEL_INTERNAL_43_139: [** `cancel_op` shall call `srw_lock_release_exclusive`. **]**

**SRS_CHANNEL_INTERNAL_43_136: [** If the `result` of the `operation` is `CHANNEL_CALLBACK_RESULT_OK`, `cancel_op` shall set it to `CHANNEL_CALLBACK_RESULT_CANCELLED`. **]**

**SRS_CHANNEL_INTERNAL_43_138: [** If the `operation` had been found in the list of pending `operations`, `cancel_op` shall call `threadpool_schedule_work` with `execute_callbacks` as `work_function` and the `operation` as `work_function_context`. **]**

**SRS_CHANNEL_INTERNAL_43_156: [** `cancel_op` shall call `sm_exec_end`. **]**

**SRS_CHANNEL_INTERNAL_43_155: [** If there are any failures, `cancel_op` shall fail. **]**


### execute_callbacks
```c
    static void execute_callbacks(void* channel_internal_op_context);
```

`execute_callbacks` is the work function that is passed to `threadpool_schedule_work` when scheduling the execution of the callbacks for an operation.

**SRS_CHANNEL_INTERNAL_43_148: [** If `channel_internal_op_context` is `NULL`, `execute_callbacks` shall terminate the process. **]**

**SRS_CHANNEL_INTERNAL_43_145: [** `execute_callbacks` shall call the stored callback(s) with the `result` of the `operation`.  **]**

**SRS_CHANNEL_INTERNAL_43_157: [** `execute_callbacks` shall call `sm_exec_end` for each callback that is called. **]**

**SRS_CHANNEL_INTERNAL_43_147: [** `execute_callbacks` shall perform cleanup of the `operation`. **]**
