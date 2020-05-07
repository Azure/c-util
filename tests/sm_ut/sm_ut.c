// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

#include "azure_macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_windows.h"

#define ENABLE_MOCKS

#include "azure_c_util/gballoc.h"
#include "azure_c_util/interlocked_hl.h"
#undef ENABLE_MOCKS

#include "real_interlocked_hl.h"

#include "azure_c_util/sm.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static SM_HANDLE TEST_sm_create(void)
{
    SM_HANDLE result;
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG));
    result = sm_create("a");
    ASSERT_IS_NOT_NULL(result);

    return result;
}

MU_DEFINE_ENUM_STRINGS(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);

BEGIN_TEST_SUITE(sm_unittests)

TEST_SUITE_INITIALIZE(setsBufferTempSize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    umocktypes_windows_register_types();

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_INTERLOCKED_HL_GLOBAL_MOCK_HOOK();

    REGISTER_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT);
    REGISTER_TYPE(SM_RESULT, SM_RESULT);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
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
/*Tests_SRS_SM_02_003: [ sm_create shall set b_now to -1, n to 0, and e to 0 succeed and return a non-NULL value. ]*/
TEST_FUNCTION(sm_create_succeeds_1)
{
    ///arrange
    SM_HANDLE sm;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG));

    ///act
    sm = sm_create(NULL);

    ///assert
    ASSERT_IS_NOT_NULL(sm);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_002: [ sm_create shall allocate memory for the instance. ]*/
/*Tests_SRS_SM_02_003: [ sm_create shall set b_now to -1, n to 0, and e to 0 succeed and return a non-NULL value. ]*/
TEST_FUNCTION(sm_create_succeeds_2)
{
    ///arrange
    SM_HANDLE sm;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG));

    ///act
    sm = sm_create("a");

    ///assert
    ASSERT_IS_NOT_NULL(sm);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_004: [ If there are any failures then sm_create shall fail and return NULL. ]*/
