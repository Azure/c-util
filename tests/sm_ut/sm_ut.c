// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"

#include "c_pal/interlocked.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_util/interlocked_hl.h"
#include "c_util/log_critical_and_terminate.h"
#undef ENABLE_MOCKS

#include "real_interlocked_hl.h"
#include "real_gballoc_hl.h"

#include "c_util/sm.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

MU_DEFINE_ENUM_STRINGS(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);

static SM_HANDLE TEST_sm_create(void)
{
    SM_HANDLE result;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    result = sm_create("a");
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    return result;
}

BEGIN_TEST_SUITE(sm_unittests)

TEST_SUITE_INITIALIZE(setsBufferTempSize)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    umocktypes_stdint_register_types();

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_HL_GLOBAL_MOCK_HOOK();

    REGISTER_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT);
    REGISTER_TYPE(SM_RESULT, SM_RESULT);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(f)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleans)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_SM_02_001: [ If name is NULL then sm_create shall behave as if name was "NO_NAME". ]*/
/*Tests_SRS_SM_02_002: [ sm_create shall allocate memory for the instance. ]*/
TEST_FUNCTION(sm_create_with_name_NULL_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    ///act
    SM_HANDLE sm = sm_create(NULL);

    ///assert
    ASSERT_IS_NOT_NULL(sm);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_037: [ sm_create shall set state of SM_CREATED and n to 0. ]*/
/*Tests_SRS_SM_02_002: [ sm_create shall allocate memory for the instance. ]*/
TEST_FUNCTION(sm_create_with_name_non_NULL_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    ///act
    SM_HANDLE sm = sm_create("bleeding edge");

    ///assert
    ASSERT_IS_NOT_NULL(sm);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_004: [ If there are any failures then sm_create shall fail and return NULL. ]*/
TEST_FUNCTION(sm_create_unhappy_path)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    SM_HANDLE sm = sm_create("bleeding edge");

    ///assert
    ASSERT_IS_NULL(sm);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SM_02_005: [ If sm is NULL then sm_destroy shall return. ]*/
TEST_FUNCTION(sm_destroy_with_sm_NULL_returns)
{
    ///arrange

    ///act
    sm_destroy(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SM_02_038: [ sm_destroy behave as if sm_close_begin would have been called. ]*/
/*Tests_SRS_SM_02_006: [ sm_destroy shall free all used resources. ]*/
TEST_FUNCTION(sm_destroy_in_SM_CREATED_succeeds)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();

    STRICT_EXPECTED_CALL(free(sm));

    ///act
    sm_destroy(sm);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SM_02_007: [ If sm is NULL then sm_open_begin shall fail and return SM_ERROR. ]*/
TEST_FUNCTION(sm_open_begin_with_sm_NULL_returns_SM_ERROR)
{
    ///arrange

    ///act
    SM_RESULT result = sm_open_begin(NULL);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SM_02_039: [ If the state is not SM_CREATED then sm_open_begin shall return SM_EXEC_REFUSED. ]*/
TEST_FUNCTION(sm_open_begin_in_SM_OPENING_refuses)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    umock_c_reset_all_calls();

    ///act
    result = sm_open_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_42_011: [ If SM_FAULTED_BIT is 1 then sm_open_begin shall return SM_EXEC_REFUSED. ]*/
TEST_FUNCTION(sm_open_begin_with_SM_FAULTED_BIT_set_refuses)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();

    sm_fault(sm);

    umock_c_reset_all_calls();

    ///act
    SM_RESULT result = sm_open_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_040: [ sm_open_begin shall switch the state to SM_OPENING. ]*/
/*Tests_SRS_SM_02_009: [ sm_open_begin shall return SM_EXEC_GRANTED. ]*/
TEST_FUNCTION(sm_open_begin_switches_state_to_SM_OPENING)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();

    ///act (1)
    SM_RESULT result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    ///act (2)
    ///one of the states where sm_close_begin has no power
    result = sm_close_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_010: [ If sm is NULL then sm_open_end shall return. ]*/
TEST_FUNCTION(sm_open_end_with_sm_NULL_returns)
{
    ///arrange

    ///act
    sm_open_end(NULL, true);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SM_02_041: [ If state is not SM_OPENING then sm_open_end shall return. ]*/
TEST_FUNCTION(sm_open_end_in_SM_CREATED_returns)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();

    ///act
    sm_open_end(sm, true);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_074: [ If success is true then sm_open_end shall switch the state to SM_OPENED. ]*/
TEST_FUNCTION(sm_open_end_switches_to_SM_OPENED)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    ///act
    SM_RESULT result1 = sm_exec_begin(sm);
    sm_open_end(sm, true);
    SM_RESULT result2 = sm_exec_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result1);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_exec_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_075: [ If success is false then sm_open_end shall switch the state to SM_CREATED. ]*/
TEST_FUNCTION(sm_open_end_switches_to_SM_CREATED)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    ///act
    SM_RESULT result1 = sm_exec_begin(sm);
    sm_open_end(sm, false);
    SM_RESULT result2 = sm_open_begin(sm); /*sm_open_begin can only be executed in SM_CREATED state*/

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result1);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_open_end(sm, true);
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_075: [ If success is false then sm_open_end shall switch the state to SM_CREATED. ]*/
TEST_FUNCTION(sm_open_end_switches_to_SM_CREATED_and_leaves_SM_FAULTED_BIT)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_fault(sm);

    ///act
    SM_RESULT result1 = sm_exec_begin(sm);
    sm_open_end(sm, false);
    SM_RESULT result2 = sm_open_begin(sm); /*sm_open_begin can only be executed in SM_CREATED state*/
    SM_RESULT result3 = sm_exec_begin(sm); /*sm_exec_begin would fail if the fault wasn't reset*/

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result1);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result2);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result3);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_013: [ If sm is NULL then sm_close_begin shall fail and return SM_ERROR. ]*/
TEST_FUNCTION(sm_close_begin_with_sm_NULL_returns_SM_ERROR)
{
    ///arrange
    SM_RESULT result;

    ///act
    result = sm_close_begin(NULL);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SM_02_045: [ sm_close_begin shall set SM_CLOSE_BIT to 1. ]*/
/*Tests_SRS_SM_02_047: [ If the state is SM_OPENED then sm_close_begin shall switch it to SM_OPENED_DRAINING_TO_CLOSE. ]*/
/*Tests_SRS_SM_02_048: [ sm_close_begin shall wait for n to reach 0. ]*/
/*Tests_SRS_SM_02_049: [ sm_close_begin shall switch the state to SM_CLOSING and return SM_EXEC_GRANTED. ]*/
TEST_FUNCTION(sm_close_begin_in_SM_OPENED_succeeds)
{///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 0, UINT32_MAX));

    ///act
    result = sm_close_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_close_end(sm);
    sm_destroy(sm);

}

/*Tests_SRS_SM_02_045: [ sm_close_begin shall set SM_CLOSE_BIT to 1. ]*/
/*Tests_SRS_SM_02_046: [ If SM_CLOSE_BIT was already 1 then sm_close_begin shall return SM_EXEC_REFUSED. ]*/
TEST_FUNCTION(sm_close_begin_with_SM_CLOSE_BIT_refuses)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    /*sets SM_CLOSE_BIT to 1*/
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 0, UINT32_MAX));
    result = sm_close_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    ///act
    result = sm_close_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_close_end(sm);
    sm_destroy(sm);
}

