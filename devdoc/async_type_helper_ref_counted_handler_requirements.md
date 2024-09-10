`async_type_helper_ref_counted_handler` requirements
============

## Overview

`async_type_helper_ref_counted_handler` is a helper module that allows generating async wrapper handlers for a reference counted object.

The reference counting is assumed as below:

Incrementing the reference count is assumed to be done by a function with the signature:

```c
void {inc_ref}({HANDLE_TYPE} handle)
```

Decrementing the reference count is assumed to be done by a function with the signature:

```c
void {inc_ref}({HANDLE_TYPE} handle)
```

## Exposed API

```c
#define DECLARE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER(REF_COUNTED_HANDLE_TYPE) \
    ...

#define DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER(REF_COUNTED_HANDLE_TYPE, inc_ref_function, dec_ref_function) \
    ...
```

Example usage:

Given a ref counted handle `MY_REF_COUNTED_HANDLE` with the inc ref function `my_ref_counted_inc_ref` and dec ref function `my_ref_counted_dec_ref`:

In the header file:

```c
DECLARE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER(MY_REF_COUNTED_HANDLE);
```

In the C file:

```c
DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER(MY_REF_COUNTED_HANDLE, my_ref_counted_inc_ref, my_ref_counted_dec_ref);
```

### DECLARE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER

```c
#define DECLARE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER(REF_COUNTED_HANDLE_TYPE) \
    ...
```

`DECLARE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER` allows declaring the synchronous wrapper handlers for a reference counted type.

**SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_001: [** `DECLARE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER` shall declare the copy handler by expanding to: **]**

```c
DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(BSDL_RECORD_HANDLE, dst, src);
```

**SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_002: [** `DECLARE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER` shall declare the free handler by expanding to: **]**

```c
DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(BSDL_RECORD_HANDLE, arg);
```

### DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER

```c
#define DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER(REF_COUNTED_HANDLE_TYPE, inc_ref_function, dec_ref_function) \
    ...
```

`DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER` expands to the implementation of the synchronous wrapper copy and free handlers for a reference counted type.

**SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_003: [** `DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER` shall implement the copy handler by expanding to: **]**

```c
DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER(REF_COUNTED_HANDLE_TYPE, dst, src)
```

**SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_004: [** If `dst` is `NULL`, the copy handler shall fail and return a non-zero value. **]**

**SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_006: [** Otherwise the copy handler shall increment the reference count for `src` by calling `inc_ref_function`. **]**

**SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_011: [** The copy handler shall increment the reference count for `src` only if `src` is not `NULL`. **]**

**SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_007: [** The copy handler shall store `src` in `dst` and return 0. **]**

**SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_008: [** `DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER` shall implement the free handler by expanding to: **]**

```c
DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER(REF_COUNTED_HANDLE_TYPE, arg)
```

**SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_009: [** If `arg` is `NULL`, the free handler shall return. **]**

**SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_010: [** Otherwise the free handler shall decrement the reference count for `arg` by calling `dec_ref_function`. **]**
