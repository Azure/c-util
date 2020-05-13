// Copyright(C) Microsoft Corporation.All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cinttypes>
#include <cstdlib>
#else
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#endif

#include "windows.h"

#include "testrunnerswitcher.h"

#include "azure_macro_utils/macro_utils.h"

#include "azure_c_util/timer.h"
#include "azure_c_util/interlocked_hl.h"
#include "azure_c_logging/xlogging.h"

#include "azure_c_util/sm.h"

TEST_DEFINE_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);

#define N_MAX_THREADS MAXIMUM_WAIT_OBJECTS

typedef struct OPEN_CLOSE_THREADS_TAG
{
    SM_HANDLE sm;
    double startTime_ms;

    HANDLE beginOpenThreads[N_MAX_THREADS];
    HANDLE endOpenThreads[N_MAX_THREADS];
    volatile LONG n_begin_open_grants;
    volatile LONG n_begin_open_refuses;
    uint32_t n_begin_open_threads;
    uint32_t n_end_open_threads;

    HANDLE beginCloseThreads[N_MAX_THREADS];
    HANDLE endCloseThreads[N_MAX_THREADS];
    volatile LONG n_begin_close_grants;
    volatile LONG n_begin_close_refuses;
    uint32_t n_begin_close_threads;
    uint32_t n_end_close_threads;

    HANDLE beginBarrierThreads[N_MAX_THREADS];
    HANDLE endBarrierThreads[N_MAX_THREADS];
    volatile LONG n_begin_barrier_grants;
    volatile LONG n_begin_barrier_refuses;
    uint32_t n_begin_barrier_threads;
    uint32_t n_end_barrier_threads;

    HANDLE beginThreads[N_MAX_THREADS];
    volatile LONG n_begin_grants;
    volatile LONG n_begin_refuses;
    uint32_t n_begin_threads;

    volatile LONG threadsShouldFinish; /*this test is time bound*/
} OPEN_CLOSE_THREADS;

static  DWORD WINAPI callsBeginOpen(
    LPVOID lpThreadParameter
)
{
    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)lpThreadParameter;

    while (InterlockedAdd(&data->threadsShouldFinish, 0) == 0)
    {
        if (sm_open_begin(data->sm) == SM_EXEC_GRANTED)
        {
            (void)InterlockedIncrement(&data->n_begin_open_grants);
        }
        else
        {
            (void)InterlockedIncrement(&data->n_begin_open_refuses);
        }
    }
    return 0;
}

static void createBeginOpenThreads(OPEN_CLOSE_THREADS* data)
{
    uint32_t iBeginOpen;
    for (iBeginOpen = 0; iBeginOpen < data->n_begin_open_threads; iBeginOpen++)
    {
        data->beginOpenThreads[iBeginOpen] = CreateThread(NULL, 0, callsBeginOpen, data, 0, NULL);
        ASSERT_IS_NOT_NULL(data->beginOpenThreads[iBeginOpen]);
    }
}

static void waitAndDestroyBeginOpenThreads(OPEN_CLOSE_THREADS* data)
{
    if (data->n_begin_open_threads > 0)
    {
        DWORD dw = WaitForMultipleObjects(data->n_begin_open_threads, data->beginOpenThreads, TRUE, INFINITE);
        ASSERT_IS_TRUE((WAIT_OBJECT_0 <= dw) && (dw <= WAIT_OBJECT_0 + data->n_begin_open_threads));
    }

    for (uint32_t iBeginOpen = 0; iBeginOpen < data->n_begin_open_threads; iBeginOpen++)
    {
        (void)CloseHandle(data->beginOpenThreads[iBeginOpen]);
    }
}

static  DWORD WINAPI callsEndOpen(
    LPVOID lpThreadParameter
)
{
    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)lpThreadParameter;

    while (InterlockedAdd(&data->threadsShouldFinish, 0) == 0)
    {
        sm_open_end(data->sm, (rand()%2==0));
    }
    return 0;
}

static void createEndOpenThreads(OPEN_CLOSE_THREADS* data)
{
    uint32_t iEndOpen;
    for (iEndOpen = 0; iEndOpen < data->n_end_open_threads; iEndOpen++)
    {
        data->endOpenThreads[iEndOpen] = CreateThread(NULL, 0, callsEndOpen, data, 0, NULL);
        ASSERT_IS_NOT_NULL(data->endOpenThreads[iEndOpen]);
    }
}

static void waitAndDestroyEndOpenThreads(OPEN_CLOSE_THREADS* data)
{
    if (data->n_end_open_threads > 0)
    {
        DWORD dw = WaitForMultipleObjects(data->n_end_open_threads, data->endOpenThreads, TRUE, INFINITE);
        ASSERT_IS_TRUE((WAIT_OBJECT_0 <= dw) && (dw <= WAIT_OBJECT_0 + data->n_end_open_threads));
    }

    for (uint32_t iEndOpen = 0; iEndOpen < data->n_end_open_threads; iEndOpen++)
    {
        (void)CloseHandle(data->endOpenThreads[iEndOpen]);
    }
}