TEST_FUNCTION(sm_create_failing_case)
{
    ///arrange
    SM_HANDLE sm;

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    sm = sm_create("a");

    ///assert
    ASSERT_IS_NULL(sm);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SM_02_005: [ If sm is NULL then sm_destroy shall return. ]*/
TEST_FUNCTION(sm_destroy_with_sm_NULL_returns)
{
    ///act
    sm_destroy(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_SM_02_006: [ sm_destroy shall free all used resources. ]*/
TEST_FUNCTION(sm_destroy_destroys_all_used_resources)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();

    STRICT_EXPECTED_CALL(gballoc_free(sm));

    ///act
    sm_destroy(sm);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_SM_02_007: [ If sm is NULL then sm_open_begin shall fail and return SM_ERROR. ]*/
TEST_FUNCTION(sm_open_begin_with_sm_NULL_fails)
{
    ///arrange
    SM_RESULT result;

    ///act
    result = sm_open_begin(NULL);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SM_02_008: [ If b_now is not -1 then sm_open_begin shall fail and return SM_EXEC_REFUSED. ]*/
/*Tests_SRS_SM_02_009: [ sm_open_begin shall set b_now to 0, succeed and return SM_EXEC_GRANTED. ]*/
TEST_FUNCTION(sm_open_begin_succeeds)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();

    ///act
    result = sm_open_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_open_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_008: [ If b_now is not -1 then sm_open_begin shall fail and return SM_EXEC_REFUSED. ]*/
TEST_FUNCTION(sm_open_begin_after_being_fails)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    ///act - open begin after open begin
    result = sm_open_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_open_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_010: [ If sm is NULL then sm_open_end shall return. ]*/
TEST_FUNCTION(sm_open_end_with_sm_NULL_returns)
{
    ///act
    sm_open_end(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_SM_02_011: [ sm_open_end shall set n to 0, b_now to INT64_MAX and return. ]*/
TEST_FUNCTION(sm_open_end_succeeds)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    ///act - open after open
    sm_open_end(sm);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_012: [ If sm_open_end doesn't follow a call to sm_open_begin then sm_open_end shall return. ]*/
TEST_FUNCTION(sm_open_end_without_sm_open_begin)
{
    ///arrange
    SM_HANDLE sm = TEST_sm_create();

    ///act
    sm_open_end(sm); /*open_end without open_begin*/

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_013: [ If sm is NULL then sm_close_begin shall return. ]*/
TEST_FUNCTION(sm_close_begin_with_sm_NULL_returns)
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

#if 0 /*vld.h sa vedem daca se poate face ceva cu el, da'slabe sanse, ca e barrier cu wait... care nu se face cu UT*/
/*Tests_SRS_SM_02_014: [ sm_close_begin shall set b_now to its own n. ]*/
/*Tests_SRS_SM_02_015: [ If setting b_now to n fails then sm_close_begin shall fail and return a non-zero value. ]*/
TEST_FUNCTION(sm_close_begin_while_barrier_fails_1) /*barrier is sm_open_begin in this test*/
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    ///act - open after open
    result = sm_close_begin(sm);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_open_end(sm);
    sm_destroy(sm);
}
#endif

/*Tests_SRS_SM_02_014: [ sm_close_begin shall set b_now to its own n. ]*/
/*Tests_SRS_SM_02_015: [ If setting b_now to n fails then sm_close_begin shall fail and return a non-zero value. ]*/
TEST_FUNCTION(sm_close_begin_while_barrier_fails_2) /*barrier is sm_close_begin in this test*/
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm);

    result = sm_close_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    
    umock_c_reset_all_calls();

    ///act - begin_close after begin_close 
    result = sm_close_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_close_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_014: [ sm_close_begin shall set b_now to its own n. ]*/
/*Tests_SRS_SM_02_015: [ If setting b_now to n fails then sm_close_begin shall fail and return a non-zero value. ]*/
TEST_FUNCTION(sm_close_begin_while_barrier_fails_3) /*barrier is other API in this test*/
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm);

    result = sm_barrier_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    umock_c_reset_all_calls();

    ///act - close_begin after barrier_begin
    result = sm_close_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_barrier_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_020: [ If there was no sm_open_begin/sm_open_end called previously, sm_close_begin shall fail and return a non-zero value. ]*/
TEST_FUNCTION(sm_close_begin_without_open_fails)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();

    ///act
    result = sm_close_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_014: [ sm_close_begin shall set b_now to its own n. ]*/
/*Tests_SRS_SM_02_016: [ sm_close_begin shall wait for e to be n. ]*/
/*Tests_SRS_SM_02_017: [ sm_close_begin shall succeed and return 0. ]*/
TEST_FUNCTION(sm_close_begin_succeeds_with_0_executing)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm);

    umock_c_reset_all_calls();

    ///act
    result = sm_close_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_close_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_034: [ If there are any failures then sm_close_begin shall fail and return a non-zero value. ]*/
TEST_FUNCTION(sm_close_begin_unhappy_path)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm);

    umock_c_reset_all_calls();

    result = sm_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, INFINITE))
        .SetReturn(INTERLOCKED_HL_ERROR);

    ///act
    result = sm_close_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_close_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_014: [ sm_close_begin shall set b_now to its own n. ]*/
/*Tests_SRS_SM_02_016: [ sm_close_begin shall wait for e to be n. ]*/
/*Tests_SRS_SM_02_017: [ sm_close_begin shall succeed and return 0. ]*/
TEST_FUNCTION(sm_close_begin_succeeds_with_1_executing) /*left to int tests*/
{

}

/*Tests_SRS_SM_02_018: [ If sm is NULL then sm_close_end shall return. ]*/
TEST_FUNCTION(sm_close_end_with_sm_NULL_returns)
{
    ///arrange

    ///act
    sm_close_end(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SM_02_019: [ sm_close_end shall switch b_now to -1. ]*/
TEST_FUNCTION(sm_close_end_switches_b_now_to_minus_1)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();
    
    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm);
    
    umock_c_reset_all_calls();

    ///act
    result = sm_close_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_close_end(sm);

    result = sm_begin(sm); /*find b_now to -1, fails*/

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_021: [ If sm is NULL then sm_begin shall fail and return a non-zero value. ]*/
TEST_FUNCTION(sm_begin_with_sm_NULL_fails)
{
    ///arrange
    SM_RESULT result;

    ///act
    result = sm_begin(NULL);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SM_02_022: [ If current n is greater than b_now then sm_begin shall fail and return a non-zero value. ]*/
/*Tests_SRS_SM_02_023: [ sm_begin shall succeed and return 0. ]*/
TEST_FUNCTION(sm_begin_when_b_now_is_INT64_MAX_succeeds)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();

    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm);

    ///act
    result = sm_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_022: [ If current n is greater than b_now then sm_begin shall fail and return a non-zero value. ]*/
/*Tests_SRS_SM_02_023: [ sm_begin shall succeed and return 0. ]*/
TEST_FUNCTION(sm_begin_without_open_fails)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();

    ///act
    result = sm_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

#if 0 /*vld.h - check if it can be uncommented*/
/*Tests_SRS_SM_02_022: [ If current n is greater than b_now then sm_begin shall fail and return a non-zero value. ]*/
/*Tests_SRS_SM_02_023: [ sm_begin shall succeed and return 0. ]*/
TEST_FUNCTION(sm_begin_middle_of_open_fails)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();

    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    ///act
    result = sm_begin(sm);
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_open_end(sm);
    sm_destroy(sm);
}
#endif

/*Tests_SRS_SM_02_022: [ If current n is greater than b_now then sm_begin shall fail and return a non-zero value. ]*/
/*Tests_SRS_SM_02_023: [ sm_begin shall succeed and return 0. ]*/
TEST_FUNCTION(sm_begin_middle_of_close_fails)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();

    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm);

    result = sm_close_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    umock_c_reset_all_calls();

    ///act
    result = sm_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_close_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_022: [ If current n is greater than b_now then sm_begin shall fail and return a non-zero value. ]*/
