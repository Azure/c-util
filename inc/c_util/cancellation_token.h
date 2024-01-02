// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CANCELLATION_TOKEN_H
#define CANCELLATION_TOKEN_H

#include "macro_utils/macro_utils.h"
#include "c_pal/thandle.h"
#include "c_util/tcall_dispatcher.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct CANCELLATION_TOKEN_TAG CANCELLATION_TOKEN;
THANDLE_TYPE_DECLARE(CANCELLATION_TOKEN);

typedef struct CANCELLATION_TOKEN_REGISTRATION_TAG CANCELLATION_TOKEN_REGISTRATION;
THANDLE_TYPE_DECLARE(CANCELLATION_TOKEN_REGISTRATION);

MOCKABLE_FUNCTION(, THANDLE(CANCELLATION_TOKEN), cancellation_token_create, bool, canceled);
MOCKABLE_FUNCTION(, int, cancellation_token_is_canceled, THANDLE(CANCELLATION_TOKEN), cancellation_token, bool*, canceled);
MOCKABLE_FUNCTION(, THANDLE(CANCELLATION_TOKEN_REGISTRATION), cancellation_token_register_notify, THANDLE(CANCELLATION_TOKEN), cancellation_token, TCALL_DISPATCHER_TARGET_FUNC_TYPE_NAME(CANCELLATION_TOKEN_CANCEL_CALL), on_cancel, void*, context);
MOCKABLE_FUNCTION(, int, cancellation_token_cancel, THANDLE(CANCELLATION_TOKEN), cancellation_token);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CANCELLATION_TOKEN_H */
