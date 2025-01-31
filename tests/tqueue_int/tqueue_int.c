// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <limits.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/sync.h"
#include "c_pal/thandle.h"
#include "c_pal/threadapi.h"
#include "c_pal/timer.h"

#include "c_util/tqueue.h"
#include "tqueue_foo.h"

// TQUEUE(FOO) is used for most int tests
// It is declared and defined in its own .h/.c files in order to emulate usage in the wilderness

// A queue with THANDLEs!
typedef struct TEST_THANDLE_TAG
{
    int64_t a_value;
} TEST_THANDLE;

THANDLE_TYPE_DECLARE(TEST_THANDLE);
THANDLE_TYPE_DEFINE(TEST_THANDLE);

TQUEUE_DEFINE_STRUCT_TYPE(THANDLE(TEST_THANDLE));
THANDLE_TYPE_DECLARE(TQUEUE_TYPEDEF_NAME(THANDLE(TEST_THANDLE)))
TQUEUE_TYPE_DECLARE(THANDLE(TEST_THANDLE));

THANDLE_TYPE_DEFINE(TQUEUE_TYPEDEF_NAME(THANDLE(TEST_THANDLE)))
TQUEUE_TYPE_DEFINE(THANDLE(TEST_THANDLE));

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(TQUEUE_PUSH_RESULT, TQUEUE_PUSH_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(TQUEUE_POP_RESULT, TQUEUE_POP_RESULT_VALUES);

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
}

