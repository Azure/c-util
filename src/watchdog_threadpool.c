// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdint.h>

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle.h"

#include "c_util/watchdog_threadpool.h"

typedef struct WATCHDOG_THREADPOOL_TAG
{
    THANDLE(THREADPOOL) threadpool;
} WATCHDOG_THREADPOOL;

#define WATCHDOG_INIT_STATE_VALUES \
        WATCHDOG_INIT_NOT_INIT, \
        WATCHDOG_INIT_INITIALIZING, \
        WATCHDOG_INIT_OK, \
        WATCHDOG_INIT_DEINITIALIZING

MU_DEFINE_ENUM(WATCHDOG_INIT_STATE, WATCHDOG_INIT_STATE_VALUES);
MU_DEFINE_ENUM_STRINGS(WATCHDOG_INIT_STATE, WATCHDOG_INIT_STATE_VALUES);

static WATCHDOG_THREADPOOL g_watchdog;
static volatile_atomic int32_t g_watchdog_init_state = WATCHDOG_INIT_NOT_INIT;


int watchdog_threadpool_init(void)
{
    int result;

    WATCHDOG_INIT_STATE state = interlocked_compare_exchange(&g_watchdog_init_state, WATCHDOG_INIT_INITIALIZING, WATCHDOG_INIT_NOT_INIT);
    if (state != WATCHDOG_INIT_NOT_INIT)
    {
        /*Codes_SRS_WATCHDOG_THREADPOOL_42_001: [ If the watchdog threadpool has already been initialized then watchdog_threadpool_init shall fail and return a non-zero value. ]*/
        LogError("Cannot init from state %" PRI_MU_ENUM,
            MU_ENUM_VALUE(WATCHDOG_INIT_STATE, state));
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_WATCHDOG_THREADPOOL_42_002: [ watchdog_threadpool_init shall create an execution engine by calling execution_engine_create. ]*/
        EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);

        if (execution_engine == NULL)
        {
            /*Codes_SRS_WATCHDOG_THREADPOOL_42_007: [ If there are any other errors then watchdog_threadpool_init shall fail and return a non-zero value. ]*/
            LogError("execution_engine_create failed");
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_WATCHDOG_THREADPOOL_42_003: [ watchdog_threadpool_init shall create a threadpool by calling threadpool_create. ]*/
            THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
            if (threadpool == NULL)
            {
                /*Codes_SRS_WATCHDOG_THREADPOOL_42_007: [ If there are any other errors then watchdog_threadpool_init shall fail and return a non-zero value. ]*/
                LogError("threadpool_create failed");
                result = MU_FAILURE;
            }
            else
            {
                /*Codes_SRS_WATCHDOG_THREADPOOL_42_006: [ watchdog_threadpool_init shall store the threadpool. ]*/
                THANDLE_INITIALIZE_MOVE(THREADPOOL)(&g_watchdog.threadpool, &threadpool);

                (void)interlocked_exchange(&g_watchdog_init_state, WATCHDOG_INIT_OK);

                /*Codes_SRS_WATCHDOG_THREADPOOL_42_008: [ watchdog_threadpool_init shall return 0. ]*/
                result = 0;
            }

            execution_engine_dec_ref(execution_engine); /*no longer needed*/

        }
        (result != 0) ? interlocked_exchange(&g_watchdog_init_state, WATCHDOG_INIT_NOT_INIT) : 0;
    }
    return result;
}

void watchdog_threadpool_deinit(void)
{
    WATCHDOG_INIT_STATE state = interlocked_compare_exchange(&g_watchdog_init_state, WATCHDOG_INIT_DEINITIALIZING, WATCHDOG_INIT_OK);
    if (state != WATCHDOG_INIT_OK)
    {
        /*Codes_SRS_WATCHDOG_THREADPOOL_42_009: [ If the watchdog threadpool has not been initialized then watchdog_deinit shall return. ]*/
    }
    else
    {
        /*Codes_SRS_WATCHDOG_THREADPOOL_42_011: [ watchdog_threadpool_deinit shall destroy the threadpool by assign threadpool to NULL. ]*/
        THANDLE_ASSIGN(THREADPOOL)(&g_watchdog.threadpool, NULL);

        /*Codes_SRS_WATCHDOG_THREADPOOL_42_013: [ After watchdog_threadpool_deinit returns then watchdog_threadpool_init may be called again. ]*/
        (void)interlocked_exchange(&g_watchdog_init_state, WATCHDOG_INIT_NOT_INIT);
    }
}

THANDLE(THREADPOOL) watchdog_threadpool_get(void)
{
    THANDLE(THREADPOOL) result = NULL;

    WATCHDOG_INIT_STATE state = interlocked_add(&g_watchdog_init_state, 0);
    if (state != WATCHDOG_INIT_OK)
    {
        /*Codes_SRS_WATCHDOG_THREADPOOL_42_015: [ If the watchdog threadpool has not been initialized then watchdog_threadpool_get shall return NULL. ]*/
        LogError("Cannot get threadpool from state %" PRI_MU_ENUM,
            MU_ENUM_VALUE(WATCHDOG_INIT_STATE, state));
    }
    else
    {
        /*Codes_SRS_WATCHDOG_THREADPOOL_42_014: [ watchdog_threadpool_get shall return the threadpool created by watchdog_threadpool_init. ]*/
        THANDLE_INITIALIZE(THREADPOOL)(&result, g_watchdog.threadpool);
    }
    return result;
}
