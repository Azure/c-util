// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/thandle.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"

#include "c_util/tcall_dispatcher_cancellation_token_cancel_call.h"
#include "c_util/cancellation_token.h"

#define CANCELLATION_TOKEN_STATE_VALUES \
    CANCELLATION_TOKEN_STATE_NOT_CANCELED, \
    CANCELLATION_TOKEN_STATE_CANCELED

MU_DEFINE_ENUM(CANCELLATION_TOKEN_STATE, CANCELLATION_TOKEN_STATE_VALUES);
MU_DEFINE_ENUM_STRINGS(CANCELLATION_TOKEN_STATE, CANCELLATION_TOKEN_STATE_VALUES);

typedef struct CANCELLATION_TOKEN_TAG
{
    volatile_atomic int32_t state;
    TCALL_DISPATCHER(CANCELLATION_TOKEN_CANCEL_CALL) cancel_notification_call_dispatcher;
} CANCELLATION_TOKEN;

THANDLE_TYPE_DEFINE(CANCELLATION_TOKEN);

typedef struct CANCELLATION_TOKEN_REGISTRATION_TAG
{
    THANDLE(CANCELLATION_TOKEN) token;
    TCALL_DISPATCHER_TARGET_HANDLE(CANCELLATION_TOKEN_CANCEL_CALL) cancel_notification_target_handle;
} CANCELLATION_TOKEN_REGISTRATION;

THANDLE_TYPE_DEFINE(CANCELLATION_TOKEN_REGISTRATION);

static void cancellation_token_dispose(CANCELLATION_TOKEN* token)
{
    TCALL_DISPATCHER_ASSIGN(CANCELLATION_TOKEN_CANCEL_CALL)(&token->cancel_notification_call_dispatcher, NULL);
}

