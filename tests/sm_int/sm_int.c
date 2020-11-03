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

#include "testrunnerswitcher.h"

#include "macro_utils/macro_utils.h"

#include "c_pal/timer.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/threadapi.h"
#include "c_pal/interlocked.h"
#include "c_pal/sysinfo.h"
#include "c_util/interlocked_hl.h"
#include "c_logging/xlogging.h"

#include "c_util/sm.h"

TEST_DEFINE_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

#define N_MAX_THREADS 256

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

static double timeSinceTestFunctionStartMs;

#define SM_BEGIN_CLOSE_DELAY 200 /*ms time between 2 consecutive sm_close_begin - thus allowing for greater times of "open" state*/

#define SM_BEGIN_BARRIER_DELAY (SM_BEGIN_CLOSE_DELAY/10) /*ms time between 2 consecutive sm_barrier_begin - thus allowing for greater times for execs to happen*/

typedef struct OPEN_CLOSE_THREADS_TAG
{
    SM_HANDLE sm;
    double startTime_ms;

    THREAD_HANDLE beginOpenThreads[N_MAX_THREADS];
    THREAD_HANDLE endOpenThreads[N_MAX_THREADS];
    volatile_atomic int32_t n_begin_open_grants;
    volatile_atomic int32_t n_begin_open_refuses;
    uint32_t n_begin_open_threads;
    uint32_t n_end_open_threads;

    THREAD_HANDLE beginCloseThreads[N_MAX_THREADS];
    THREAD_HANDLE endCloseThreads[N_MAX_THREADS];
    volatile_atomic int32_t n_begin_close_grants;
    volatile_atomic int32_t n_begin_close_refuses;
    uint32_t n_begin_close_threads;
    uint32_t n_end_close_threads;

    THREAD_HANDLE beginBarrierThreads[N_MAX_THREADS];
    THREAD_HANDLE endBarrierThreads[N_MAX_THREADS];
    volatile_atomic int32_t n_begin_barrier_grants;
    volatile_atomic int32_t n_begin_barrier_refuses;
    uint32_t n_begin_barrier_threads;
    uint32_t n_end_barrier_threads;

    THREAD_HANDLE beginThreads[N_MAX_THREADS];
    volatile_atomic int32_t n_begin_grants;
    volatile_atomic int32_t n_begin_refuses;
    uint32_t n_begin_threads;

    volatile_atomic int32_t threadsShouldFinish; /*this test is time bound*/
} OPEN_CLOSE_THREADS;

static int callsBeginOpen(
    void* lpThreadParameter
)
{
    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)lpThreadParameter;

    while (interlocked_add(&data->threadsShouldFinish, 0) == 0)
    {
        if (sm_open_begin(data->sm) == SM_EXEC_GRANTED)
        {
            (void)interlocked_increment(&data->n_begin_open_grants);
        }
        else
        {
            (void)interlocked_increment(&data->n_begin_open_refuses);
        }
    }
    return 0;
}

static void createBeginOpenThreads(OPEN_CLOSE_THREADS* data)
{
    uint32_t iBeginOpen;
    for (iBeginOpen = 0; iBeginOpen < data->n_begin_open_threads; iBeginOpen++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&data->beginOpenThreads[iBeginOpen], callsBeginOpen, data));
    }
}

static void waitAndDestroyBeginOpenThreads(OPEN_CLOSE_THREADS* data)
{
    for (uint32_t iBeginOpen = 0; iBeginOpen < data->n_begin_open_threads; iBeginOpen++)
    {
        int dont_care;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(data->beginOpenThreads[iBeginOpen], &dont_care));
    }
}

static int callsEndOpen(
    void* lpThreadParameter
)
{
    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)lpThreadParameter;

    while (interlocked_add(&data->threadsShouldFinish, 0) == 0)
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
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&data->endOpenThreads[iEndOpen], callsEndOpen, data));
    }
}

static void waitAndDestroyEndOpenThreads(OPEN_CLOSE_THREADS* data)
{
    for (uint32_t iEndOpen = 0; iEndOpen < data->n_end_open_threads; iEndOpen++)
    {
        int dont_care;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(data->endOpenThreads[iEndOpen], &dont_care));
    }
}


