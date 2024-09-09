`async_retry_wrapper` requirements
============

## Overview

`async_retry_wrapper` is a set of macros that generate a wrapper function for calling an asynchronous function and retrying in case it returns specific enumeration values.

The retries can be triggered asynchronously by callback parameters with `RETRY_ON_ASYNC` or synchronously in a loop by return values with `RETRY_ON_SYNC`.

`async_retry_wrapper` does the following:

 - Allocate a context and copy the parameters passed to the async call (to be used for retries)
 - Call the async function
 - While the async function returns any of the configured enum values, call the async function again synchronously.
   - These synchronous retries use exponential back-off
   - Synchronous retries can be limited with the version of the wrapper that uses timeouts
 - Check the async function callback for any one of the retry configured enum values and if so, call the async function again from a different thread.
 - While the retry of the async function returns any of the configured enum values, call the async function again.
 - Call the original callback function when the call returns with a value that does not require a retry

When a retry is needed due to a callback parameter matching one in `RETRY_ON_ASYNC`, it is executed on a threadpool thread using `threadpool_schedule_work`.

When the function is called and returns a value from `RETRY_ON_SYNC`, then it is retried synchronously in a loop.

Notes:

- The conventions for what an asynchronous function that can be wrapped looks like are:

  - Second to last argument must be the callback function pointer.

  - Last argument must be the `void* context` that is passed into the completion callback as the first argument.

  - Completion callback must have the declaration:

  ```c
  void (*)(void*, OUT_ARGS)
  ```

  - `async_retry_wrapper` uses [`async_type_helpers`](async_type_helpers_requirements.md) to copy types.

This module currently does not support cancellation (though cancellation can generally be achieved by closing the underlying async handle). It also only supports timeouts in case of synchronous retries (retry based on the synchronous return value of the called function, not based on the callback result).

## Exposed API

```c
#define ASYNC_RETRY_WRAPPER_RESULT_VALUES \
    ASYNC_RETRY_WRAPPER_OK, \
    ASYNC_RETRY_WRAPPER_ERROR, \
    ASYNC_RETRY_WRAPPER_INVALID_ARGS, \
    ASYNC_RETRY_WRAPPER_COPY_ARG_ERROR, \
    ASYNC_RETRY_WRAPPER_CALL_ERROR, \
    ASYNC_RETRY_WRAPPER_TIMEOUT \

MU_DEFINE_ENUM(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_RESULT_VALUES)

#define ASYNC_RETRY_WRAPPER(async_function_name) \
    ...

#define DECLARE_ASYNC_RETRY_WRAPPER(async_handle_type, async_function_name, return_type, expected_return, in_args, out_args, retry_async_enums, retry_sync_enums) \
    ...

#define DEFINE_ASYNC_RETRY_WRAPPER(async_handle_type, async_function_name, return_type, expected_return, in_args, out_args, retry_async_enums, retry_sync_enums) \
    ...

```

Example usage:

Given an asynchronous API:

```c
typedef enum TEST_ASYNC_API_RESULT_TAG
{
    TEST_ASYNC_API_1,
    TEST_ASYNC_API_RETRY_1,
    TEST_ASYNC_API_RETRY_2,
    TEST_ASYNC_API_ERROR
} TEST_ASYNC_API_RESULT;

typedef void (*ON_DO_SOMETHING_ASYNC_COMPLETE)(void* context, TEST_ASYNC_API_RESULT result, int callback_arg1, int callback_arg2);

typedef struct TEST_ASYNC_TAG* TEST_ASYNC_HANDLE;

int test_async_do_something_async(TEST_ASYNC_HANDLE test_async_api, int arg1, int arg2, ON_DO_SOMETHING_ASYNC_COMPLETE on_do_something_async_complete, void* context);
```

Assume that the function contract says that a value of `TEST_ASYNC_API_RETRY_1` or `TEST_ASYNC_API_RETRY_2` indicates a transient state and the caller may retry the function until another value is reported.

