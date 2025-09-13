# `channel` requirements

## Overview

## Exposed API

```
#define SEALABLE_CHANNEL_RESULT_VALUES \
    SEALABLE_CHANNEL_RESULT_OK, \
    SEALABLE_CHANNEL_RESULT_INVALID_ARGS, \
    SEALABLE_CHANNEL_RESULT_ERROR, \
    SEALABLE_CHANNEL_RESULT_OVER_CAPACITY, \
    SEALABLE_CHANNEL_RESULT_SEALED

MU_DEFINE_ENUM(SEALABLE_CHANNEL_RESULT, SEALABLE_CHANNEL_RESULT_VALUES);

#define SEALABLE_CHANNEL_CALLBACK_RESULT_VALUES \
    SEALABLE_CHANNEL_CALLBACK_RESULT_OK, \
    SEALABLE_CHANNEL_CALLBACK_RESULT_CANCELLED, \
    SEALABLE_CHANNEL_CALLBACK_RESULT_ABANDONED

MU_DEFINE_ENUM(SEALABLE_CHANNEL_CALLBACK_RESULT, SEALABLE_CHANNEL_CALLBACK_RESULT_VALUES);

typedef void(*ON_SEALABLE_CHANNEL_DATA_AVAILABLE_CB)(void* pull_context, SEALABLE_CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id, THANDLE(RC_PTR) data);
typedef void(*ON_SEALABLE_CHANNEL_DATA_CONSUMED_CB)(void* push_context, SEALABLE_CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id);

typedef struct SEALABLE_CHANNEL_REPORTING_INFO_TAG
{
    uint32_t max_channel_item_count;
    size_t max_channel_size;
    uint32_t current_channel_item_count;
    size_t current_channel_size;
} SEALABLE_CHANNEL_REPORTING_INFO;

typedef struct SEALABLE_CHANNEL_TAG SEALABLE_CHANNEL;

THANDLE_TYPE_DECLARE(SEALABLE_CHANNEL);

    MOCKABLE_FUNCTION(, THANDLE(SEALABLE_CHANNEL), sealable_channel_create, THANDLE(PTR(LOG_CONTEXT_HANDLE)), log_context, THANDLE(THREADPOOL), threadpool, uint32_t, max_channel_item_count, size_t, max_channel_item_size);
    MOCKABLE_FUNCTION(, int, sealable_channel_open, THANDLE(SEALABLE_CHANNEL), sealable_channel);
    MOCKABLE_FUNCTION(, void, sealable_channel_close, THANDLE(SEALABLE_CHANNEL), sealable_channel);
    MOCKABLE_FUNCTION(, SEALABLE_CHANNEL_RESULT, sealable_channel_pull, THANDLE(SEALABLE_CHANNEL), sealable_channel, THANDLE(RC_STRING), correlation_id, ON_SEALABLE_CHANNEL_DATA_AVAILABLE_CB, on_data_available_cb, void*, pull_context);
    MOCKABLE_FUNCTION(, SEALABLE_CHANNEL_RESULT, sealable_channel_push, THANDLE(SEALABLE_CHANNEL), sealable_channel, THANDLE(RC_STRING), correlation_id, THANDLE(RC_PTR), data, ON_SEALABLE_CHANNEL_DATA_CONSUMED_CB, on_data_consumed_cb, void*, push_context);
    MOCKABLE_FUNCTION(, int, sealable_channel_get_reporting_info, THANDLE(SEALABLE_CHANNEL), sealable_channel, SEALABLE_CHANNEL_REPORTING_INFO*, reporting_info);
    MOCKABLE_FUNCTION(, int, sealable_channel_seal_channel, THANDLE(SEALABLE_CHANNEL), sealable_channel);
```

### sealable_channel_create

```c
THANDLE(SEALABLE_CHANNEL) sealable_channel_create(THANDLE(PTR(LOG_CONTEXT_HANDLE)) log_context, THANDLE(THREADPOOL) threadpool, uint32_t max_channel_item_count, size_t max_channel_item_size);
```

`sealable_channel_create` creates a new `SEALABLE_CHANNEL` instance.

If `log_context` is `NULL`, `sealable_channel_create` shall fail and return `NULL`.

If `threadpool` is `NULL`, `sealable_channel_create` shall fail and return `NULL`.

If `max_channel_item_count` is 0, `sealable_channel_create` shall fail and return `NULL`.

