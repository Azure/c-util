// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_QUEUE_PROCESSOR_H
#define REAL_QUEUE_PROCESSOR_H

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_QUEUE_PROCESSOR_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        queue_processor_create,\
        queue_processor_destroy,\
        queue_processor_open,\
        queue_processor_close,\
        queue_processor_schedule_item\
    )

#ifdef __cplusplus
extern "C"
{
#endif

    QUEUE_PROCESSOR_HANDLE real_queue_processor_create(THANDLE(THREADPOOL) threadpool);
    void real_queue_processor_destroy(QUEUE_PROCESSOR_HANDLE queue_processor);
    int real_queue_processor_open(QUEUE_PROCESSOR_HANDLE queue_processor, ON_QUEUE_PROCESSOR_ERROR on_error, void* on_error_context);
    void real_queue_processor_close(QUEUE_PROCESSOR_HANDLE queue_processor);

    int real_queue_processor_schedule_item(QUEUE_PROCESSOR_HANDLE queue_processor, QUEUE_PROCESSOR_PROCESS_ITEM process_item_func, void* process_item_context);

#ifdef __cplusplus
}
#endif

#endif //REAL_QUEUE_PROCESSOR_H
