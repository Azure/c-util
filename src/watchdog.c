// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/threadpool.h"
#include "c_pal/log_critical_and_terminate.h"
#include "c_util/rc_string.h"
#include "c_pal/thandle.h"

#include "c_util/watchdog.h"

#define WATCHDOG_STATE_VALUES \
        WATCHDOG_RUNNING, \
        WATCHDOG_EXPIRING, \
        WATCHDOG_STOP

MU_DEFINE_ENUM(WATCHDOG_STATE, WATCHDOG_STATE_VALUES);

typedef struct WATCHDOG_TAG
{
    WATCHDOG_EXPIRED_CALLBACK callback;
    void* context;

    volatile_atomic int32_t state; // WATCHDOG_STATE
    uint32_t timeout_ms;
    TIMER_INSTANCE_HANDLE timer;
    THANDLE(RC_STRING) message;
} WATCHDOG;

static void watchdog_expired_callback(void* context)
{

    if (context == NULL)
    {
        /*Codes_SRS_WATCHDOG_42_027: [ If context is NULL then watchdog_expired_callback shall terminate the process. ]*/
        LogCriticalAndTerminate("Invalid args: void* context = %p",
            context);
    }
    else
    {
        WATCHDOG_HANDLE handle = context;
        /* Codes_SRS_WATCHDOG_45_005: [ If the state of the watchdog is RUNNING then ]*/
          /* Codes_SRS_WATCHDOG_45_001: [ watchdog_expired_callback shall set the state to EXPIRING ]*/
        WATCHDOG_STATE state = interlocked_compare_exchange(&handle->state, WATCHDOG_EXPIRING, WATCHDOG_RUNNING);

        if (state == WATCHDOG_RUNNING)
        {
            /*Codes_SRS_WATCHDOG_42_021: [ watchdog_expired_callback shall call callback with the context and message from watchdog_start. ]*/
            LogError("Watchdog timer %p fired!", handle);
            handle->callback(handle->context, (handle->message == NULL ? "" : handle->message->string));
            /* Codes_SRS_WATCHDOG_45_002: [ watchdog_expired_callback shall return the state to RUNNING. ]*/
            (void)InterlockedHL_SetAndWake(&handle->state, WATCHDOG_RUNNING);
        }
        else
        {
            LogVerbose("Watchdog timer %p fired after stop", handle);
        }
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, WATCHDOG_HANDLE, watchdog_start, THANDLE(THREADPOOL), threadpool, uint32_t, timeout_ms, THANDLE(RC_STRING), message, WATCHDOG_EXPIRED_CALLBACK, callback, void*, context)
{
    WATCHDOG_HANDLE result;

    if (
        /*Codes_SRS_WATCHDOG_42_029: [ If threadpool is NULL then watchdog_start shall fail and return NULL. ]*/
        threadpool == NULL ||
        /*Codes_SRS_WATCHDOG_42_030: [ If callback is NULL then watchdog_start shall fail and return NULL. ]*/
        callback == NULL
        )
    {
        LogError("Invalid args: THANDLE(THREADPOOL) threadpool = %p, uint32_t timeout_ms = %" PRIu32 ", THANDLE(RC_STRING) message = %" PRI_RC_STRING ", WATCHDOG_EXPIRED_CALLBACK callback = %p, void* context = %p",
            threadpool, timeout_ms, RC_STRING_VALUE_OR_NULL(message), callback, context);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_WATCHDOG_42_016: [ watchdog_start shall allocate memory for the WATCHDOG_HANDLE. ]*/
        result = malloc(sizeof(WATCHDOG));

        if (result == NULL)
        {
            /*Codes_SRS_WATCHDOG_42_019: [ If there are any errors then watchdog_start shall fail and return NULL. ]*/
            LogError("malloc(sizeof(WATCHDOG)) failed");
        }
        else
        {
            /*Codes_SRS_WATCHDOG_42_028: [ watchdog_start shall store the message. ]*/
            THANDLE_INITIALIZE(RC_STRING)(&result->message, message);

            result->callback = callback;
            result->context = context;
            result->timeout_ms = timeout_ms;

            /*Codes_SRS_WATCHDOG_42_017: [ watchdog_start shall set the state of the watchdog to RUNNING. ]*/
            (void)interlocked_exchange(&result->state, WATCHDOG_RUNNING);

            LogVerbose("Starting watchdog %p", result);

            /*Codes_SRS_WATCHDOG_42_018: [ watchdog_start shall create a timer that expires after timeout_ms by calling threadpool_timer_start with watchdog_expired_callback as the callback. ]*/
            if (threadpool_timer_start(threadpool, timeout_ms, 0, watchdog_expired_callback, result, &result->timer) != 0)
            {
                /*Codes_SRS_WATCHDOG_42_019: [ If there are any errors then watchdog_start shall fail and return NULL. ]*/
                LogError("threadpool_timer_start failed");
            }
            else
            {
                /*Codes_SRS_WATCHDOG_42_020: [ watchdog_start shall succeed and return the allocated handle. ]*/
                goto all_ok;
            }

            THANDLE_ASSIGN(RC_STRING)(&result->message, NULL);
            free(result);
            result = NULL;
        }
    }

all_ok:
    return result;
}


IMPLEMENT_MOCKABLE_FUNCTION(, void, watchdog_reset, WATCHDOG_HANDLE, watchdog)
{
    if (watchdog == NULL)
    {
        /*Codes_SRS_WATCHDOG_42_031: [ If watchdog is NULL then watchdog_reset shall return. ]*/
        LogError("Invalid args: WATCHDOG_HANDLE watchdog = %p", watchdog);
    }
    else
    {
        /* Codes_SRS_WATCHDOG_45_003: [ watchdog_reset shall wait until state is not EXPIRING. ]*/
        (void)InterlockedHL_WaitForNotValue(&watchdog->state, WATCHDOG_EXPIRING, UINT32_MAX);
        /*Codes_SRS_WATCHDOG_42_032: [ watchdog_reset shall set the state of the watchdog to STOP. ]*/
        (void)interlocked_exchange(&watchdog->state, WATCHDOG_STOP);

        LogVerbose("Stopping watchdog for reset %p", watchdog);

        /*Codes_SRS_WATCHDOG_42_033: [ watchdog_reset shall cancel the current timer by calling threadpool_timer_cancel. ]*/
        threadpool_timer_cancel(watchdog->timer);

        LogVerbose("Restarting watchdog %p", watchdog);

        /*Codes_SRS_WATCHDOG_42_034: [ watchdog_reset shall set the state of the watchdog to RUNNING. ]*/
        (void)interlocked_exchange(&watchdog->state, WATCHDOG_RUNNING);

        /*Codes_SRS_WATCHDOG_42_035: [ watchdog_reset shall restart the timer by calling threadpool_timer_restart with the original timeout_ms from the call to start. ]*/
        (void)threadpool_timer_restart(watchdog->timer, watchdog->timeout_ms, 0);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, watchdog_stop, WATCHDOG_HANDLE, watchdog)
{
    if (watchdog == NULL)
    {
        /*Codes_SRS_WATCHDOG_42_022: [ If watchdog is NULL then watchdog_stop shall return. ]*/
        LogError("Invalid args: WATCHDOG_HANDLE watchdog = %p", watchdog);
    }
    else
    {
        /* Codes_SRS_WATCHDOG_45_004: [ watchdog_stop shall wait until state is not EXPIRING. ]*/
        (void)InterlockedHL_WaitForNotValue(&watchdog->state, WATCHDOG_EXPIRING, UINT32_MAX);

        /*Codes_SRS_WATCHDOG_42_023: [ watchdog_stop shall set the state of the watchdog to STOP. ]*/
        (void)interlocked_exchange(&watchdog->state, WATCHDOG_STOP);

        /*Codes_SRS_WATCHDOG_42_024: [ watchdog_stop shall stop and cleanup the timer by calling threadpool_timer_destroy. ]*/
        threadpool_timer_destroy(watchdog->timer);

        /*Codes_SRS_WATCHDOG_42_025: [ watchdog_stop shall free the watchdog. ]*/
        THANDLE_ASSIGN(RC_STRING)(&watchdog->message, NULL);
        free(watchdog);
    }
}