In the header file:

```c
DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_async, int,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void*, context)));
```

In the C file:

```c
DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_async, int, 0,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void*, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_RESULT, result, TEST_ASYNC_API_ERROR, TEST_ASYNC_API_TIMEOUT_ERROR), ARG(int, callback_arg1, 0), ARG(int, callback_arg2, -1)),
    RETRY_ON_ASYNC(TEST_ASYNC_API_RETRY_1, TEST_ASYNC_API_RETRY_2), RETRY_ON_SYNC());
```

### ASYNC_RETRY_WRAPPER

```c
#define ASYNC_RETRY_WRAPPER(async_function_name) \
    ...
```

**SRS_ASYNC_RETRY_WRAPPER_42_015: [** `ASYNC_RETRY_WRAPPER` shall expand `async_function_name` to the name of the asynchronous retry wrapper around `async_function_name`. **]**

### DECLARE_ASYNC_RETRY_WRAPPER

```c
#define DECLARE_ASYNC_RETRY_WRAPPER(async_handle_type, async_function_name, return_type, in_args) \
    ...
```

`DECLARE_ASYNC_RETRY_WRAPPER` allows declaring the asynchronous retry wrapper (typically used in the header file).

`in_args` is in the form of `IN_ARGS(arg_1, arg_2, ..., arg_N)`. Existence of `in_args` is mandatory, and a function must have at least a callback and callback context argument.

Within `in_args`, each `arg_N` takes one of the following forms:
 - `ARG(type, name)` : default for most arguments.
 - `ARG_EX(type, name, copy_function, free_function, ...)` : define a custom `copy_function` and `free_function` along with a variable number of arguments to pass to those functions (which must come from other `in_args`).
 - `ARG_CB(type, name)` : The callback function which will be called when the asynchronous function completes with a value that does not require a retry. This must be specified exactly once in `IN_ARGS(...)`.
 - `ARG_CONTEXT(type, name)` : The context to pass as the first argument to the callback function. This must be specified exactly once in `IN_ARGS(...)`.

**SRS_ASYNC_RETRY_WRAPPER_42_001: [** `DECLARE_ASYNC_RETRY_WRAPPER` shall expand to the function declaration: **]**

```c
MOCKABLE_FUNCTION(, ASYNC_RETRY_WRAPPER_RESULT {async_function_name}_async_retry_wrapper, async_handle_type, async_handle, THANDLE(THREADPOOL), threadpool, ... in_args ..., return_type*, async_function_result);
```

Example (for the earlier asynchronous API):

```c
MOCKABLE_FUNCTION(, ASYNC_RETRY_WRAPPER_RESULT, test_async_do_something_async_async_retry_wrapper, TEST_ASYNC_HANDLE, async_handle, THANDLE(THREADPOOL), threadpool, int, arg1, int, arg2, ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete, void*, context, int*, async_function_result);
```

**SRS_ASYNC_RETRY_WRAPPER_42_046: [** `DECLARE_ASYNC_RETRY_WRAPPER` shall also expand to the function declaration: **]**

```c
MOCKABLE_FUNCTION(, ASYNC_RETRY_WRAPPER_RESULT {async_function_name}_async_retry_wrapper_with_timeout, async_handle_type, async_handle, THANDLE(THREADPOOL), threadpool, uint32_t, timeout_ms, ... in_args ..., return_type*, async_function_result);
```

Note that this function is only supported for retry wrappers that use synchronous retries. It will not have a definition for wrappers that use asynchronous retries. This is a current limitation of the async retry wrapper.

### DEFINE_ASYNC_RETRY_WRAPPER

```c
#define DEFINE_ASYNC_RETRY_WRAPPER(async_handle_type, async_function_name, return_type, expected_return, in_args, out_args, retry_async_enums, retry_sync_enums) \
    ...
```

`DEFINE_ASYNC_RETRY_WRAPPER` expands to the implementation of the asynchronous retry wrapper, the callback handler, function to initiate a retry, and the context used by the callback.

