// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_THREAD_NOTIFICATIONS_LACKEY_DLL_H
#define REAL_THREAD_NOTIFICATIONS_LACKEY_DLL_H

#include "macro_utils/macro_utils.h"

#include "../../thread_notifications_lackey_dll/inc/thread_notifications_lackey_dll/thread_notifications_lackey_dll.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_THREAD_NOTIFICATIONS_LACKEY_DLL_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        thread_notifications_lackey_dll_init_callback, \
        thread_notifications_lackey_dll_deinit_callback \
    )

int real_thread_notifications_lackey_dll_init_callback(THREAD_NOTIFICATIONS_LACKEY_DLL_CALLBACK_FUNC thread_notifications_cb);
void real_thread_notifications_lackey_dll_deinit_callback(void);

#endif // REAL_THREAD_NOTIFICATIONS_LACKEY_DLL_H
