`async_type_helper_copy_value_handler` requirements
============

## Overview

`async_type_helper_copy_value_handler` is a helper module that allows generating async wrapper handlers for a value that is copied by value (like a struct having all its fields copied).

## Exposed API

```c
#define DECLARE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER(COPY_VALUE_TYPE) \
    ...

#define DEFINE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER(COPY_VALUE_TYPE) \
    ...
```

Example usage:

Given a struct `MY_STRUCT`:

In the header file:

```c
DECLARE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER(MY_STRUCT);
```

In the C file:

```c
DEFINE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER(MY_STRUCT);
```

### DECLARE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER

```c
#define DECLARE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER(COPY_VALUE_TYPE) \
    ...
```

`DECLARE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER` allows declaring the synchronous wrapper handlers for a type to be copied by value.

**SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_001: [** `DECLARE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER` shall declare the copy handler by expanding to: **]**

```c
DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(COPY_VALUE_TYPE, dst, src);
```

**SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_002: [** `DECLARE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER` shall declare the free handler by expanding to: **]**

```c
DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(COPY_VALUE_TYPE, arg);
```

### DEFINE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER

```c
#define DEFINE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER(COPY_VALUE_TYPE) \
    ...
```

`DEFINE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER` expands to the implementation of the synchronous wrapper copy and free handlers for a type to be copied by value.

**SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_003: [** `DEFINE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER` shall implement the copy handler by expanding to: **]**

```c
DEFINE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER(COPY_VALUE_TYPE, dst, src)
```

**SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_004: [** If `dst` is `NULL`, the copy handler shall fail and return a non-zero value. **]**

**SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_005: [** Otherwise the copy handler shall copy all the contents of `src` to `dst`. **]**

**SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_006: [** The copy handler shall succeed and return 0. **]**

**SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_007: [** `DEFINE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER` shall implement the free handler by expanding to: **]**

```c
DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER(COPY_VALUE_TYPE, arg)
```

**SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_009: [** The free handler shall return. **]**