The parameters to this macro are described above in addition to the following:

`out_args` is in the form of `OUT_ARGS(arg_1, arg_2, ..., arg_N)`. Existence of `out_args` is mandatory and a function must have at most one enum value specified here that is checked to determine when a retry is needed.

Within `out_args`, each `arg_N` takes one of the following forms:
 - `ARG(type, name, error_value)` : default case, where `error_value` is the value to return to the caller in case the retry fails and the callback must be called directly by this wrapper.
 - `ENUM(type, name, error_value [, timeout_error_value])` : This is the one enum value that is checked for the retry values to determine if a retry is needed. This must be specified at most once in `OUT_ARGS(...)`. It is mandatory if `RETRY_ON_ASYNC(...)` is not empty. `timeout_error_value` will be used as a callback argument when the time for retries is exceeded.  `timeout_error_value` is optional. When `timeout_error_value` doesn't exist then `error_value` will be used instead.

`retry_async_enums` is in the form `RETRY_ON_ASYNC(value_1, value_2, ... value_N)`. It will be checked against the `ENUM(...)` in the `OUT_ARGS(...)`.

`retry_sync_enums` is in the form `RETRY_ON_SYNC(value_1, value_2, ... value_N)`. It will be checked against the return value when the function is called.

Note: the order of `retry_async_enums` and `retry_sync_enums` is: `retry_async_enums` first followed by `retry_sync_enums`.

**SRS_ASYNC_RETRY_WRAPPER_42_002: [** `DEFINE_ASYNC_RETRY_WRAPPER` shall generate an asynchronous retry wrapper with the declaration: **]**

```c
ASYNC_RETRY_WRAPPER_RESULT {async_function_name}_async_retry_wrapper(async_handle_type async_handle, THANDLE(THREADPOOL) threadpool, ... in_args ..., int*, async_function_result);
```

**SRS_ASYNC_RETRY_WRAPPER_42_003: [** If `async_handle` is `NULL`, the asynchronous retry wrapper shall fail and return `ASYNC_RETRY_WRAPPER_INVALID_ARGS`.  **]**