/*ZRS_SM_02_050, ZRS_02_051 left to int tests*/
/*Tests_SRS_SM_02_052: [ If the state is any other value then sm_close_begin shall return SM_EXEC_REFUSED. ]*/
TEST_FUNCTION(sm_close_begin_in_SM_CREATED_fails)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;

    ///act
    result = sm_close_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_053: [ sm_close_begin shall set SM_CLOSE_BIT to 0. ]*/
TEST_FUNCTION(sm_close_begin_after_close_begin_close_end_open_begin_open_end_succeeds)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;

    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 0, UINT32_MAX));

    result = sm_close_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_close_end(sm);

    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 0, UINT32_MAX));

    ///act
    result = sm_close_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_close_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_071: [ If there are any failures then sm_close_begin shall fail and return SM_ERROR. ]*/
TEST_FUNCTION(sm_close_unhappy_path)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;

    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 0, UINT32_MAX))
        .SetReturn(INTERLOCKED_HL_ERROR);

    ///act
    result = sm_close_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_018: [ If sm is NULL then sm_close_end shall return. ]*/
TEST_FUNCTION(sm_close_end_with_sm_NULL_returns)
{
    ///arrange

    ///act
    sm_close_end(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_SM_02_043: [ If the state is not SM_CLOSING then sm_close_end shall return. ]*/
TEST_FUNCTION(sm_close_end_in_SM_CREATED_returns)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();

    ///act
    sm_close_end(sm);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_044: [ sm_close_end shall switch the state to SM_CREATED. ]*/
TEST_FUNCTION(sm_close_end_switches_state_to_SM_CREATED) /*allows sm_open_begin to be called*/
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;

    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 0, UINT32_MAX));

    result = sm_close_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    umock_c_reset_all_calls();

    ///act
    sm_close_end(sm);

    ///assert
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    sm_open_end(sm, true);

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_42_012: [ sm_close_end shall not reset the SM_FAULTED_BIT. ]*/
TEST_FUNCTION(sm_close_end_switches_state_to_SM_CREATED_leaves_SM_FAULTED_BIT_set)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;

    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 0, UINT32_MAX));

    result = sm_close_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    sm_fault(sm);

    umock_c_reset_all_calls();

    ///act
    sm_close_end(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, sm_open_begin(sm));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_021: [ If sm is NULL then sm_exec_begin shall fail and return SM_ERROR. ]*/
