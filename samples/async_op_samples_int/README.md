# ASYNC_OP Samples

## Overview

The files in this directory are meant to show samples for how to use [`async_op`](../../devdoc/async_op.md).
`async_op` is a flexible module meant to aid in cancellation of async operations, but it carries many complexities
in real-world usage.

Specifically, as soon as there is more than one layer of modules involved, there are many ways to use `async_op` which have bugs due to races, re-entrancy of callbacks, and other synchronization issues. These samples are meant to show "correct" ways to use `async_op` in various scenarios. The samples themselves have tests and should demonstrate the `async_op` functionality with minimal additional code.

## Samples

The samples are broken down into the following layers:

- "low-level" (LL) samples which call into something else that does not have `async_op`.
- "middle-level" (ML) samples which call into other LL and ML samples and propagate the `async_op` cancellation.
- "high-level" (HL) samples which call into other LL and ML samples and will drive the cancellation of the entire operation.

Each of the specific cases below provides at least one async operation. The modules follow the standard design of other modules, by having a "create" function which produces an opaque "handle", as well as "open", "close", and "destroy".

Note that the modules use a common interface defined in `common_async_op_module_interface.h` to simplify testing.

Each async operation takes at least a handle, an out argument for a `THANDLE(ASYNC_OP)`, a callback, and a callback context. The async operations return an `int`, with 0 indicating that the operation succeeded and the callback will _eventually_ be called and any other value indicating an error and the callback will _never_ be called.

### Reading Samples

Generally, the relevant parts of a sample will be in these functions:

 - API which starts the async operation
 - Callback handler for the async operation
 - Cancel function for the async operation

Within each of those, key steps are outlined in comments starting with numbers to indicate specific sequence of events that should be followed for safety. These steps are listed below as well, but the code shows more details.

### Individual Cases

#### ll_async_op_module_real_cancel

This sample is similar to the previous one, except that it can cancel the underlying operation. This is the ideal scenario, but it is not always possible.

##### Notes

This is a simple case but relies on the underlying operation to support cancellation in its own way (but not using `async_op`).

##### Steps

API:

1. Create an ASYNC_OP for the operation context
2. Any context needed is stored in `async_op->context`
3. Take an additional reference on the async_op, we have 1 reference for returning to the caller (`async_op`), and 1 reference for the async work callback (`async_op_ref_for_callback`)
4. Start the async work, passing the `async_op_ref_for_callback` as the context
5. Provide the `async_op` to the caller

Callback:

1. If the underlayer reported the reason was cancel, then report the result as canceled
2. Otherwise, complete normally
3. Clean up the `async_op`

Cancel:

1. Just signal to the underlying operation that it should cancel (e.g. stop the IO)

#### ll_async_op_module_fake_cancel

This is a sample for cases where the async operation itself does not use `async_op` and there is no way to cancel it directly. The cancellation is faked by just calling the callback and handling the cleanup when the real callback eventually does come. This simplifies the calling code so that it no longer needs to wait for the underlying operation to complete, but it does not truly cancel the operation (which may take until the process exits or the underlying IO is closed for example).

##### Notes

This case is also simple, but since it calls the callback directly on cancel, it needs to synchronize for races with the actual underlying callback completing and cancel. Thus, it must have a synchronization variable to track whether the callback has been called in the real callback or in the cancellation.

This is a "fake" cancel because the underlying operation still must complete on its own eventually and the memory related to it is not reclaimed until that point, however it means that the upper layer does not need to wait for the operation to complete.

##### Steps

API:

1. Create an ASYNC_OP for the operation context
2. Any context needed is stored in `async_op->context`
3. Need to synchronize fake cancellation with the real underlying operation completion, set a flag to indicate callback has not been called
4. Take an additional reference on the async_op, we have 1 reference for returning to the caller (`async_op`), and 1 reference for the async work callback (`async_op_ref_for_callback`)
5. Start the async work, passing the `async_op_ref_for_callback` as the context
6. Provide the `async_op` to the caller

Callback:

1. Check our synchronization flag to call the callback exactly once
2. If the callback has not been called yet, we call it now with the result value
3. Otherwise, the operation was canceled and this is a no-op
4. Clean up the `async_op`

