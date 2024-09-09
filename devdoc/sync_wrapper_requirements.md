`sync_wrapper` requirements
============

## Overview

`sync_wrapper` is a set of macros that generate a wrapper function allowing to make a synchronous call to an asynchronous function and wait for the completion callback.

`sync_wrapper` passes the in args to the asynchronous function call that is wrapped.

`sync_wrapper` picks up the out args from the callback completion called as result of the asynchronous function call.

Notes:

- The conventions for what an asynchronous function that can be wrapped looks like are:

  - Second to last argument must be the callback function pointer.

  - Last argument must be the `void* context` that is passed into the completion callback as the first argument.

  - Completion callback must have the declaration:
  
  ```c
  void (*)(void*, OUT_ARGS)
  ```

  - `sync_wrapper` uses [`async_type_helpers`](async_type_helpers_requirements.md) to copy types.

## Exposed API

```c
#define SYNC_WRAPPER_RESULT_VALUES \
    SYNC_WRAPPER_OK, \
    SYNC_WRAPPER_INVALID_ARGS, \
    SYNC_WRAPPER_CALLBACK_COPY_ARG_ERROR, \
    SYNC_WRAPPER_CALL_ERROR, \
    SYNC_WRAPPER_OTHER_ERROR \

MU_DEFINE_ENUM(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_RESULT_VALUES)

#define DECLARE_SYNC_WRAPPER(async_handle_type, async_function_name, return_type, in_args, ...) \
    ...

#define DEFINE_SYNC_WRAPPER(async_handle_type, async_function_name, return_type, expected_return, in_args, ...) \
    ...

#define SYNC_WRAPPER(async_function_name) \
    ...
```

Example usage:

Given an asynchronous API:

```c
    typedef void (*ON_DO_SOMETHING_ASYNC_COMPLETE)(void* context, TEST_ASYNC_API_RESULT result, int callback_arg1, int callback_arg2);

    typedef struct TEST_ASYNC_TAG* TEST_ASYNC_HANDLE;

    int test_async_do_something_async(TEST_ASYNC_HANDLE test_async_api, int arg1, int arg2, ON_DO_SOMETHING_ASYNC_COMPLETE on_do_something_async_complete, void* context);
```

In the header file:

```c
DECLARE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_async, int, IN_ARGS(int, arg1, int, arg2), OUT_ARG(TEST_ASYNC_API_RESULT, result), OUT_ARG(int, callback_arg1), OUT_ARG(int, callback_arg2)));
```

In the C file:

```c
DEFINE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_async, int, 0, IN_ARGS(int, arg1, int, arg2), OUT_ARG(TEST_ASYNC_API_RESULT, result), OUT_ARG(int, callback_arg1), OUT_ARG(int, callback_arg2)));
```

### DECLARE_SYNC_WRAPPER

```c
#define DECLARE_SYNC_WRAPPER(async_handle_type, async_function_name, return_type, in_args, ...) \
    ...
```

`DECLARE_SYNC_WRAPPER` allows declaring the synchronous wrapper (typically used in the header file).

`in_args` is in the form of IN_ARGS(in_arg1_type, in_arg1_name, in_arg2_type, in_arg2_name, ...). Existence of `in_args` is mandatory; it shall be used as `IN_ARGS()` when `async_function_name` takes no input arguments.

`...` is in the form of OUT_ARG(out_arg1_type, out_arg1_name), OUT_ARG(out_arg2_type, out_arg2_name), ...).

Each out argument should only be declared with the `OUT_ARG`. (even when in `DEFINE_SYNC_WRAPPER` the argument is declared with using `OUT_ARG_EX`)

**SRS_SYNC_WRAPPER_01_001: [** `DECLARE_SYNC_WRAPPER` shall expand to the function declaration: **]**

```c
SYNC_WRAPPER_RESULT {async_function_name}_sync_wrapper(async_handle_type async_handle, ... in_args ..., return_type* async_function_call_result, ... out args ...);
```

Example (for the earlier asynchronous API):