static void cancellation_token_registration_dispose(CANCELLATION_TOKEN_REGISTRATION* registration)
{
    CANCELLATION_TOKEN* token_ptr = THANDLE_GET_T(CANCELLATION_TOKEN)(registration->token);
    TCALL_DISPATCHER_UNREGISTER_TARGET(CANCELLATION_TOKEN_CANCEL_CALL)(token_ptr->cancel_notification_call_dispatcher, registration->cancel_notification_target_handle);
    THANDLE_ASSIGN(CANCELLATION_TOKEN)(&registration->token, NULL);
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CANCELLATION_TOKEN), cancellation_token_create, bool, canceled)
{
    /*Codes_SRS_CANCELLATION_TOKEN_04_001: [ cancellation_token_create shall allocate memory for a THANDLE(CANCELLATION_TOKEN). ]*/
    THANDLE(CANCELLATION_TOKEN) token = THANDLE_MALLOC(CANCELLATION_TOKEN)(cancellation_token_dispose);
    if (token == NULL)
    {
        LogError("THANDLE_MALLOC(CANCELLATION_TOKEN)(cancellation_token_dispose) failed.");
    }
    else
    {
        CANCELLATION_TOKEN* token_ptr = THANDLE_GET_T(CANCELLATION_TOKEN)(token);

        /*Codes_SRS_CANCELLATION_TOKEN_04_003: [ cancellation_token_create shall create a TCALL_DISPATCHER handle by calling TCALL_DISPATCHER_CREATE(CANCELLATION_TOKEN_CANCEL_CALL). ]*/
        TCALL_DISPATCHER(CANCELLATION_TOKEN_CANCEL_CALL) dispatcher = TCALL_DISPATCHER_CREATE(CANCELLATION_TOKEN_CANCEL_CALL)();

        if (dispatcher == NULL)
        {
            LogError("TCALL_DISPATCHER_CREATE(CANCELLATION_TOKEN_CANCEL_CALL)() failed.");
            TCALL_DISPATCHER_INITIALIZE(CANCELLATION_TOKEN_CANCEL_CALL)(&token_ptr->cancel_notification_call_dispatcher, NULL);
        }
        else
        {
            if (canceled == false)
            {
                /*Codes_SRS_CANCELLATION_TOKEN_04_004: [ cancellation_token_create shall set the initial state to be equal to canceled parameter. ]*/
                (void)interlocked_exchange(&token_ptr->state, CANCELLATION_TOKEN_STATE_NOT_CANCELED);
            }
            else
            {
                /*Codes_SRS_CANCELLATION_TOKEN_04_004: [ cancellation_token_create shall set the initial state to be equal to canceled parameter. ]*/
                (void)interlocked_exchange(&token_ptr->state, CANCELLATION_TOKEN_STATE_CANCELED);
            }

            TCALL_DISPATCHER_INITIALIZE_MOVE(CANCELLATION_TOKEN_CANCEL_CALL)(&token_ptr->cancel_notification_call_dispatcher, &dispatcher);

            /*Codes_SRS_CANCELLATION_TOKEN_04_005: [ cancellation_token_create shall return a valid THANDLE(CANCELLATION_TOKEN) when successful. ]*/
            goto all_ok;
        }

        /*Codes_SRS_CANCELLATION_TOKEN_04_002: [ If any underlying error occurs cancellation_token_create shall fail and return NULL. ]*/
        THANDLE_ASSIGN(CANCELLATION_TOKEN)(&token, NULL);
    }

all_ok:
    return token;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, cancellation_token_is_canceled, THANDLE(CANCELLATION_TOKEN), cancellation_token, bool*, canceled)
{
    int result;

    if (cancellation_token == NULL || canceled == NULL)
    {
        /*Codes_SRS_CANCELLATION_TOKEN_04_006: [ cancellation_token_is_canceled shall return a non-zero value if cancellation_token is NULL. ]*/
        /*Codes_SRS_CANCELLATION_TOKEN_04_007: [ cancellation_token_is_canceled shall return a non-zero value if canceled is NULL. ]*/
        LogError("Invalid args: THANDLE(CANCELLATION_TOKEN) cancellation_token = %p, canceled = %p", cancellation_token, canceled);
        result = MU_FAILURE;
    }
    else
    {
        CANCELLATION_TOKEN* token_ptr = THANDLE_GET_T(CANCELLATION_TOKEN)(cancellation_token);
        CANCELLATION_TOKEN_STATE state = interlocked_add(&token_ptr->state, 0);

        /*Codes_SRS_CANCELLATION_TOKEN_04_008: [ cancellation_token_is_canceled shall assign true to *canceled if the token has been canceled and false otherwise. ]*/
        if (state == CANCELLATION_TOKEN_STATE_CANCELED)
        {
            *canceled = true;
        }
        else
        {
            *canceled = false;
        }

        /*Codes_SRS_CANCELLATION_TOKEN_04_009: [ cancellation_token_is_canceled shall return 0 when successful. ]*/
        result = 0;
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CANCELLATION_TOKEN_REGISTRATION), cancellation_token_register_notify, THANDLE(CANCELLATION_TOKEN), cancellation_token, TCALL_DISPATCHER_TARGET_FUNC_TYPE_NAME(CANCELLATION_TOKEN_CANCEL_CALL), on_cancel, void*, context)
{
    THANDLE(CANCELLATION_TOKEN_REGISTRATION) result = NULL;

    if (cancellation_token == NULL || on_cancel == NULL)
    {
        /*Codes_SRS_CANCELLATION_TOKEN_04_010: [ cancellation_token_register_notify shall fail and return NULL when cancellation_token is NULL. ]*/
        /*Codes_SRS_CANCELLATION_TOKEN_04_011: [ cancellation_token_register_notify shall fail and return NULL when on_cancel is NULL. ]*/
        LogError("Invalid args: THANDLE(CANCELLATION_TOKEN) cancellation_token = %p, TCALL_DISPATCHER_TARGET_FUNC_TYPE_NAME(CANCELLATION_TOKEN_CANCEL_CALL) on_cancel = %p, void* context = %p", cancellation_token, on_cancel, context);
    }
    else
    {
        CANCELLATION_TOKEN* token_ptr = THANDLE_GET_T(CANCELLATION_TOKEN)(cancellation_token);

        /*Codes_SRS_CANCELLATION_TOKEN_04_013: [ cancellation_token_register_notify shall call TCALL_DISPATCHER_REGISTER_TARGET(CANCELLATION_TOKEN_CANCEL_CALL) to register on_cancel with the dispatcher. ]*/
        TCALL_DISPATCHER_TARGET_HANDLE(CANCELLATION_TOKEN_CANCEL_CALL) cancel_notification_target_handle = TCALL_DISPATCHER_REGISTER_TARGET(CANCELLATION_TOKEN_CANCEL_CALL)(token_ptr->cancel_notification_call_dispatcher, on_cancel, context);
        if (cancel_notification_target_handle == NULL)
        {
            /*Codes_SRS_CANCELLATION_TOKEN_04_012: [ cancellation_token_register_notify shall fail and return NULL when any underlying call fails. ]*/
            LogError("TCALL_DISPATCHER_REGISTER_TARGET(CANCELLATION_TOKEN_CANCEL_CALL)(token_ptr->cancel_notification_call_dispatcher, on_cancel, context) failed.");
        }
        else
        {
            /*Codes_SRS_CANCELLATION_TOKEN_04_014: [ cancellation_token_register_notify shall call THANDLE_MALLOC(CANCELLATION_TOKEN_REGISTRATION) to allocate a CANCELLATION_TOKEN_REGISTRATION handle. ]*/
            THANDLE(CANCELLATION_TOKEN_REGISTRATION) registration = THANDLE_MALLOC(CANCELLATION_TOKEN_REGISTRATION)(cancellation_token_registration_dispose);
            if (registration == NULL)
            {
                /*Codes_SRS_CANCELLATION_TOKEN_04_012: [ cancellation_token_register_notify shall fail and return NULL when any underlying call fails. ]*/
                LogError("THANDLE_MALLOC(CANCELLATION_TOKEN)(cancellation_token_dispose) failed.");
            }
            else
            {
                CANCELLATION_TOKEN_REGISTRATION* registration_ptr = THANDLE_GET_T(CANCELLATION_TOKEN_REGISTRATION)(registration);
                registration_ptr->cancel_notification_target_handle = cancel_notification_target_handle;
                THANDLE_INITIALIZE(CANCELLATION_TOKEN)(&registration_ptr->token, cancellation_token);

                /*Codes_SRS_CANCELLATION_TOKEN_04_015: [ cancellation_token_register_notify shall initialize and return a valid THANDLE(CANCELLATION_TOKEN_REGISTRATION) when successful. ]*/
                THANDLE_INITIALIZE_MOVE(CANCELLATION_TOKEN_REGISTRATION)(&result, &registration);
                goto all_ok;
            }

            /*Codes_SRS_CANCELLATION_TOKEN_04_016: [ cancellation_token_register_notify shall call TCALL_DISPATCHER_UNREGISTER_TARGET(CANCELLATION_TOKEN_CANCEL_CALL) when an error occurs to undo the callback registration. ]*/
            TCALL_DISPATCHER_UNREGISTER_TARGET(CANCELLATION_TOKEN_CANCEL_CALL)(token_ptr->cancel_notification_call_dispatcher, cancel_notification_target_handle);
        }
    }

 all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, cancellation_token_cancel, THANDLE(CANCELLATION_TOKEN), cancellation_token)
{
    int result;

    if (cancellation_token == NULL)
    {
        /*Codes_SRS_CANCELLATION_TOKEN_04_017: [ cancellation_token_cancel shall fail and return a non-zero value if cancellation_token is NULL. ]*/
        LogError("Invalid args: THANDLE(CANCELLATION_TOKEN) cancellation_token = %p", cancellation_token);
        result = MU_FAILURE;
    }
    else
    {
        CANCELLATION_TOKEN* token_ptr = THANDLE_GET_T(CANCELLATION_TOKEN)(cancellation_token);

        /*Codes_SRS_CANCELLATION_TOKEN_04_019: [ cancellation_token_cancel shall set the state of the token to be "canceled". ]*/
        if (interlocked_compare_exchange(&token_ptr->state, CANCELLATION_TOKEN_STATE_CANCELED, CANCELLATION_TOKEN_STATE_NOT_CANCELED) != CANCELLATION_TOKEN_STATE_NOT_CANCELED)
        {
            /*Codes_SRS_CANCELLATION_TOKEN_04_018: [ cancellation_token_cancel shall fail and return a non-zero value if the state of the token is already "canceled". ]*/
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_CANCELLATION_TOKEN_04_021: [ cancellation_token_cancel shall invoke all callbacks that have been registered on the token via cancellation_token_register_notify. ]*/
            TCALL_DISPATCHER_DISPATCH_CALL(CANCELLATION_TOKEN_CANCEL_CALL)(token_ptr->cancel_notification_call_dispatcher);

            /*Codes_SRS_CANCELLATION_TOKEN_04_020: [ cancellation_token_cancel shall return 0 when successful. ]*/
            result = 0;
        }
    }

    return result;
}