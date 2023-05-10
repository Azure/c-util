// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef WAITER_H
#define WAITER_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"

#include "c_util/async_op.h"

#define WAITER_RESULT_VALUES \
    WAITER_RESULT_OK, \
    WAITER_RESULT_CANCELED, \
    WAITER_RESULT_ABANDONED

MU_DEFINE_ENUM(WAITER_RESULT, WAITER_RESULT_VALUES);

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef void* void_ptr;
THANDLE_TYPE_DECLARE(void_ptr);
typedef struct WAITER_TAG* WAITER_HANDLE;
typedef void(*NOTIFICATION_CALLBACK)(void* context, THANDLE(void_ptr) data, WAITER_RESULT result);
typedef void(*NOTIFY_COMPLETE_CALLBACK)(void* context, WAITER_RESULT result);

    MOCKABLE_FUNCTION(, WAITER_HANDLE, waiter_create)
    MOCKABLE_FUNCTION(, void, waiter_destroy, WAITER_HANDLE, waiter)
    MOCKABLE_FUNCTION(, THANDLE(ASYNC_OP), waiter_register_notification, WAITER_HANDLE, waiter, NOTIFICATION_CALLBACK, notification_callback, void*, context);
    MOCKABLE_FUNCTION(, THANDLE(ASYNC_OP), waiter_notify, WAITER_HANDLE, waiter, THANDLE(void_ptr), data, NOTIFY_COMPLETE_CALLBACK, notify_complete_callback, void*, context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WAITER_H */
