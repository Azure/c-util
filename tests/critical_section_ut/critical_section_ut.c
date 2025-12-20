// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "critical_section_ut_pch.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static volatile_atomic int32_t test_access_value = 0;

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

    TEST_SUITE_INITIALIZE(test_suite_intialize)
    {
        ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
        ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");
    }

    TEST_SUITE_CLEANUP(test_class_cleanup)
    {
        umock_c_deinit();
    }

    TEST_FUNCTION_INITIALIZE(function_init)
    {
    }

    TEST_FUNCTION_CLEANUP(function_cleanup)
    {
    }


    /*Tests_SRS_CRITICAL_SECTION_18_001: [ If access_value is NULL, critical_section_enter shall fail and return a non-zero value. ]*/
    TEST_FUNCTION(critical_section_enter_fails_with_NULL_access_value)
    {
        //arrange

        //act
        int result = critical_section_enter(NULL);

        //assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
    }


    /*Tests_SRS_CRITICAL_SECTION_18_002: [ critical_section_enter shall call interlocked_compare_exchange to set access_value to 1 if it was previously 0. ]*/
    /*Tests_SRS_CRITICAL_SECTION_18_003: [ If interlocked_compare_exchange indicates that access_value was changed from 0 to 1, critical_section_enter shall succeed and return 0. ]*/
    TEST_FUNCTION(critical_section_enter_succeeds_on_first_try)
    {
        //arrange
        real_interlocked_exchange(&test_access_value ,0);
        STRICT_EXPECTED_CALL(interlocked_compare_exchange(&test_access_value, 1, 0)).SetReturn(0);

        //act
        int result = critical_section_enter(&test_access_value);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 0, result);
    }


    /*Tests_SRS_CRITICAL_SECTION_18_004: [ Otherwise, critical_section_enter shall call wait_on_address passing access_value. ]*/
    /*Tests_SRS_CRITICAL_SECTION_18_005: [ After wait_on_address returns, critical_section_enter shall loop around to the beginning of the function to call interlocked_compare_exchange again. ]*/
    TEST_FUNCTION(critical_section_enter_succeeds_on_second_try)
    {
        //arrange
        real_interlocked_exchange(&test_access_value ,1);
        STRICT_EXPECTED_CALL(interlocked_compare_exchange(&test_access_value, 1, 0)).SetReturn(1);
        STRICT_EXPECTED_CALL(wait_on_address(&test_access_value, 1, UINT32_MAX));
        STRICT_EXPECTED_CALL(interlocked_compare_exchange(&test_access_value, 1, 0)).SetReturn(0);

        //act
        int result = critical_section_enter(&test_access_value);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 0, result);
    }

    /*Tests_SRS_CRITICAL_SECTION_18_004: [ Otherwise, critical_section_enter shall call wait_on_address passing access_value. ]*/
    /*Tests_SRS_CRITICAL_SECTION_18_005: [ After wait_on_address returns, critical_section_enter shall loop around to the beginning of the function to call interlocked_compare_exchange again. ]*/
    TEST_FUNCTION(critical_section_enter_succeeds_on_third_try_because_why_not_test_this_anyway_it_doesnt_cost_anything)
    {
        //arrange
        real_interlocked_exchange(&test_access_value ,1);
        STRICT_EXPECTED_CALL(interlocked_compare_exchange(&test_access_value, 1, 0)).SetReturn(1);
        STRICT_EXPECTED_CALL(wait_on_address(&test_access_value, 1, UINT32_MAX));
        STRICT_EXPECTED_CALL(interlocked_compare_exchange(&test_access_value, 1, 0)).SetReturn(1);
        STRICT_EXPECTED_CALL(wait_on_address(&test_access_value, 1, UINT32_MAX));
        STRICT_EXPECTED_CALL(interlocked_compare_exchange(&test_access_value, 1, 0)).SetReturn(0);

        //act
        int result = critical_section_enter(&test_access_value);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 0, result);
    }

    /*Tests_SRS_CRITICAL_SECTION_18_006: [ If access_value is NULL, critical_section_leave shall fail and return a non-zero value. ]*/
    TEST_FUNCTION(critical_section_leave_fails_with_NULL_access_value)
    {
        //arrange

        //act
        int result = critical_section_leave(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
    }

    /*Tests_SRS_CRITICAL_SECTION_18_007: [ critical_section_leave shall call interlocked_exchange to set access_value to 0. ]*/
    /*Tests_SRS_CRITICAL_SECTION_18_008: [ critical_section_leave shall call wake_by_address_single to wake any threads that may be waiting for access_value to change. ]*/
    /*Tests_SRS_CRITICAL_SECTION_18_009: [ critical_section_leave shall succeed and return 0. ]*/
    TEST_FUNCTION(critical_section_leave_succeeds)
    {
        //arrange
        real_interlocked_exchange(&test_access_value ,1);
        STRICT_EXPECTED_CALL(interlocked_exchange(&test_access_value, 0));
        STRICT_EXPECTED_CALL(wake_by_address_single(&test_access_value));

        //act
        int result = critical_section_leave(&test_access_value);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 0, result);

    }

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
