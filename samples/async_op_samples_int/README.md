# ASYNC_OP Samples

## Overview

The files in this directory are meant to show samples for how to use [`async_op`](../../devdoc/async_op.md).
`async_op` is a flexible module meant to aid in cancellation of async operations, but it carries many complexities
in real-world usage.

Specifically, there are many ways to use `async_op` which have bugs due to races, re-entrancy of callbacks, and other synchronization issues. These samples are meant to show "correct" ways to use `async_op` in various scenarios. The samples themselves have tests and should demonstrate the `async_op` functionality with minimal additional code.

## Samples

The samples are broken down into the following layers:

- "low-level" (LL) samples which call into something else that does not have `async_op`.
- "middle-level" (ML) samples which call into other LL and ML samples and propagate the `async_op` cancellation.
- "high-level" (HL) samples which call into other LL and ML samples and will drive the cancellation of the entire operation.

Each of the specific cases below provides at least one async operation. The modules follow the standard design of other modules, by having a "create" function which produces an opaque "handle", as well as "open", "close", and "destroy".

Each async operation takes at least a handle, an out argument for a `THANDLE(ASYNC_OP)`, a callback, and a callback context. The async operations return an `int`, with 0 indicating that the operation succeeded and the callback will _eventually_ be called and any other value indicating an error and the callback will _never_ be called.

Modules may have multiple operations to show variants or provide additional functionality for testing.

### Reading Samples

Generally, the relevant parts of a sample will be in these functions:

 - API which starts the async operation
 - Callback handler for the async operation
 - Cancel function for the async operation

Within each of those, key steps are outlined in comments starting with numbers to indicate specific sequence of events that should be followed for safety.

### Individual Cases

#### ll_async_module_real_cancel

This sample is similar to the previous one, except that it can cancel the underlying operation. This is the ideal scenario, but it is not always possible.

##### Notes

This is a simple case but relies on the underlying operation to support cancellation in its own way (but not using `async_op`).

#### ll_async_module_fake_cancel

This is a sample for cases where the async operation itself does not use `async_op` and there is no way to cancel it directly. The cancellation is faked by just calling the callback and handling the cleanup when the real callback eventually does come. This simplifies the calling code so that it no longer needs to wait for the underlying operation to complete, but it does not truly cancel the operation (which may take until the process exits or the underlying IO is closed for example).

##### Notes

This case is also simple, but since it calls the callback directly on cancel, it needs to synchronize for races with the actual underlying callback completing and cancel. Thus, it must have a synchronization variable to track whether the callback has been called in the real callback or in the cancellation.

This is a "fake" cancel because the underlying operation still must complete on its own eventually and the memory related to it is not reclaimed until that point, however it means that the upper layer does not need to wait for the operation to complete.

#### ml_async_module

This sample simply shows how `async_op` can be passed through to another layer.

##### Notes

This is also a simple example and is almost the same as `ll_async_module_real_cancel`.

#### ml_async_module_with_async_chain

This sample makes calls to LL modules and does additional calls to LL modules in response to the async calls completing. For example, it calls function A, and in the callback for A, it calls function B, and so on until the last call in the chain which calls the upper layer callback. Cancellation must handle canceling regardless of which step has completed.

##### Notes

This sample must handle synchronizing based on which step is complete. There are some complexities in the timing of when the callbacks are called and care must be taken that the lower layer `async_op` to be cancelled is tracked as the latest call. It also must avoid starting the next async step after cancellation.

#### ml_async_module_with_retries

This is similar to the previous sample, but it calls the same underlying operation in the callback until it succeeds. It should work as the previous example except that it has a (possibly unlimited) variable number of calls until the upper layer callback is called.

##### Notes

This is more of a general case of the previous sample.

#### hl_async_module

This is similar to the ML sample, but it is meant to be the top-level operation which will be passed to the "user".

#### hl_async_module_cancel_all

This is similar to the previous sample, but instead of letting the "user" cancel an individual operation, it tracks all pending calls and provides a method to cancel everything. That means this module must keep track of all the pending operations.

##### Notes

The complexity here comes in how to track a list of pending `async_op`s which need to be cancelled at some time, but also need to be cleaned up if they complete on their own.
