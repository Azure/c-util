# Cancellation Token Requirements

## Overview

The cancellation token module implements an object that can be used to propagate cancellation signals through multi-threaded programs. The design is roughly equivalent to the [`CancellationToken`](https://learn.microsoft.com/en-us/dotnet/api/system.threading.cancellationtoken?view=net-8.0) type in .NET. The module allows applications to create cancellation token handles which can then be passed around to various functions. The following operations can be performed on cancellation tokens:

- Check if the token has been canceled.
- Register a callback to be invoked when the token is canceled.
- Initiate cancellation of the token causing the callback registrations to be invoked and the state of the token to be changed to "canceled".

Because a cancellation token is a kind of `THANDLE`, all of `THANDLE`'s APIs apply to cancellation token handles. All API calls are threadsafe.

Here is an example of creating a cancellation token and registering to be notified of cancellation.

```c
static void on_cancel(void* context)
{
    (void)context;

    // do something when the operation is canceled
}

// create a cancellation token
THANDLE(CANCELLATION_TOKEN) token = cancellation_token_create(false);
ASSERT_IS_NOT_NULL(token);

// check that it hasn't been canceled
bool canceled;
int result = cancellation_token_is_canceled(token, &canceled)
ASSERT_ARE_EQUAL(int, 0, result);
ASSERT_IS_FALSE(canceled);

// register to be notified when the token is canceled
THANDLE(CANCELLATION_TOKEN_REGISTRATION) registration = cancellation_token_register_notify(token, on_cancel, NULL);
ASSERT_IS_NOT_NULL(registration);

// signal cancellation - causes 'on_cancel' to be called
result = cancellation_token_cancel(token);
ASSERT_ARE_EQUAL(int, 0, result);

// check that token has been canceled
result = cancellation_token_is_canceled(token, &canceled)
ASSERT_ARE_EQUAL(int, 0, result);
ASSERT_IS_TRUE(canceled);

// clean up
THANDLE_ASSIGN(CANCELLATION_TOKEN_REGISTRATION)(&registration, NULL);
THANDLE_ASSIGN(CANCELLATION_TOKEN)(&token, NULL);
```

## Exposed API

```c
typedef struct CANCELLATION_TOKEN_TAG CANCELLATION_TOKEN;
THANDLE_TYPE_DECLARE(CANCELLATION_TOKEN);

typedef struct CANCELLATION_TOKEN_REGISTRATION_TAG CANCELLATION_TOKEN_REGISTRATION;
THANDLE_TYPE_DECLARE(CANCELLATION_TOKEN_REGISTRATION);

MOCKABLE_FUNCTION(, THANDLE(CANCELLATION_TOKEN), cancellation_token_create, bool, canceled);
MOCKABLE_FUNCTION(, int, cancellation_token_is_canceled, THANDLE(CANCELLATION_TOKEN), cancellation_token, bool*, canceled);
MOCKABLE_FUNCTION(, THANDLE(CANCELLATION_TOKEN_REGISTRATION), cancellation_token_register_notify, THANDLE(CANCELLATION_TOKEN), cancellation_token, TCALL_DISPATCHER_TARGET_FUNC_TYPE_NAME(CANCELLATION_TOKEN_CANCEL_CALL), on_cancel, void*, context);
MOCKABLE_FUNCTION(, int, cancellation_token_cancel, THANDLE(CANCELLATION_TOKEN), cancellation_token);
```

## cancellation_token_create

```c
MOCKABLE_FUNCTION(, THANDLE(CANCELLATION_TOKEN), cancellation_token_create, bool, canceled);
```

`cancellation_token_create` creates a new cancellation token with the specified initial cancellation state.

**SRS_CANCELLATION_TOKEN_04_001: [** `cancellation_token_create` shall allocate memory for a `THANDLE(CANCELLATION_TOKEN)`. **]**

**SRS_CANCELLATION_TOKEN_04_002: [** If any underlying error occurs `cancellation_token_create` shall fail and return `NULL`. **]**

**SRS_CANCELLATION_TOKEN_04_003: [** `cancellation_token_create` shall create a `TCALL_DISPATCHER` handle by calling `TCALL_DISPATCHER_CREATE(CANCELLATION_TOKEN_CANCEL_CALL)`. **]**

**SRS_CANCELLATION_TOKEN_04_004: [** `cancellation_token_create` shall set the initial state to be equal to `canceled` parameter. **]**

**SRS_CANCELLATION_TOKEN_04_005: [** `cancellation_token_create` shall return a valid `THANDLE(CANCELLATION_TOKEN)` when successful. **]**

## cancellation_token_is_canceled

```c
MOCKABLE_FUNCTION(, int, cancellation_token_is_canceled, THANDLE(CANCELLATION_TOKEN), cancellation_token, bool*, canceled);
```

`cancellation_token_is_canceled` assigns `true` to `*canceled` if the cancellation token has been canceled and `false` otherwise.

**SRS_CANCELLATION_TOKEN_04_006: [** `cancellation_token_is_canceled` shall return a non-zero value if `cancellation_token` is `NULL`. **]**

**SRS_CANCELLATION_TOKEN_04_007: [** `cancellation_token_is_canceled` shall return a non-zero value if `canceled` is `NULL`. **]**

**SRS_CANCELLATION_TOKEN_04_008: [** `cancellation_token_is_canceled` shall assign `true` to `*canceled` if the token has been canceled and `false` otherwise. **]**

**SRS_CANCELLATION_TOKEN_04_009: [** `cancellation_token_is_canceled` shall return `0` when successful. **]**

## cancellation_token_register_notify

```c
MOCKABLE_FUNCTION(, THANDLE(CANCELLATION_TOKEN_REGISTRATION), cancellation_token_register_notify, THANDLE(CANCELLATION_TOKEN), cancellation_token, TCALL_DISPATCHER_TARGET_FUNC_TYPE_NAME(CANCELLATION_TOKEN_CANCEL_CALL), on_cancel, void*, context);
```

`cancellation_token_register_notify` registers `on_cancel` to be called when the token is canceled.

**SRS_CANCELLATION_TOKEN_04_010: [** `cancellation_token_register_notify` shall fail and return `NULL` when `cancellation_token` is `NULL`. **]**

**SRS_CANCELLATION_TOKEN_04_011: [** `cancellation_token_register_notify` shall fail and return `NULL` when `on_cancel` is `NULL`. **]**

**SRS_CANCELLATION_TOKEN_04_012: [** `cancellation_token_register_notify` shall fail and return `NULL` when any underlying call fails. **]**

**SRS_CANCELLATION_TOKEN_04_013: [** `cancellation_token_register_notify` shall call `TCALL_DISPATCHER_REGISTER_TARGET(CANCELLATION_TOKEN_CANCEL_CALL)` to register `on_cancel` with the dispatcher. **]**

**SRS_CANCELLATION_TOKEN_04_014: [** `cancellation_token_register_notify` shall call `THANDLE_MALLOC(CANCELLATION_TOKEN_REGISTRATION)` to allocate a `CANCELLATION_TOKEN_REGISTRATION` handle. **]**

**SRS_CANCELLATION_TOKEN_04_015: [** `cancellation_token_register_notify` shall initialize and return a valid `THANDLE(CANCELLATION_TOKEN_REGISTRATION)` when successful. **]**

**SRS_CANCELLATION_TOKEN_04_016: [** `cancellation_token_register_notify` shall call `TCALL_DISPATCHER_UNREGISTER_TARGET(CANCELLATION_TOKEN_CANCEL_CALL)` when an error occurs to undo the callback registration. **]**

## cancellation_token_cancel

```c
MOCKABLE_FUNCTION(, int, cancellation_token_cancel, THANDLE(CANCELLATION_TOKEN), cancellation_token);
```

`cancellation_token_cancel` sets the state of the cancellation token to be "canceled" and invokes all the callbacks that have been registered via `cancellation_token_register_notify`.

**SRS_CANCELLATION_TOKEN_04_017: [** `cancellation_token_cancel` shall fail and return a non-zero value if `cancellation_token` is `NULL`. **]**

**SRS_CANCELLATION_TOKEN_04_018: [** `cancellation_token_cancel` shall fail and return a non-zero value if the state of the token is already "canceled". **]**

**SRS_CANCELLATION_TOKEN_04_019: [** `cancellation_token_cancel` shall set the state of the token to be "canceled". **]**

**SRS_CANCELLATION_TOKEN_04_020: [** `cancellation_token_cancel` shall return `0` when successful. **]**

**SRS_CANCELLATION_TOKEN_04_021: [** `cancellation_token_cancel` shall invoke all callbacks that have been registered on the token via `cancellation_token_register_notify`. **]**