static  DWORD WINAPI callsBeginClose(
    LPVOID lpThreadParameter
)
{
    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)lpThreadParameter;

    while (InterlockedAdd(&data->threadsShouldFinish, 0) == 0)
    {
        if (sm_close_begin(data->sm) == SM_EXEC_GRANTED)
        {
            (void)InterlockedIncrement(&data->n_begin_close_grants);
        }
        else
        {
            (void)InterlockedIncrement(&data->n_begin_close_refuses);
        }
    }
    return 0;
}

static void createBeginCloseThreads(OPEN_CLOSE_THREADS* data)
{
    uint32_t iBeginClose;
    for (iBeginClose = 0; iBeginClose < data->n_begin_close_threads; iBeginClose++)
    {
        data->beginCloseThreads[iBeginClose] = CreateThread(NULL, 0, callsBeginClose, data, 0, NULL);
        ASSERT_IS_NOT_NULL(data->beginCloseThreads[iBeginClose]);
    }
}

static void waitAndDestroyBeginCloseThreads(OPEN_CLOSE_THREADS* data)
{
    if (data->n_begin_close_threads > 0)
    {
        DWORD dw = WaitForMultipleObjects(data->n_begin_close_threads, data->beginCloseThreads, TRUE, INFINITE);
        ASSERT_IS_TRUE((WAIT_OBJECT_0 <= dw) && (dw <= WAIT_OBJECT_0 + data->n_begin_close_threads));
    }

    for (uint32_t iBeginClose = 0; iBeginClose < data->n_begin_close_threads; iBeginClose++)
    {
        (void)CloseHandle(data->beginCloseThreads[iBeginClose]);
    }
}

static  DWORD WINAPI callsEndClose(
    LPVOID lpThreadParameter
)
{
    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)lpThreadParameter;

    while (InterlockedAdd(&data->threadsShouldFinish, 0) == 0)
    {
        sm_close_end(data->sm); /*might as well fail*/
    }
    return 0;
}

static void createEndCloseThreads(OPEN_CLOSE_THREADS* data)
{
    uint32_t iEndClose;
    for (iEndClose = 0; iEndClose < data->n_end_close_threads; iEndClose++)
    {
        data->endCloseThreads[iEndClose] = CreateThread(NULL, 0, callsEndClose, data, 0, NULL);
        ASSERT_IS_NOT_NULL(data->endCloseThreads[iEndClose]);
    }
}

static void waitAndDestroyEndCloseThreads(OPEN_CLOSE_THREADS* data)
{
    if (data->n_end_close_threads > 0)
    {
        DWORD dw = WaitForMultipleObjects(data->n_end_close_threads, data->endCloseThreads, TRUE, INFINITE);
        ASSERT_IS_TRUE((WAIT_OBJECT_0 <= dw) && (dw <= WAIT_OBJECT_0 + data->n_end_close_threads));
    }

    for (uint32_t iEndClose = 0; iEndClose < data->n_end_close_threads; iEndClose++)
    {
        (void)CloseHandle(data->endCloseThreads[iEndClose]);
    }
}

static  DWORD WINAPI callsBeginBarrier(
    LPVOID lpThreadParameter
)
{
    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)lpThreadParameter;

    while (InterlockedAdd(&data->threadsShouldFinish, 0) == 0)
    {
        if (sm_barrier_begin(data->sm) == SM_EXEC_GRANTED)
        {
            (void)InterlockedIncrement(&data->n_begin_barrier_grants);
        }
        else
        {
            (void)InterlockedIncrement(&data->n_begin_barrier_refuses);
        }
    }
    return 0;
}

static void createBeginBarrierThreads(OPEN_CLOSE_THREADS* data)
{
    uint32_t iBeginBarrier;
    for (iBeginBarrier = 0; iBeginBarrier < data->n_begin_barrier_threads; iBeginBarrier++)
    {
        data->beginBarrierThreads[iBeginBarrier] = CreateThread(NULL, 0, callsBeginBarrier, data, 0, NULL);
        ASSERT_IS_NOT_NULL(data->beginBarrierThreads[iBeginBarrier]);
    }
}

static void waitAndDestroyBeginBarrierThreads(OPEN_CLOSE_THREADS* data)
{
    if (data->n_begin_barrier_threads > 0)
    {
        DWORD dw = WaitForMultipleObjects(data->n_begin_barrier_threads, data->beginBarrierThreads, TRUE, INFINITE);
        ASSERT_IS_TRUE((WAIT_OBJECT_0 <= dw) && (dw <= WAIT_OBJECT_0 + data->n_begin_barrier_threads));
    }

    for (uint32_t iBeginBarrier = 0; iBeginBarrier < data->n_begin_barrier_threads; iBeginBarrier++)
    {
        (void)CloseHandle(data->beginBarrierThreads[iBeginBarrier]);
    }
}

