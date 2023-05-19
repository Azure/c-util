// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef WAITER_H
#define WAITER_H

#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/thandle.h"

#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"

#define WAITER_RESULT_VALUES \
    WAITER_RESULT_SYNC, \
    WAITER_RESULT_ASYNC, \
    WAITER_RESULT_REFUSED, \
    WAITER_RESULT_ERROR

MU_DEFINE_ENUM(WAITER_RESULT, WAITER_RESULT_VALUES);

#define WAITER_CALLBACK_RESULT_VALUES \
    WAITER_CALLBACK_RESULT_OK, \
    WAITER_CALLBACK_RESULT_CANCELLED, \
    WAITER_CALLBACK_RESULT_ABANDONED

MU_DEFINE_ENUM(WAITER_CALLBACK_RESULT, WAITER_CALLBACK_RESULT_VALUES);

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void(*NOTIFICATION_CALLBACK)(void* context, THANDLE(RC_PTR) data, WAITER_CALLBACK_RESULT result);
typedef void(*NOTIFY_COMPLETE_CALLBACK)(void* context, WAITER_CALLBACK_RESULT result);
typedef struct WAITER_TAG WAITER;

THANDLE_TYPE_DECLARE(WAITER);

    MOCKABLE_FUNCTION(, THANDLE(WAITER), waiter_create)
    MOCKABLE_FUNCTION(, WAITER_RESULT, waiter_register_notification, THANDLE(WAITER), waiter, NOTIFICATION_CALLBACK, notification_callback, void*, context, THANDLE(ASYNC_OP)*, out_op);
    MOCKABLE_FUNCTION(, WAITER_RESULT, waiter_notify, THANDLE(WAITER), waiter, THANDLE(RC_PTR), data, NOTIFY_COMPLETE_CALLBACK, notify_complete_callback, void*, context, THANDLE(ASYNC_OP)*, out_op);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WAITER_H */