static int callsBeginClose(
    void* lpThreadParameter
)
{
    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)lpThreadParameter;

    while (interlocked_add(&data->threadsShouldFinish, 0) == 0)
    {
        if (sm_close_begin(data->sm) == SM_EXEC_GRANTED)
        {
            (void)interlocked_increment(&data->n_begin_close_grants);
        }
        else
        {
            (void)interlocked_increment(&data->n_begin_close_refuses);
        }

        ThreadAPI_Sleep(SM_BEGIN_CLOSE_DELAY);
    }
    return 0;
}

static void createBeginCloseThreads(OPEN_CLOSE_THREADS* data)
{
    uint32_t iBeginClose;
    for (iBeginClose = 0; iBeginClose < data->n_begin_close_threads; iBeginClose++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&data->beginCloseThreads[iBeginClose], callsBeginClose, data));
    }
}

static void waitAndDestroyBeginCloseThreads(OPEN_CLOSE_THREADS* data)
{
    for (uint32_t iBeginClose = 0; iBeginClose < data->n_begin_close_threads; iBeginClose++)
    {
        int dont_care;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(data->beginCloseThreads[iBeginClose], &dont_care));
    }
}

static int callsEndClose(
    void* lpThreadParameter
)
{
    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)lpThreadParameter;

    while (interlocked_add(&data->threadsShouldFinish, 0) == 0)
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
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&data->endCloseThreads[iEndClose], callsEndClose, data));
    }
}

static void waitAndDestroyEndCloseThreads(OPEN_CLOSE_THREADS* data)
{
    for (uint32_t iEndClose = 0; iEndClose < data->n_end_close_threads; iEndClose++)
    {
        int dont_care;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(data->endCloseThreads[iEndClose], &dont_care));
    }
}

static int callsBeginBarrier(
    void* lpThreadParameter
)
{
    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)lpThreadParameter;

    while (interlocked_add(&data->threadsShouldFinish, 0) == 0)
    {
        if (sm_barrier_begin(data->sm) == SM_EXEC_GRANTED)
        {
            (void)interlocked_increment(&data->n_begin_barrier_grants);
        }
        else
        {
            (void)interlocked_increment(&data->n_begin_barrier_refuses);
        }
        ThreadAPI_Sleep(SM_BEGIN_BARRIER_DELAY);
    }
    return 0;
}

static void createBeginBarrierThreads(OPEN_CLOSE_THREADS* data)
{
    uint32_t iBeginBarrier;
    for (iBeginBarrier = 0; iBeginBarrier < data->n_begin_barrier_threads; iBeginBarrier++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&data->beginBarrierThreads[iBeginBarrier], callsBeginBarrier, data));
    }
}

static void waitAndDestroyBeginBarrierThreads(OPEN_CLOSE_THREADS* data)
{
    for (uint32_t iBeginBarrier = 0; iBeginBarrier < data->n_begin_barrier_threads; iBeginBarrier++)
    {
        int dont_care;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(data->beginBarrierThreads[iBeginBarrier], &dont_care));
    }
}

static int callsEndBarrier(
    void* lpThreadParameter
)
{
    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)lpThreadParameter;

    while (interlocked_add(&data->threadsShouldFinish, 0) == 0)
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
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&data->endBarrierThreads[iEndBarrier], callsEndBarrier, data));
    }
}

static void waitAndDestroyEndBarrierThreads(OPEN_CLOSE_THREADS* data)
{
    for (uint32_t iEndBarrier = 0; iEndBarrier < data->n_end_barrier_threads; iEndBarrier++)
    {
        int dont_care;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(data->endBarrierThreads[iEndBarrier], &dont_care));
    }
}

static int callsBeginAndEnd(
    void* lpThreadParameter
)
{
    OPEN_CLOSE_THREADS* data = (OPEN_CLOSE_THREADS*)lpThreadParameter;

    while (interlocked_add(&data->threadsShouldFinish, 0) == 0)
    {
        if (sm_exec_begin(data->sm) == SM_EXEC_GRANTED)
        {
            (void)interlocked_increment(&data->n_begin_grants);
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
            (void)interlocked_increment(&data->n_begin_refuses);
        }
    }
    return 0;
}

static void createBeginAndEndThreads(OPEN_CLOSE_THREADS* data)
{
    uint32_t iBegin;
    for (iBegin = 0; iBegin < data->n_begin_threads; iBegin++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&data->beginThreads[iBegin], callsBeginAndEnd, data));
    }
}