```c
SYNC_WRAPPER_RESULT test_async_do_something_async_sync_wrapper(TEST_ASYNC_HANDLE async_handle, int arg1, int arg2, int* async_function_call_result, TEST_ASYNC_API_RESULT* result, int* callback_arg1, int* callback_arg2);
```

### DEFINE_SYNC_WRAPPER

```c
#define DEFINE_SYNC_WRAPPER(async_handle_type, async_function_name, return_type, expected_return, in_args, ...) \
    ...
```

`DEFINE_SYNC_WRAPPER` expands to the implementation of the synchronous wrapper and the associated completion callback that is passed to the asynchronous function that is wrapped.

Notes:

`in_args` is in the form of IN_ARGS(in_arg1_type, in_arg1_name, in_arg2_type, in_arg2_name, ...).

`...` is in the form of OUT_ARG(out_arg1_type, out_arg1_name), OUT_ARG(out_arg2_type, out_arg2_name), ...).

Each out argument can be declared with the `OUT_ARG` macro or `OUT_ARG_EX` macro.

**SRS_SYNC_WRAPPER_01_002: [** The generated synchronous wrapper shall have the declaration: **]**

```c
SYNC_WRAPPER_RESULT {async_function_name}_sync_wrapper(async_handle_type async_handle, ... in_args ..., return_type* async_function_call_result, ... out args ...);
```

**SRS_SYNC_WRAPPER_01_003: [** If `async_handle` is `NULL`, the synchronous wrapper shall fail and return `SYNC_WRAPPER_INVALID_ARGS`. **]**

**SRS_SYNC_WRAPPER_42_002: [** If `async_function_call_result` is `NULL` then the synchronous wrapper shall fail and return `SYNC_WRAPPER_INVALID_ARGS`. **]**

**SRS_SYNC_WRAPPER_01_004: [** If any of the out arguments pointers is `NULL`, the synchronous wrapper shall fail and return `SYNC_WRAPPER_INVALID_ARGS`. **]**

**SRS_SYNC_WRAPPER_01_006: [** The synchronous wrapper shall call the asynchronous function `async_function_name`, pass the in_args as arguments together with the generated completion function and a context used to store out argument pointers. **]**

**SRS_SYNC_WRAPPER_42_003: [** If the asynchronous function returns something other than `expected_return` then the synchronous wrapper shall return `SYNC_WRAPPER_CALL_ERROR`. **]**

**SRS_SYNC_WRAPPER_01_007: [** The synchronous wrapper shall wait for the callback to be finished by using `InterlockedHL_WaitForValue`. **]**

**SRS_SYNC_WRAPPER_01_008: [** On success, the synchronous wrapper shall return `SYNC_WRAPPER_OK`. **]**

**SRS_SYNC_WRAPPER_01_009: [** On success, the synchronous wrapper shall return in the out args the values of the arguments received in the callback. **]**

**SRS_SYNC_WRAPPER_42_001: [** The synchronous wrapper shall store the result of the asynchronous function call in `async_function_call_result`. **]**

**SRS_SYNC_WRAPPER_01_020: [** If the callback fails to copy any argument value, the synchronous wrapper shall return `SYNC_WRAPPER_CALLBACK_COPY_ARG_ERROR`. **]**

**SRS_SYNC_WRAPPER_01_010: [** If any other error occurs, the synchronous wrapper shall fail and return `SYNC_WRAPPER_OTHER_ERROR`. **]**

**SRS_SYNC_WRAPPER_01_011: [** `DEFINE_SYNC_WRAPPER` shall generate a callback to be passed to the asynchronous function with the following declaration: **]**

```c
static void on_{async_function_name}_complete(void* context, ... out args ...);
```

**SRS_SYNC_WRAPPER_01_012: [** If `context` is NULL, `on_{async_function_name}_complete` shall terminate the process. **]**

**SRS_SYNC_WRAPPER_01_013: [** Otherwise, `on_{async_function_name}_complete` shall store the values of the out args into the context created in synchronous wrapper function. **]**

**SRS_SYNC_WRAPPER_01_017: [** For each argument: **]**

