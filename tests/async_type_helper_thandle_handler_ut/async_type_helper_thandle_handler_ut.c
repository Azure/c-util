// Copyright (c) Microsoft. All rights reserved.


#include "async_type_helper_thandle_handler_ut_pch.h"

static struct G_TAG /*g comes from "global*/
{
    THANDLE(TEST_THANDLE) test_thandle;
} g = { NULL };

typedef TEST_THANDLE real_TEST_THANDLE;

THANDLE_TYPE_DECLARE(real_TEST_THANDLE);
THANDLE_TYPE_DEFINE(real_TEST_THANDLE);

THANDLE(TEST_THANDLE) test_thandle_create(void)
{
    return THANDLE_MALLOC(real_TEST_THANDLE)(NULL);
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_flex, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(TEST_THANDLE), THANDLE_ASSIGN(real_TEST_THANDLE));
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(TEST_THANDLE), THANDLE_INITIALIZE(real_TEST_THANDLE));

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(TEST_THANDLE), void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    THANDLE(TEST_THANDLE) temp_test_thandle = test_thandle_create();
    ASSERT_IS_NOT_NULL(temp_test_thandle);
    THANDLE_INITIALIZE_MOVE(real_TEST_THANDLE)(&g.test_thandle, &temp_test_thandle);

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    THANDLE_ASSIGN(real_TEST_THANDLE)(&g.test_thandle, NULL);
}

/* DECLARE_ASYNC_TYPE_HELPER_THANDLE_HANDLER */

/* Tests_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_004: [ If dst is NULL, the copy handler shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_type_helper_thandle_copy_handler_with_NULL_dst_fails)
{
    // arrange

    // act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(THANDLE(TEST_THANDLE))(NULL, g.test_thandle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_006: [ Otherwise the copy handler shall assign src to dst by calling THANDLE_ASSIGN. ]*/
/* Tests_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_007: [ The copy handler shall succeed and return 0. ]*/
TEST_FUNCTION(async_type_helper_thandle_copy_handler_succeeds)
{
    // arrange
    THANDLE(TEST_THANDLE) dst = NULL;

    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(TEST_THANDLE)(&dst, g.test_thandle));

    // act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(THANDLE(TEST_THANDLE))(&dst, g.test_thandle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(void_ptr, dst, g.test_thandle);

    // cleanup
    THANDLE_ASSIGN(TEST_THANDLE)(&dst, NULL);
}

/* Tests_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_006: [ Otherwise the copy handler shall assign src to dst by calling THANDLE_ASSIGN. ]*/
/* Tests_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_007: [ The copy handler shall succeed and return 0. ]*/
TEST_FUNCTION(async_type_helper_thandle_copy_handler_succeeds_src_NULL)
{
    // arrange
    THANDLE(TEST_THANDLE) dst = (void*)0x4242;

    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(TEST_THANDLE)(&dst, NULL));

    // act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(THANDLE(TEST_THANDLE))(&dst, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NULL(dst);

    // cleanup
    THANDLE_ASSIGN(TEST_THANDLE)(&dst, NULL);
}

/* Tests_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_009: [ If arg is NULL, the free handler shall return. ]*/
TEST_FUNCTION(async_type_helper_thandle_free_handler_returns)
{
    // arrange
    // act
    ASYNC_TYPE_HELPER_FREE_HANDLER(THANDLE(TEST_THANDLE))(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_010: [ Otherwise the free handler shall release arg by assigning NULL to it. ]*/
TEST_FUNCTION(async_type_helper_thandle_free_handler_decrements_the_ref_count)
{
    // arrange
    THANDLE(TEST_THANDLE) dst = NULL;

    ASSERT_ARE_EQUAL(int, 0, ASYNC_TYPE_HELPER_COPY_HANDLER(THANDLE(TEST_THANDLE))(&dst, g.test_thandle));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(TEST_THANDLE)(IGNORED_ARG, NULL));

    // act
    ASYNC_TYPE_HELPER_FREE_HANDLER(THANDLE(TEST_THANDLE))(dst);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
