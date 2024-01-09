// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_THREAD_NOTIFICATION_LACKEY_DLL_H
#define REAL_THREAD_NOTIFICATION_LACKEY_DLL_H

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_ASYNC_OP_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        thread_notifications_lackey_dll_init_callback, \
        thread_notifications_lackey_dll_deinit_callback, \
        DllMain \
    ) 

#ifdef __cplusplus
extern "C" {
#endif

int real_thread_notifications_lackey_dll_init_callback(THREAD_NOTIFICATIONS_LACKEY_DLL_CALLBACK_FUNC thread_notifications_cb);
void real_thread_notifications_lackey_dll_deinit_callback(void);
BOOL WINAPI real_DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // REAL_THREAD_NOTIFICATION_LACKEY_DLL_H
