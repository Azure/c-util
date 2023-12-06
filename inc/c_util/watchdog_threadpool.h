// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef BS_WATCHDOG_THREADPOOL_H
#define BS_WATCHDOG_THREADPOOL_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "c_pal/threadpool.h"
#include "c_pal/thandle.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, int, bs_watchdog_threadpool_init);
MOCKABLE_FUNCTION(, void, bs_watchdog_threadpool_deinit);

MOCKABLE_FUNCTION(, THANDLE(THREADPOOL), bs_watchdog_threadpool_get);

#ifdef __cplusplus
}
#endif

#endif // BS_WATCHDOG_THREADPOOL_H
