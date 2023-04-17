// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef WAITER_QUEUE_LL_H
#define WAITER_QUEUE_LL_H

#ifdef __cplusplus
#include <cstdbool>
#else
#include <stdbool.h>
#endif


#include "macro_utils/macro_utils.h"

#define WAITER_QUEUE_CALL_REASON_VALUES \
    WAITER_QUEUE_CALL_REASON_POPPED, \
    WAITER_QUEUE_CALL_REASON_ABANDONED

MU_DEFINE_ENUM(WAITER_QUEUE_CALL_REASON, WAITER_QUEUE_CALL_REASON_VALUES);

typedef struct WAITER_QUEUE_LL_TAG* WAITER_QUEUE_LL_HANDLE;
typedef bool(*POP_CALLBACK)(void* context, void* data, bool* continue_processing, WAITER_QUEUE_CALL_REASON reason);

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

    MOCKABLE_FUNCTION(, WAITER_QUEUE_LL_HANDLE, waiter_queue_ll_create);
    MOCKABLE_FUNCTION(, void, waiter_queue_ll_destroy, WAITER_QUEUE_LL_HANDLE, waiter_queue_ll);

    MOCKABLE_FUNCTION(, int, waiter_queue_ll_push, WAITER_QUEUE_LL_HANDLE, waiter_queue_ll, POP_CALLBACK, pop_callback, void*, pop_callback_context);
    MOCKABLE_FUNCTION(, int, waiter_queue_ll_pop, WAITER_QUEUE_LL_HANDLE, waiter_queue_ll, void*, data);
    MOCKABLE_FUNCTION(, void, waiter_queue_ll_abandon, WAITER_QUEUE_LL_HANDLE, waiter_queue_ll);

#ifdef __cplusplus
}
#endif

#endif /*WAITER_QUEUE_LL_H*/