Cancel:

1. Check our synchronization flag to call the callback exactly once
2. If the callback has not been called yet, we call it now with a canceled/abandoned result
3. Otherwise, the operation is already complete and this is a no-op

#### ml_async_op_module

This sample simply shows how `async_op` can be passed through to another layer.

##### Notes

This is also a simple example and is almost the same as `ll_async_op_module_real_cancel`.

##### Steps

API:

1. Create an ASYNC_OP for the operation context
2. Any context needed is stored in `async_op->context`
3. Initialize the `ll_async_op` to `NULL`
4. Store the async_op from the LL in a temporary variable
5. Take an additional reference on the async_op, we have 1 reference for returning to the caller (`async_op`), and 1 reference for the async work callback (`async_op_ref_for_callback`)
6. Start the async work, passing the `async_op_ref_for_callback` as the context
7. Store the `ll_async_op` in the context on success so that it can be canceled
8. Provide the `async_op` to the caller

Callback:

1. Do any processing and call the callback based on the result
2. Clean up the `async_op`

Cancel:

1. Just call cancel on the lower layer async_op

#### ml_async_op_module_with_async_chain

This sample makes calls to LL modules and does additional calls to LL modules in response to the async calls completing. For example, it calls function A, and in the callback for A, it calls function B, and so on until the last call in the chain which calls the upper layer callback. Cancellation must handle canceling regardless of which step has completed.

##### Notes

This sample must handle synchronizing based on which step is complete. There are some complexities in the timing of when the callbacks are called and care must be taken that the lower layer `async_op` to be cancelled is tracked as the latest call. It also must avoid starting the next async step after cancellation.

##### Steps

API:

1. Create an ASYNC_OP for the operation context
2. Any context needed is stored in `async_op->context`
3. Synchronization of ll_async_op between chained async calls requires a lock, a cancelation flag, and a counter (`ll_async_op_step`)
4. Initialize the `ll_async_op` to `NULL`
5. Store the async_op from the LL in a temporary variable
6. Take an additional reference on the async_op, we have 1 reference for returning to the caller (`async_op`), and 1 reference for the async work callback (`async_op_ref_for_callback`)
7. Start the first step of the async work, passing the `async_op_ref_for_callback` as the context
8. Take a lock to synchronize with the possibility of the step 1 callback already being called (or any subsequent steps)
9. Make sure we only store the latest `ll_async_op` by synchronizing on `ll_async_op_step`
10. Store the `ll_async_op` in the context on success so that it can be canceled
11. Provide the `async_op` to the caller

Callback:

Part 1 through N-1:

1. Take a lock to synchronize with calls in the chain completing and the async_op_ll changing
2. Reset the ll_async_op, it is complete and we don't need it anymore. Note that this is an optimization and not strictly necessary
3. Check if cancel has already been called, if so, we should call the callback instead of calling the chained async operation
4. Store the `async_op` from the LL in a temporary variable
5. Call the next step in the chain
6. Take a lock to synchronize with calls in the chain completing and the async_op_ll changing
7. Check if cancel has already been called, if so, we should cancel the new ll_async_op which wasn't stored in the context yet
8. Make sure we only store the latest `ll_async_op` by synchronizing on `ll_async_op_step`
9. Store the `ll_async_op` in the context on success so that it can be canceled
10. If we are canceled, we need to cancel the lower layer async_op which was just started in this call
11. In case of failure or cancellation, call the callback now
12. ...and clean up the `async_op`

Part N:

1. Do any processing and call the callback based on the result
2. Clean up the `async_op`

Cancel:

1. Take a lock to synchronize with calls in the chain completing and the async_op_ll changing
2. Set the state of this module to canceled so that we do not begin new async calls for lower-layer(s)
3. Get the current lower layer async op under the lock, note that it may be NULL if in between calls
4. Call cancel on the lower layer async_op if we had a non-NULL

#### ml_async_op_module_with_retries

This is similar to the previous sample, but it calls the same underlying operation in the callback until it succeeds. It should work as the previous example except that it has a (possibly unlimited) variable number of calls until the upper layer callback is called.

##### Notes

This is more of a general case of the previous sample.

##### Steps

API:

