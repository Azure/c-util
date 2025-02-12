// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_stdint.h"

#define ENABLE_MOCKS

#include "c_pal/sync.h"
#include "c_pal/interlocked.h"
#include "c_pal/ps_util.h"

#undef ENABLE_MOCKS

#include "c_util/critical_section.h"

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
        umock_c_reset_all_calls();
    }

    TEST_FUNCTION_CLEANUP(function_cleanup)
    {
    }


    /*Tests_SRS_CRITICAL_SECTION_18_001: [ If access_value is NULL, enter_crit_section shall terminate the process. ]*/
    TEST_FUNCTION(enter_crit_section_terminates_process_with_NULL_access_value)
    {
        //arrange
        STRICT_EXPECTED_CALL(ps_util_terminate_process());

        //act
        enter_crit_section(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }


    /*Tests_SRS_CRITICAL_SECTION_18_002: [ enter_crit_section shall call interlocked_compare_exchange to set access_value to 1 if it was previously 0. ]*/
    /*Tests_SRS_CRITICAL_SECTION_18_003: [ If interlocked_compare_exchange indicates that access_value was changed from 0 to 1, enter_crit_section returns. ]*/
    TEST_FUNCTION(enter_crit_section_succeeds_on_first_try)
    {
        //arrange
        test_access_value = 0;
        STRICT_EXPECTED_CALL(interlocked_compare_exchange(&test_access_value, 1, 0)).SetReturn(0);

        //act
        enter_crit_section(&test_access_value);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }


    /*Tests_SRS_CRITICAL_SECTION_18_004: [ Otherwise, enter_crit_section shall call wait_on_address passing access_value. ]*/
    /*Tests_SRS_CRITICAL_SECTION_18_005: [ After wait_on_address returns, enter_crit_section shall loop around to the beginning of the function to call interlocked_compare_exchange again. ]*/
    TEST_FUNCTION(enter_crit_section_succeeds_on_second_try)
    {
        //arrange
        test_access_value = 1;
        STRICT_EXPECTED_CALL(interlocked_compare_exchange(&test_access_value, 1, 0)).SetReturn(1);
        STRICT_EXPECTED_CALL(wait_on_address(&test_access_value, 1, UINT32_MAX));
        STRICT_EXPECTED_CALL(interlocked_compare_exchange(&test_access_value, 1, 0)).SetReturn(0);

        //act
        enter_crit_section(&test_access_value);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /*Tests_SRS_CRITICAL_SECTION_18_004: [ Otherwise, enter_crit_section shall call wait_on_address passing access_value. ]*/
    /*Tests_SRS_CRITICAL_SECTION_18_005: [ After wait_on_address returns, enter_crit_section shall loop around to the beginning of the function to call interlocked_compare_exchange again. ]*/
    TEST_FUNCTION(enter_crit_section_succeeds_on_third_try_because_why_not_test_this_anyway_it_doesnt_cost_anything)
    {
        //arrange
        test_access_value = 1;
        STRICT_EXPECTED_CALL(interlocked_compare_exchange(&test_access_value, 1, 0)).SetReturn(1);
        STRICT_EXPECTED_CALL(wait_on_address(&test_access_value, 1, UINT32_MAX));
        STRICT_EXPECTED_CALL(interlocked_compare_exchange(&test_access_value, 1, 0)).SetReturn(1);
        STRICT_EXPECTED_CALL(wait_on_address(&test_access_value, 1, UINT32_MAX));
        STRICT_EXPECTED_CALL(interlocked_compare_exchange(&test_access_value, 1, 0)).SetReturn(0);

        //act
        enter_crit_section(&test_access_value);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /*Tests_SRS_CRITICAL_SECTION_18_006: [ If access_value is NULL, leave_crit_section shall terminate the process. ]*/
    TEST_FUNCTION(leave_crit_section_terminates_process_with_NULL_access_value)
    {
        //arrange
        STRICT_EXPECTED_CALL(ps_util_terminate_process());

        //act
        leave_crit_section(NULL);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /*Tests_SRS_CRITICAL_SECTION_18_007: [ leave_crit_section shall call interlocked_exchange to set access_value to 0. ]*/
    /*Tests_SRS_CRITICAL_SECTION_18_008: [ leave_crit_section shall call wake_by_address_single to wake any threads that may be waiting for access_value to change. ]*/
    TEST_FUNCTION(leave_crit_section_succeeds)
    {
        //arrange
        test_access_value = 1;
        STRICT_EXPECTED_CALL(interlocked_exchange(&test_access_value, 0));
        STRICT_EXPECTED_CALL(wake_by_address_single(&test_access_value));

        //act
        leave_crit_section(&test_access_value);

        //assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)