TEST_FUNCTION_INITIALIZE(method_init)
{
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

TEST_FUNCTION(TQUEUE_CREATE_with_NULL_callbacks_succeds)
{
    // arrange

    // act
    TQUEUE(FOO) queue = TQUEUE_CREATE(FOO)(16, 1024, NULL, NULL, NULL);

    // assert
    ASSERT_IS_NOT_NULL(queue);

    // clean
    TQUEUE_ASSIGN(FOO)(&queue, NULL);
}

TEST_FUNCTION(TQUEUE_PUSH_succeeds)
{
    // arrange
    TQUEUE(FOO) queue = TQUEUE_CREATE(FOO)(16, 1024, NULL, NULL, NULL);
    ASSERT_IS_NOT_NULL(queue);
    FOO foo_1 = { 42 };

    // act
    TQUEUE_PUSH_RESULT result = TQUEUE_PUSH(FOO)(queue, &foo_1, NULL);

    // assert
    ASSERT_ARE_EQUAL(TQUEUE_PUSH_RESULT, TQUEUE_PUSH_OK, result);

    // clean
    TQUEUE_ASSIGN(FOO)(&queue, NULL);
}

TEST_FUNCTION(TQUEUE_POP_succeeds)
{
    // arrange
    TQUEUE(FOO) queue = TQUEUE_CREATE(FOO)(16, 1024, NULL, NULL, NULL);
    ASSERT_IS_NOT_NULL(queue);
    FOO foo_1 = { 42 };
    FOO foo_1_popped;
    ASSERT_ARE_EQUAL(TQUEUE_PUSH_RESULT, TQUEUE_PUSH_OK, TQUEUE_PUSH(FOO)(queue, &foo_1, NULL));

    // act
    TQUEUE_POP_RESULT result = TQUEUE_POP(FOO)(queue, &foo_1_popped, NULL, NULL, NULL);

    // assert
    ASSERT_ARE_EQUAL(TQUEUE_POP_RESULT, TQUEUE_POP_OK, result);
    ASSERT_ARE_EQUAL(int32_t, foo_1.x, foo_1_popped.x);

    // clean
    TQUEUE_ASSIGN(FOO)(&queue, NULL);
}

TEST_FUNCTION(TQUEUE_GET_COUNT_succeeds)
{
    // arrange
    TQUEUE(FOO) queue = TQUEUE_CREATE(FOO)(16, 1024, NULL, NULL, NULL);
    ASSERT_IS_NOT_NULL(queue);
    FOO foo_1 = { 42 };
    ASSERT_ARE_EQUAL(TQUEUE_PUSH_RESULT, TQUEUE_PUSH_OK, TQUEUE_PUSH(FOO)(queue, &foo_1, NULL));

    // act
    int64_t result = TQUEUE_GET_VOLATILE_COUNT(FOO)(queue);

    // assert
    ASSERT_ARE_EQUAL(int64_t, 1, result);

    // clean
    TQUEUE_ASSIGN(FOO)(&queue, NULL);
}

static bool pop_condition_function_42(void* context, FOO* foo)
{
    (void)context;
    return (foo->x == 42) ? true : false;
}

static bool pop_condition_function_43(void* context, FOO* foo)
{
    (void)context;
    return (foo->x == 43) ? true : false;
}

TEST_FUNCTION(TQUEUE_POP_IF_succeeds)
{
    // arrange
    TQUEUE(FOO) queue = TQUEUE_CREATE(FOO)(16, 1024, NULL, NULL, NULL);
    ASSERT_IS_NOT_NULL(queue);
    FOO foo_1 = { .x = 42 };
    FOO foo_1_popped = { .x = 45 };
    ASSERT_ARE_EQUAL(TQUEUE_PUSH_RESULT, TQUEUE_PUSH_OK, TQUEUE_PUSH(FOO)(queue, &foo_1, NULL));

    // act
    // assert
    ASSERT_ARE_EQUAL(TQUEUE_POP_RESULT, TQUEUE_POP_REJECTED, TQUEUE_POP(FOO)(queue, &foo_1_popped, NULL, pop_condition_function_43, NULL));
    ASSERT_ARE_NOT_EQUAL(int32_t, foo_1.x, foo_1_popped.x);
    ASSERT_ARE_EQUAL(int64_t, 1, TQUEUE_GET_VOLATILE_COUNT(FOO)(queue));
    ASSERT_ARE_EQUAL(TQUEUE_POP_RESULT, TQUEUE_POP_OK, TQUEUE_POP(FOO)(queue, &foo_1_popped, NULL, pop_condition_function_42, NULL));
    ASSERT_ARE_EQUAL(int32_t, foo_1.x, foo_1_popped.x);
    ASSERT_ARE_EQUAL(int64_t, 0, TQUEUE_GET_VOLATILE_COUNT(FOO)(queue));

    // clean
    TQUEUE_ASSIGN(FOO)(&queue, NULL);
}

typedef struct TQUEUE_CHAOS_TEST_CONTEXT_TAG
{
    TQUEUE(FOO) queue;
    volatile_atomic int64_t next_push_number;
    volatile_atomic int64_t successful_push_count;
    volatile_atomic int64_t successful_pop_count;
    volatile_atomic int32_t terminate_test;
} TQUEUE_CHAOS_TEST_CONTEXT;

#define TQUEUE_ACTION_TYPE_VALUES \
    TQUEUE_ACTION_TYPE_PUSH, \
    TQUEUE_ACTION_TYPE_POP \

MU_DEFINE_ENUM_WITHOUT_INVALID(TQUEUE_ACTION_TYPE, TQUEUE_ACTION_TYPE_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(TQUEUE_ACTION_TYPE, TQUEUE_ACTION_TYPE_VALUES);

static void TEST_THANDLE_copy_item(void* context, THANDLE(TEST_THANDLE)* dst, THANDLE(TEST_THANDLE)* src)
{
    (void)context;
    THANDLE_INITIALIZE(TEST_THANDLE)(dst, *src);
}

static void TEST_THANDLE_dispose(void* context, THANDLE(TEST_THANDLE)* item)
{
    (void)context;
    THANDLE_ASSIGN(TEST_THANDLE)(item, NULL);
}

#define CHAOS_TEST_RUNTIME 1000 // ms

static int tqueue_chaos_thread_func(void* arg)
{
    TQUEUE_CHAOS_TEST_CONTEXT* test_context = arg;

    // Checking timeout locally in each thread since yielding of CPU to main thread may be delayed or not materialized
    // fast enough due to high number of threads doing intensive work. Due to this the ctest timeout of 1500 sec may get
    // triggered and kill the test
    double start_time = timer_global_get_elapsed_ms();
    double current_time = start_time;
    while ((interlocked_add(&test_context->terminate_test, 0) == 0) && (current_time - start_time < CHAOS_TEST_RUNTIME))
    {
        TQUEUE_ACTION_TYPE action_type = (TQUEUE_ACTION_TYPE)((uint32_t)rand() * ((MU_COUNT_ARG(TQUEUE_ACTION_TYPE_VALUES))) / ((uint32_t)RAND_MAX + 1));

        switch (action_type)
        {
        default:
            ASSERT_FAIL("Unexpected action type %" PRI_MU_ENUM "", MU_ENUM_VALUE(TQUEUE_ACTION_TYPE, action_type));
            break;

        case TQUEUE_ACTION_TYPE_PUSH:
        {
            int64_t next_push_number = interlocked_increment_64(&test_context->next_push_number);
            FOO item = { .x = next_push_number };
            TQUEUE_PUSH_RESULT push_result = TQUEUE_PUSH(FOO)(test_context->queue, &item, NULL);
            ASSERT_IS_TRUE((push_result == TQUEUE_PUSH_OK) || (push_result == TQUEUE_PUSH_QUEUE_FULL), "TQUEUE_PUSH(FOO) failed with %" PRI_MU_ENUM "", MU_ENUM_VALUE(TQUEUE_PUSH_RESULT, push_result));
            if (push_result == TQUEUE_PUSH_OK)
            {
                (void)interlocked_increment_64(&test_context->successful_push_count);
            }
            break;
        }
        case TQUEUE_ACTION_TYPE_POP:
        {
            FOO item = { .x = -1 };
            TQUEUE_POP_RESULT pop_result = TQUEUE_POP(FOO)(test_context->queue, &item, NULL, NULL, NULL);
            ASSERT_IS_TRUE((pop_result == TQUEUE_POP_OK) || (pop_result == TQUEUE_POP_QUEUE_EMPTY), "TQUEUE_POP(FOO) failed with %" PRI_MU_ENUM "", MU_ENUM_VALUE(TQUEUE_POP_RESULT, pop_result));
            if (pop_result == TQUEUE_POP_OK)
            {
                ASSERT_ARE_NOT_EQUAL(int64_t, -1, item.x);
                (void)interlocked_increment_64(&test_context->successful_pop_count);
            }
            break;
        }
        }

        current_time = timer_global_get_elapsed_ms();
#ifdef USE_VALGRIND
        // yield
        ThreadAPI_Sleep(0);
#endif
    }

    return 0;
}

#define TEST_CHECK_PERIOD 500 // ms

#define N_THREADS 16

// This test is rather chaotic and has a number of threads performing random actions on the queue
TEST_FUNCTION(TQUEUE_chaos_knight_test)
{
    // arrange
    TQUEUE_CHAOS_TEST_CONTEXT test_context = { .queue = TQUEUE_CREATE(FOO)(16, 1024, NULL, NULL, NULL) };
    ASSERT_IS_NOT_NULL(test_context.queue);

    (void)interlocked_exchange_64(&test_context.next_push_number, 1);

    // count how many successful pushes and pops we have
    (void)interlocked_exchange_64(&test_context.successful_push_count, 0);
    (void)interlocked_exchange_64(&test_context.successful_pop_count, 0);
    (void)interlocked_exchange(&test_context.terminate_test, 0);

    // act
    // assert
    THREAD_HANDLE thread_handles[N_THREADS];
    for (uint32_t i = 0; i < N_THREADS; i++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&thread_handles[i], tqueue_chaos_thread_func, &test_context));
    }

    double start_time = timer_global_get_elapsed_ms();
    double current_time = start_time;
    do
    {
        // get how many pushes and pops at the beginning of the time slice
        int64_t last_successful_push_count = interlocked_add_64(&test_context.successful_push_count, 0);
        int64_t last_successful_pop_count = interlocked_add_64(&test_context.successful_pop_count, 0);

        ThreadAPI_Sleep(TEST_CHECK_PERIOD);

        // get how many pushes and pops at the end of the time slice
        int64_t current_successful_push_count = interlocked_add_64(&test_context.successful_push_count, 0);
        int64_t current_successful_pop_count = interlocked_add_64(&test_context.successful_pop_count, 0);

        // make sure we had at least one successful push and one pop (not stuck)
        ASSERT_IS_TRUE(current_successful_push_count > last_successful_push_count);
        ASSERT_IS_TRUE(current_successful_pop_count > last_successful_pop_count);

        current_time = timer_global_get_elapsed_ms();

        LogInfo("%.02f seconds elapsed, successful push count=%" PRId64 ", successful pop count=%" PRId64 "",
            (current_time - start_time) / 1000, current_successful_push_count, current_successful_pop_count);
    } while (current_time - start_time < CHAOS_TEST_RUNTIME);

    // terminate test
    (void)interlocked_exchange(&test_context.terminate_test, 1);

    for (uint32_t i = 0; i < N_THREADS; i++)
    {
        int dont_care;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(thread_handles[i], &dont_care));
    }

    // clean
    TQUEUE_ASSIGN(FOO)(&test_context.queue, NULL);
}

