// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdlib.h>


#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"

static TEST_MUTEX_HANDLE g_testByTest;

#include "c_util/ps_util.h"



MOCK_FUNCTION_WITH_CODE(, void, mock_abort);
MOCK_FUNCTION_END()
MOCK_FUNCTION_WITH_CODE(, void, mock_exit, int, exit_code);
MOCK_FUNCTION_END()




MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        umock_c_deinit();

        TEST_MUTEX_DESTROY(g_testByTest);
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
        if (TEST_MUTEX_ACQUIRE(g_testByTest))
        {
            ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
        }

        umock_c_reset_all_calls();
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        TEST_MUTEX_RELEASE(g_testByTest);
    }

    /* Tests_SRS_PS_UTIL_01_001: [ ps_util_terminate_process shall call abort. ]*/
    TEST_FUNCTION(ps_util_terminate_process_calls_abort)
    {
        ///arrange
        STRICT_EXPECTED_CALL(mock_abort());

        ///act
        ps_util_terminate_process();

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /* Tests_SRS_PS_UTIL_01_002: [ ps_util_exit_process shall call exit, passing exit_code as argument. ]*/
    TEST_FUNCTION(ps_util_exit_process_calls_exit)
    {
        ///arrange
        STRICT_EXPECTED_CALL(mock_exit(42));

        ///act
        ps_util_exit_process(42);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /* Tests_SRS_PS_UTIL_01_002: [ ps_util_exit_process shall call exit, passing exit_code as argument. ]*/
    TEST_FUNCTION(ps_util_exit_process_calls_exit_0_exit_code)
    {
        ///arrange
        STRICT_EXPECTED_CALL(mock_exit(0));

        ///act
        ps_util_exit_process(0);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /* Tests_SRS_PS_UTIL_01_002: [ ps_util_exit_process shall call exit, passing exit_code as argument. ]*/
    TEST_FUNCTION(ps_util_exit_process_calls_negative_exit_code)
    {
        ///arrange
        STRICT_EXPECTED_CALL(mock_exit(-43));

        ///act
        ps_util_exit_process(-43);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
