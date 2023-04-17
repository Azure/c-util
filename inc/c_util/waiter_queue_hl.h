// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef WAITER_QUEUE_HL_H
#define WAITER_QUEUE_HL_H

#ifdef __cplusplus
#include <cstdbool>
#else
#include <stdbool.h>
#endif


#include "macro_utils/macro_utils.h"

typedef struct WAITER_QUEUE_HL_TAG* WAITER_QUEUE_HL_HANDLE;

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

    MOCKABLE_FUNCTION(, WAITER_QUEUE_HL_HANDLE, waiter_queue_hl_create);
    MOCKABLE_FUNCTION(, void, waiter_queue_hl_destroy, WAITER_QUEUE_HL_HANDLE, waiter_queue_hl);

    MOCKABLE_FUNCTION(, int, waiter_queue_hl_push, WAITER_QUEUE_HL_HANDLE, waiter_queue_hl, POP_CALLBACK, pop_callback, void*, pop_callback_context);
    MOCKABLE_FUNCTION(, int, waiter_queue_hl_pop, WAITER_QUEUE_HL_HANDLE, waiter_queue_hl, void*, data);

#ifdef __cplusplus
}
#endif

#endif /*WAITER_QUEUE_HL_H*/