static void waitAndDestroyBeginAndEndThreads(OPEN_CLOSE_THREADS* data)
{
    for (uint32_t iBegin = 0; iBegin < data->n_begin_threads; iBegin++)
    {
        int dont_care;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(data->beginThreads[iBegin], &dont_care));
    }
}

#ifdef _MSC_VER
#define ARRAY_SIZE 1000000
#else
// on Linux because we run with Helgrind and DRD this will be waaaay slower, so reduce the number of items
#define ARRAY_SIZE 50000
#endif

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

static volatile_atomic int32_t barrier_grants = 0;
static volatile_atomic int64_t barrier_refusals = 0; /*this is just for giggles*/

static volatile_atomic int32_t non_barrier_grants = 0;
static volatile_atomic int64_t non_barrier_refusals = 0;

typedef struct ONE_WRITE_TAG
{
    int32_t what_was_source;
    bool is_barrier;
}ONE_WRITE;

typedef struct THREADS_COMMON_TAG
{
    volatile_atomic int32_t nFinishedThreads;
    SM_HANDLE sm;

    volatile_atomic int32_t source_of_numbers;
    volatile_atomic int32_t current_index;
    ONE_WRITE writes[ARRAY_SIZE];

    double startTimems;

}THREADS_COMMON;

#define THREAD_TYPE_VALUES \
    THREAD_BARRIER, \
    THREAD_NON_BARRIER

MU_DEFINE_ENUM(THREAD_TYPE, THREAD_TYPE_VALUE)

static int barrier_thread(
    void* lpThreadParameter
)
{
    THREADS_COMMON* data = (THREADS_COMMON*)lpThreadParameter;
    /*a non barrier thread granted execution will interlocked increment the index, interlocked increment the source of numbers and write it*/
    while (interlocked_add(&data->current_index, 0) < ARRAY_SIZE)
    {
        if (sm_barrier_begin(data->sm) == SM_EXEC_GRANTED)
        {
            int32_t index = interlocked_increment(&data->current_index) - 1;
            int32_t source = interlocked_increment(&data->source_of_numbers);
            (void)interlocked_increment(&barrier_grants);

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
            (void)interlocked_increment_64(&barrier_refusals);
        }
    }
    return 0;
}

static int non_barrier_thread(
    void* lpThreadParameter
)
{
    THREADS_COMMON* data = (THREADS_COMMON*)lpThreadParameter;
    /*a non barrier thread granted execution will interlocked increment the index, interlocked increment the source of numbers and write it*/
    while (interlocked_add(&data->current_index, 0) < ARRAY_SIZE)
    {
        if (sm_exec_begin(data->sm) == SM_EXEC_GRANTED)
        {
            int32_t index = interlocked_increment(&data->current_index) - 1;
            int32_t source = interlocked_increment(&data->source_of_numbers);
            (void)interlocked_increment(&non_barrier_grants);

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
            (void)interlocked_increment_64(&non_barrier_refusals);
        }
    }
    return 0;
}