**SRS_ASYNC_RETRY_WRAPPER_42_042: [** If `async_function_result` is `NULL`, the asynchronous retry wrapper shall fail and return `ASYNC_RETRY_WRAPPER_INVALID_ARGS`. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_037: [** If `threadpool` is `NULL`, the asynchronous retry wrapper shall fail and return `ASYNC_RETRY_WRAPPER_INVALID_ARGS`.  **]**

**SRS_ASYNC_RETRY_WRAPPER_42_004: [** If the parameter specified in `ARG_CB(type, name)` is `NULL`, the asynchronous retry wrapper shall fail and return `ASYNC_RETRY_WRAPPER_INVALID_ARGS`. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_005: [** The asynchronous retry wrapper shall allocate a context for the asynchronous call. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_006: [** The asynchronous retry wrapper shall copy each argument from `IN_ARGS(...)` to the allocated context. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_007: [** If an argument in `IN_ARGS(...)` is specified with `ARG_EX(type, name, copy_function, free_function, ...)` then the asynchronous retry wrapper shall call `copy_function` and pass the `...` arguments in order to copy to the context. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_008: [** If an argument in `IN_ARGS(...)` has a type that does not define `ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type}` as `1`, then the asynchronous retry wrapper shall copy the argument by calling `async_retry_wrapper_{type}_copy`. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_009: [** Otherwise, the asynchronous retry wrapper shall copy the argument to the context via assignment. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_010: [** If there are any failures when copying the input arguments then the asynchronous retry wrapper shall fail and return `ASYNC_RETRY_WRAPPER_COPY_ARG_ERROR`. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_011: [** The asynchronous retry wrapper shall call the function `async_function_name`, passing the `in_args` that have been copied to the retry context with the exception of `ARG_CB(...)` and `ARG_CONTEXT(...)` which are instead passed as the generated callback handler and the allocated context. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_038: [** While `async_function_name` returns one of the values from `RETRY_ON_SYNC(...)`, it shall be called again in a loop. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_044: [** Before each retry of the function, the asynchronous retry wrapper shall yield execution by a call to `ThreadAPI_Sleep`. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_043: [** The asynchronous retry wrapper shall store the result of `async_function_name` in `async_function_result`. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_039: [** If `async_function_name` returns a value other than `expected_return` then the asynchronous retry wrapper shall fail and return `ASYNC_RETRY_WRAPPER_CALL_ERROR`. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_013: [** On success, the asynchronous retry wrapper shall return `ASYNC_RETRY_WRAPPER_OK`. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_047: [** `DEFINE_ASYNC_RETRY_WRAPPER` shall generate an asynchronous retry wrapper that supports timeouts with the declaration: **]**

```c
ASYNC_RETRY_WRAPPER_RESULT {async_function_name}_async_retry_wrapper_with_timeout(async_handle_type async_handle, THANDLE(THREADPOOL) threadpool, uint32_t timeout_ms, ... in_args ..., int*, async_function_result);
```

**SRS_ASYNC_RETRY_WRAPPER_42_048: [** `{async_function_name}_async_retry_wrapper_with_timeout` shall get the current time by calling `timer_global_get_elapsed_ms`. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_049: [** Before each retry of the function, If `timeout_ms` milliseconds have elapsed then `{async_function_name}_async_retry_wrapper_with_timeout` shall fail and return `ASYNC_RETRY_WRAPPER_TIMEOUT`. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_050: [** `{async_function_name}_async_retry_wrapper_with_timeout` shall otherwise behave the same as `{async_function_name}_async_retry_wrapper`. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_014: [** `DEFINE_ASYNC_RETRY_WRAPPER` shall generate a callback to be passed to the asynchronous function with the following declaration: **]**

```c
static void on_{async_function_name}_complete(void* context, ... out args ...);
```

**SRS_ASYNC_RETRY_WRAPPER_42_016: [** If `context` is `NULL`, `on_{async_function_name}_complete` shall terminate the process. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_017: [** If the `out_arg` specified as `ENUM(type, name, error_value)` has one of the values from `RETRY_ON_ASYNC(...)` then `on_{async_function_name}_complete` shall call `threadpool_schedule_work` with `{async_function_name}_do_retry` as the `work_function` to retry the asynchronous call and return. **]**

**SRS_ASYNC_RETRY_WRAPPER_02_003: [** If the `out_arg` specified as `ENUM(type, name, error_value)` has one of the values from `RETRY_ON_ASYNC(...)` and the the time measured from the initial call to `ASYNC_RETRY_WRAPPER()` exceeded `timeout_ms` then the user callback  specified in `ARG_CB(...)` shall be called, passing the context from `ARG_CONTEXT(...)`, and the `out_args` as they were received by this callback handler with the exception of the ENUM(...) which will have the value specified for `timeout_error_value`. **]**

**SRS_ASYNC_RETRY_WRAPPER_02_004: [** If there's no timeout_error_value specified by ENUM(...) macro then the `error_value` shall be used instead. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_018: [** `on_{async_function_name}_complete` shall call the user callback specified in `ARG_CB(...)`, passing the context from `ARG_CONTEXT(...)`, and the `out_args` as they were received by this callback handler. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_019: [** `on_{async_function_name}_complete` shall free each argument in `IN_ARGS(...)` that is specified with `ARG_EX(type, name, copy_function, free_function, ...)` by calling `free_function` and passing the `...` arguments. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_020: [** `on_{async_function_name}_complete` shall free each argument in `IN_ARGS(...)` that has a type that does not define `ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type}` as `1` by calling `async_retry_wrapper_{type}_free`. **]**

**SRS_ASYNC_RETRY_WRAPPER_11_001: [** `on_{async_function_name}_complete` shall assign the threadpool to `NULL`. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_021: [** `on_{async_function_name}_complete` shall free the allocated context. **]**

**SRS_ASYNC_RETRY_WRAPPER_01_001: [** If any error occurs, `on_{async_function_name}_complete` shall call the user callback specified in `ARG_CB(...)`, passing the context from `ARG_CONTEXT(...)`, and the `error_value` from all of the `ARG(type, name, error_value)`'s in `out_args`. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_022: [** `DEFINE_ASYNC_RETRY_WRAPPER` shall generate a function to retry the asynchronous function with the following declaration: **]**

```c
static void {async_function_name}_do_retry(void* context);
```

**SRS_ASYNC_RETRY_WRAPPER_42_023: [** `{async_function_name}_do_retry` shall call the function `async_function_name`, passing the `in_args` that have been copied to the retry context with the exception of `ARG_CB(...)` and `ARG_CONTEXT(...)` which are instead passed as the generated callback handler and the allocated context. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_040: [** While `async_function_name` returns one of the values from `RETRY_ON_SYNC(...)`, it shall be called again in a loop. **]**

**SRS_ASYNC_RETRY_WRAPPER_02_001: [** Before each retry of the function, if `timeout_ms` milliseconds have elapsed since the initial call to `ASYNC_RETRY_WRAPPER`(`async_function_name`) then `{async_function_name}_async_retry_wrapper_with_timeout` shall call shall call the user callback specified in `ARG_CB(...)`, passing the context from `ARG_CONTEXT(...)`, the `error_value` from all of the `ARG(type, name, error_value)`'s in `out_args`. and the `timeout_error_value` for the ENUM(...) argument. **]**

**SRS_ASYNC_RETRY_WRAPPER_02_002: [** If there's no `timeout_error_value` for ENUM(...) argument then the `error_value` shall be instead passed. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_045: [** Before each retry of the function, `{async_function_name}_do_retry` shall yield execution by a call to `ThreadAPI_Sleep`. **]**

**SRS_ASYNC_RETRY_WRAPPER_42_041: [** If `async_function_name` returns a value other than `expected_return` then: **]**

 - **SRS_ASYNC_RETRY_WRAPPER_42_025: [** `{async_function_name}_do_retry` shall call the user callback specified in `ARG_CB(...)`, passing the context from `ARG_CONTEXT(...)`, and the `error_value` from all of the `ARG(type, name, error_value)`'s in `out_args`. **]**

 - **SRS_ASYNC_RETRY_WRAPPER_42_026: [** `{async_function_name}_do_retry` shall free each argument in `IN_ARGS(...)` that is specified with `ARG_EX(type, name, copy_function, free_function, ...)` by calling `free_function` and passing the `...` arguments. **]**

 - **SRS_ASYNC_RETRY_WRAPPER_42_027: [** `{async_function_name}_do_retry` shall free each argument in `IN_ARGS(...)` that has a type that does not define `ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type}` as `1` by calling `async_retry_wrapper_{type}_free`. **]**

 - **SRS_ASYNC_RETRY_WRAPPER_42_028: [** `{async_function_name}_do_retry` shall free the allocated context. **]**

### IN_ARG_EX

```c
#define IN_ARG_EX(arg_type, arg_name, copy_function, free_function, ...) \
    ...
```

`IN_ARG_EX` allows declaring input arguments by specifying the argument type and name, the copy and free functions and a list of arguments to be passed to the copy and free functions.

**SRS_ASYNC_RETRY_WRAPPER_42_035: [** `copy_function` shall be used as a function with the following declaration: **]**

```c
int copy_function(arg_type_non_const* dst, const arg_type src, ...)
```

where `...` are additional arguments expected to be passed to the copy function (for example the count of items in an array).

**SRS_ASYNC_RETRY_WRAPPER_42_036: [** `free_function` shall be used as a function with the following declaration: **]**

```c
void free_function(const arg_type arg, ...)
```

where `...` are additional arguments expected to be passed to the free function (for example the count of items in an array).