// This test is very similar to the one without the THANDLE
// They could be generated with a macro, but then debugability would really be horrible
typedef struct TQUEUE_CHAOS_TEST_THANDLE_CONTEXT_TAG
{
    TQUEUE(THANDLE(TEST_THANDLE)) queue;
    volatile_atomic int64_t next_push_number;
    volatile_atomic int64_t successful_push_count;
    volatile_atomic int64_t successful_pop_count;
    volatile_atomic int64_t successful_get_volatile_count;
    volatile_atomic int32_t terminate_test;
    uint32_t max_queue_size;
} TQUEUE_CHAOS_TEST_THANDLE_CONTEXT;

static int pusher_threads_func(void* arg)
{
    TQUEUE_CHAOS_TEST_THANDLE_CONTEXT* test_context = arg;

    // Checking timeout locally in each thread since yielding of CPU to main thread may be delayed or not materialized
    // fast enough due to high number of threads doing intensive work. Due to this the ctest timeout of 1500 sec may get
    // triggered and kill the test
    double start_time = timer_global_get_elapsed_ms();
    double current_time = start_time;
    while ((interlocked_add(&test_context->terminate_test, 0) == 0) && (current_time - start_time < CHAOS_TEST_RUNTIME))
    {
        int64_t next_push_number = interlocked_increment_64(&test_context->next_push_number);
        TEST_THANDLE test_thandle = { .a_value = next_push_number };
        THANDLE(TEST_THANDLE) item = THANDLE_CREATE_FROM_CONTENT(TEST_THANDLE)(&test_thandle, NULL, NULL);
        TQUEUE_PUSH_RESULT push_result = TQUEUE_PUSH(THANDLE(TEST_THANDLE))(test_context->queue, &item, NULL);
        ASSERT_IS_TRUE((push_result == TQUEUE_PUSH_OK) || (push_result == TQUEUE_PUSH_QUEUE_FULL), "TQUEUE_PUSH(THANDLE(TEST_THANDLE)) failed with %" PRI_MU_ENUM "", MU_ENUM_VALUE(TQUEUE_PUSH_RESULT, push_result));
        if (push_result == TQUEUE_PUSH_OK)
        {
            (void)interlocked_increment_64(&test_context->successful_push_count);
            wake_by_address_single_64(&test_context->successful_push_count);
        }
        THANDLE_ASSIGN(TEST_THANDLE)(&item, NULL);

        current_time = timer_global_get_elapsed_ms();

#ifdef USE_VALGRIND
        // yield
        ThreadAPI_Sleep(0);
#endif
    }

    return 0;
}

