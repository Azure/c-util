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
    ONE_WRITE writes[ARRAY_SIZE + 100];

    double startTimems;

}THREADS_COMMON;

#define THREAD_TYPE_VALUES \
    THREAD_BARRIER, \
    THREAD_NON_BARRIER

MU_DEFINE_ENUM(THREAD_TYPE, THREAD_TYPE_VALUE)

BEGIN_TEST_SUITE(sm_int_tests)

TEST_SUITE_INITIALIZE(suite_init)
{

}

TEST_SUITE_CLEANUP(s)
{

}

TEST_FUNCTION_INITIALIZE(init)
{

}

TEST_FUNCTION_CLEANUP(cleanup)
{

}

static volatile LONG barrier_executions = 0;

static DWORD barrier_thread(
    LPVOID lpThreadParameter
)
{
    THREADS_COMMON* data = (THREADS_COMMON*)lpThreadParameter;
    /*a non barrier thread granted execution will interlocked increment the index, interlocked increment the source of numbers and write it*/
    while (InterlockedAdd(&data->current_index, 0) < ARRAY_SIZE)
    {
        if (sm_barrier_begin(data->sm) == 0)
        {

            LONG source = InterlockedIncrement(&data->source_of_numbers);
            LONG index = InterlockedIncrement(&data->current_index) - 1;

            if (index >= ARRAY_SIZE)
            {
                sm_barrier_end(data->sm);
                break;
            }

            InterlockedIncrement(&barrier_executions);

            data->writes[index].what_was_source = source;
            data->writes[index].is_barrier = true;
            sm_barrier_end(data->sm);
        }
        else
        {
            /*not granted execution, so just hammer*/
        }
    }
    return 0;
}

static volatile LONG non_barrier_executions = 0;

static DWORD non_barrier_thread(
    LPVOID lpThreadParameter
)
{
    THREADS_COMMON* data = (THREADS_COMMON*)lpThreadParameter;
    /*a non barrier thread granted execution will interlocked increment the index, interlocked increment the source of numbers and write it*/
    while (InterlockedAdd(&data->current_index, 0)<ARRAY_SIZE)
    {
        if (sm_begin(data->sm) == 0)
        {
            LONG source = InterlockedIncrement(&data->source_of_numbers);
            LONG index = InterlockedIncrement(&data->current_index) - 1;

            if (index >= ARRAY_SIZE)
            {
                sm_end(data->sm);
                break;
            }

            InterlockedIncrement(&non_barrier_executions);

            data->writes[index].what_was_source = source;
            data->writes[index].is_barrier = false;
            sm_end(data->sm);
        }
        else
        {
            /*not granted execution, so just hammer*/
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

TEST_FUNCTION(sm_does_not_block)
{
    ///arrange
    THREADS_COMMON* data = (THREADS_COMMON*)malloc(sizeof(THREADS_COMMON));
    ASSERT_IS_NOT_NULL(data);
    data->sm = sm_create(NULL);
    ASSERT_IS_NOT_NULL(data->sm);

    ///act
    for (uint32_t nthreads = 32; nthreads <= N_MAX_THREADS; nthreads++)
    {
        for(uint32_t n_barrier_threads=0; n_barrier_threads<=nthreads; n_barrier_threads++)
        {
            uint32_t n_non_barrier_threads = nthreads - n_barrier_threads;

            HANDLE barrierThreads[N_MAX_THREADS];
            (void)memset(barrierThreads, 0, sizeof(barrierThreads));

            HANDLE nonBarrierThreads[N_MAX_THREADS];
            (void)memset(nonBarrierThreads, 0, sizeof(nonBarrierThreads));

            (void)InterlockedExchange(&non_barrier_executions, 0);
            (void)InterlockedExchange(&barrier_executions, 0);
            (void)InterlockedExchange(&data->source_of_numbers, 0);
            (void)InterlockedExchange(&data->current_index, 0);
            (void)memset(data->writes, 0, sizeof(data->writes));

            data->startTimems = timer_global_get_elapsed_ms();

            ASSERT_IS_TRUE(sm_open_begin(data->sm) == 0);
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

            printf("took %f ms, non_barrier_executions=%" PRId32 ", barrier_executions=%" PRId32 "\n", timer_global_get_elapsed_ms() - data->startTimems, InterlockedAdd(&non_barrier_executions, 0), InterlockedAdd(&barrier_executions, 0));

            ASSERT_IS_TRUE(sm_close_begin(data->sm) == 0);
            sm_close_end(data->sm);
        }
    }

    ///assert

    ///clean
    sm_destroy(data->sm);
    free(data);
}



END_TEST_SUITE(sm_int_tests)