static void verify(THREADS_COMMON* data) /*ASSERTS*/
{
    volatile size_t i;
    volatile_atomic int32_t maxBeforeBarrier = -1;
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

static uint32_t numberOfProcessors;

#define SM_APIS_VALUES      \
SM_OPEN_BEGIN,              \
SM_CLOSE_BEGIN,             \
SM_EXEC_BEGIN,              \
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
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
#if _MSC_VER
    numberOfProcessors = sysinfo_get_processor_count();
    ASSERT_IS_TRUE(numberOfProcessors * 4 <= N_MAX_THREADS, "for systems with maaany processors, modify N_MAX_THREADS to be bigger");
#else
    numberOfProcessors = 2;
#endif

    LogInfo("numberOfProcessors was detected as %" PRIu32 "", numberOfProcessors);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(function_initialize)
{
    timeSinceTestFunctionStartMs = timer_global_get_elapsed_ms();
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

    (void)interlocked_exchange(&data->threadsShouldFinish, 0);

    for (uint32_t nthreads = 1; nthreads <= MIN(4 * numberOfProcessors, N_MAX_THREADS); nthreads*=2)
    {
        data->n_begin_open_threads = nthreads;
        data->n_end_open_threads = nthreads;
        data->n_begin_close_threads = nthreads;
        data->n_end_close_threads = nthreads;
        data->n_begin_barrier_threads = nthreads;
        data->n_end_barrier_threads = nthreads;
        data->n_begin_threads = nthreads;
        
        (void)interlocked_exchange(&data->threadsShouldFinish, 0);
        (void)interlocked_exchange(&data->n_begin_open_grants, 0);
        (void)interlocked_exchange(&data->n_begin_open_refuses, 0);
        (void)interlocked_exchange(&data->n_begin_close_grants, 0);
        (void)interlocked_exchange(&data->n_begin_close_refuses, 0);
        (void)interlocked_exchange(&data->n_begin_barrier_grants, 0);
        (void)interlocked_exchange(&data->n_begin_barrier_refuses, 0);
        (void)interlocked_exchange(&data->n_begin_grants, 0);
        (void)interlocked_exchange(&data->n_begin_refuses, 0);

        createBeginOpenThreads(data);
        createEndOpenThreads(data);
        createBeginCloseThreads(data);
        createEndCloseThreads(data);
        createBeginBarrierThreads(data);
        createEndBarrierThreads(data);
        createBeginAndEndThreads(data);

        ThreadAPI_Sleep(1000);
        uint32_t counterSleep = 1;
        int32_t n_begin_open_grants_local;
        int32_t n_begin_grants_local;
        while (
            (n_begin_open_grants_local=interlocked_add(&data->n_begin_open_grants, 0)), 
            (n_begin_grants_local = interlocked_add(&data->n_begin_grants, 0)),
            ((n_begin_open_grants_local==0) ||(n_begin_grants_local==0))
            )
        {
            toBeRestored(AZ_LOG_INFO, __FILE__, FUNC_NAME, __LINE__, 0, "Slept %" PRIu32 " ms, no sign of n_begin_open_grants=%" PRId32 ", n_begin_grants=%" PRId32 "\n", counterSleep * 1000, n_begin_open_grants_local, n_begin_grants_local);
            counterSleep++;
            ThreadAPI_Sleep(1000);
        }

        (void)interlocked_exchange(&data->threadsShouldFinish, 1);

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

        toBeRestored(AZ_LOG_INFO, __FILE__, FUNC_NAME, __LINE__, 0, "nthreads=%" PRIu32
            ", n_begin_open_grants=%" PRIu32 ", n_begin_open_refuses=%" PRIu32 
            ", n_begin_close_grants=%" PRIu32 ", n_begin_close_refuses=%" PRIu32 
            ", n_begin_barrier_grants=%" PRIu32 ", n_begin_barrier_refuses=%" PRIu32
            ", n_begin_grants=%" PRIu32 ", n_begin_refuses=%" PRIu32
            "\n",
            nthreads,
            interlocked_add(&data->n_begin_open_grants, 0),
            interlocked_add(&data->n_begin_open_refuses, 0),
            interlocked_add(&data->n_begin_close_grants, 0),
            interlocked_add(&data->n_begin_close_refuses, 0),
            interlocked_add(&data->n_begin_barrier_grants, 0),
            interlocked_add(&data->n_begin_barrier_refuses, 0),
            interlocked_add(&data->n_begin_grants, 0),
            interlocked_add(&data->n_begin_refuses, 0)
        );

        ASSERT_IS_TRUE(interlocked_add(&data->n_begin_open_grants, 0) >= 1);
    }
    sm_destroy(data->sm);
    free(data);

    xlogging_set_log_function(toBeRestored);
}

TEST_FUNCTION(sm_does_not_block)
{
    LogInfo("disabling logging for the duration of sm_does_not_block. Logging takes additional locks that \"might help\" the test pass");
    LOGGER_LOG toBeRestored = xlogging_get_log_function();
    xlogging_set_log_function(NULL);

    ///arrange
    THREADS_COMMON* data = (THREADS_COMMON*)malloc(sizeof(THREADS_COMMON));
    ASSERT_IS_NOT_NULL(data);
    data->sm = sm_create(NULL);
    ASSERT_IS_NOT_NULL(data->sm);

    ///act
    for (uint32_t nthreads = 1; nthreads <= MIN(4 * numberOfProcessors, N_MAX_THREADS); nthreads*=2)
    {
        for(uint32_t n_barrier_threads=0; n_barrier_threads<=nthreads; n_barrier_threads+=4)
        {
            uint32_t n_non_barrier_threads = nthreads - n_barrier_threads;

            THREAD_HANDLE barrierThreads[N_MAX_THREADS];
            (void)memset(barrierThreads, 0, sizeof(barrierThreads));

            THREAD_HANDLE nonBarrierThreads[N_MAX_THREADS];
            (void)memset(nonBarrierThreads, 0, sizeof(nonBarrierThreads));

            (void)interlocked_exchange(&non_barrier_grants, 0);
            (void)interlocked_exchange_64(&non_barrier_refusals, 0);
            (void)interlocked_exchange(&barrier_grants, 0);
            (void)interlocked_exchange_64(&barrier_refusals, 0);
            (void)interlocked_exchange(&data->source_of_numbers, 0);
            (void)interlocked_exchange(&data->current_index, 0);
            (void)memset(data->writes, 0, sizeof(data->writes));

            data->startTimems = timer_global_get_elapsed_ms();

            ASSERT_IS_TRUE(sm_open_begin(data->sm) == SM_EXEC_GRANTED);
            sm_open_end(data->sm, true);

            toBeRestored(AZ_LOG_INFO, __FILE__, FUNC_NAME, __LINE__, 0, "Info: nthreads=%" PRIu32 " n_barrier_threads=%" PRIu32 " n_non_barrier_threads=%" PRIu32 "\n", nthreads, n_barrier_threads, n_non_barrier_threads);

            /*create them barrier threads*/
            for (uint32_t iBarrier = 0; iBarrier < n_barrier_threads; iBarrier++)
            {
                ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&barrierThreads[iBarrier], barrier_thread, data));
            }

            /*create them non barrier threads*/
            for (uint32_t iNonBarrier = 0; iNonBarrier < n_non_barrier_threads; iNonBarrier++)
            {
                ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&nonBarrierThreads[iNonBarrier], non_barrier_thread, data));
            }

            /*wait for them threads to finish*/
            if (n_barrier_threads > 0)
            {
                for (uint32_t iBarrier = 0; iBarrier < n_barrier_threads; iBarrier++)
                {
                    int dont_care;
                    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(barrierThreads[iBarrier], &dont_care));
                }
            }
            if (n_non_barrier_threads > 0)
            {
                for (uint32_t iNonBarrier = 0; iNonBarrier < n_non_barrier_threads; iNonBarrier++)
                {
                    int dont_care;
                    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(nonBarrierThreads[iNonBarrier], &dont_care));
                }
            }

            /*verify the all numbers written by barriers are greater than all previous numbers*/
            verify(data);

            toBeRestored(AZ_LOG_INFO, __FILE__, FUNC_NAME, __LINE__, 0, "Info: took %f ms, non_barrier_grants=%" PRId32 ", non_barrier_refusals=%" PRId64 " barrier_grants=%" PRId32 ", barrier_refusals=%" PRId64 "\n", timer_global_get_elapsed_ms() - data->startTimems,
                interlocked_add(&non_barrier_grants, 0), 
                interlocked_add_64(&non_barrier_refusals, 0),
                interlocked_add(&barrier_grants, 0),
                interlocked_add_64(&barrier_refusals, 0));

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