1. Create an ASYNC_OP for the operation context
2. Any context needed is stored in `async_op->context`
3. Synchronization of ll_async_op between chained async calls requires a lock, a cancelation flag, and a counter (`ll_async_op_step`)
4. Initialize the `ll_async_op` to `NULL`
5. Store the async_op from the LL in a temporary variable
6. Take an additional reference on the async_op, we have 1 reference for returning to the caller (`async_op`), and 1 reference for the async work callback (`async_op_ref_for_callback`)
7. Start the first step of the async work, passing the `async_op_ref_for_callback` as the context
8. Take a lock to synchronize with the possibility of the callback already being called (or any subsequent retries)
9. Make sure we only store the latest `ll_async_op` by synchronizing on `ll_async_op_epoch`
10. Store the `ll_async_op` in the context on success so that it can be canceled
11. Provide the `async_op` to the caller

Callback:

When retry is required:

1. Take a lock to synchronize with calls in the chain completing and the async_op_ll changing
2. Increment the epoch so that we do not try to store the old async_op from before this call
3. Reset the ll_async_op, it is complete and we don't need it anymore. Note that this is an optimization and not strictly necessary
4. Check if cancel has already been called, if so, we should call the callback instead of calling the chained async operation
5. Store the `async_op` from the LL in a temporary variable
6. Call the operation again for a retry
7. Take a lock to synchronize with other retry calls completing and the async_op_ll changing
8. Check if cancel has already been called, if so, we should cancel the new ll_async_op which wasn't stored in the context yet
9. Make sure we only store the latest `ll_async_op` by synchronizing on `ll_async_op_epoch`
10. Store the `ll_async_op` in the context on success so that it can be canceled
11. If we are canceled, we need to cancel the lower layer async_op which was just started in this call

When retry is not required:

12. In case of no retry needed, failure, or cancellation, call the callback now
13. ...and clean up the `async_op`

Cancel:

1. Take a lock to synchronize with calls in the chain completing and the async_op_ll changing
2. Set the state of this module to canceled so that we do not begin new async calls for lower-layer(s)
3. Get the current lower layer async op under the lock, note that it may be NULL if in between calls
4. Call cancel on the lower layer async_op if we had a non-NULL

#### hl_async_op_module

This is similar to the ML sample, but it is meant to be the top-level operation which will be passed to the "user".

#### hl_async_op_module_cancel_all

This is similar to the previous sample, but instead of letting the "user" cancel an individual operation, it tracks all pending calls and provides a method to cancel everything (in this case, the "close" of the module). That means this module must keep track of all the pending operations and does not have an `async_op` as an out argument.

##### Notes

The complexity here comes in how to track a list of pending `async_op`s which need to be cancelled at some time, but also need to be cleaned up if they complete on their own. Note that there are multiple ways to handle this list tracking and this is just one way.

##### Steps

API:

1. Create an ASYNC_OP for the operation context
2. Any context needed is stored in `async_op->context`
3. Initialize the ll_async_op to `NULL`
4. Store the async_op from the LL in a temporary variable
5. Take an additional reference on the async_op, we have 1 reference for storing in the list (`async_op`), and 1 reference for the async work callback (`async_op_ref_for_callback`)
6. Start the async work, passing the `async_op_ref_for_callback` as the context
7. Store the ll_async_op in the context on success so that it can be canceled
8. Take a lock to synchronize with close accessing the list of pending operations
9. If the module is not closing, add this operation to the list of pending operations
10. Mark this operation as in the list
11. If the module is closing, we need to cancel this operation
12. And release the reference on the `async_op`

Callback:

1. Remove this item from the list of pending operations under the lock and release the list reference on the async op
2. ...unless it was already removed during the start of a cancel
3. Do any processing and call the callback based on the result
4. Clean up the `async_op`

Close (in the "closing" callback of `sm_close_begin_with_cb`):

1. Get a temporary list to copy all operations under the lock so we can call cancel outside of the lock
2. Mark the module as closing to prevent new operations from being added
3. Remove each operation from the list
4. ...and add it to the temporary list
5. Marking it as not in the list
6. Outside of the lock, call cancel on each operation in the temporary list, and release the reference on the `async_op`
