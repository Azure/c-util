// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif 

    /* this is the callback that is to be called each time processing (worker executing) is scheduled */
    /* Note that if a processing (worker executing) is scheduled already, a new one might not be scheduled 
    (the number of executed WORKER_FUNC calls is <= number of worker_thread_schedule_process calls made ) */
    typedef void(*WORKER_FUNC)(void* worker_func_context);

    typedef struct WORKER_THREAD_TAG* WORKER_THREAD_HANDLE;

    #define WORKER_THREAD_SCHEDULE_PROCESS_RESULT_VALUES \
        WORKER_THREAD_SCHEDULE_PROCESS_OK, \
        WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE, \
        WORKER_THREAD_SCHEDULE_PROCESS_ERROR \

    MU_DEFINE_ENUM(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_RESULT_VALUES)

    MOCKABLE_FUNCTION(, WORKER_THREAD_HANDLE, worker_thread_create, WORKER_FUNC, worker_func, void*, worker_func_context);
    MOCKABLE_FUNCTION(, void, worker_thread_destroy, WORKER_THREAD_HANDLE, worker_thread);

    MOCKABLE_FUNCTION(, int, worker_thread_open, WORKER_THREAD_HANDLE, worker_thread);
    MOCKABLE_FUNCTION(, void, worker_thread_close, WORKER_THREAD_HANDLE, worker_thread);

    MOCKABLE_FUNCTION(, WORKER_THREAD_SCHEDULE_PROCESS_RESULT, worker_thread_schedule_process, WORKER_THREAD_HANDLE, worker_thread);

#ifdef __cplusplus
}
#endif

#endif // WORKER_THREAD_H