#define THREAD_DELAY 1000

/*forward*/
typedef struct SM_RESULT_AND_NEXT_STATE_AFTER_API_CALL_TAG
{
    SM_RESULT expected_sm_result;
    SM_STATES sm_state_after_api;
}SM_RESULT_AND_NEXT_STATE_AFTER_API_CALL;

typedef struct SM_GO_TO_STATE_TAG
{
    SM_HANDLE sm;
    SM_STATES targetState;
    const SM_RESULT_AND_NEXT_STATE_AFTER_API_CALL* expected;
    THREAD_HANDLE threadSwitchesTo; /*this thread switches the state to the state which is intended to have when sm_..._begin API are called. Then the thread might block (because the state switch is waiting on some draining) or might proceed to end*/
    THREAD_HANDLE threadBack; /*this thread unblocks threadSwitchesTo and the main thread. Main thread might become blocked because the API it is calling might be waiting - such is the case when waiting for a drain to happen*/

    volatile_atomic int32_t targetStateAPICalledInNextLine; /*event set when the target state will be switched in the next line of code. That call might or not return. For example when in the case of wanting to reach the state of SM_OPENED_DRAINING_TO_BARRIER. The call doesn't return until all the sm_exec_end have been called*/

    volatile_atomic int32_t targetAPICalledInNextLine; /*event set just before calling the API in a specific state. This is needed because some of the APIs are blocking (such as calling sm_close_begin when a barrier is executing)*/
}SM_GO_TO_STATE; 