static  DWORD WINAPI callsEndBarrier(
    LPVOID lpThreadParameter
)
{
    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)lpThreadParameter;

    while (InterlockedAdd(&data->threadsShouldFinish, 0) == 0)
    {
        sm_barrier_end(data->sm); /*might as well fail*/
    }
    return 0;
}

static void createEndBarrierThreads(OPEN_CLOSE_THREADS* data)
{
    uint32_t iEndBarrier;
    for (iEndBarrier = 0; iEndBarrier < data->n_end_barrier_threads; iEndBarrier++)
    {
        data->endBarrierThreads[iEndBarrier] = CreateThread(NULL, 0, callsEndBarrier, data, 0, NULL);
        ASSERT_IS_NOT_NULL(data->endBarrierThreads[iEndBarrier]);
    }
}

static void waitAndDestroyEndBarrierThreads(OPEN_CLOSE_THREADS* data)
{
    if (data->n_end_barrier_threads > 0)
    {
        DWORD dw = WaitForMultipleObjects(data->n_end_barrier_threads, data->endBarrierThreads, TRUE, INFINITE);
        ASSERT_IS_TRUE((WAIT_OBJECT_0 <= dw) && (dw <= WAIT_OBJECT_0 + data->n_end_barrier_threads));
    }

    for (uint32_t iEndBarrier = 0; iEndBarrier < data->n_end_barrier_threads; iEndBarrier++)
    {
        (void)CloseHandle(data->endBarrierThreads[iEndBarrier]);
    }
}

static  DWORD WINAPI callsBeginAndEnd(
    LPVOID lpThreadParameter
)
{
    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)lpThreadParameter;

    while (InterlockedAdd(&data->threadsShouldFinish, 0) == 0)
    {
        if (sm_exec_begin(data->sm) == SM_EXEC_GRANTED)
        {
            (void)InterlockedIncrement(&data->n_begin_grants);
            double startTime = timer_global_get_elapsed_ms();
            uint32_t pretend_to_do_something_time_in_ms = rand() % 10;
            while (timer_global_get_elapsed_ms() - startTime < pretend_to_do_something_time_in_ms)
            {
                /*well-pretend*/
            }
            sm_exec_end(data->sm);
        }
        else
        {
            (void)InterlockedIncrement(&data->n_begin_refuses);
        }
    }
    return 0;
}

static void createBeginAndEndThreads(OPEN_CLOSE_THREADS* data)
{
    uint32_t iBegin;
    for (iBegin = 0; iBegin < data->n_begin_threads; iBegin++)
    {
        data->beginThreads[iBegin] = CreateThread(NULL, 0, callsBeginAndEnd, data, 0, NULL);
        ASSERT_IS_NOT_NULL(data->beginThreads[iBegin]);
    }
}

static void waitAndDestroyBeginAndEndThreads(OPEN_CLOSE_THREADS* data)
{
    if (data->n_begin_threads > 0)
    {
        DWORD dw = WaitForMultipleObjects(data->n_begin_threads, data->beginThreads, TRUE, INFINITE);
        ASSERT_IS_TRUE((WAIT_OBJECT_0 <= dw) && (dw <= WAIT_OBJECT_0 + data->n_begin_threads));
    }

    for (uint32_t iBegin = 0; iBegin < data->n_begin_threads; iBegin++)
    {
        (void)CloseHandle(data->beginThreads[iBegin]);
    }
}

#define ARRAY_SIZE 1000000

/*how many threads maximum. This needs to be slightly higher than the number of CPU threads because we want to see interrupted threads*/
/*the tests will start from 1*/
/*for every number of threads (starting from 1... N_MAX_THREADS)*/
/*the test will spawn 0... N_MAX_THREADS of threads that do "non-barrier" operations*/
/*the rest of the threads will do "barrier" operations*/
/*all the threads write in an array of 1.000.000 elements. Each element contains 2 items: 1) a number 2) a type (true = barrier, false = non-barrier)*/

/*there's a shared index the array (starts at zero)*/
/*there's a shared "source of numbers" (starts at zero)*/

/*a non barrier thread granted execution will interlocked increment the index, interlocked increment the source of numbers and write it*/
/*a thread granted execution will interlocked increment the index, interlocked increment the source of numbers and write it*/

static volatile LONG barrier_grants = 0;
static volatile LONG64 barrier_refusals = 0; /*this is just for giggles*/

static volatile LONG non_barrier_grants = 0;
static volatile LONG64 non_barrier_refusals = 0;

typedef struct ONE_WRITE_TAG
{
    LONG what_was_source;
    bool is_barrier;
}ONE_WRITE;

typedef struct THREADS_COMMON_TAG
{
    volatile LONG nFinishedThreads;
    SM_HANDLE sm;

    volatile LONG source_of_numbers;
    volatile LONG current_index;
    ONE_WRITE writes[ARRAY_SIZE];

    double startTimems;

}THREADS_COMMON;

#define THREAD_TYPE_VALUES \
    THREAD_BARRIER, \
    THREAD_NON_BARRIER