static int popper_threads_func(void* arg)
{
    TQUEUE_CHAOS_TEST_THANDLE_CONTEXT* test_context = arg;

    // Checking timeout locally in each thread since yielding of CPU to main thread may be delayed or not materialized
    // fast enough due to high number of threads doing intensive work. Due to this the ctest timeout of 1500 sec may get
    // triggered and kill the test
    double start_time = timer_global_get_elapsed_ms();
    double current_time = start_time;
    while ((interlocked_add(&test_context->terminate_test, 0) == 0) && (current_time - start_time < CHAOS_TEST_RUNTIME))
    {
        THANDLE(TEST_THANDLE) item = NULL;
        TQUEUE_POP_RESULT pop_result = TQUEUE_POP(THANDLE(TEST_THANDLE))(test_context->queue, &item, NULL, NULL, NULL);
        ASSERT_IS_TRUE((pop_result == TQUEUE_POP_OK) || (pop_result == TQUEUE_POP_QUEUE_EMPTY), "TQUEUE_POP(THANDLE(TEST_THANDLE)) failed with %" PRI_MU_ENUM "", MU_ENUM_VALUE(TQUEUE_POP_RESULT, pop_result));
        if (pop_result == TQUEUE_POP_OK)
        {
            (void)interlocked_increment_64(&test_context->successful_pop_count);
            wake_by_address_single_64(&test_context->successful_pop_count);
            THANDLE_ASSIGN(TEST_THANDLE)(&item, NULL);
        }

        current_time = timer_global_get_elapsed_ms();

#ifdef USE_VALGRIND
        // yield
        ThreadAPI_Sleep(0);
#endif
    }

    return 0;
}