TEST_FUNCTION(sm_exec_begin_with_sm_NULL_returns_SM_ERROR)
{
    ///arrange
    SM_RESULT result;

    ///act
    result = sm_exec_begin(NULL);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SM_02_054: [ If state is not SM_OPENED then sm_exec_begin shall return SM_EXEC_REFUSED. ]*/
TEST_FUNCTION(sm_exec_begin_in_SM_CREATED_returns_SM_EXEC_REFUSED)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;

    umock_c_reset_all_calls();

    ///act
    result = sm_exec_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_055: [ If SM_CLOSE_BIT is 1 then sm_exec_begin shall return SM_EXEC_REFUSED. ]*/ /*left to int tests, SM_CLOSE_BIT is set WHILE sm_close_begin is called*/
/*Tests_SRS_SM_02_057: [ If the state changed after incrementing n then sm_exec_begin shall return SM_EXEC_REFUSED. ]*/ /*left to int tests, SM_CLOSE_BIT is set WHILE sm_close_begin is called*/

/*Tests_SRS_SM_02_056: [ sm_exec_begin shall increment n. ]*/
/*Tests_SRS_SM_02_058: [ sm_exec_begin shall return SM_EXEC_GRANTED. ]*/
TEST_FUNCTION(sm_exec_begin_succeeds)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    umock_c_reset_all_calls();

    ///act
    result = sm_exec_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_exec_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_42_007: [ sm_fault shall set SM_FAULTED_BIT to 1. ]*/
/*Tests_SRS_SM_42_002: [ If SM_FAULTED_BIT is 1 then sm_exec_begin shall return SM_EXEC_REFUSED. ]*/
TEST_FUNCTION(sm_exec_begin_fails_when_faulted)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    sm_fault(sm);

    umock_c_reset_all_calls();

    ///act
    result = sm_exec_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_024: [ If sm is NULL then sm_exec_end shall return. ]*/
TEST_FUNCTION(sm_exec_end_with_sm_NULL_returns)
{
    ///arrange

    ///act
    sm_exec_end(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SM_02_059: [ If state is not SM_OPENED then sm_exec_end shall return. ]*/
/*Tests_SRS_SM_02_060: [ If state is not SM_OPENED_DRAINING_TO_BARRIER then sm_exec_end shall return. ]*/
/*Tests_SRS_SM_02_061: [ If state is not SM_OPENED_DRAINING_TO_CLOSE then sm_exec_end shall return. ]*/
TEST_FUNCTION(sm_exec_end_in_SM_CREATED_returns)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();

    ///act
    sm_exec_end(sm);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_063: [ If n reaches 0 then sm_exec_end shall signal that. ]*/ /*left to int tests...*/
/*Tests_SRS_SM_02_062: [ sm_exec_end shall decrement n with saturation at 0. ]*/
TEST_FUNCTION(sm_exec_end_signals)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    result = sm_exec_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    umock_c_reset_all_calls();

    ///act
    sm_exec_end(sm);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    result = sm_close_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result); /*if somehow n would be -1... sm_close_end would wait forever...*/
    sm_close_end(sm);

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_42_010: [ If n would decrement below 0, then sm_exec_end shall terminate the process. ]*/
TEST_FUNCTION(sm_exec_end_called_additional_time_terminates_process)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    result = sm_exec_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    sm_exec_end(sm);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(ps_util_terminate_process());

    ///act
    sm_exec_end(sm);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    result = sm_close_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result); /*if somehow n would be -1... sm_close_end would wait forever...*/
    sm_close_end(sm);

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_42_013: [ sm_exec_end may be called when SM_FAULTED_BIT is 1. ]*/
TEST_FUNCTION(sm_exec_end_works_when_faulted)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    result = sm_exec_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    sm_fault(sm);

    umock_c_reset_all_calls();

    ///act
    sm_exec_end(sm);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    result = sm_close_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result); /*if somehow n would be -1... sm_close_end would wait forever...*/
    sm_close_end(sm);

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_027: [ If sm is NULL then sm_barrier_begin shall fail and return SM_ERROR. ]*/
TEST_FUNCTION(sm_barrier_begin_with_sm_NULL_returns_SM_ERROR)
{
    ///arrange
    SM_RESULT result;

    ///act
    result = sm_barrier_begin(NULL);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_SM_02_064: [ If state is not SM_OPENED then sm_barrier_begin shall return SM_EXEC_REFUSED. ]*/
TEST_FUNCTION(sm_barrier_begin_in_SM_CREATED_returns_SM_EXEC_REFUSED)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;

    ///act
    result = sm_barrier_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_065: [ If SM_CLOSE_BIT is set to 1 then sm_barrier_begin shall return SM_EXEC_REFUSED. ]*/
TEST_FUNCTION(sm_barrier_begin_with_SM_CLOSE_BIT_set_refuses)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    result = sm_close_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    umock_c_reset_all_calls();

    ///act
    result = sm_barrier_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_close_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_42_007: [ sm_fault shall set SM_FAULTED_BIT to 1. ]*/
/*Tests_SRS_SM_42_003: [ If SM_FAULTED_BIT is set to 1 then sm_barrier_begin shall return SM_EXEC_REFUSED. ]*/
TEST_FUNCTION(sm_barrier_begin_with_SM_FAULTED_BIT_set_refuses)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    sm_fault(sm);

    umock_c_reset_all_calls();

    ///act
    result = sm_barrier_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_066: [ sm_barrier_begin shall switch the state to SM_OPENED_DRAINING_TO_BARRIER. ]*/
/*Tests_SRS_SM_02_069: [ sm_barrier_begin shall switch the state to SM_OPENED_BARRIER and return SM_EXEC_GRANTED. ]*/
TEST_FUNCTION(sm_barrier_begin_returns_SM_EXEC_GRANTED)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result, result1, result2;
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 0, UINT32_MAX));

    ///act
    result1 = sm_barrier_begin(sm);
    result2 = sm_exec_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result1);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_barrier_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_42_014: [ sm_barrier_end may be called when SM_FAULTED_BIT is 1. ]*/