static int switchesToState(
    void* lpThreadParameter
)
{
    SM_GO_TO_STATE* goToState = (SM_GO_TO_STATE*)lpThreadParameter;

    LogInfo("time[s]=%.2f, switchesToState thread: will now switch state to %" PRI_MU_ENUM "", (timer_global_get_elapsed_ms()-timeSinceTestFunctionStartMs)/1000, MU_ENUM_VALUE(SM_STATES, goToState->targetState));

    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWake(&goToState->targetStateAPICalledInNextLine, 1));
    switch (goToState->targetState)
    {
        case SM_CREATED:
        {
            break;
        }
        case SM_OPENING:
        {
            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_open_begin(goToState->sm));
            break;
        }
        case SM_OPENED:
        {
            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_open_begin(goToState->sm));
            sm_open_end(goToState->sm, true);
            break;
        }
        case SM_OPENED_DRAINING_TO_BARRIER:
        {
            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_open_begin(goToState->sm));
            sm_open_end(goToState->sm, true);

            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_exec_begin(goToState->sm));

            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_barrier_begin(goToState->sm)); /*switches to draining mode - and stays there, because sm_exec_end was not called yet */
            LogInfo("time[s]=%.2f, switches state thread: returning from sm_barrier_begin(goToState->sm)", (timer_global_get_elapsed_ms() - timeSinceTestFunctionStartMs) / 1000);
            break;

        }
        case SM_OPENED_DRAINING_TO_CLOSE:
        {
            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_open_begin(goToState->sm));
            sm_open_end(goToState->sm, true);

            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_exec_begin(goToState->sm));

            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_close_begin(goToState->sm)); /*switches to draining mode*/
            LogInfo("time[s]=%.2f, switchesToState thread: returning from sm_close_begin(goToState->sm)", (timer_global_get_elapsed_ms() - timeSinceTestFunctionStartMs) / 1000);

            break;
        }
        case SM_OPENED_BARRIER:
        {
            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_open_begin(goToState->sm));
            sm_open_end(goToState->sm, true);

            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_barrier_begin(goToState->sm));
            break;
        }
        case SM_CLOSING:
        {
            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_open_begin(goToState->sm));
            sm_open_end(goToState->sm, true);

            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_close_begin(goToState->sm));
            LogInfo("time[s]=%.2f, switchesToState thread: returning from sm_close_begin(goToState->sm)", (timer_global_get_elapsed_ms() - timeSinceTestFunctionStartMs) / 1000);
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


static void sm_switchesToState(SM_GO_TO_STATE* goToState)
{
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&goToState->threadSwitchesTo, switchesToState, goToState));
    /*depending on the requested state, the thread might have finished by now...*/
}


