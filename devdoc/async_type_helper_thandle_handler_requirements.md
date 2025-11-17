`async_type_helper_thandle_handler` requirements
============

## Overview

`async_type_helper_thandle_handler` is a helper module that allows generating async wrapper handlers for a thandle object.

## Exposed API

```c
#define DECLARE_ASYNC_TYPE_HELPER_THANDLE_HANDLER(THANDLE_HANDLE_TYPE) \
    ...

#define DEFINE_ASYNC_TYPE_HELPER_THANDLE_HANDLER(THANDLE_HANDLE_TYPE) \
    ...
```

Example usage:

Given a `THANDLE` type `MY_THANDLE`:

In the header file:

```c
DECLARE_ASYNC_TYPE_HELPER_THANDLE_HANDLER(MY_THANDLE);
```

In the C file:

```c
DEFINE_ASYNC_TYPE_HELPER_THANDLE_HANDLER(MY_THANDLE);
```

### DECLARE_ASYNC_TYPE_HELPER_THANDLE_HANDLER

```c
#define DECLARE_ASYNC_TYPE_HELPER_THANDLE_HANDLER(THANDLE_TYPE) \
    ...
```

`DECLARE_ASYNC_TYPE_HELPER_THANDLE_HANDLER` allows declaring the synchronous wrapper handlers for a `THANDLE` type.

**SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_001: [** `DECLARE_ASYNC_TYPE_HELPER_THANDLE_HANDLER` shall declare the copy handler by expanding to: **]**

```c
MOCKABLE_FUNCTION(, int, ASYNC_TYPE_HELPER_COPY_HANDLER(THANDLE(THANDLE_TYPE)), THANDLE(THANDLE_TYPE)*, dst, THANDLE(THANDLE_TYPE), src);
```

**SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_002: [** `DECLARE_ASYNC_TYPE_HELPER_THANDLE_HANDLER` shall declare the free handler by expanding to: **]**

```c
MOCKABLE_FUNCTION(, void, ASYNC_TYPE_HELPER_FREE_HANDLER(THANDLE(THANDLE_TYPE)), THANDLE(THANDLE_TYPE), arg);
```

### DEFINE_ASYNC_TYPE_HELPER_THANDLE_HANDLER

```c
#define DEFINE_ASYNC_TYPE_HELPER_THANDLE_HANDLER(THANDLE_HANDLE_TYPE) \
    ...
```

`DEFINE_ASYNC_TYPE_HELPER_THANDLE_HANDLER` expands to the implementation of the synchronous wrapper copy and free handlers for a `THANDLE` type.

**SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_003: [** `DEFINE_ASYNC_TYPE_HELPER_THANDLE_HANDLER` shall implement the copy handler by expanding to: **]**

```c
int ASYNC_TYPE_HELPER_COPY_HANDLER(THANDLE(THANDLE_TYPE))(THANDLE(THANDLE_TYPE)* dst, THANDLE(THANDLE_TYPE) src)
```

**SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_004: [** If `dst` is `NULL`, the copy handler shall fail and return a non-zero value. **]**

**SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_006: [** Otherwise the copy handler shall assign `src` to `dst` by calling `THANDLE_INITIALIZE`. **]**

**SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_007: [** The copy handler shall succeed and return 0. **]**

**SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_008: [** `DEFINE_ASYNC_TYPE_HELPER_THANDLE_HANDLER` shall implement the free handler by expanding to: **]**

```c
void ASYNC_TYPE_HELPER_FREE_HANDLER(THANDLE(THANDLE_TYPE))(THANDLE(THANDLE_TYPE) arg)
```

**SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_009: [** If `arg` is `NULL`, the free handler shall return. **]**

**SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_010: [** Otherwise the free handler shall release `arg` by assigning `NULL` to it. **]**