MU_DEFINE_ENUM(THREAD_TYPE, THREAD_TYPE_VALUE)

static DWORD WINAPI barrier_thread(
    LPVOID lpThreadParameter
)
{
    THREADS_COMMON* data = (THREADS_COMMON*)lpThreadParameter;
    /*a non barrier thread granted execution will interlocked increment the index, interlocked increment the source of numbers and write it*/
    while (InterlockedAdd(&data->current_index, 0) < ARRAY_SIZE)
    {
        if (sm_barrier_begin(data->sm) == SM_EXEC_GRANTED)
        {
            LONG index = InterlockedIncrement(&data->current_index) - 1;
            LONG source = InterlockedIncrement(&data->source_of_numbers);
            (void)InterlockedIncrement(&barrier_grants);

            if (index >= ARRAY_SIZE)
            {
                sm_barrier_end(data->sm);
                break;
            }

            data->writes[index].what_was_source = source;
            data->writes[index].is_barrier = true;
            sm_barrier_end(data->sm);
        }
        else
        {
            (void)InterlockedIncrement64(&barrier_refusals);
        }
    }
    return 0;
}

static  DWORD WINAPI non_barrier_thread(
    LPVOID lpThreadParameter
)
{
    THREADS_COMMON* data = (THREADS_COMMON*)lpThreadParameter;
    /*a non barrier thread granted execution will interlocked increment the index, interlocked increment the source of numbers and write it*/
    while (InterlockedAdd(&data->current_index, 0) < ARRAY_SIZE)
    {
        if (sm_exec_begin(data->sm) == SM_EXEC_GRANTED)
        {
            LONG index = InterlockedIncrement(&data->current_index) - 1;
            LONG source = InterlockedIncrement(&data->source_of_numbers);
            (void)InterlockedIncrement(&non_barrier_grants);

            if (index >= ARRAY_SIZE)
            {
                sm_exec_end(data->sm);
                break;
            }

            data->writes[index].what_was_source = source;
            data->writes[index].is_barrier = false;
            sm_exec_end(data->sm);
        }
        else
        {
            /*not granted execution, so just hammer*/
            (void)InterlockedIncrement64(&non_barrier_refusals);
        }
    }
    return 0;
}

static void verify(THREADS_COMMON* data) /*ASSERTS*/
{
    volatile size_t i;
    volatile LONG maxBeforeBarrier = -1;
    for (i = 0; i < ARRAY_SIZE; i++)
    {
        if (!data->writes[i].is_barrier)
        {
            if (data->writes[i].what_was_source > maxBeforeBarrier)
            {
                maxBeforeBarrier = data->writes[i].what_was_source;
            }
        }
        else
        {
            ASSERT_IS_TRUE(data->writes[i].what_was_source > maxBeforeBarrier);
            maxBeforeBarrier = data->writes[i].what_was_source;
        }
    }
}

static SYSTEM_INFO systemInfo;
static DWORD dwNumberOfProcessors;

#define SM_APIS_VALUES      \
SM_OPEN_BEGIN,              \
SM_CLOSE_BEGIN,             \
SM_BEGIN,                   \
SM_BARRIER_BEGIN            \

MU_DEFINE_ENUM(SM_APIS, SM_APIS_VALUES)
MU_DEFINE_ENUM_STRINGS(SM_APIS, SM_APIS_VALUES)

#define SM_STATES_VALUES             \
SM_CREATED,                          \
SM_OPENING,                          \
SM_OPENED,                           \
SM_OPENED_DRAINING_TO_BARRIER,       \
SM_OPENED_DRAINING_TO_CLOSE,         \
SM_OPENED_BARRIER,                   \
SM_CLOSING                           \


MU_DEFINE_ENUM(SM_STATES, SM_STATES_VALUES)
MU_DEFINE_ENUM_STRINGS(SM_STATES, SM_STATES_VALUES)

BEGIN_TEST_SUITE(sm_int_tests)

TEST_SUITE_INITIALIZE(suite_init)
{
    GetSystemInfo(&systemInfo);
    dwNumberOfProcessors = systemInfo.dwNumberOfProcessors;
    ASSERT_IS_TRUE(dwNumberOfProcessors * 4 <= N_MAX_THREADS, "for systems with maaany processors, modify N_MAX_THREADS to be bigger");
}

