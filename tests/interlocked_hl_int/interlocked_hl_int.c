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
    ASSERT_IS_NULL(context);
    ThreadAPI_Sleep(1000);
    InterlockedHL_DecrementAndWake(&globalValue);
    return 0;
}

TEST_FUNCTION(decrement_and_wake_operates_successfully)
{
    // + have a helper thread which decrements and wakes a global address
    // + main thread creates helper thread and waits on the global address
    // + main thread wakes up and checks that the global address value is decremented by 1
    // arrange
    globalValue = 10;
    THREAD_HANDLE helper_thread_handle;
    THREADAPI_RESULT thread_result = ThreadAPI_Create(&helper_thread_handle, decrement_and_wake_helper_thread_function, NULL);
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, thread_result);

    // act
    INTERLOCKED_HL_RESULT result = InterlockedHL_WaitForValue(&globalValue, 9, UINT32_MAX);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);

    // cleanup
    (void)ThreadAPI_Join(helper_thread_handle, NULL);
}


END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
