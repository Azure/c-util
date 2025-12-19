// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef WATCHDOG_THREADPOOL_H
#define WATCHDOG_THREADPOOL_H

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

MOCKABLE_FUNCTION(, int, watchdog_threadpool_init);
MOCKABLE_FUNCTION(, void, watchdog_threadpool_deinit);

MOCKABLE_FUNCTION(, THANDLE(THREADPOOL), watchdog_threadpool_get);

#ifdef __cplusplus
}
#endif

#endif // WATCHDOG_THREADPOOL_H