If `max_channel_item_size` is 0, `sealable_channel_create` shall fail and return `NULL`.

`sealable_channel_create` shall call `sm_create`.

`sealable_channel_create` shall call `channel_create`.

`sealable_channel_create` shall succeed and return the newly created `SEALABLE_CHANNEL`.

If any error occurs, `sealable_channel_create` shall fail and return `NULL`.

### sealable_channel_open

```c
int sealable_channel_open(THANDLE(SEALABLE_CHANNEL) sealable_channel);
```

`sealable_channel_open` opens a `sealable_channel`.

If `sealable_channel` is `NULL`, `sealable_channel_open` shall fail and return a non-zero value.

`sealable_channel_open` shall call `sm_open_begin`.

`sealable_channel_open` shall call `channel_open`.

`sealable_channel_open` shall call `sm_open_end`

`sealable_channel_open` shall succeed and return 0.

If any error occurs, `sealable_channel_open` shall fail and return a non-zero value.

### sealable_channel_close

```c
void sealable_channel_close(THANDLE(SEALABLE_CHANNEL) sealable_channel);
```

`sealable_channel_close` closes a `sealable_channel`.

If `sealable_channel` is `NULL`, `sealable_channel_close` shall return immediately.

`sealable_channel_close` shall call `sm_close_begin`.

`sealable_channel_close` shall call `channel_close`.

`sealable_channel_close` shall call `sm_close_end`.

`sealable_channel_close` shall succeed.

If any errors occur, `sealable_channel_close` shall fail.

### sealable_channel_pull

```c
SEALABLE_CHANNEL_RESULT sealable_channel_pull(THANDLE(SEALABLE_CHANNEL) sealable_channel, THANDLE(RC_STRING) correlation_id, ON_SEALABLE_CHANNEL_DATA_AVAILABLE_CB on_data_available_cb, void* pull_context);
```

`sealable_channel_pull` starts a pull operation on a `sealable_channel`. It returns `SEALABLE_CHANNEL_RESULT_SEALED` if the `sealable_channel` is sealed and no more operations are available.

If `sealable_channel` is `NULL`, `sealable_channel_pull` shall fail and return a non-zero value.

If `on_data_available_cb` is `NULL`, `sealable_channel_pull` shall fail and return a non-zero value.

`sealable_channel_pull` shall call `sm_exec_begin`.

`sealable_channel_pull` shall call `critical_section_enter`.

If the `sealable_channel` is sealed and `current_channel_item_count` is 0, `sealable_channel_pull` shall fail and return `SEALABLE_CHANNEL_RESULT_SEALED`.

`sealable_channel_pull` shall call `critical_section_leave`.

`sealable_channel_pull` shall allocate an `on_underlying_data_available_context`.

`sealable_channel_pull` shall call `channel_pull`, passing `on_underlying_data_available_callback` and `on_underlying_data_available_context`.

`sealable_channel_pull` shall call `sm_exec_end`.

`sealable_channel_pull` shall succeed and return 0.

If any errors occur, `sealable_channel_pull` shall fail and return a non-zero value.

### on_underlying_data_available_callback

```c
static void on_underlying_data_available_callback(void* pull_context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id, THANDLE(RC_PTR) data);
```

`on_underlying_data_available_callback` is called when the underlying `channel` has data available. It is responsible for recording count and size information for data leaving the channel.

If `pull_context` is `NULL`, `on_underlying_data_available_callback` shall terminate the process.

`on_underlying_data_available_callback` shall call `critical_section_enter`.

`on_underlying_data_available_callback` shall decrement the `current_channel_item_count`.

`on_underlying_data_available_callback` shall subtract `data->size` from the `current_channel_item_size`.

`on_underlying_data_available_callback` shall call `critical_section_leave`.

`on_underlying_data_available_callback` shall call the original `on_data_available_cb`.

`on_underlying_data_available_callback` shall deallocate the `on_underlying_data_available_context`.

### sealable_channel_push

```c
SEALABLE_CHANNEL_RESULT sealable_channel_push(THANDLE(SEALABLE_CHANNEL) sealable_channel, THANDLE(RC_STRING) correlation_id, THANDLE(RC_PTR) data, ON_SEALABLE_CHANNEL_DATA_CONSUMED_CB on_data_consumed_cb, void* push_context);
```

