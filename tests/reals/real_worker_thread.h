// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_WORKER_THREAD_H
#define REAL_WORKER_THREAD_H

#include <stdint.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"


#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_WORKER_THREAD_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        worker_thread_create, \
        worker_thread_destroy, \
        worker_thread_open, \
        worker_thread_close, \
        worker_thread_schedule_process \
    )


#include "c_util/worker_thread.h"

WORKER_THREAD_HANDLE real_worker_thread_create(WORKER_FUNC worker_func, void* worker_func_context);
void real_worker_thread_destroy(WORKER_THREAD_HANDLE worker_thread);

int real_worker_thread_open(WORKER_THREAD_HANDLE worker_thread);
void real_worker_thread_close(WORKER_THREAD_HANDLE worker_thread);

WORKER_THREAD_SCHEDULE_PROCESS_RESULT real_worker_thread_schedule_process(WORKER_THREAD_HANDLE worker_thread);


#endif //REAL_WORKER_THREAD_H