/*tests aims to mindlessly execute the APIs.
at least 1 sm_open_begin and at least 1 sm_exec_begin are waited to happen*/
TEST_FUNCTION(sm_chaos)
{
    LogInfo("disabling logging for the duration of sm_chaos. Logging takes additional locks that \"might\" help the test pass");
    LOGGER_LOG toBeRestored = xlogging_get_log_function();
    xlogging_set_log_function(NULL);

    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)malloc(sizeof(OPEN_CLOSE_THREADS));
    ASSERT_IS_NOT_NULL(data);

    data->sm = sm_create(NULL);
    ASSERT_IS_NOT_NULL(data->sm);

    data->startTime_ms = timer_global_get_elapsed_ms();

    (void)InterlockedExchange(&data->threadsShouldFinish, 0);

    for (uint32_t nthreads = 1; nthreads <= min(4 * dwNumberOfProcessors, N_MAX_THREADS); nthreads*=2)
    {
        data->n_begin_open_threads = nthreads;
        data->n_end_open_threads = nthreads;
        data->n_begin_close_threads = nthreads;
        data->n_end_close_threads = nthreads;
        data->n_begin_barrier_threads = nthreads;
        data->n_end_barrier_threads = nthreads;
        data->n_begin_threads = nthreads;
        
        (void)InterlockedExchange(&data->threadsShouldFinish, 0);
        (void)InterlockedExchange(&data->n_begin_open_grants, 0);
        (void)InterlockedExchange(&data->n_begin_open_refuses, 0);
        (void)InterlockedExchange(&data->n_begin_close_grants, 0);
        (void)InterlockedExchange(&data->n_begin_close_refuses, 0);
        (void)InterlockedExchange(&data->n_begin_barrier_grants, 0);
        (void)InterlockedExchange(&data->n_begin_barrier_refuses, 0);
        (void)InterlockedExchange(&data->n_begin_grants, 0);
        (void)InterlockedExchange(&data->n_begin_refuses, 0);

        createBeginOpenThreads(data);
        createEndOpenThreads(data);
        createBeginCloseThreads(data);
        createEndCloseThreads(data);
        createBeginBarrierThreads(data);
        createEndBarrierThreads(data);
        createBeginAndEndThreads(data);

        Sleep(1000);
        uint32_t counterSleep = 1;
        LONG n_begin_open_grants_local;
        LONG n_begin_grants_local;
        while (
            (n_begin_open_grants_local=InterlockedAdd(&data->n_begin_open_grants, 0)), 
            (n_begin_grants_local = InterlockedAdd(&data->n_begin_grants, 0)),
            ((n_begin_open_grants_local==0) ||(n_begin_grants_local==0))
            )
        {
            LogInfo("Slept %" PRIu32 " ms, no sign of n_begin_open_grants=%" PRId32 ", n_begin_grants=%" PRId32 "", counterSleep * 1000, n_begin_open_grants_local, n_begin_grants_local);
            counterSleep++;
            Sleep(1000);
        }

        (void)InterlockedExchange(&data->threadsShouldFinish, 1);

        waitAndDestroyBeginAndEndThreads(data);

        /*there might be a sm_barrier_begin that is not followed by a sm_barrier_end. So this is calling it "just in case"*/
        
        sm_barrier_end(data->sm);

        /*there might be a sm_open_begin that is not followed by a sm_open_end. So this is calling it "just in case"*/
        sm_open_end(data->sm, true);

        waitAndDestroyEndBarrierThreads(data);
        waitAndDestroyBeginBarrierThreads(data);
        waitAndDestroyEndCloseThreads(data);
        waitAndDestroyBeginCloseThreads(data);
        waitAndDestroyEndOpenThreads(data);
        waitAndDestroyBeginOpenThreads(data);

        /*just in case anything needs to close*/

        LogInfo("nthreads=%" PRIu32 
            ", n_begin_open_grants=%" PRIu32 ", n_begin_open_refuses=%" PRIu32 
            ", n_begin_close_grants=%" PRIu32 ", n_begin_close_refuses=%" PRIu32 
            ", n_begin_barrier_grants=%" PRIu32 ", n_begin_barrier_refuses=%" PRIu32
            ", n_begin_grants=%" PRIu32 ", n_begin_refuses=%" PRIu32
            "",
            nthreads,
            InterlockedAdd(&data->n_begin_open_grants, 0),
            InterlockedAdd(&data->n_begin_open_refuses, 0),
            InterlockedAdd(&data->n_begin_close_grants, 0),
            InterlockedAdd(&data->n_begin_close_refuses, 0),
            InterlockedAdd(&data->n_begin_barrier_grants, 0),
            InterlockedAdd(&data->n_begin_barrier_refuses, 0),
            InterlockedAdd(&data->n_begin_grants, 0),
            InterlockedAdd(&data->n_begin_refuses, 0)
        );

        ASSERT_IS_TRUE(InterlockedAdd(&data->n_begin_open_grants, 0) >= 1);
    }

    free(data);

    xlogging_set_log_function(toBeRestored);
}