static int switchesFromStateToCreated(
    void* lpThreadParameter
)
{
    SM_GO_TO_STATE* goToState = (SM_GO_TO_STATE*)lpThreadParameter;

    /*waits on 1 handles that says the API is about to be called. It waits 1 second then it resumes executiong */

    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&goToState->targetAPICalledInNextLine, 1, UINT32_MAX));

    LogInfo("time[s]=%.2f, switchesFromStateToCreated thread : will now switch state back from %" PRI_MU_ENUM " to %" PRI_MU_ENUM " after sleeping %" PRIu32 "", (timer_global_get_elapsed_ms() - timeSinceTestFunctionStartMs) / 1000, MU_ENUM_VALUE(SM_STATES, (SM_STATES)(goToState->expected->sm_state_after_api)), MU_ENUM_VALUE(SM_STATES, (SM_STATES)(SM_CREATED)), THREAD_DELAY);

    ThreadAPI_Sleep(THREAD_DELAY);

    switch (goToState->expected->sm_state_after_api)
    {
        case SM_CREATED:
        {
            break;
        }
        case SM_OPENING:
        {
            sm_open_end(goToState->sm, true);
            ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_close_begin(goToState->sm));
            sm_close_end(goToState->sm);
            break;
        }
        case SM_OPENED:
        {
            if (sm_close_begin(goToState->sm) == SM_EXEC_GRANTED)
            {
                sm_close_end(goToState->sm);
            }
            
            break;
        }
        case SM_OPENED_DRAINING_TO_BARRIER:
        {
            sm_exec_end(goToState->sm);
            /*calling sm_exec_end will unblock sm_barrier_begin from the switchesToThread (if any)*/

            ThreadAPI_Sleep(THREAD_DELAY);

            sm_barrier_end(goToState->sm);
            /*returns to SM_OPENED...*/

            ThreadAPI_Sleep(THREAD_DELAY);

            if (sm_close_begin(goToState->sm) == SM_EXEC_GRANTED)
            {
                sm_close_end(goToState->sm);
            }
            break;

        }
        case SM_OPENED_DRAINING_TO_CLOSE:
        {
            sm_exec_end(goToState->sm);
            /*unblocks sm_close_begin in the switchesState thread*/
            
            sm_close_end(goToState->sm);
            break;
        }
        case SM_OPENED_BARRIER:
        {
            sm_barrier_end(goToState->sm);

            if (sm_close_begin(goToState->sm) == SM_EXEC_GRANTED)
            {
                sm_close_end(goToState->sm);
            }
            break;
        }
        case SM_CLOSING:
        {
            sm_exec_end(goToState->sm);
            ThreadAPI_Sleep(THREAD_DELAY);
            sm_barrier_end(goToState->sm);
            ThreadAPI_Sleep(THREAD_DELAY);
            sm_close_end(goToState->sm);
            break;
        }
        default:
        {
            ASSERT_FAIL("unknown state");
            break;
        }
    }

    LogInfo("time[s]=%.2f, switchesFromStateToCreated thread : state switched back to %" PRI_MU_ENUM "", (timer_global_get_elapsed_ms() - timeSinceTestFunctionStartMs) / 1000, MU_ENUM_VALUE(SM_STATES, (SM_STATES)(SM_CREATED)));

    return 0;
}

