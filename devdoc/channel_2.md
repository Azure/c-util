# channel_2 requirements:

## Overview

The current implementation of `channel` has certain pitfalls. `channel_2` aims to address these pitfalls by making the following changes:

1. Introduce a `channel_2_reset` API: This API will allow the user to reset the `channel_2` to a clean state. It will cause all pending operations to be abandoned.

2. Remove `ASYNC_OP`: The current implementation of `channel` provides the user with an `ASYNC_OP` when an operation (`channel_push` or `channel_pull`) is initiated. The user can call `async_op_cancel` to do a best-effort cancellation of the operation. There are 2 issues with this:
    - Complicates object lifetimes: The lifetime of the operation must be linked to the `ASYNC_OP`and the `channel`, since each can be independently destroyed by the user.
    - Inconsistent callback order: Consider when there are 3 `PUSH` operations in the `channel`: [`op1`, `op2`, `op3`] with data [`data1`, `data2`, `data3`] respectively.
    
    The receiver calls `channel_pull` 3 times and the sender cancels `op2` concurrently.
    
    The callback for `op1` is called with `data1` and `CHANNEL_CALLBACK_RESULT_OK`. 

    The callback for `op2` is called with `NULL`  and `CHANNEL_CALLBACK_RESULT_CANCELLED`.

    The callback for `op3` is called with `data3` and `CHANNEL_CALLBACK_RESULT_OK`.

    The receiver will now contain [`data1`, `data3`].

    This order of the data on the receiver side of the `channel` is not the same as the order of the `PUSH` operations on the sender side. This is not desirable.

    - Insufficient value: There is also not much value in providing the user with an `ASYNC_OP` for a single operation. We have not found a use case where the user would need to cancel a single operation while maintaining the other operations active. The user can always abandon all operations by calling `channel_2_reset`.

3. Remove `channel_internal`: The current implementation of `channel` has a `channel_internal` layer that was made necessary because of the `ASYNC_OP`. This layer is not needed in `channel_2` since the `ASYNC_OP` is removed.

4. Adding `open`/`close` APIs: These will allow the `channel_2` to guarantee that all pending operations are abandoned when the `channel_2` is closed.

## Exposed API

```c

#define CHANNEL_2_RESULT_VALUES \
    CHANNEL_2_RESULT_OK, \
    CHANNEL_2_RESULT_INVALID_ARGS, \
    CHANNEL_2_RESULT_ERROR

MU_DEFINE_ENUM(CHANNEL_2_RESULT, CHANNEL_2_RESULT_VALUES);

#define CHANNEL_2_CALLBACK_RESULT_VALUES \
    CHANNEL_2_CALLBACK_RESULT_OK, \
    CHANNEL_2_CALLBACK_RESULT_ABANDONED

MU_DEFINE_ENUM(CHANNEL_2_CALLBACK_RESULT, CHANNEL_2_CALLBACK_RESULT_VALUES);

typedef void(*CHANNEL_2_PULL_CALLBACK)(void* pull_context, CHANNEL_2_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id, THANDLE(RC_PTR) data);
typedef void(*CHANNEL_2_PUSH_CALLBACK)(void* push_context, CHANNEL_2_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id);

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct CHANNEL_2_TAG CHANNEL_2;

THANDLE_TYPE_DECLARE(CHANNEL_2);

    MOCKABLE_FUNCTION(, THANDLE(CHANNEL_2), channel_2_create_and_open, THANDLE(PTR(LOG_CONTEXT_HANDLE)), log_context, THANDLE(THREADPOOL), threadpool);
    MOCKABLE_FUNCTION(, void, channel_2_close, THANDLE(CHANNEL_2), channel_2);
    MOCKABLE_FUNCTION(, int, channel_2_reset, THANDLE(CHANNEL_2), channel_2);
    MOCKABLE_FUNCTION(, CHANNEL_2_RESULT, channel_2_pull, THANDLE(CHANNEL_2), channel_2, THANDLE(RC_STRING), correlation_id, CHANNEL_2_PULL_CALLBACK, pull_callback, void*, pull_context);
    MOCKABLE_FUNCTION(, CHANNEL_2_RESULT, channel_2_push, THANDLE(CHANNEL_2), channel_2, THANDLE(RC_STRING), correlation_id, THANDLE(RC_PTR), data, CHANNEL_2_PUSH_CALLBACK, push_callback, void*, push_context);

#ifdef __cplusplus
}
#endif /* __cplusplus */


```