TEST_FUNCTION(sm_does_not_block)
{
    LogInfo("disabling logging for the duration of sm_does_not_block. Logging takes additional locks that \"might\" help the test pass");
    LOGGER_LOG toBeRestored = xlogging_get_log_function();
    xlogging_set_log_function(NULL);

    ///arrange
    THREADS_COMMON* data = (THREADS_COMMON*)malloc(sizeof(THREADS_COMMON));
    ASSERT_IS_NOT_NULL(data);
    data->sm = sm_create(NULL);
    ASSERT_IS_NOT_NULL(data->sm);

    ///act
    for (uint32_t nthreads = 1; nthreads <= min(4 * dwNumberOfProcessors, N_MAX_THREADS); nthreads*=2)
    {
        for(uint32_t n_barrier_threads=0; n_barrier_threads<=nthreads; n_barrier_threads+=4)
        {
            uint32_t n_non_barrier_threads = nthreads - n_barrier_threads;

            HANDLE barrierThreads[N_MAX_THREADS];
            (void)memset(barrierThreads, 0, sizeof(barrierThreads));

            HANDLE nonBarrierThreads[N_MAX_THREADS];
            (void)memset(nonBarrierThreads, 0, sizeof(nonBarrierThreads));

            (void)InterlockedExchange(&non_barrier_grants, 0);
            (void)InterlockedExchange64(&non_barrier_refusals, 0);
            (void)InterlockedExchange(&barrier_grants, 0);
            (void)InterlockedExchange64(&barrier_refusals, 0);
            (void)InterlockedExchange(&data->source_of_numbers, 0);
            (void)InterlockedExchange(&data->current_index, 0);
            (void)memset(data->writes, 0, sizeof(data->writes));

            data->startTimems = timer_global_get_elapsed_ms();

            ASSERT_IS_TRUE(sm_open_begin(data->sm) == SM_EXEC_GRANTED);
            sm_open_end(data->sm, true);

            LogInfo("\nnthreads=%" PRIu32 " n_barrier_threads=%" PRIu32 " n_non_barrier_threads=%" PRIu32 "", nthreads, n_barrier_threads, n_non_barrier_threads);

            /*create them barrier threads*/
            for (uint32_t iBarrier = 0; iBarrier < n_barrier_threads; iBarrier++)
            {
                barrierThreads[iBarrier] = CreateThread(NULL, 0, barrier_thread, data, 0, NULL);
                ASSERT_IS_NOT_NULL(barrierThreads[iBarrier]);
            }

            /*create them non barrier threads*/
            for (uint32_t iNonBarrier = 0; iNonBarrier < n_non_barrier_threads; iNonBarrier++)
            {
                nonBarrierThreads[iNonBarrier] = CreateThread(NULL, 0, non_barrier_thread, data, 0, NULL);
                ASSERT_IS_NOT_NULL(nonBarrierThreads[iNonBarrier]);
            }

            /*wait for them threads to finish*/
            if (n_barrier_threads > 0)
            {
                uint32_t waitResult = WaitForMultipleObjects(n_barrier_threads, barrierThreads, TRUE, INFINITE);
                ASSERT_IS_TRUE(
                    (WAIT_OBJECT_0 <= waitResult) &&
                    (waitResult <= WAIT_OBJECT_0 + n_barrier_threads - 1)
                );
            }
            if (n_non_barrier_threads > 0)
            {
                uint32_t waitResult = WaitForMultipleObjects(n_non_barrier_threads, nonBarrierThreads, TRUE, INFINITE);
                ASSERT_IS_TRUE(
                    (WAIT_OBJECT_0 <= waitResult) &&
                    (waitResult <= WAIT_OBJECT_0 + n_barrier_threads - 1)
                );
            }

            /*verify the all numbers written by barriers are greater than all previous numbers*/
            verify(data);

            LogInfo("took %f ms, non_barrier_grants=%" PRId32 ", non_barrier_refusals=%" PRId64 " barrier_grants=%" PRId32 ", barrier_refusals=%" PRId64 "", timer_global_get_elapsed_ms() - data->startTimems, 
                InterlockedAdd(&non_barrier_grants, 0), 
                InterlockedAdd64(&non_barrier_refusals, 0),
                InterlockedAdd(&barrier_grants, 0),
                InterlockedAdd64(&barrier_refusals, 0));

            ASSERT_IS_TRUE(sm_close_begin(data->sm) == SM_EXEC_GRANTED);
            sm_close_end(data->sm);
        }
    }

    ///assert - in "verify"

    ///clean
    sm_destroy(data->sm);
    free(data);

    xlogging_set_log_function(toBeRestored);
}

/*below tests aim to see that calling any API produces GRANT/REFUSED from any state*/
/*these are states
SM_CREATED	SM_CREATED(1)	SM_STATE_TAG
SM_OPENING	SM_OPENING(2)	SM_STATE_TAG
SM_OPENED	SM_OPENED(3)	SM_STATE_TAG
SM_OPENED_DRAINING_TO_BARRIER	SM_OPENED_DRAINING_TO_BARRIER(4)	SM_STATE_TAG
SM_OPENED_DRAINING_TO_CLOSE	SM_OPENED_DRAINING_TO_CLOSE(5)	SM_STATE_TAG
SM_OPENED_BARRIER	SM_OPENED_BARRIER(6)	SM_STATE_TAG
SM_CLOSING	SM_CLOSING(7)	SM_STATE_TAG

these are APIs:
sm_open_begin
sm_close_begin
sm_exec_begin
sm_barrier_begin
*/

#define THREAD_TO_BACK_DELAY 1000