`sealable_channel_push` pushed data into the `sealable_channel`.
If the `sealable_channel` is sealed, it fails and returns `SEALABLE_CHANNEL_RESULT_SEALED`.
If the data would push the `sealable_channel` over capacity, it fails and returns `SEALABLE_CHANNEL_RESULT_OVER_CAPACITY`.
When this `sealable_channel_push` returns `SEALABLE_CHANNEL_RESULT_OVER_CAPACITY`, the caller is responsible for calling `sealable_channel_seal_channel`.

If `sealable_channel` is `NULL`, `sealable_channel_push` shall fail and return `SEALABLE_CHANNEL_RESULT_INVALID_ARGS`.

If `data` is `NULL`, `sealable_channel_push` shall fail and return `SEALABLE_CHANNEL_RESULT_INVALID_ARGS`.

If `on_data_consumed_cb` is `NULL`, `sealable_channel_push` shall fail and return a `SEALABLE_CHANNEL_RESULT_INVALID_ARGS`.

`sealable_channel_push` shall call `sm_exec_begin`.

If the `sealable_channel` is sealed, `sealable_channel_push` shall fail and return `SEALABLE_CHANNEL_RESULT_SEALED`.

`sealable_channel_push` shall call `critical_section_enter`.

If `data->size` + `current_channel_item_size` is greater than `max_channel_item_size`, `sealable_channel_push` shall fail and return `SEALABLE_CHANNEL_RESULT_OVER_CAPACITY`.

If `current_channel_item_count` + 1 is greater than `max_channel_item_count`, `sealable_channel_push` shall fail and return `SEALABLE_CHANNEL_RESULT_OVER_CAPACITY`.

`sealable_channel_push` shall add `data->size` to `current_channel_item_size`.

`sealable_channel_push` shall increment the `current_channel_item_count`.

`sealable_channel_push` shall call `critical_section_leave`.

`sealable_channel_push` shall allocate an `on_underlying_data_consumed_context".

`sealable_channel_push` shall call `channel_push`, passing `on_underling_data_consumed_callback`, and `on_underlying_data_consumed_context`

`sealable_channel_push` shall call `sm_exec_end`.

`sealable_channel_push` shall succeed and return `SEALABLE_CHANNEL_RESULT_OK`.

If any error occurs, `sealable_channel_push` shall undo changes to `current_channel_item_size` and `current_channel_item_count`.

If any error occurs, `sealable_channel_push` shall fail and return `SEALABLE_CHANNEL_RESULT_ERROR`.

### on_underlying_data_consumed

```c
static void on_underlying_data_consumed(void* push_context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id);
```

``on_underlying_data_consumed`` is called by the underlying `channel` when data is pulled from channel.
It exists primary to translate callback enums from `CHANNEL_CALLBACK_RESULT` to `SEALABLE_CHANNEL_CALLBACK_RESULT` values.

If `push_context` is NULL, `on_underlying_data_consumed` shall terminate the process.

`on_underlying_data_consumed` shall call the original `on_data_consumed_cb`.

`on_underlying_data_consumed` shall free the `on_underlying_data_consumed_context`.

### sealable_channel_get_reporting_info

```c
int sealable_channel_get_reporting_info(THANDLE(SEALABLE_CHANNEL) sealable_channel, SEALABLE_CHANNEL_REPORTING_INFO* reporting_info);
```

`sealable_channel_get_reporting_info` returns reporting information for the `sealable_channel`.

If `sealable_channel` is `NULL`, `sealable_channel_get_reporting_info` shall fail and return a non-zero value.

If `reporting_info` is `NULL`, `sealable_channel_get_reporting_info` shall fail and return a non-zero value.

`sealable_channel_get_reporting_info` shall copy information into the `reporting_info` structure.

`sealable_channel_get_reporting_info` shall succeed and return 0.

### sealable_channel_seal_channel

```c
int sealable_channel_seal_channel(THANDLE(SEALABLE_CHANNEL) sealable_channel);
```

`sealable_channel_seal_channel` seals the `sealable_channel`.

If `sealable_channel` is `NULL`, `sealable_channel_seal_channel` shall fail and return a non-zero value.

`sealable_channel_seal_channel` shall call `sm_exec_begin`

`sealable_channel_seal_channel` shall mark the `sealable_channel` as sealed.

`sealable_channel_seal_channel` shall call `sm_exec_end`.

`sealable_channel_seal_channel` shall succeed and return 0.

If any error occurs, `sealable_channel_seal_channel` shall fail and return a non-zero value.