**SRS_SYNC_WRAPPER_01_018: [** If `ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type}` is defined as 1, `on_{async_function_name}_complete` shall copy the argument value by assigning it. **]**

**SRS_SYNC_WRAPPER_01_019: [** If `ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type}` is not defined as 1, `on_{async_function_name}_complete` shall call a copy function with the following declaration: **]**

```c
int sync_wrapper_{arg_type}_copy(arg_type* dst, arg_type src)
```

**SRS_SYNC_WRAPPER_01_015: [** `on_{async_function_name}_complete` shall unblock the synchronous wrapper function call. **]**

### SYNC_WRAPPER

```c
#define SYNC_WRAPPER(async_function_name) \
    ...
```

**SRS_SYNC_WRAPPER_01_016: [** `SYNC_WRAPPER` shall expand `async_function_name` to the name of the synchronous wrapper around `async_function_name`. **]**

### OUT_ARG

```c
#define OUT_ARG(arg_type, arg_name) \
    ...
```

`OUT_ARG` allows declaring out arguments by specifying just the argument type and name.

**SRS_SYNC_WRAPPER_01_026: [** `OUT_ARG` shall be expanded to the appropriate argument wherever needed by using `arg_type` and `arg_name`. **]**

### OUT_ARG_EX

```c
#define OUT_ARG_EX(arg_type, arg_name, copy_function, free_function, ...) \
    ...
```

`OUT_ARG_EX` allows declaring out arguments by specifying the argument type and name, the copy and free functions and a list of arguments to be passed to the copy and free functions.

**SRS_SYNC_WRAPPER_01_025: [** `copy_function` shall be used as a function with the following declaration **]**:

```c
int copy_function(arg_type* dst, const arg_type src, ...)
```

where `...` are additional arguments expected to be passed to the copy function (for example the count of items in an array).

**SRS_SYNC_WRAPPER_01_027: [** `free_function` shall be used as a function with the following declaration **]**:

```c
void free_function(const arg_type arg, ...)
```

where `...` are additional arguments expected to be passed to the copy function (for example the count of items in an array).

### OUT_ARG Special Cases

By default, `OUT_ARG(T)` or `OUT_ARG_EX(T)` expands in the following ways:

The sync wrapper function makes the type `T` a pointer, as in:

```c
SYNC_WRAPPER_RESULT my_func_sync_wrapper(MY_HANDLE async_handle, int arg1, int* async_function_call_result, T* my_out_arg);
```

The callback context makes the type `T` a pointer, as in:

```c
typedef struct my_func_sync_wrapper_CONTEXT_TAG
{
    volatile_atomic int32_t complete;
    bool error_copying_out_args;
    T* my_out_arg;
} my_func_sync_wrapper_CONTEXT;
```

And the callback function is generated to take an argument of type `T`, as in:

```c
static void on_my_func_complete(void* context, T my_out_arg);
```

#### Callbacks that pass const

The `OUT_ARG` may be modified so that the callback is called with a const argument and expands like:

```c
static void on_my_func_complete(void* context, CONST_T my_out_arg);
```

This is done by defining `ASYNC_TYPE_HELPER_USE_CONST_TYPE_T` as `1` and adding a define of `ASYNC_TYPE_HELPER_CONST_TYPE_T` to some `CONST_T` type. This is useful to ensure the callback signature matches the expected type.

#### Out arguments that are already pointers

The `OUT_ARG` may be modified so that the type is not expanded to a pointer in the out argument of the sync wrapper or in the internal context, like:

```c
SYNC_WRAPPER_RESULT my_func_sync_wrapper(MY_HANDLE async_handle, int arg1, int* async_function_call_result, T my_out_arg);

typedef struct my_func_sync_wrapper_CONTEXT_TAG
{
    volatile_atomic int32_t complete;
    bool error_copying_out_args;
    T my_out_arg;
} my_func_sync_wrapper_CONTEXT;
```

For example, `T` may be a pointer type already with a custom copy function that does not need to allocate memory (so a pointer to a pointer is not needed).

This is done by defining `ASYNC_TYPE_HELPER_NO_POINTER_DECLARATION_T` as `1`.