typedef struct SM_GO_TO_STATE_TAG
{
    SM_HANDLE sm;
    uint32_t targetState;
    HANDLE threadTo;
    HANDLE threadBack;
}SM_GO_TO_STATE;

static DWORD WINAPI switchesState(
    LPVOID lpThreadParameter
)
{
    SM_GO_TO_STATE* goToState = (SM_GO_TO_STATE*)lpThreadParameter;

    LogInfo("set state thread: will now switch state to %" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_STATES, (SM_STATES)(SM_CREATED + goToState->targetState)));

    switch (goToState->targetState)
    {
        case 0:/*SM_CREATED*/
        {
            break;
        }
        case 1:/*SM_OPENING*/
        {
            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_open_begin(goToState->sm));
            break;
        }
        case 2:/*SM_OPENED*/
        {
            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_open_begin(goToState->sm));
            sm_open_end(goToState->sm, true);
            break;
        }
        case 3:/*SM_OPENED_DRAINING_TO_BARRIER*/
        {
            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_open_begin(goToState->sm));
            sm_open_end(goToState->sm, true);

            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_exec_begin(goToState->sm));

            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_barrier_begin(goToState->sm)); /*switches to draining mode*/
            break;

        }
        case 4:/*SM_OPENED_DRAINING_TO_CLOSE*/
        {
            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_open_begin(goToState->sm));
            sm_open_end(goToState->sm, true);

            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_exec_begin(goToState->sm));

            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_close_begin(goToState->sm)); /*switches to draining mode*/

            break;
        }
        case 5:/*SM_OPENED_BARRIER*/
        {
            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_open_begin(goToState->sm));
            sm_open_end(goToState->sm, true);

            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_barrier_begin(goToState->sm));
            break;
        }
        case 6:/*SM_CLOSING*/
        {
            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_open_begin(goToState->sm));
            sm_open_end(goToState->sm, true);

            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_close_begin(goToState->sm));
            break;
        }
        default:
        {
            ASSERT_FAIL("unknown state");
            break;
        }
    }

    

    return 0;
}


static void sm_gotostate(SM_GO_TO_STATE* goToState)
{
    goToState->threadTo = CreateThread(NULL, 0, switchesState, goToState, 0, NULL);
    ASSERT_IS_NOT_NULL(goToState->threadTo);
    /*depending on the requested state, the thread might have finished by now...*/
}


static DWORD WINAPI switchesToCreated(
    LPVOID lpThreadParameter
)
{
    SM_GO_TO_STATE* goToState = (SM_GO_TO_STATE*)lpThreadParameter;
    
    Sleep(2* THREAD_TO_BACK_DELAY);

    LogInfo("thread reset sate: will now switch state back to %" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_STATES, (SM_STATES)(SM_CREATED)));

    switch (goToState->targetState)
    {
        case 0:/*SM_CREATED*/
        {
            break;
        }
        case 1:/*SM_OPENING*/
        {
            sm_open_end(goToState->sm, true);
            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_close_begin(goToState->sm));
            sm_close_end(goToState->sm);
            break;
        }
        case 2:/*SM_OPENED*/
        {
            if (sm_close_begin(goToState->sm) == SM_EXEC_GRANTED)
            {
                sm_close_end(goToState->sm);
            }
            
            break;
        }
        case 3:/*SM_OPENED_DRAINING_TO_BARRIER*/
        {
            sm_exec_end(goToState->sm);
            Sleep(THREAD_TO_BACK_DELAY);
            sm_barrier_end(goToState->sm);

            if (sm_close_begin(goToState->sm) == SM_EXEC_GRANTED)
            {
                sm_close_end(goToState->sm);
            }
            break;

        }
        case 4:/*SM_OPENED_DRAINING_TO_CLOSE*/
        {
            sm_exec_end(goToState->sm);
            sm_close_end(goToState->sm);

            break;
        }
        case 5:/*SM_OPENED_BARRIER*/
        {
            sm_barrier_end(goToState->sm);

            if (sm_close_begin(goToState->sm) == SM_EXEC_GRANTED)
            {
                sm_close_end(goToState->sm);
            }
            break;
        }
        case 6:/*SM_CLOSING*/
        {
            sm_close_end(goToState->sm);
            break;
        }
        default:
        {
            ASSERT_FAIL("unknown state");
            break;
        }
    }
    return 0;
}

static void sm_gofromstate(SM_GO_TO_STATE* goToState)
{
    goToState->threadBack = CreateThread(NULL, 0, switchesToCreated, goToState, 0, NULL);
    ASSERT_IS_NOT_NULL(goToState->threadBack);
    /*depending on the requested state, the thread might have finished by now...*/
}




/*Tests_SRS_SM_02_050: [ If the state is SM_OPENED_BARRIER then sm_close_begin shall re-evaluate the state. ]*/
/*Tests_SRS_SM_02_051: [ If the state is SM_OPENED_DRAINING_TO_BARRIER then sm_close_begin shall re-evaluate the state. ]*/