static void test_and_terminate_chaos_test(TQUEUE_CHAOS_TEST_THANDLE_CONTEXT *test_context)
{
    double start_time = timer_global_get_elapsed_ms();
    double current_time = start_time;
    do
    {
        // get how many pushes and pops at the beginning of the time slice
        int64_t last_successful_push_count = interlocked_add_64(&test_context->successful_push_count, 0);
        int64_t last_successful_pop_count = interlocked_add_64(&test_context->successful_pop_count, 0);

        // Ensures that the tests below for current_successful_push_count > last_successful_push_count and/or current_successful_pop_count > last_successful_pop_count
        // do not fail due to multi-threading synchronization issues
        ASSERT_IS_TRUE(INTERLOCKED_HL_OK == InterlockedHL_WaitForNotValue64(&test_context->successful_push_count, last_successful_push_count, INT_MAX));
        ASSERT_IS_TRUE(INTERLOCKED_HL_OK == InterlockedHL_WaitForNotValue64(&test_context->successful_pop_count, last_successful_pop_count, INT_MAX));

        ThreadAPI_Sleep(TEST_CHECK_PERIOD);

        // get how many pushes and pops at the end of the time slice
        int64_t current_successful_push_count = interlocked_add_64(&test_context->successful_push_count, 0);
        int64_t current_successful_pop_count = interlocked_add_64(&test_context->successful_pop_count, 0);
        int64_t current_successful_get_volatile_count = interlocked_add_64(&test_context->successful_get_volatile_count, 0);
        
        // make sure we had at least one successful push and one pop (not stuck)
        ASSERT_IS_TRUE(current_successful_push_count > last_successful_push_count);
        ASSERT_IS_TRUE(current_successful_pop_count > last_successful_pop_count);

        current_time = timer_global_get_elapsed_ms();

        LogInfo("%.02f seconds elapsed, successful push count=%" PRId64 ", successful pop count=%" PRId64 ", successful get_count count=%" PRId64 "",
            (current_time - start_time) / 1000, current_successful_push_count, current_successful_pop_count, current_successful_get_volatile_count);
    } while (current_time - start_time < CHAOS_TEST_RUNTIME);

    // terminate test
    (void)interlocked_exchange(&test_context->terminate_test, 1);
}

static void TQUEUE_test_with_N_pushers_and_N_poppers_with_queue_size(uint32_t initial_queue_size, uint32_t max_queue_size, uint32_t pusher_count, uint32_t popper_count)
{
    // arrange
    TQUEUE_CHAOS_TEST_THANDLE_CONTEXT test_context = { .queue = TQUEUE_CREATE(THANDLE(TEST_THANDLE))(initial_queue_size, max_queue_size, TEST_THANDLE_copy_item, TEST_THANDLE_dispose, NULL) };
    ASSERT_IS_NOT_NULL(test_context.queue);

    // count how many successful pushes and pops we have
    (void)interlocked_exchange_64(&test_context.successful_push_count, 0);
    (void)interlocked_exchange_64(&test_context.successful_pop_count, 0);
    (void)interlocked_exchange_64(&test_context.successful_get_volatile_count, 0);
    (void)interlocked_exchange(&test_context.terminate_test, 0);

    THREAD_HANDLE *pusher_threads = malloc_2(pusher_count, sizeof(THREAD_HANDLE));
    ASSERT_IS_NOT_NULL(pusher_threads);
    THREAD_HANDLE *popper_threads = malloc_2(popper_count, sizeof(THREAD_HANDLE));
    ASSERT_IS_NOT_NULL(popper_threads);

    // act
    for (size_t i = 0; i < pusher_count; i++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&pusher_threads[i], pusher_threads_func, &test_context));
    }

    for (size_t i = 0; i < popper_count; i++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&popper_threads[i], popper_threads_func, &test_context));
    }

    test_and_terminate_chaos_test(&test_context);

    for (size_t i = 0; i < pusher_count; i++)
    {
        int dont_care;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(pusher_threads[i], &dont_care));
    }

    for (size_t i = 0; i < popper_count; i++)
    {
        int dont_care;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(popper_threads[i], &dont_care));
    }

    // assert

    // clean
    free(pusher_threads);
    free(popper_threads);
    TQUEUE_ASSIGN(THANDLE(TEST_THANDLE))(&test_context.queue, NULL);
}

