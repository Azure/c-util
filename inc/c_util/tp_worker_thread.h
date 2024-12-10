// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef TP_WORKER_THREAD_H
#define TP_WORKER_THREAD_H

#include "macro_utils/macro_utils.h"

#include "c_pal/execution_engine.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*TP_WORKER_THREAD_FUNC)(void* worker_func_context);

typedef struct TP_WORKER_THREAD_TAG* TP_WORKER_THREAD_HANDLE;

#define TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT_VALUES \
    TP_WORKER_THREAD_SCHEDULE_PROCESS_OK, \
    TP_WORKER_THREAD_SCHEDULE_PROCESS_INVALID_ARGS, \
    TP_WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE \

MU_DEFINE_ENUM(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT_VALUES)

MOCKABLE_FUNCTION(, TP_WORKER_THREAD_HANDLE, tp_worker_thread_create, EXECUTION_ENGINE_HANDLE, execution_engine, TP_WORKER_THREAD_FUNC, worker_func, void*, worker_func_context);
MOCKABLE_FUNCTION(, void, tp_worker_thread_destroy, TP_WORKER_THREAD_HANDLE, worker_thread);

MOCKABLE_FUNCTION(, int, tp_worker_thread_open, TP_WORKER_THREAD_HANDLE, worker_thread);
MOCKABLE_FUNCTION(, void, tp_worker_thread_close, TP_WORKER_THREAD_HANDLE, worker_thread);

MOCKABLE_FUNCTION(, TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, tp_worker_thread_schedule_process, TP_WORKER_THREAD_HANDLE, worker_thread);

#ifdef __cplusplus
}
#endif

#endif /* TP_WORKER_THREAD_H */
