// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef LOG_CRITICAL_AND_TERMINATE_H
#define LOG_CRITICAL_AND_TERMINATE_H

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_util/ps_util.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LogCriticalAndTerminate(FORMAT, ...) \
    do \
    { \
        static volatile int work_around_terminate_never_returning = 1; \
        LogError(FORMAT, ##__VA_ARGS__); \
        /* Codes_SRS_LOG_CRITICAL_AND_TERMINATE_01_001: [ LogCriticalAndTerminate shall call ps_util_terminate_process. ]*/ \
        work_around_terminate_never_returning ? ps_util_terminate_process():(void)0; \
    } while(0)

MOCKABLE_FUNCTION(, void, log_critical_terminate_process);

#ifdef __cplusplus
}
#endif

#endif /* LOG_CRITICAL_AND_TERMINATE_H */