// This test has one pusher and one popper and validates the fact that order is preserved in this case
// queue size used is 16
TEST_FUNCTION(TQUEUE_test_with_1_pusher_and_1_popper_queue_size_16)
{
    TQUEUE_test_with_N_pushers_and_N_poppers_with_queue_size(16, 16, 1, 1);
}

// This test has one pusher and one popper and validates the fact that order is preserved in this case
// queue size used is 1 for maximum torture
TEST_FUNCTION(TQUEUE_test_with_1_pusher_and_1_popper_queue_size_1)
{
    TQUEUE_test_with_N_pushers_and_N_poppers_with_queue_size(1, 1, 1, 1);
}

static bool TEST_THANDLE_should_pop(void* context, THANDLE(TEST_THANDLE)* item)
{
    (void)context;
    (void)item;
    int should_pop = ((uint32_t)rand() * 2 / ((uint32_t)RAND_MAX + 1));
    return (should_pop != 0);
}

#define TQUEUE_ACTION_TYPE_THANDLE_TEST_VALUES \
    TQUEUE_ACTION_TYPE_THANDLE_TEST_PUSH, \
    TQUEUE_ACTION_TYPE_THANDLE_TEST_POP, \
    TQUEUE_ACTION_TYPE_THANDLE_TEST_POP_WITH_CONDITION_FUNCTION, \
    TQUEUE_ACTION_TYPE_THANDLE_TEST_GET_VOLATILE_COUNT

MU_DEFINE_ENUM_WITHOUT_INVALID(TQUEUE_ACTION_TYPE_THANDLE_TEST, TQUEUE_ACTION_TYPE_THANDLE_TEST_VALUES);
MU_DEFINE_ENUM_STRINGS_WITHOUT_INVALID(TQUEUE_ACTION_TYPE_THANDLE_TEST, TQUEUE_ACTION_TYPE_THANDLE_TEST_VALUES);

MU_DEFINE_ENUM_STRINGS(QUEUE_ENTRY_STATE, QUEUE_ENTRY_STATE_VALUES);

