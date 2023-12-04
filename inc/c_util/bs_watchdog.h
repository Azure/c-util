// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef BS_WATCHDOG_H
#define BS_WATCHDOG_H

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

typedef void(*BS_WATCHDOG_EXPIRED_CALLBACK)(void* context, const char* message);

typedef struct BS_WATCHDOG_TAG* BS_WATCHDOG_HANDLE;

MOCKABLE_FUNCTION(, BS_WATCHDOG_HANDLE, bs_watchdog_start, THANDLE(THREADPOOL), threadpool, uint32_t, timeout_ms, THANDLE(RC_STRING), message, BS_WATCHDOG_EXPIRED_CALLBACK, callback, void*, context);
MOCKABLE_FUNCTION(, void, bs_watchdog_reset, BS_WATCHDOG_HANDLE, watchdog);
MOCKABLE_FUNCTION(, void, bs_watchdog_stop, BS_WATCHDOG_HANDLE, watchdog);

#ifdef __cplusplus
}
#endif

#endif // BS_WATCHDOG_H