/*Tests_SRS_SM_02_023: [ sm_begin shall succeed and return 0. ]*/
TEST_FUNCTION(sm_begin_after_barrier_fails)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();

    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm);

    result = sm_barrier_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    umock_c_reset_all_calls();

    ///act
    result = sm_begin(sm);
    
    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_barrier_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_024: [ If sm is NULL then sm_end shall return. ]*/
TEST_FUNCTION(sm_end_with_sm_NULL_returns)
{
    ///arrange

    ///act
    sm_end(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_SM_02_025: [ sm_end shall increment the number of executed APIs (e). ]*/
/*Tests_SRS_SM_02_026: [ If the number of executed APIs matches the waiting barrier then sm_end shall wake up the waiting barrier. ]*/
TEST_FUNCTION(sm_end_increments_number_of_executed_APIs)
{
    /*left to int tests*/
}

/*Tests_SRS_SM_02_027: [ If sm is NULL then sm_barrier_begin shall fail and return a non-zero value. ]*/
TEST_FUNCTION(sm_barrier_begin_with_sm_NULL_fails)
{
    ///arrange
    SM_RESULT result;

    ///act
    result = sm_barrier_begin(NULL);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_SM_02_028: [ If current n is greater than b_now then sm_barrier_begin shall fail and return a non-zero value. ]*/
TEST_FUNCTION(sm_barrier_begin_after_create_fails)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();

    ///act
    result = sm_barrier_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

#if 0 /*uncomment when code allows for checking sm_barrier_begin_after_open_fails*/
/*Tests_SRS_SM_02_028: [ If current n is greater than b_now then sm_barrier_begin shall fail and return a non-zero value. ]*/
TEST_FUNCTION(sm_barrier_begin_after_open_fails)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();

    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    ///act
    result = sm_barrier_begin(sm);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_open_end(sm);
    sm_destroy(sm);
}
#endif

/*Tests_SRS_SM_02_028: [ If current n is greater than b_now then sm_barrier_begin shall fail and return a non-zero value. ]*/
TEST_FUNCTION(sm_barrier_begin_after_barrier_fails)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();

    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm);

    result = sm_barrier_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    umock_c_reset_all_calls();

    ///act
    result = sm_barrier_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_REFUSED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_barrier_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_029: [ sm_barrier_begin shall wait for the completion of all the previous operations. ]*/
/*Tests_SRS_SM_02_030: [ sm_barrier_begin shall succeed and return 0. ]*/
TEST_FUNCTION(sm_barrier_begin_succeeds)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();

    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm);

    ///act
    result = sm_barrier_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_031: [ If there are any failures then sm_barrier_begin shall fail and return a non-zero value. ]*/
TEST_FUNCTION(sm_barrier_begin_fails_when_wait_for_value_fails)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();

    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm);

    result = sm_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, INFINITE))
        .SetReturn(INTERLOCKED_HL_ERROR);

    ///act
    result = sm_barrier_begin(sm);

    ///assert
    ASSERT_ARE_EQUAL(SM_RESULT, SM_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_end(sm);
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_032: [ If sm is NULL then sm_barrier_end shall return. ]*/
TEST_FUNCTION(sm_barrier_end_with_sm_NULL_returns)
{
    ///act
    sm_barrier_end(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_SM_02_033: [ sm_barrier_end shall increment the number of executed operations (e), switch b_now to INT64_MAX and return, ]*/
TEST_FUNCTION(sm_barrier_end_succeeds)
{
    ///arrange
    SM_RESULT result;
    SM_HANDLE sm = TEST_sm_create();

    result = sm_open_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);
    sm_open_end(sm);
    result = sm_barrier_begin(sm);
    ASSERT_ARE_EQUAL(SM_RESULT, SM_EXEC_GRANTED, result);

    umock_c_reset_all_calls();

    ///act
    sm_barrier_end(sm);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    sm_destroy(sm);
}

END_TEST_SUITE(sm_unittests)