/*tests different expected returns from different states of SM*/
/*at time=0                     a thread is started that switched state to the desired state to execute some API. This thread might block there. For example when the "to" state is SM_OPENED_DRAINING_TO_BARRIER. */
/*at time=0                     a thread is started. The thread waits 2*THREAD_TO_BACK_DELAY and then it will unstuck the thread above.*/
/*at time=THREAD_TO_BACK_DELAY  an API is executed, result is collected and asserted*/
/*at time = 2*THREAD_TO_BACK_DELAY the second thread unblocks execution and reverts execution to SM_CREATED*/

TEST_FUNCTION(STATE_and_API)
{
    SM_RESULT expected[][4]=
    {
                                                /*sm_open_begin*/       /*sm_close_begin*/      /*sm_exec_begin*/       /*sm_barrier_begin*/
        /*SM_CREATED*/                      {   SM_EXEC_GRANTED,        SM_EXEC_REFUSED,        SM_EXEC_REFUSED,        SM_EXEC_REFUSED     },
        /*SM_OPENING*/                      {   SM_EXEC_REFUSED,        SM_EXEC_REFUSED,        SM_EXEC_REFUSED,        SM_EXEC_REFUSED     },
        /*SM_OPENED*/                       {   SM_EXEC_REFUSED,        SM_EXEC_GRANTED,        SM_EXEC_GRANTED,        SM_EXEC_GRANTED     },
        /*SM_OPENED_DRAINING_TO_BARRIER*/   {   SM_EXEC_REFUSED,        SM_EXEC_GRANTED,        SM_EXEC_REFUSED,        SM_EXEC_REFUSED     },
        /*SM_OPENED_DRAINING_TO_CLOSE*/     {   SM_EXEC_REFUSED,        SM_EXEC_REFUSED,        SM_EXEC_REFUSED,        SM_EXEC_REFUSED     },
        /*SM_OPENED_BARRIER*/               {   SM_EXEC_REFUSED,        SM_EXEC_GRANTED,        SM_EXEC_REFUSED,        SM_EXEC_REFUSED     },
        /*SM_CLOSING*/                      {   SM_EXEC_REFUSED,        SM_EXEC_REFUSED,        SM_EXEC_REFUSED,        SM_EXEC_REFUSED     },
    };

    for (uint32_t i = 0 ; i < sizeof(expected) / sizeof(expected[0]); i++)
    {
        for (uint32_t j = 0; j < sizeof(expected[0]) / sizeof(expected[0][0]); j++)
        {
            SM_GO_TO_STATE goToState;
            goToState.sm = sm_create(NULL);
            ASSERT_IS_NOT_NULL(goToState.sm);
            goToState.targetState = i;

            LogInfo("going to state =%" PRI_MU_ENUM "; calling=%" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_STATES, (SM_STATES)(i + SM_CREATED)), MU_ENUM_VALUE(SM_APIS, (SM_APIS)(j + SM_OPEN_BEGIN)));
            sm_gotostate(&goToState);
            sm_gofromstate(&goToState);

            Sleep(THREAD_TO_BACK_DELAY);

            switch (j)
            {
                case 0:/*sm_open_begin*/
                {
                    ASSERT_ARE_EQUAL(SM_RESULT, expected[i][j], sm_open_begin(goToState.sm));
                    if (expected[i][j] == SM_EXEC_GRANTED)
                    {
                        sm_open_end(goToState.sm, true);
                    }
                    break;
                }
                case 1:/*sm_close_begin*/
                {
                    ASSERT_ARE_EQUAL(SM_RESULT, expected[i][j], sm_close_begin(goToState.sm));
                    if (expected[i][j] == SM_EXEC_GRANTED)
                    {
                        sm_close_end(goToState.sm);
                    }
                    break;
                }
                case 2:/*sm_exec_begin*/
                {
                    ASSERT_ARE_EQUAL(SM_RESULT, expected[i][j], sm_exec_begin(goToState.sm));
                    if (expected[i][j] == SM_EXEC_GRANTED)
                    {
                        sm_exec_end(goToState.sm);
                    }
                    break;
                }
                case 3:/*sm_barrier_begin*/
                {
                    ASSERT_ARE_EQUAL(SM_RESULT, expected[i][j], sm_barrier_begin(goToState.sm));
                    if (expected[i][j] == SM_EXEC_GRANTED)
                    {
                        sm_barrier_end(goToState.sm);
                    }
                    break;
                }
                default:
                {
                    ASSERT_FAIL("unknown action");
                }
            }

            ASSERT_IS_TRUE(WaitForSingleObject(goToState.threadTo, INFINITE) == WAIT_OBJECT_0);
            (void)CloseHandle(goToState.threadTo);
            
            ASSERT_IS_TRUE(WaitForSingleObject(goToState.threadBack, INFINITE) == WAIT_OBJECT_0);
            (void)CloseHandle(goToState.threadBack);

            sm_destroy(goToState.sm);
        }
    }

}

END_TEST_SUITE(sm_int_tests)
