// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef BATCH_QUEUE_H
#define BATCH_QUEUE_H

#ifndef __cplusplus
#include <stdint.h>
#else
#include <cstdint>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/execution_engine.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BATCH_QUEUE_ENQUEUE_RESULT_VALUES \
    BATCH_QUEUE_ENQUEUE_OK, \
    BATCH_QUEUE_ENQUEUE_INVALID_ARGS, \
    BATCH_QUEUE_ENQUEUE_INVALID_STATE, \
    BATCH_QUEUE_ENQUEUE_ERROR \

MU_DEFINE_ENUM(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_RESULT_VALUES)

#define BATCH_QUEUE_PROCESS_COMPLETE_RESULT_VALUES \
    BATCH_QUEUE_PROCESS_COMPLETE_OK, \
    BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED, \
    BATCH_QUEUE_PROCESS_COMPLETE_ERROR \

MU_DEFINE_ENUM(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_RESULT_VALUES)

#define BATCH_QUEUE_PROCESS_SYNC_RESULT_VALUES \
    BATCH_QUEUE_PROCESS_SYNC_OK, \
    BATCH_QUEUE_PROCESS_SYNC_NOT_OPEN, \
    BATCH_QUEUE_PROCESS_SYNC_INVALID_ARGS, \
    BATCH_QUEUE_PROCESS_SYNC_ERROR \

MU_DEFINE_ENUM(BATCH_QUEUE_PROCESS_SYNC_RESULT, BATCH_QUEUE_PROCESS_SYNC_RESULT_VALUES)

typedef void(*BATCH_QUEUE_ON_BATCH_FAULTED)(void* context);

typedef void(*BATCH_QUEUE_ON_ITEM_COMPLETE)(void* context, BATCH_QUEUE_PROCESS_COMPLETE_RESULT result, void* ll_result);

typedef void(*BATCH_QUEUE_ON_LL_BATCH_COMPLETE)(void* batch_context, BATCH_QUEUE_PROCESS_COMPLETE_RESULT result, void* ll_result);

typedef BATCH_QUEUE_PROCESS_SYNC_RESULT(*BATCH_QUEUE_PROCESS_BATCH)(void* process_batch_context, void** items, uint32_t item_count, BATCH_QUEUE_ON_LL_BATCH_COMPLETE on_ll_batch_complete, void* batch_context);

typedef struct BATCH_QUEUE_TAG* BATCH_QUEUE_HANDLE;

typedef struct BATCH_QUEUE_SETTINGS_TAG
{
    uint32_t max_pending_requests;
    uint32_t max_batch_size;
    uint32_t min_batch_size;
    uint32_t min_wait_time;
} BATCH_QUEUE_SETTINGS;

#define PRI_BATCH_QUEUE_SETTINGS \
    "sBATCH_QUEUE_SETTINGS{max_pending_requests=%" PRIu32 ", max_batch_size=%" PRIu32 ", min_batch_size=%" PRIu32 ", min_wait_time=%" PRIu32 "}"
#define BATCH_QUEUE_SETTINGS_VALUES(settings) \
    "", \
    settings.max_pending_requests, \
    settings.max_batch_size, \
    settings.min_batch_size, \
    settings.min_wait_time

MOCKABLE_FUNCTION(, BATCH_QUEUE_HANDLE, batch_queue_create, BATCH_QUEUE_SETTINGS, settings, EXECUTION_ENGINE_HANDLE, execution_engine, BATCH_QUEUE_PROCESS_BATCH, process_batch_func, void*, process_batch_context, BATCH_QUEUE_ON_BATCH_FAULTED, on_batch_faulted_func, void*, on_batch_faulted_context);
MOCKABLE_FUNCTION(, void, batch_queue_destroy, BATCH_QUEUE_HANDLE, batch_queue);

MOCKABLE_FUNCTION(, int, batch_queue_open, BATCH_QUEUE_HANDLE, batch_queue);
MOCKABLE_FUNCTION(, void, batch_queue_close, BATCH_QUEUE_HANDLE, batch_queue);

MOCKABLE_FUNCTION(, BATCH_QUEUE_ENQUEUE_RESULT, batch_queue_enqueue, BATCH_QUEUE_HANDLE, batch_queue, void*, item, uint32_t, item_size, BATCH_QUEUE_ON_ITEM_COMPLETE, on_item_complete, void*, context);

#ifdef __cplusplus
}
#endif

#endif /* BATCH_QUEUE_H */
