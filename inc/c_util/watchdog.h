// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef WATCHDOG_H
#define WATCHDOG_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "c_pal/threadpool.h"
#include "c_pal/thandle.h"

#include "c_util/rc_string.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void(*WATCHDOG_EXPIRED_CALLBACK)(void* context, const char* message);

typedef struct WATCHDOG_TAG* WATCHDOG_HANDLE;

MOCKABLE_FUNCTION(, WATCHDOG_HANDLE, watchdog_start, THANDLE(THREADPOOL), threadpool, uint32_t, timeout_ms, THANDLE(RC_STRING), message, WATCHDOG_EXPIRED_CALLBACK, callback, void*, context);
MOCKABLE_FUNCTION(, void, watchdog_reset, WATCHDOG_HANDLE, watchdog);
MOCKABLE_FUNCTION(, void, watchdog_stop, WATCHDOG_HANDLE, watchdog);

#ifdef __cplusplus
}
#endif

#endif // WATCHDOG_H
