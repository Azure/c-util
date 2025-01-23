// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef QUEUE_PROCESSOR_H
#define QUEUE_PROCESSOR_H

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "c_pal/threadpool.h"
#include "c_pal/thandle.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct QUEUE_PROCESSOR_TAG* QUEUE_PROCESSOR_HANDLE;

typedef void(*ON_QUEUE_PROCESSOR_ERROR)(void* context);
typedef void(*QUEUE_PROCESSOR_PROCESS_ITEM)(void* context);

MOCKABLE_FUNCTION(, QUEUE_PROCESSOR_HANDLE, queue_processor_create, THANDLE(THREADPOOL), threadpool);
MOCKABLE_FUNCTION(, void, queue_processor_destroy, QUEUE_PROCESSOR_HANDLE, queue_processor);
MOCKABLE_FUNCTION(, int, queue_processor_open, QUEUE_PROCESSOR_HANDLE, queue_processor, ON_QUEUE_PROCESSOR_ERROR, on_error, void*, on_error_context);
MOCKABLE_FUNCTION(, void, queue_processor_close, QUEUE_PROCESSOR_HANDLE, queue_processor);

MOCKABLE_FUNCTION(, int, queue_processor_schedule_item, QUEUE_PROCESSOR_HANDLE, queue_processor, QUEUE_PROCESSOR_PROCESS_ITEM, process_item_func, void*, process_item_context);

#ifdef __cplusplus
}
#endif

#endif /* QUEUE_PROCESSOR_H */
