// Copyright(C) Microsoft Corporation.All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>


#include "testrunnerswitcher.h"

#include "c_pal/threadapi.h"
#include "macro_utils/macro_utils.h"

#include "c_util/interlocked_hl.h"

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

volatile_atomic int32_t globalValue = 10;

/*
Tests:
InterlockedHL_WaitForValue
InterlockedHL_DecrementAndWake
*/
static int decrement_and_wake_helper_thread_function(void* context)
{
    (void)context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&globalValue, 9, UINT32_MAX));
    return 0;
}

TEST_FUNCTION(interlocked_hl_decrement_and_wake_operates_successfully)
{
    // + have a helper thread which waits for decremented value
    // + main thread creates helper thread and decrements value
    // + helper thread wakes up when the global address value is decremented by 1
    // arrange
    globalValue = 10;
    THREAD_HANDLE helper_thread_handle;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&helper_thread_handle, decrement_and_wake_helper_thread_function, NULL));

    // act and assert
    //sleep so that the helper thread can go into wait mode
    ThreadAPI_Sleep(5000);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_DecrementAndWake(&globalValue));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, 9, globalValue);

    // cleanup
    int return_code;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(helper_thread_handle, &return_code));
    ASSERT_ARE_EQUAL(int, 0, return_code);
}

/*
Tests:
InterlockedHL_WaitForValue
InterlockedHL_SetAndWakeAll
*/
static int set_and_wake_all_helper_thread_function(void* context)
{
    (void)context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&globalValue, 15, UINT32_MAX));
    return 0;
}

TEST_FUNCTION(interlocked_hl_set_and_wake_all_operates_successfully)
{
    // + create 10 helper threads which wait on a value
    // + main thread creates helper threads and sets value for the helper threads to wake up
    // + main thread joins on the helper threads and ensures that all threads wake up and terminate
    // arrange
    globalValue = 10;

    enum { NUMBER_OF_HELPER_THREADS = 10 };
    THREAD_HANDLE helper_threads[NUMBER_OF_HELPER_THREADS];

    for (uint32_t i = 0; i < NUMBER_OF_HELPER_THREADS; ++i)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&helper_threads[i], set_and_wake_all_helper_thread_function, NULL));
    }

    // wait time so that all helper threads go into wait mode
    ThreadAPI_Sleep(5000);

    // act and assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWakeAll(&globalValue, 15));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, 15, globalValue);

    // cleanup
    for (uint32_t i = 0; i < NUMBER_OF_HELPER_THREADS; ++i)
    {
        int return_code;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(helper_threads[i], &return_code));
        ASSERT_ARE_EQUAL(int, 0, return_code);
    }
}

/*
Tests:
InterlockedHL_WaitForValue
InterlockedHL_SetAndWake
*/
static int set_and_wake_helper_thread_function(void* context)
{
    (void)context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&globalValue, 20, UINT32_MAX));
    return 0;
}

TEST_FUNCTION(interlocked_hl_set_and_wake_operates_successfully)
{
    // + create 1 helper thread which waits on a value
    // + main thread creates helper thread and sets a value and wakes up the helper thread
    // + helper thread return from wait as the value it is waiting on is same
    // + main thread joins on the helper thread and ensures that the helper thread wakes up and terminates
    // arrange
    globalValue = 10;

    THREAD_HANDLE helper_thread;

    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&helper_thread, set_and_wake_helper_thread_function, NULL));

    // wait time so that all helper thread goes into wait mode
    ThreadAPI_Sleep(5000);

    // act and assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWake(&globalValue, 20));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, 20, globalValue);

    // cleanup
    int return_code;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(helper_thread, &return_code));
    ASSERT_ARE_EQUAL(int, 0, return_code);
}

/*
Tests:
InterlockedHL_WaitForNotValue
InterlockedHL_SetAndWake
*/
static int wait_for_not_value_helper_thread_function(void* context)
{
    (void)context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForNotValue(&globalValue, 25, UINT32_MAX));
    return 0;
}

TEST_FUNCTION(interlocked_hl_wait_for_not_value_operates_successfully)
{
    // + create 1 helper thread which waits on a value
    // + main thread creates helper thread and sets a different value and wakes up the helper thread
    // + helper thread returns from wait as the value changed
    // + main thread joins on the helper thread and ensures that the helper thread wakes up and terminates
    // arrange
    globalValue = 25;

    THREAD_HANDLE helper_thread;

    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&helper_thread, wait_for_not_value_helper_thread_function, NULL));

    // wait time so that all helper thread goes into wait mode as the value is 25
    ThreadAPI_Sleep(5000);

    // act and assert
    //set and wake up helper thread
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWake(&globalValue, 30));
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, 30, globalValue);

    // cleanup
    int return_code;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(helper_thread, &return_code));
    ASSERT_ARE_EQUAL(int, 0, return_code);
}

/*
Tests:
InterlockedHL_Add64WithCeiling
*/
TEST_FUNCTION(interlocked_hl_add64_with_ceiling_operates_successfully)
{
    // + tests the trivial test case of adding a value to 64bit integer
    // arrange
    volatile_atomic int64_t addend = 55;
    int64_t original_addend = 1;
    const int64_t CEILING = 100;
    int64_t value = 20;

    // act and assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_Add64WithCeiling(&addend, CEILING, value, &original_addend));
    ASSERT_ARE_EQUAL(int64_t, 55, original_addend);
    ASSERT_ARE_EQUAL(int64_t, 75, addend);
}

/*
Tests:
InterlockedHL_CompareExchangeIf
*/
static bool helper_int32_compare_function(int32_t target, int32_t exchange)
{
    //always returns true
    (void)target;
    (void)exchange;
    return true;
}

TEST_FUNCTION(interlocked_hl_compare_exchange_if_operates_successfully)
{
    // + tests the trivial test case of successfully exchanging a value into target
    // arrange
    volatile_atomic int32_t target = 60;
    int32_t original_target = 1;
    int32_t exchange = 88;

    // act and assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_CompareExchangeIf(&target, exchange, helper_int32_compare_function, &original_target));
    ASSERT_ARE_EQUAL(int32_t, 60, original_target);
    ASSERT_ARE_EQUAL(int32_t, 88, target);
}

/*
Tests:
InterlockedHL_CompareExchange64If
*/
static bool helper_int64_compare_function(int64_t target, int64_t exchange)
{
    //always returns true
    (void)target;
    (void)exchange;
    return true;
}

TEST_FUNCTION(interlocked_hl_compare_exchange_64_if_operates_successfully)
{
    // + tests the trivial test case of successfully exchanging a value into target
    // arrange
    volatile_atomic int64_t target = 120;
    int64_t original_target = 1;
    int64_t exchange = 97;

    // act and assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_CompareExchange64If(&target, exchange, helper_int64_compare_function, &original_target));
    ASSERT_ARE_EQUAL(int64_t, 120, original_target);
    ASSERT_ARE_EQUAL(int64_t, 97, target);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