static void sm_switches_from_state_to_created(SM_GO_TO_STATE* goToState)
{
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&goToState->threadBack, switchesFromStateToCreated, goToState));
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
    SM_RESULT_AND_NEXT_STATE_AFTER_API_CALL expected[][4]=
    {
                                                /*sm_open_begin*/                                    /*sm_close_begin*/                              /*sm_exec_begin*/                                    /*sm_barrier_begin*/
        /*SM_CREATED*/                      {   {SM_EXEC_GRANTED, SM_OPENING},                       {SM_EXEC_REFUSED, SM_CREATED},                  {SM_EXEC_REFUSED, SM_CREATED},                       {SM_EXEC_REFUSED, SM_CREATED}},
        /*SM_OPENING*/                      {   {SM_EXEC_REFUSED, SM_OPENING},                       {SM_EXEC_REFUSED, SM_OPENING},                  {SM_EXEC_REFUSED, SM_OPENING},                       {SM_EXEC_REFUSED, SM_OPENING}},
        /*SM_OPENED*/                       {   {SM_EXEC_REFUSED, SM_OPENED},                        {SM_EXEC_GRANTED, SM_CLOSING},                  {SM_EXEC_GRANTED, SM_OPENED},                        {SM_EXEC_GRANTED, SM_OPENED_BARRIER}},
        /*SM_OPENED_DRAINING_TO_BARRIER*/   {   {SM_EXEC_REFUSED, SM_OPENED_DRAINING_TO_BARRIER},    {SM_EXEC_GRANTED, SM_CLOSING},                  {SM_EXEC_REFUSED, SM_OPENED_DRAINING_TO_BARRIER},    {SM_EXEC_REFUSED, SM_OPENED_DRAINING_TO_BARRIER}},
        /*SM_OPENED_DRAINING_TO_CLOSE*/     {   {SM_EXEC_REFUSED, SM_OPENED_DRAINING_TO_CLOSE},      {SM_EXEC_REFUSED, SM_OPENED_DRAINING_TO_CLOSE}, {SM_EXEC_REFUSED, SM_OPENED_DRAINING_TO_CLOSE},      {SM_EXEC_REFUSED, SM_OPENED_DRAINING_TO_CLOSE}},
        /*SM_OPENED_BARRIER*/               {   {SM_EXEC_REFUSED, SM_OPENED_BARRIER},                {SM_EXEC_GRANTED, SM_CLOSING},                  {SM_EXEC_REFUSED, SM_OPENED_BARRIER},                {SM_EXEC_REFUSED, SM_OPENED_BARRIER}},
        /*SM_CLOSING*/                      {   {SM_EXEC_REFUSED, SM_CLOSING},                       {SM_EXEC_REFUSED, SM_CLOSING},                  {SM_EXEC_REFUSED, SM_CLOSING},                       {SM_EXEC_REFUSED, SM_CLOSING}}
    };

    for (uint32_t i = 0 ; i < sizeof(expected) / sizeof(expected[0]); i++)
    {
        for (uint32_t j = 0; j < sizeof(expected[0]) / sizeof(expected[0][0]); j++)
        {
            SM_GO_TO_STATE goToState;
            (void)interlocked_exchange(&goToState.targetStateAPICalledInNextLine, 0);

            (void)interlocked_exchange(&goToState.targetAPICalledInNextLine, 0);

            goToState.expected = &expected[i][j];

            goToState.sm = sm_create(NULL);
            ASSERT_IS_NOT_NULL(goToState.sm);
            goToState.targetState = (SM_STATES)(i + SM_CREATED);

            LogInfo("\n\n");
            LogInfo("time[s]=%.2f, going to state=%" PRI_MU_ENUM "; will call=%" PRI_MU_ENUM "", (timer_global_get_elapsed_ms() - timeSinceTestFunctionStartMs) / 1000, MU_ENUM_VALUE(SM_STATES, goToState.targetState), MU_ENUM_VALUE(SM_APIS, (SM_APIS)(j + SM_OPEN_BEGIN)));
            sm_switchesToState(&goToState);
            sm_switches_from_state_to_created(&goToState);

            ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&goToState.targetStateAPICalledInNextLine, 1, UINT32_MAX));
            
            LogInfo("time[s]=%.2f, main thread: sleeping %" PRIu32 " miliseconds letting switchesToState thread finish its call", (timer_global_get_elapsed_ms() - timeSinceTestFunctionStartMs) / 1000, THREAD_DELAY);
            ThreadAPI_Sleep(THREAD_DELAY);

            LogInfo("time[s]=%.2f, went to state=%" PRI_MU_ENUM "; calling=%" PRI_MU_ENUM "", (timer_global_get_elapsed_ms() - timeSinceTestFunctionStartMs) / 1000, MU_ENUM_VALUE(SM_STATES, (SM_STATES)(i + SM_CREATED)), MU_ENUM_VALUE(SM_APIS, (SM_APIS)(j + SM_OPEN_BEGIN)));

            switch (j)
            {
                case 0:/*sm_open_begin*/
                {
                    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWake(&goToState.targetAPICalledInNextLine, 1));
                    ASSERT_ARE_EQUAL(SM_RESULT, expected[i][j].expected_sm_result, sm_open_begin(goToState.sm));
                    break;
                }
                case 1:/*sm_close_begin*/
                {
                    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWake(&goToState.targetAPICalledInNextLine, 1));
                    ASSERT_ARE_EQUAL(SM_RESULT, expected[i][j].expected_sm_result, sm_close_begin(goToState.sm));
                    break;
                }
                case 2:/*sm_exec_begin*/
                {
                    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWake(&goToState.targetAPICalledInNextLine, 1));
                    ASSERT_ARE_EQUAL(SM_RESULT, expected[i][j].expected_sm_result, sm_exec_begin(goToState.sm));
                    sm_exec_end(goToState.sm);
                    break;
                }
                case 3:/*sm_barrier_begin*/
                {
                    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWake(&goToState.targetAPICalledInNextLine, 1));
                    ASSERT_ARE_EQUAL(SM_RESULT, expected[i][j].expected_sm_result, sm_barrier_begin(goToState.sm));
                    break;
                }
                default:
                {
                    ASSERT_FAIL("unknown action");
                }
            }

            LogInfo("time[s]=%.2f, went to state=%" PRI_MU_ENUM " and called =%" PRI_MU_ENUM " switchesFromStateToCreated thread might already have run", (timer_global_get_elapsed_ms() - timeSinceTestFunctionStartMs) / 1000, MU_ENUM_VALUE(SM_STATES, (SM_STATES)(i + SM_CREATED)), MU_ENUM_VALUE(SM_APIS, (SM_APIS)(j + SM_OPEN_BEGIN)));

            int dont_care;
            ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(goToState.threadSwitchesTo, &dont_care));
            ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(goToState.threadBack, &dont_care));

            sm_destroy(goToState.sm);
        }
    }
}

END_TEST_SUITE(sm_int_tests)
