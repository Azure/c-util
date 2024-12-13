# ASYNC_OP Samples

## Overview

The files in this directory are meant to show samples for how to use [`async_op`](../../devdoc/async_op.md).
`async_op` is a flexible module meant to aid in cancellation of async operations, but it carries many complexities
in real-world usage.

### Samples

The samples are broken down into the following layers:

- "low-level" (LL) samples which call into something else that does not have `async_op`.
- "middle-level" (ML) samples which call into other LL and ML samples and propagate the `async_op` cancellation.
- "high-level" (HL) samples which call into other LL and ML samples and will drive the cancellation of the entire operation.

Each of the specific cases below provides at least one async operation. The modules follow the standard design of other modules, by having a "create" and "destroy" function which produces an opaque "handle".

Each async operation takes at least a handle, an out argument for a `THANDLE(ASYNC_OP)`, a callback, and a callback context. The async operations return an `int`, with 0 indicating that the operation succeeded and the callback will eventually be called and any other value indicating an error and the callback will not be called.

Modules may have multiple operations to show variants or provide additional functionality for testing.

#### ll_async_module_fake_cancel

This is a sample for cases where the async operation itself does not use `async_op` and there is no way to cancel it directly. The cancellation is faked by just calling the callback and handling the cleanup when the real callback eventually does come. This simplifies the calling code so that it no longer needs to wait for the underlying operation to complete, but it does not truly cancel the operation (which may take until the process exits or the underlying IO is closed for example).

#### ll_async_module_real_cancel

This sample is similar to the previous one, except that it can cancel the underlying operation. This is the ideal scenario, but it is not always possible.

#### ml_async_module

This sample simply shows how `async_op` can be passed through to another layer.

#### ml_async_module_with_retries

This sample makes calls to LL modules and will do retries in response to the asynchronous results. The cancellation must handle the retries.

#### ml_async_module_with_async_chain

This is similar to the previous sample, but it calls a different async operation in response to the callback from the first one. Again, cancellation must handle canceling regardless of which step has completed.

#### hl_async_module

This is similar to the ML sample, but it is meant to be the top-level operation which will be passed to the "user".

#### hl_async_module_cancel_all

This is similar to the previous sample, but instead of letting the "user" cancel an individual operation, it tracks all pending calls and provides a method to cancel everything. That means this module must keep track of all the pending operations.