static int tqueue_chaos_thread_THANDLE_func(void* arg)
{
    TQUEUE_CHAOS_TEST_THANDLE_CONTEXT* test_context = arg;

    // Checking timeout locally in each thread since yielding of CPU to main thread may be delayed or not materialized
    // fast enough due to high number of threads doing intensive work. Due to this the ctest timeout of 1500 sec may get
    // triggered and kill the test
    double start_time = timer_global_get_elapsed_ms();
    double current_time = start_time;
    while ((interlocked_add(&test_context->terminate_test, 0) == 0) && (current_time - start_time < CHAOS_TEST_RUNTIME))
    {
        TQUEUE_ACTION_TYPE_THANDLE_TEST action_type = (TQUEUE_ACTION_TYPE_THANDLE_TEST)((uint32_t)rand() * ((MU_COUNT_ARG(TQUEUE_ACTION_TYPE_THANDLE_TEST_VALUES))) / ((uint32_t)RAND_MAX + 1));

        switch (action_type)
        {
        default:
            ASSERT_FAIL("Unexpected action type %" PRI_MU_ENUM "", MU_ENUM_VALUE(TQUEUE_ACTION_TYPE, action_type));
            break;

        case TQUEUE_ACTION_TYPE_THANDLE_TEST_PUSH:
        {
            int64_t next_push_number = interlocked_increment_64(&test_context->next_push_number);
            TEST_THANDLE test_thandle = { .a_value = next_push_number };
            THANDLE(TEST_THANDLE) item = THANDLE_CREATE_FROM_CONTENT(TEST_THANDLE)(&test_thandle, NULL, NULL);
            TQUEUE_PUSH_RESULT push_result = TQUEUE_PUSH(THANDLE(TEST_THANDLE))(test_context->queue, &item, NULL);
            ASSERT_IS_TRUE((push_result == TQUEUE_PUSH_OK) || (push_result == TQUEUE_PUSH_QUEUE_FULL), "TQUEUE_PUSH(THANDLE(TEST_THANDLE)) failed with %" PRI_MU_ENUM "", MU_ENUM_VALUE(TQUEUE_PUSH_RESULT, push_result));
            if (push_result == TQUEUE_PUSH_OK)
            {
                (void)interlocked_increment_64(&test_context->successful_push_count);
                wake_by_address_single_64(&test_context->successful_push_count);
            }
            THANDLE_ASSIGN(TEST_THANDLE)(&item, NULL);
            break;
        }
        case TQUEUE_ACTION_TYPE_THANDLE_TEST_POP:
        {
            THANDLE(TEST_THANDLE) item = NULL;
            TQUEUE_POP_RESULT pop_result = TQUEUE_POP(THANDLE(TEST_THANDLE))(test_context->queue, &item, NULL, NULL, NULL);
            ASSERT_IS_TRUE((pop_result == TQUEUE_POP_OK) || (pop_result == TQUEUE_POP_QUEUE_EMPTY), "TQUEUE_POP(THANDLE(TEST_THANDLE)) failed with %" PRI_MU_ENUM "", MU_ENUM_VALUE(TQUEUE_POP_RESULT, pop_result));
            if (pop_result == TQUEUE_POP_OK)
            {
                ASSERT_ARE_NOT_EQUAL(int64_t, -1, item->a_value);
                (void)interlocked_increment_64(&test_context->successful_pop_count);
                wake_by_address_single_64(&test_context->successful_pop_count);
                THANDLE_ASSIGN(TEST_THANDLE)(&item, NULL);
            }
            break;
        }
        case TQUEUE_ACTION_TYPE_THANDLE_TEST_POP_WITH_CONDITION_FUNCTION:
        {
            THANDLE(TEST_THANDLE) item = NULL;
            TQUEUE_POP_RESULT pop_result = TQUEUE_POP(THANDLE(TEST_THANDLE))(test_context->queue, &item, NULL, TEST_THANDLE_should_pop, NULL);
            ASSERT_IS_TRUE((pop_result == TQUEUE_POP_OK) || (pop_result == TQUEUE_POP_QUEUE_EMPTY) || (pop_result == TQUEUE_POP_REJECTED), "TQUEUE_POP(THANDLE(TEST_THANDLE)) failed with %" PRI_MU_ENUM "", MU_ENUM_VALUE(TQUEUE_POP_RESULT, pop_result));
            if (pop_result == TQUEUE_POP_OK)
            {
                ASSERT_ARE_NOT_EQUAL(int64_t, -1, item->a_value);
                (void)interlocked_increment_64(&test_context->successful_pop_count);
                wake_by_address_single_64(&test_context->successful_pop_count);
                THANDLE_ASSIGN(TEST_THANDLE)(&item, NULL);
            }
            break;
        }
        case TQUEUE_ACTION_TYPE_THANDLE_TEST_GET_VOLATILE_COUNT:
        {
            int64_t current_count = TQUEUE_GET_VOLATILE_COUNT(THANDLE(TEST_THANDLE))(test_context->queue);
            ASSERT_IS_TRUE(((current_count >= 0) && (current_count <= test_context->max_queue_size)));
            (void)interlocked_increment_64(&test_context->successful_get_volatile_count);
        }
        }
        current_time = timer_global_get_elapsed_ms();

#ifdef USE_VALGRIND
        // yield
        ThreadAPI_Sleep(0);
#endif
    }

    return 0;
}

