// Copyright(C) Microsoft Corporation.All rights reserved.

#ifdef __cplusplus
#include <cinttypes>
#else
#include <inttypes.h>
#include <stdbool.h>
#endif

#include "windows.h"

#include "testrunnerswitcher.h"

#include "azure_c_util/timer.h"
#include "azure_c_util/interlocked_hl.h"

#include "azure_c_util/sm.h"

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

#define N_MAX_THREADS 32

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
            InterlockedIncrement(&data->n_begin_open_grants);
        }
        else
        {
            InterlockedIncrement(&data->n_begin_open_refuses);
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
        sm_open_end(data->sm); /*might as well fail*/
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
            InterlockedIncrement(&data->n_begin_close_grants);
        }
        else
        {
            InterlockedIncrement(&data->n_begin_close_refuses);
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
            InterlockedIncrement(&data->n_begin_barrier_grants);
        }
        else
        {
            InterlockedIncrement(&data->n_begin_barrier_refuses);
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
        if (sm_begin(data->sm) == SM_EXEC_GRANTED)
        {
            InterlockedIncrement(&data->n_begin_grants);
            double startTime = timer_global_get_elapsed_ms();
            uint32_t pretend_to_do_something_time_in_ms = rand() % 10;
            while (timer_global_get_elapsed_ms() - startTime < pretend_to_do_something_time_in_ms)
            {
                /*well-pretend*/
            }
            sm_end(data->sm);
        }
        else
        {
            InterlockedIncrement(&data->n_begin_refuses);
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
            InterlockedIncrement(&barrier_grants);

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
            InterlockedIncrement64(&barrier_refusals);
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
        if (sm_begin(data->sm) == SM_EXEC_GRANTED)
        {
            LONG index = InterlockedIncrement(&data->current_index) - 1;
            LONG source = InterlockedIncrement(&data->source_of_numbers);
            InterlockedIncrement(&non_barrier_grants);

            if (index >= ARRAY_SIZE)
            {
                sm_end(data->sm);
                break;
            }

            data->writes[index].what_was_source = source;
            data->writes[index].is_barrier = false;
            sm_end(data->sm);
        }
        else
        {
            /*not granted execution, so just hammer*/
            InterlockedIncrement64(&non_barrier_refusals);
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

BEGIN_TEST_SUITE(sm_int_tests)

TEST_SUITE_INITIALIZE(suite_init)
{
    GetSystemInfo(&systemInfo);
    dwNumberOfProcessors = systemInfo.dwNumberOfProcessors;
}

/*tests aims to mindlessly execute the APIs.
at least 1 sm_open_begin and at least 1 sm_begin are waited to happen*/
TEST_FUNCTION(sm_chaos)
{
    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)malloc(sizeof(OPEN_CLOSE_THREADS));

    ASSERT_IS_NOT_NULL(data);
    data->sm = sm_create(NULL);
    ASSERT_IS_NOT_NULL(data->sm);

    data->startTime_ms = timer_global_get_elapsed_ms();

    InterlockedExchange(&data->threadsShouldFinish, 0);

    for (uint32_t nthreads = 1; nthreads <= 4 * dwNumberOfProcessors; nthreads*=2)
    {
        data->n_begin_open_threads = nthreads;
        data->n_end_open_threads = nthreads;
        data->n_begin_close_threads = nthreads;
        data->n_end_close_threads = nthreads;
        data->n_begin_barrier_threads = nthreads;
        data->n_end_barrier_threads = nthreads;
        data->n_begin_threads = nthreads;
        
        InterlockedExchange(&data->threadsShouldFinish, 0);
        InterlockedExchange(&data->n_begin_open_grants, 0);
        InterlockedExchange(&data->n_begin_open_refuses, 0);
        InterlockedExchange(&data->n_begin_close_grants, 0);
        InterlockedExchange(&data->n_begin_close_refuses, 0);
        InterlockedExchange(&data->n_begin_barrier_grants, 0);
        InterlockedExchange(&data->n_begin_barrier_refuses, 0);
        InterlockedExchange(&data->n_begin_grants, 0);
        InterlockedExchange(&data->n_begin_refuses, 0);

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
            printf("Slept %" PRIu32 " ms, no sign of n_begin_open_grants=%" PRId32 ", n_begin_grants=%" PRId32 " \n", counterSleep * 1000, n_begin_open_grants_local, n_begin_grants_local);
            counterSleep++;
            Sleep(1000);
        }

        InterlockedExchange(&data->threadsShouldFinish, 1);

        waitAndDestroyBeginAndEndThreads(data);
        waitAndDestroyEndBarrierThreads(data);
        waitAndDestroyBeginBarrierThreads(data);
        waitAndDestroyEndCloseThreads(data);
        waitAndDestroyBeginCloseThreads(data);
        waitAndDestroyEndOpenThreads(data);
        waitAndDestroyBeginOpenThreads(data);

        /*just in case anything needs to close*/
        sm_barrier_end(data->sm);
        sm_close_end(data->sm);

        printf("nthreads=%" PRIu32 
            ", n_begin_open_grants=%" PRIu32 ", n_begin_open_refuses=%" PRIu32 
            ", n_begin_close_grants=%" PRIu32 ", n_begin_close_refuses=%" PRIu32 
            ", n_begin_barrier_grants=%" PRIu32 ", n_begin_barrier_refuses=%" PRIu32
            ", n_begin_grants=%" PRIu32 ", n_begin_refuses=%" PRIu32
            "\n",
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
        ASSERT_IS_TRUE(InterlockedAdd(&data->n_begin_open_grants, 0) - InterlockedAdd(&data->n_begin_close_grants, 0) <= 1);
    }

    free(data);
}

TEST_FUNCTION(sm_does_not_block)
{
    ///arrange
    THREADS_COMMON* data = (THREADS_COMMON*)malloc(sizeof(THREADS_COMMON));
    ASSERT_IS_NOT_NULL(data);
    data->sm = sm_create(NULL);
    ASSERT_IS_NOT_NULL(data->sm);

    ///act
    for (uint32_t nthreads = 1; nthreads <= 4 * dwNumberOfProcessors; nthreads*=2)
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
            sm_open_end(data->sm);

            printf("\nnthreads=%" PRIu32 " n_barrier_threads=%" PRIu32 " n_non_barrier_threads=%" PRIu32 "\n", nthreads, n_barrier_threads, n_non_barrier_threads);

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

            printf("took %f ms, non_barrier_grants=%" PRId32 ", non_barrier_refusals=%" PRId64 " barrier_grants=%" PRId32 ", barrier_refusals=%" PRId64 "\n", timer_global_get_elapsed_ms() - data->startTimems, 
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
}

END_TEST_SUITE(sm_int_tests)
