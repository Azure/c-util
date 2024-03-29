// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.See LICENSE file in the project root for full license information.

#ifndef TCALL_DISPATCHER_THREAD_NOTIFICATION_CALL_H
#define TCALL_DISPATCHER_THREAD_NOTIFICATION_CALL_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"

#include "thread_notifications_lackey_dll/thread_notifications_lackey_dll.h"

#include "c_util/tcall_dispatcher.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

TCALL_DISPATCHER_DEFINE_CALL_TYPE(THREAD_NOTIFICATION_CALL, THREAD_NOTIFICATIONS_LACKEY_DLL_REASON, thread_notification_reason);
THANDLE_TYPE_DECLARE(TCALL_DISPATCHER_TYPEDEF_NAME(THREAD_NOTIFICATION_CALL));
TCALL_DISPATCHER_TYPE_DECLARE(THREAD_NOTIFICATION_CALL, THREAD_NOTIFICATIONS_LACKEY_DLL_REASON, thread_notification_reason);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // TCALL_DISPATCHER_THREAD_NOTIFICATION_CALL_H