static void TQUEUE_chaos_knight_test_with_THANDLE_template(uint32_t initial_queue_size, uint32_t max_queue_size)
{
    // arrange
    LogInfo("Running chaos test with uint32_t initial_queue_size=%" PRIu32 ", uint32_t max_queue_size=%" PRIu32 "", initial_queue_size, max_queue_size);

    TQUEUE_CHAOS_TEST_THANDLE_CONTEXT test_context = { .queue = TQUEUE_CREATE(THANDLE(TEST_THANDLE))(initial_queue_size, max_queue_size, TEST_THANDLE_copy_item, TEST_THANDLE_dispose, NULL) };
    ASSERT_IS_NOT_NULL(test_context.queue);

    test_context.max_queue_size = max_queue_size;

    (void)interlocked_exchange_64(&test_context.next_push_number, 1);

    // count how many successful pushes and pops we have
    (void)interlocked_exchange_64(&test_context.successful_push_count, 0);
    (void)interlocked_exchange_64(&test_context.successful_pop_count, 0);
    (void)interlocked_exchange_64(&test_context.successful_get_volatile_count, 0);
    (void)interlocked_exchange(&test_context.terminate_test, 0);

    // act
    // assert
    THREAD_HANDLE thread_handles[N_THREADS];
    for (uint32_t i = 0; i < N_THREADS; i++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&thread_handles[i], tqueue_chaos_thread_THANDLE_func, &test_context));
    }

    test_and_terminate_chaos_test(&test_context);

    for (uint32_t i = 0; i < N_THREADS; i++)
    {
        int dont_care;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(thread_handles[i], &dont_care));
    }

    // clean
    TQUEUE_ASSIGN(THANDLE(TEST_THANDLE))(&test_context.queue, NULL);
}

// This test is rather chaotic and has a number of threads performing random actions on the queue
// But on top it is exercising a queue of THANDLE elements!
// It also uses a condition function for popping which randomly rejects pops
TEST_FUNCTION(TQUEUE_chaos_knight_test_with_THANDLE)
{
    TQUEUE_chaos_knight_test_with_THANDLE_template(16, 16);
}

TEST_FUNCTION(MU_C3(TQUEUE_chaos_knight_test_with_THANDLE_queue_size_16_and_1_pusher_and_, N_THREADS, _poppers))
{
    TQUEUE_test_with_N_pushers_and_N_poppers_with_queue_size(16, 16, 1, N_THREADS);
}

TEST_FUNCTION(MU_C5(TQUEUE_chaos_knight_test_with_THANDLE_queue_size_16_and_, N_THREADS, _pushers_and_, N_THREADS, _poppers))
{
    TQUEUE_test_with_N_pushers_and_N_poppers_with_queue_size(16, 16, N_THREADS, N_THREADS);
}

TEST_FUNCTION(MU_C3(TQUEUE_chaos_knight_test_with_THANDLE_queue_size_16_and_, N_THREADS, _pushers_and_1_popper))
{
    TQUEUE_test_with_N_pushers_and_N_poppers_with_queue_size(16, 16, N_THREADS, 1);
}

// And now the same with queues that grow
TEST_FUNCTION(TQUEUE_chaos_knight_test_with_THANDLE_grow_queue)
{
    TQUEUE_chaos_knight_test_with_THANDLE_template(16, 1024 * 1024);
}

TEST_FUNCTION(MU_C5(TQUEUE_chaos_knight_test_with_THANDLE_queue_size_16_and_, N_THREADS, _pushers_, N_THREADS, _poppers_grow_queue))
{
    TQUEUE_test_with_N_pushers_and_N_poppers_with_queue_size(16, 1024 * 1024, N_THREADS, N_THREADS);
}

TEST_FUNCTION(MU_C3(TQUEUE_chaos_knight_test_with_THANDLE_queue_size_16_and_1_pusher_and_, N_THREADS, _poppers_grow_queue))
{
    TQUEUE_test_with_N_pushers_and_N_poppers_with_queue_size(16, 1024 * 1024, 1, N_THREADS);
}

TEST_FUNCTION(MU_C3(TQUEUE_chaos_knight_test_with_THANDLE_queue_size_16_and_, N_THREADS, _pushers_and_1_popper_grow_queue))
{
    TQUEUE_test_with_N_pushers_and_N_poppers_with_queue_size(16, 1024 * 1024, N_THREADS, 1);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
