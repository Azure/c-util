// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/gballoc.h"
#include <stdint.h>
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/xlogging.h"

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif


#ifndef __MACH__
clockid_t time_basis = -1;
#endif

void set_time_basis(void)
{
// The time basis depends on what clock is available. Prefer CLOCK_MONOTONIC,
// then CLOCK_REALTIME, otherwise query the default pthread_condattr_t value
// and use that. Note the time basis stuff requires _POSIX_TIMERS [TMR] at a
// minimum; querying pthread_condattr_t requires _POSIX_CLOCK_SELECTION [CS].
// OSX has neither so we use a platform-specific clock.
#ifndef __MACH__
#if defined(CLOCK_MONOTONIC)
    time_basis = CLOCK_MONOTONIC;
#elif defined(CLOCK_REALTIME)
    time_basis = CLOCK_REALTIME;
#else
    pthread_condattr_t cattr;
    pthread_condattr_init(&cattr);
    pthread_condattr_getclock(&cattr, &time_basis);
    pthread_condattr_destroy(&cattr);
#endif
#endif
}

int get_time_ns(struct timespec* ts)
{
    int err;

#ifdef __MACH__
    clock_serv_t cclock;
    mach_timespec_t mts;
    err = host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    if (!err)
    {
        err = clock_get_time(cclock, &mts);
        mach_port_deallocate(mach_task_self(), cclock);

        if (!err)
        {
            ts->tv_sec = mts.tv_sec;
            ts->tv_nsec = mts.tv_nsec;
        }
    }
#else
    err = clock_gettime(time_basis, ts);
#endif
    return err;
}

time_t get_time_s()
{
    struct timespec ts;
    if (get_time_ns(&ts) != 0)
    {
        LogError("Failed to get the current time");
        return INVALID_TIME_VALUE;
    }

    return (time_t)ts.tv_sec;
}

typedef struct TICK_COUNTER_INSTANCE_TAG
{
    time_t init_time_value;
    tickcounter_ms_t current_ms;
} TICK_COUNTER_INSTANCE;

TICK_COUNTER_HANDLE tickcounter_create(void)
{
    TICK_COUNTER_INSTANCE* result = (TICK_COUNTER_INSTANCE*)malloc(sizeof(TICK_COUNTER_INSTANCE));
    if (result != NULL)
    {
        set_time_basis();

        result->init_time_value = get_time_s();
        if (result->init_time_value == INVALID_TIME_VALUE)
        {
            LogError("tickcounter failed: time return INVALID_TIME.");
            free(result);
            result = NULL;
        }
        else
        {
            result->current_ms = 0;
        }
    }
    return result;
}

void tickcounter_destroy(TICK_COUNTER_HANDLE tick_counter)
{
    if (tick_counter != NULL)
    {
        free(tick_counter);
    }
}

int tickcounter_get_current_ms(TICK_COUNTER_HANDLE tick_counter, tickcounter_ms_t * current_ms)
{
    int result;

    if (tick_counter == NULL || current_ms == NULL)
    {
        LogError("tickcounter failed: Invalid Arguments.");
        result = MU_FAILURE;
    }
    else
    {
        time_t time_value = get_time_s();
        if (time_value == INVALID_TIME_VALUE)
        {
            result = MU_FAILURE;
        }
        else
        {
            TICK_COUNTER_INSTANCE* tick_counter_instance = (TICK_COUNTER_INSTANCE*)tick_counter;
            tick_counter_instance->current_ms = (tickcounter_ms_t)(difftime(time_value, tick_counter_instance->init_time_value) * 1000);
            *current_ms = tick_counter_instance->current_ms;
            result = 0;
        }
    }

    return result;
}
