// Copyright(C) Microsoft Corporation.All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>


#include "testrunnerswitcher.h"

#include "c_pal/threadapi.h"
#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h"
#include "c_logging/xlogging.h"
#include "c_util/rc_string_array.h"
#include "c_util/interlocked_hl.h"

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

// If we are building with VLD, then the test tool also has VLD
// VLD is set to be disabled for the exe, but that still prints two lines:
//   Visual Leak Detector read settings from: PATH_TO_CMAKE_BUILD\external_command_helper_int_exe\ext\vld.ini
//   Visual Leak Detector is turned off.
// Just ignore those lines
#if defined _DEBUG && defined VLD_OPT_REPORT_TO_STDOUT
#define ADDITIONAL_LINE_COUNT 2
#else
#define ADDITIONAL_LINE_COUNT 0
#endif

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(function_initialize)
{
}

TEST_FUNCTION_CLEANUP(function_cleanup)
{
}

volatile int32_t globalValue = 10;
static int decrement_and_wake_helper_thread_function(void* context)
{
    (void)context;
    INTERLOCKED_HL_RESULT result = InterlockedHL_WaitForValue(&globalValue, 9, UINT32_MAX);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
    return 0;
}

/*
Tests: 
InterlockedHL_WaitForValue
InterlockedHL_DecrementAndWake
*/
TEST_FUNCTION(decrement_and_wake_operates_successfully)
{
    // + have a helper thread which waits for decremented value
    // + main thread creates helper thread and decrements value
    // + helper thread wakes up when the global address value is decremented by 1
    // arrange
    globalValue = 10;
    THREAD_HANDLE helper_thread_handle;
    THREADAPI_RESULT thread_result = ThreadAPI_Create(&helper_thread_handle, decrement_and_wake_helper_thread_function, NULL);
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, thread_result);

    // act
    //sleep so that the helper thread can go into wait mode
    ThreadAPI_Sleep(5000);
    INTERLOCKED_HL_RESULT result = InterlockedHL_DecrementAndWake(&globalValue);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);

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

static int set_and_wakeall_helper_thread_function(void* context)
{
    (void)context;
    INTERLOCKED_HL_RESULT result = InterlockedHL_WaitForValue(&globalValue, 15, UINT32_MAX);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
    return 0;
}

TEST_FUNCTION(set_and_wakeall_operates_successfully)
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
        THREADAPI_RESULT thread_result = ThreadAPI_Create(&helper_threads[i], set_and_wakeall_helper_thread_function, NULL);
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, thread_result);
    }

    // wait time so that all helper threads go into wait mode
    ThreadAPI_Sleep(5000);

    // act
    INTERLOCKED_HL_RESULT result = InterlockedHL_SetAndWakeAll(&globalValue, 15);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);

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
    INTERLOCKED_HL_RESULT result = InterlockedHL_WaitForValue(&globalValue, 20, UINT32_MAX);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
    return 0;
}

TEST_FUNCTION(set_and_wake_operates_successfully)
{
    // + create 1 helper thread which waits on a value
    // + main thread creates helper thread and sets value for the helper thread to wake up
    // + main thread joins on the helper thread and ensures that the helper thread wakes up and terminates
    // arrange
    globalValue = 10;

    THREAD_HANDLE helper_thread;

    THREADAPI_RESULT thread_result = ThreadAPI_Create(&helper_thread, set_and_wake_helper_thread_function, NULL);
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, thread_result);

    // wait time so that all helper thread goes into wait mode
    ThreadAPI_Sleep(5000);

    // act
    INTERLOCKED_HL_RESULT result = InterlockedHL_SetAndWake(&globalValue, 20);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);

    // cleanup
    int return_code;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(helper_thread, &return_code));
    ASSERT_ARE_EQUAL(int, 0, return_code);
}

/*
Tests:
InterlockedHL_WaitForNotValue
*/

static int wait_for_not_value_helper_thread_function(void* context)
{
    (void)context;
    INTERLOCKED_HL_RESULT result = InterlockedHL_WaitForNotValue(&globalValue, 25, UINT32_MAX);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
    return 0;
}

TEST_FUNCTION(wait_for_not_value_operates_successfully)
{
    // + create 1 helper thread which waits on a value
    // + main thread creates helper thread and sets value for the helper thread to wake up
    // + main thread joins on the helper thread and ensures that the helper thread wakes up and terminates
    // arrange
    globalValue = 25;

    THREAD_HANDLE helper_thread;

    THREADAPI_RESULT thread_result = ThreadAPI_Create(&helper_thread, wait_for_not_value_helper_thread_function, NULL);
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, thread_result);

    // wait time so that all helper thread goes into wait mode as the value is 25
    ThreadAPI_Sleep(5000);

    // act
    //set and wake up helper thread
    INTERLOCKED_HL_RESULT result = InterlockedHL_SetAndWake(&globalValue, 30);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);

    // cleanup
    int return_code;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(helper_thread, &return_code));
    ASSERT_ARE_EQUAL(int, 0, return_code);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
