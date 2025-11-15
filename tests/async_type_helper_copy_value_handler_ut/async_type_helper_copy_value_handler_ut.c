// Copyright (c) Microsoft. All rights reserved.



#include "async_type_helper_copy_value_handler_ut_pch.h"

typedef struct MY_STRUCT_TAG
{
    uint32_t a;
    uint8_t b;
} MY_STRUCT;

/* Tests_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_001: [ DECLARE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER shall declare the copy handler by expanding to: ]*/
/* Tests_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_002: [ DECLARE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER shall declare the free handler by expanding to: ]*/
DECLARE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER(MY_STRUCT);

/* Tests_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_003: [ DEFINE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER shall implement the copy handler by expanding to: ]*/
/* Tests_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_007: [ DEFINE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER shall implement the free handler by expanding to: ]*/
DEFINE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER(MY_STRUCT);

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
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

/* DEFINE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER */

/* Tests_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_004: [ If dst is NULL, the copy handler shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_type_helper_ref_counted_copy_handler_with_NULL_dst_fails)
{
    // arrange
    MY_STRUCT src;
    src.a = 42;
    src.b = 43;

    // act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(MY_STRUCT)(NULL, src);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_005: [ Otherwise the copy handler shall copy all the contents of src to dst. ]*/
/* Tests_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_006: [ The copy handler shall succeed and return 0. ]*/
TEST_FUNCTION(async_type_helper_ref_counted_copy_handler_succeeds)
{
    // arrange
    MY_STRUCT dst;
    MY_STRUCT src;
    src.a = 42;
    src.b = 43;

    // act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(MY_STRUCT)(&dst, src);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint32_t, dst.a, src.a);
    ASSERT_ARE_EQUAL(uint8_t, dst.b, src.b);
}

/* Tests_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_009: [ The free handler shall return. ]*/
TEST_FUNCTION(async_type_helper_ref_counted_free_handler_returns)
{
    // arrange
    MY_STRUCT arg;
    arg.a = 42;
    arg.b = 43;

    // act
    ASYNC_TYPE_HELPER_FREE_HANDLER(MY_STRUCT)(arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
