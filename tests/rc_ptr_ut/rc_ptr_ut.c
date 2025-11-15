// Copyright (c) Microsoft. All rights reserved.


#include "rc_ptr_ut_pch.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static void* test_free_func = (void*)0x1000;
static void* test_free_func_context = (void*)0x1001;

static void test_dispose(void* context, void* ptr)
{
    ASSERT_ARE_EQUAL(void_ptr, test_free_func_context, context);
    ASSERT_ARE_EQUAL(void_ptr, test_free_func, ptr);
}

static void check_free_func_called(void* context, void* ptr)
{
    ASSERT_ARE_EQUAL(void_ptr, test_free_func_context, context);
    ASSERT_ARE_EQUAL(int, 0, (*(int*)ptr)++);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_negative_tests_init();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

/* rc_ptr_create_with_move_pointer */

/*Tests_SRS_RC_PTR_43_001: [ If ptr is NULL, rc_ptr_create_with_move_pointer shall fail and return NULL. ]*/
TEST_FUNCTION(rc_ptr_create_with_move_pointer_fails_with_null_ptr)
{
    //arrange

    //act
    THANDLE(RC_PTR) rc_ptr = rc_ptr_create_with_move_pointer(NULL, test_dispose, test_free_func_context);

    //assert
    ASSERT_IS_NULL(rc_ptr);
}

/*Tests_SRS_RC_PTR_43_002: [ rc_ptr_create_with_move_pointer create a THANDLE(RC_PTR) by calling THANDLE_MALLOC with rc_ptr_dispose as the dispose function. ]*/
/*Tests_SRS_RC_PTR_43_003: [ rc_ptr_create_with_move_pointer shall store the given ptr, free_func and free_func_context in the created THANDLE(RC_PTR). ]*/
/*Tests_SRS_RC_PTR_43_005: [ rc_ptr_create_with_move_pointer shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(rc_ptr_create_with_move_pointer_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    //act
    THANDLE(RC_PTR) rc_ptr = rc_ptr_create_with_move_pointer(test_free_func, test_dispose, test_free_func_context);

    //assert
    ASSERT_IS_NOT_NULL(rc_ptr);
    ASSERT_ARE_EQUAL(void_ptr, test_free_func, rc_ptr->ptr);
    ASSERT_ARE_EQUAL(void_ptr, test_free_func_context, rc_ptr->free_func_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(RC_PTR)(&rc_ptr, NULL);
}

/*Tests_SRS_RC_PTR_43_004: [ If there are any failures, rc_ptr_create_with_move_pointer shall fail and return NULL. ]*/
TEST_FUNCTION(rc_ptr_with_move_pointer_fail_when_underlying_functions_fail)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
    .SetReturn(NULL);

    //act
    THANDLE(RC_PTR) rc_ptr = rc_ptr_create_with_move_pointer(test_free_func, test_dispose, test_free_func_context);

    //assert
    ASSERT_IS_NULL(rc_ptr);
}

/*Tests_SRS_RC_PTR_43_006: [ If free_func is not NULL, rc_ptr_dispose shall call free_func with free_func_context and ptr. ]*/
TEST_FUNCTION(rc_ptr_dispose_calls_free_func)
{
    //arrange
    int num = 0;
    THANDLE(RC_PTR) rc_ptr = rc_ptr_create_with_move_pointer(&num, check_free_func_called, test_free_func_context);
    ASSERT_IS_NOT_NULL(rc_ptr);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    THANDLE_ASSIGN(RC_PTR)(&rc_ptr, NULL);

    //assert
    ASSERT_ARE_EQUAL(int, 1, num);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