TEST_FUNCTION(sm_barrier_end_works_when_faulted)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 0, UINT32_MAX));

    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, sm_barrier_begin(sm));

    sm_fault(sm);

    ///act
    sm_barrier_end(sm);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_067: [ If the state changed meanwhile then sm_barrier_begin shall return SM_EXEC_REFUSED. ]*/ /*left to int tests*/
/*Tests_SRS_SM_02_068: [ sm_barrier_begin shall wait for n to reach 0. ]*/ /*fixeable maybe by interlcoked mocking*/

/*Tests_SRS_SM_02_070: [ If there are any failures then sm_barrier_begin shall return SM_ERROR. ]*/
TEST_FUNCTION(sm_barrier_begin_fails_when_interlocked_fails)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result;
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 0, UINT32_MAX))
        .SetReturn(INTERLOCKED_HL_ERROR);

    ///act
    result = sm_barrier_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_032: [ If sm is NULL then sm_barrier_end shall return. ]*/
TEST_FUNCTION(sm_barrier_end_with_sm_NULL_returns)
{
    ///arrange

    ///act
    sm_barrier_end(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_SM_02_072: [ If state is not SM_OPENED_BARRIER then sm_barrier_end shall return. ]*/
TEST_FUNCTION(sm_barrier_end_in_SM_CREATED_returns)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();

    ///act
    sm_barrier_end(sm);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_073: [ sm_barrier_end shall switch the state to SM_OPENED. ]*/
TEST_FUNCTION(sm_barrier_begin_switched_to_SM_OPENED)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();
    SM_RESULT result, result1, result2;
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 0, UINT32_MAX));
    result = sm_barrier_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    umock_c_reset_all_calls();

    ///act
    result1 = sm_exec_begin(sm);
    sm_barrier_end(sm);
    result2 = sm_exec_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result1);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_exec_end(sm);
    sm_destroy(sm);
}

//
// sm_fault
//

/*Tests_SRS_SM_42_004: [ If sm is NULL then sm_fault shall return. ]*/
TEST_FUNCTION(sm_fault_with_NULL_sm_returns)
{
    ///arrange

    ///act
    sm_fault(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_SM_42_007: [ sm_fault shall set SM_FAULTED_BIT to 1. ]*/
TEST_FUNCTION(sm_fault_in_SM_CREATED_disallows_open)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();

    ///act
    sm_fault(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, sm_open_begin(sm));
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, sm_close_begin(sm));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_42_007: [ sm_fault shall set SM_FAULTED_BIT to 1. ]*/
TEST_FUNCTION(sm_fault_in_SM_CLOSING_prevents_open)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();

    SM_RESULT result;

    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm, true);

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 0, UINT32_MAX));

    result = sm_close_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    ///act
    sm_fault(sm);

    ///assert
    sm_close_end(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, sm_open_begin(sm));
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, sm_close_begin(sm));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

END_TEST_SUITE(sm_unittests)


