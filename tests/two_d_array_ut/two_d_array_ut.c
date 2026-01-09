// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "two_d_array_ut_pch.h"

/*TWO_D_ARRAY with regular types*/
TWO_D_ARRAY_DEFINE_STRUCT_TYPE(uint32_t)

THANDLE_TYPE_DECLARE(TWO_D_ARRAY_TYPEDEF_NAME(uint32_t));
THANDLE_TYPE_DEFINE(TWO_D_ARRAY_TYPEDEF_NAME(uint32_t));

TWO_D_ARRAY_TYPE_DECLARE(uint32_t);
TWO_D_ARRAY_TYPE_DEFINE(uint32_t);

typedef struct B_TEST_TAG
{
    int b;
}B_TEST;

TWO_D_ARRAY_DEFINE_STRUCT_TYPE(B_TEST)
THANDLE_TYPE_DECLARE(TWO_D_ARRAY_TYPEDEF_NAME(B_TEST));
THANDLE_TYPE_DEFINE(TWO_D_ARRAY_TYPEDEF_NAME(B_TEST));

TWO_D_ARRAY_TYPE_DECLARE(B_TEST);
TWO_D_ARRAY_TYPE_DEFINE(B_TEST);

typedef struct A_TEST_TAG
{
    int a;
}A_TEST;

THANDLE_TYPE_DECLARE(A_TEST);
THANDLE_TYPE_DEFINE(A_TEST);

TWO_D_ARRAY_DEFINE_STRUCT_TYPE(THANDLE(A_TEST))
THANDLE_TYPE_DECLARE(TWO_D_ARRAY_TYPEDEF_NAME(THANDLE(A_TEST)));
THANDLE_TYPE_DEFINE(TWO_D_ARRAY_TYPEDEF_NAME(THANDLE(A_TEST)));

/*TWO_D_ARRAY works with THANDLEs too*/
TWO_D_ARRAY_TYPE_DECLARE(THANDLE(A_TEST));
TWO_D_ARRAY_TYPE_DEFINE(THANDLE(A_TEST));

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}


BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(it_does_something)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    umock_c_init(on_umock_c_error);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(f)
{
    umock_c_negative_tests_init();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleans)
{
    umock_c_negative_tests_deinit();
}

/*TWO_D_ARRAY_CREATE(T)*/

/* Tests_SRS_TWO_D_ARRAY_07_003: [ TWO_D_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX with TWO_D_ARRAY_FREE(T) as dispose function, nmemb set to row_size and size set to sizeof(T*). ]*/
/* Tests_SRS_TWO_D_ARRAY_07_004: [ TWO_D_ARRAY_CREATE(T) shall set all rows pointers to NULL. ]*/
/* Tests_SRS_TWO_D_ARRAY_07_006: [ TWO_D_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(TWO_D_ARRAY_CREATE_create_with_uint32_t_type_and_row_size_one_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, sizeof(uint32_t*)));

    //act
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(1, 5);

    //assert
    ASSERT_IS_NOT_NULL(tdarr);
    ASSERT_IS_NULL(tdarr->row_arrays[0]);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}

/* Tests_SRS_TWO_D_ARRAY_07_003: [ TWO_D_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX with TWO_D_ARRAY_FREE(T) as dispose function, nmemb set to row_size and size set to sizeof(T*). ]*/
/* Tests_SRS_TWO_D_ARRAY_07_004: [ TWO_D_ARRAY_CREATE(T) shall set all rows pointers to NULL. ]*/
/* Tests_SRS_TWO_D_ARRAY_07_006: [ TWO_D_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(TWO_D_ARRAY_CREATE_create_with_uint32_t_type_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 5, sizeof(uint32_t*)));

    //act
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 5);

    //assert
    ASSERT_IS_NOT_NULL(tdarr);
    for (uint32_t i = 0; i < 5; i++)
    {
        ASSERT_IS_NULL(tdarr->row_arrays[i]);
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}

#define TEST_UINT32_MAX 0
/* running too long in the gate, commented out*/
#if TEST_UINT32_MAX
/* Tests_SRS_TWO_D_ARRAY_07_003: [ TWO_D_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX with TWO_D_ARRAY_FREE(T) as dispose function, nmemb set to row_size and size set to sizeof(T*). ]*/
/* Tests_SRS_TWO_D_ARRAY_07_004: [ TWO_D_ARRAY_CREATE(T) shall set all rows pointers to NULL. ]*/
/* Tests_SRS_TWO_D_ARRAY_07_006: [ TWO_D_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(TWO_D_ARRAY_CREATE_create_with_uint32_t_type_and_UINT32_MAX_minus_one_row_size_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, UINT32_MAX-1, sizeof(uint32_t*)));

    //act
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(UINT32_MAX-1, 5);

    //assert
    ASSERT_IS_NOT_NULL(tdarr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}


/* Tests_SRS_TWO_D_ARRAY_07_003: [ TWO_D_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX with TWO_D_ARRAY_FREE(T) as dispose function, nmemb set to row_size and size set to sizeof(T*). ]*/
/* Tests_SRS_TWO_D_ARRAY_07_004: [ TWO_D_ARRAY_CREATE(T) shall set all rows pointers to NULL. ]*/
/* Tests_SRS_TWO_D_ARRAY_07_006: [ TWO_D_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(TWO_D_ARRAY_CREATE_create_with_uint32_t_type_and_UINT32_MAX_row_size_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, UINT32_MAX, sizeof(uint32_t*)));

    //act
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(UINT32_MAX, 5);

    //assert
    ASSERT_IS_NOT_NULL(tdarr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}
#endif

/* Tests_SRS_TWO_D_ARRAY_07_003: [ TWO_D_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX with TWO_D_ARRAY_FREE(T) as dispose function, nmemb set to row_size and size set to sizeof(T*). ]*/
/* Tests_SRS_TWO_D_ARRAY_07_004: [ TWO_D_ARRAY_CREATE(T) shall set all rows pointers to NULL. ]*/
/* Tests_SRS_TWO_D_ARRAY_07_006: [ TWO_D_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(TWO_D_ARRAY_CREATE_create_with_THANDLE_type_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 5, sizeof(THANDLE(A_TEST)*)));

    //act
    TWO_D_ARRAY(THANDLE(A_TEST)) tdarr = TWO_D_ARRAY_CREATE(THANDLE(A_TEST))(5, 5);

    //assert
    ASSERT_IS_NOT_NULL(tdarr);
    for (uint32_t i = 0; i < 5; i++)
    {
        ASSERT_IS_NULL(tdarr->row_arrays[i]);
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(THANDLE(A_TEST))(&tdarr, NULL);
}

/* Tests_SRS_TWO_D_ARRAY_07_003: [ TWO_D_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX with TWO_D_ARRAY_FREE(T) as dispose function, nmemb set to row_size and size set to sizeof(T*). ]*/
/* Tests_SRS_TWO_D_ARRAY_07_004: [ TWO_D_ARRAY_CREATE(T) shall set all rows pointers to NULL. ]*/
/* Tests_SRS_TWO_D_ARRAY_07_006: [ TWO_D_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(TWO_D_ARRAY_CREATE_create_with_struct_type_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 5, sizeof(B_TEST*)));

    //act
    TWO_D_ARRAY(B_TEST) tdarr = TWO_D_ARRAY_CREATE(B_TEST)(5, 5);

    //assert
    ASSERT_IS_NOT_NULL(tdarr);
    for (uint32_t i = 0; i < 5; i++)
    {
        ASSERT_IS_NULL(tdarr->row_arrays[i]);
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(B_TEST)(&tdarr, NULL);
}

/* running too long in the gate, commented out*/
#if TEST_UINT32_MAX
/* Tests_SRS_TWO_D_ARRAY_07_003: [ TWO_D_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX with TWO_D_ARRAY_FREE(T) as dispose function, nmemb set to row_size and size set to sizeof(T*). ]*/
/* Tests_SRS_TWO_D_ARRAY_07_004: [ TWO_D_ARRAY_CREATE(T) shall set all rows pointers to NULL. ]*/
/* Tests_SRS_TWO_D_ARRAY_07_006: [ TWO_D_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(TWO_D_ARRAY_CREATE_create_with_THANDLE_type_and_UINT32_MAX_row_size_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, UINT32_MAX, sizeof(THANDLE(A_TEST)*)));

    //act
    TWO_D_ARRAY(THANDLE(A_TEST)) tdarr = TWO_D_ARRAY_CREATE(THANDLE(A_TEST))(UINT32_MAX, 5);

    //assert
    ASSERT_IS_NOT_NULL(tdarr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(THANDLE(A_TEST))(&tdarr, NULL);
}
#endif

/* Tests_SRS_TWO_D_ARRAY_07_005: [ If there are any errors then TWO_D_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fails_TWO_D_ARRAY_CREATE_also_fails)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 5, sizeof(uint32_t*)));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 5);

            //assert
            ASSERT_IS_NULL(tdarr);
        }
    }

    //clean
}

/* Tests_SRS_TWO_D_ARRAY_07_001: [ If row_size equals to zero, TWO_D_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(TWO_D_ARRAY_CREATE_fails_when_row_size_is_zero)
{
    //arrange

    //act
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(0, 5);

    //assert
    ASSERT_IS_NULL(tdarr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_TWO_D_ARRAY_07_002: [ If col_size equals to zero, TWO_D_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(TWO_D_ARRAY_CREATE_fails_when_col_size_is_zero)
{
    //arrange

    //act
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 0);

    //assert
    ASSERT_IS_NULL(tdarr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* TWO_D_ARRAY_ALLOCATE_NEW_ROW(T) */

/* Tests_SRS_TWO_D_ARRAY_07_017: [ Otherwise, TWO_D_ARRAY_ALLOCATE_NEW_ROW(T) shall allocate memory for the new row and return zero on success. ]*/
TEST_FUNCTION(TWO_D_ARRAY_ALLOCATE_NEW_ROW_with_uint32_t_type_succeeds)
{
    //arrange
    int result;
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 5000000);
    ASSERT_IS_NOT_NULL(tdarr);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc_2(5000000, sizeof(uint32_t)));

    //act
    result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(tdarr, 0);

    //assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[0]);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}

/* running too long in the gate, commented out */
#if TEST_UINT32_MAX
/* Tests_SRS_TWO_D_ARRAY_07_017: [ Otherwise, TWO_D_ARRAY_ALLOCATE_NEW_ROW(T) shall allocate memory for the new row and return zero on success. ]*/
TEST_FUNCTION(TWO_D_ARRAY_ALLOCATE_NEW_ROW_with_uint32_t_type_all_rows_can_be_allocated_with_col_size_UINT32_MAX)
{
    //arrange
    int result;
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, UINT32_MAX);
    ASSERT_IS_NOT_NULL(tdarr);
    umock_c_reset_all_calls();
    for (uint32_t i = 0; i < 5; i++)
    {
        STRICT_EXPECTED_CALL(malloc_2(UINT32_MAX, sizeof(uint32_t)));
    }

    //act
    for (uint32_t i = 0; i < 5; i++)
    {
        result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(tdarr, i);
        ASSERT_ARE_EQUAL(int, result, 0);
        ASSERT_IS_NOT_NULL(tdarr->row_arrays[i]);
    }

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}
#endif

/* Tests_SRS_TWO_D_ARRAY_07_017: [ Otherwise, TWO_D_ARRAY_ALLOCATE_NEW_ROW(T) shall allocate memory for the new row and return zero on success. ]*/
TEST_FUNCTION(TWO_D_ARRAY_ALLOCATE_NEW_ROW_with_uint32_t_type_all_rows_can_be_allocated_1)
{
    //arrange
    int result;
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(1000, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    umock_c_reset_all_calls();
    for (uint32_t i = 0; i < 1000; i++)
    {
        STRICT_EXPECTED_CALL(malloc_2(5, sizeof(uint32_t)));
    }

    //act
    for (uint32_t i = 0; i < 1000; i++)
    {
        result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(tdarr, i);
        ASSERT_ARE_EQUAL(int, result, 0);
        ASSERT_IS_NOT_NULL(tdarr->row_arrays[i]);
    }

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}

/* Tests_SRS_TWO_D_ARRAY_07_017: [ Otherwise, TWO_D_ARRAY_ALLOCATE_NEW_ROW(T) shall allocate memory for the new row and return zero on success. ]*/
TEST_FUNCTION(TWO_D_ARRAY_ALLOCATE_NEW_ROW_with_THANDLE_type_succeeds)
{
    //arrange
    int result;
    TWO_D_ARRAY(THANDLE(A_TEST)) tdarr = TWO_D_ARRAY_CREATE(THANDLE(A_TEST))(5, 5000000);
    ASSERT_IS_NOT_NULL(tdarr);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc_2(5000000, sizeof(THANDLE(A_TEST))));

    //act
    result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(THANDLE(A_TEST))(tdarr, 0);

    //assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[0]);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    THANDLE(A_TEST) a = THANDLE_MALLOC(A_TEST)(NULL);
    ASSERT_IS_NOT_NULL(a);

    THANDLE_INITIALIZE(A_TEST)(&tdarr->row_arrays[0][0], a);
    THANDLE_ASSIGN(A_TEST)(&tdarr->row_arrays[0][0], NULL);
    THANDLE_ASSIGN(A_TEST)(&a, NULL);

    //clean
    TWO_D_ARRAY_ASSIGN(THANDLE(A_TEST))(&tdarr, NULL);
}

/* Tests_SRS_TWO_D_ARRAY_07_017: [ Otherwise, TWO_D_ARRAY_ALLOCATE_NEW_ROW(T) shall allocate memory for the new row and return zero on success. ]*/
TEST_FUNCTION(TWO_D_ARRAY_ALLOCATE_NEW_ROW_with_struct_type_succeeds)
{
    //arrange
    int result;
    TWO_D_ARRAY(B_TEST) tdarr = TWO_D_ARRAY_CREATE(B_TEST)(5, 5000000);
    ASSERT_IS_NOT_NULL(tdarr);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc_2(5000000, sizeof(B_TEST)));

    //act
    result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(B_TEST)(tdarr, 0);

    //assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[0]);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    /*able to assign*/
    (tdarr->row_arrays[0][0]).b = 1;

    //clean
    TWO_D_ARRAY_ASSIGN(B_TEST)(&tdarr, NULL);
}

/* Tests_SRS_TWO_D_ARRAY_07_014: [ If two_d_array is NULL, TWO_D_ARRAY_ALLOCATE_NEW_ROW(T) shall fail and return a non-zero value. ]*/
TEST_FUNCTION(TWO_D_ARRAY_ALLOCATE_NEW_ROW_fails_when_handle_is_null)
{
    //arrange
    int result;

    //act
    result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(NULL, 5);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_TWO_D_ARRAY_07_015: [ If row_index is equal or greater than row_size, TWO_D_ARRAY_ALLOCATE_NEW_ROW(T) shall fail and return a non-zero value. ]*/
TEST_FUNCTION(TWO_D_ARRAY_ALLOCATE_NEW_ROW_fails_when_row_index_too_high)
{
    //arrange
    int result;
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    umock_c_reset_all_calls();

    //act
    result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(tdarr, 5);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}

/* Tests_SRS_TWO_D_ARRAY_07_016: [ If the row specified by row_index has already been allocated, TWO_D_ARRAY_ALLOCATE_NEW_ROW(T) shall fail and return a non-zero value. ]*/
TEST_FUNCTION(TWO_D_ARRAY_ALLOCATE_NEW_ROW_fails_when_that_row_has_already_been_allocated)
{
    //arrange
    int result;
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    int add_result_1 = TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(tdarr, 0);
    ASSERT_ARE_EQUAL(int, add_result_1, 0);
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[0]);
    umock_c_reset_all_calls();

    //act
    result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(tdarr, 0);


    //assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}

/* Tests_SRS_TWO_D_ARRAY_07_018: [ If there are any errors then TWO_D_ARRAY_ALLOCATE_NEW_ROW(T) shall fail and return a non-zero value. ]*/
TEST_FUNCTION(TWO_D_ARRAY_ALLOCATE_NEW_ROW_fails_when_malloc_fails)
{
    //arrange
    int result;
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc_2(5, sizeof(uint32_t)))
        .SetReturn(NULL);

    //act
    result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(tdarr, 0);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_IS_NULL(tdarr->row_arrays[0]);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}

/* TWO_D_ARRAY_FREE(T) */

/* Tests_SRS_TWO_D_ARRAY_07_007: [ If two_d_array is NULL, TWO_D_ARRAY_FREE(T) shall do nothing. ]*/
TEST_FUNCTION(TWO_D_ARRAY_FREE_do_nothing)
{
    //arrange
    TWO_D_ARRAY(uint32_t) tdarr = NULL;

    //act
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);

    //assert
    ASSERT_IS_NULL(tdarr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_TWO_D_ARRAY_07_009: [ TWO_D_ARRAY_FREE(T) shall free the memory associated with TWO_D_ARRAY(T). ]*/
TEST_FUNCTION(TWO_D_ARRAY_FREE_succeeds_without_row_allocated)
{
    //arrange
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(10000000, 5000);
    ASSERT_IS_NOT_NULL(tdarr);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);

    //assert
    ASSERT_IS_NULL(tdarr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_TWO_D_ARRAY_07_008: [ TWO_D_ARRAY_FREE(T) shall free all rows that are non-NULL. ]*/
TEST_FUNCTION(TWO_D_ARRAY_FREE_succeeds_with_rows_allocated)
{
    //arrange
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(10000000, 5000);
    ASSERT_IS_NOT_NULL(tdarr);
    int add_row_result_0 = TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(tdarr, 0);
    ASSERT_ARE_EQUAL(int, add_row_result_0, 0);
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[0]);
    int add_row_result_70 = TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(tdarr, 70);
    ASSERT_ARE_EQUAL(int, add_row_result_70, 0);
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[70]);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);

    //assert
    ASSERT_IS_NULL(tdarr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* TWO_D_ARRAY_FREE_ROW(T) */

/* Tests_SRS_TWO_D_ARRAY_07_010: [ If two_d_array is NULL, TWO_D_ARRAY_FREE_ROW(T) shall fail return a non-zero value. ]*/
TEST_FUNCTION(TWO_D_ARRAY_FREE_ROW_fails_when_handle_is_null)
{
    //arrange
    int result;

    //act
    result = TWO_D_ARRAY_FREE_ROW(uint32_t)(NULL, 0);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_TWO_D_ARRAY_07_011: [ If row_index is equal or greater than row_size, TWO_D_ARRAY_FREE_ROW(T) shall fail and return a non-zero value. ]*/
TEST_FUNCTION(TWO_D_ARRAY_FREE_ROW_fails_when_row_index_too_high)
{
    //arrange
    int result;
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    int add_row_result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(tdarr, 0);
    ASSERT_ARE_EQUAL(int, add_row_result, 0);
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[0]);
    umock_c_reset_all_calls();

    //act
    result = TWO_D_ARRAY_FREE_ROW(uint32_t)(tdarr, 5);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}

/* Tests_SRS_TWO_D_ARRAY_07_012: [ TWO_D_ARRAY_FREE_ROW(T) shall free the memory associated with the row specified by row_index and set it to NULL. ]*/
/* Tests_SRS_TWO_D_ARRAY_07_013: [ TWO_D_ARRAY_FREE_ROW(T) shall return zero on success. ]*/
TEST_FUNCTION(TWO_D_ARRAY_FREE_ROW_with_uint32_type_succeeds)
{
    //arrange
    int result;
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    int add_row_result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(tdarr, 0);
    ASSERT_ARE_EQUAL(int, add_row_result, 0);
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[0]);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    result = TWO_D_ARRAY_FREE_ROW(uint32_t)(tdarr, 0);

    //assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_IS_NULL(tdarr->row_arrays[0]);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}

/* Tests_SRS_TWO_D_ARRAY_07_012: [ TWO_D_ARRAY_FREE_ROW(T) shall free the memory associated with the row specified by row_index and set it to NULL. ]*/
/* Tests_SRS_TWO_D_ARRAY_07_013: [ TWO_D_ARRAY_FREE_ROW(T) shall return zero on success. ]*/
TEST_FUNCTION(TWO_D_ARRAY_FREE_ROW_with_THANDLE_type_succeeds)
{
    //arrange
    int result;
    TWO_D_ARRAY(THANDLE(A_TEST)) tdarr = TWO_D_ARRAY_CREATE(THANDLE(A_TEST))(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    int add_row_result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(THANDLE(A_TEST))(tdarr, 0);
    ASSERT_ARE_EQUAL(int, add_row_result, 0);
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[0]);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    result = TWO_D_ARRAY_FREE_ROW(THANDLE(A_TEST))(tdarr, 0);

    //assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_IS_NULL(tdarr->row_arrays[0]);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(THANDLE(A_TEST))(&tdarr, NULL);
}

/* Tests_SRS_TWO_D_ARRAY_07_012: [ TWO_D_ARRAY_FREE_ROW(T) shall free the memory associated with the row specified by row_index and set it to NULL. ]*/
/* Tests_SRS_TWO_D_ARRAY_07_013: [ TWO_D_ARRAY_FREE_ROW(T) shall return zero on success. ]*/
TEST_FUNCTION(TWO_D_ARRAY_FREE_ROW_with_struct_type_succeeds)
{
    //arrange
    int result;
    TWO_D_ARRAY(B_TEST) tdarr = TWO_D_ARRAY_CREATE(B_TEST)(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    int add_row_result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(B_TEST)(tdarr, 0);
    ASSERT_ARE_EQUAL(int, add_row_result, 0);
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[0]);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    result = TWO_D_ARRAY_FREE_ROW(B_TEST)(tdarr, 0);

    //assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_IS_NULL(tdarr->row_arrays[0]);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(B_TEST)(&tdarr, NULL);
}

/* TWO_D_ARRAY_GET_ROW(T) */

/* Tests_SRS_TWO_D_ARRAY_07_019: [ If two_d_array is NULL, TWO_D_ARRAY_GET_ROW(T) shall fail return NULL. ]*/
TEST_FUNCTION(TWO_D_ARRAY_GET_ROW_fails_when_handle_is_null)
{
    //arrange
    uint32_t* row;

    //act
    row = TWO_D_ARRAY_GET_ROW(uint32_t)(NULL, 0);

    //assert
    ASSERT_IS_NULL(row);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_TWO_D_ARRAY_07_020: [ If row_index is equal or greater than row_size, TWO_D_ARRAY_GET_ROW(T) shall fail return NULL. ]*/
TEST_FUNCTION(TWO_D_ARRAY_GET_ROW_fails_when_row_index_too_high)
{
    //arrange
    uint32_t* row_result;
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    int add_row_result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(tdarr, 0);
    ASSERT_ARE_EQUAL(int, add_row_result, 0);
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[0]);
    umock_c_reset_all_calls();

    //act
    row_result = TWO_D_ARRAY_GET_ROW(uint32_t)(tdarr, 5);

    //assert
    ASSERT_IS_NULL(row_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}

/* Tests_SRS_TWO_D_ARRAY_07_021: [ If the array stored in row_index is NULL, TWO_D_ARRAY_GET_ROW(T) shall fail and return NULL. ]*/
TEST_FUNCTION(TWO_D_ARRAY_GET_ROW_fails_when_row_not_initialized_yet)
{
    //arrange
    uint32_t* row_result;
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    int add_row_result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(tdarr, 0);
    ASSERT_ARE_EQUAL(int, add_row_result, 0);
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[0]);
    umock_c_reset_all_calls();

    //act
    row_result = TWO_D_ARRAY_GET_ROW(uint32_t)(tdarr, 1);

    //assert
    ASSERT_IS_NULL(row_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);

}

/* Tests_SRS_TWO_D_ARRAY_07_022: [ Otherwise, TWO_D_ARRAY_GET_ROW(T) shall return the entire column stored in the corresponding row_index. ]*/
TEST_FUNCTION(TWO_D_ARRAY_GET_ROW_get_all_rows_succeeds_with_uint32_t_type)
{
    //arrange
    uint32_t* row_result;
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    int add_row_result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(tdarr, 0);
    ASSERT_ARE_EQUAL(int, add_row_result, 0);
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[0]);
    for (uint32_t i = 0; i < 5; i++)
    {
        tdarr->row_arrays[0][i] = i;
    }
    umock_c_reset_all_calls();

    //act
    row_result = TWO_D_ARRAY_GET_ROW(uint32_t)(tdarr, 0);

    //assert
    ASSERT_IS_NOT_NULL(row_result);
    for (uint32_t i = 0; i < 5; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, row_result[i], i);
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}

/* TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED(T) */

/* Tests_SRS_TWO_D_ARRAY_88_001: [ If two_d_array is NULL, TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED(T) shall fail and return NULL. ]*/
TEST_FUNCTION(TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED_fails_when_handle_is_null)
{
    //arrange
    uint32_t* row;

    //act
    row = TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED(uint32_t)(NULL, 0);

    //assert
    ASSERT_IS_NULL(row);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_TWO_D_ARRAY_88_002: [ If row_index is equal or greater than row_size, TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED(T) shall fail and return NULL. ]*/
TEST_FUNCTION(TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED_fails_when_row_index_too_high)
{
    //arrange
    uint32_t* row_result;
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    umock_c_reset_all_calls();

    //act
    row_result = TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED(uint32_t)(tdarr, 5);

    //assert
    ASSERT_IS_NULL(row_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}

/* Tests_SRS_TWO_D_ARRAY_88_004: [ If the array stored in row_index is NULL, TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED(T) shall allocate a new row, store it in row_index, and return it. ]*/
TEST_FUNCTION(TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED_allocates_row_when_row_is_not_allocated)
{
    //arrange
    uint32_t* allocated_row;
    uint32_t* row_result;
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    ASSERT_IS_NULL(tdarr->row_arrays[1]);

    allocated_row = real_gballoc_hl_malloc_2(5, sizeof(uint32_t));
    ASSERT_IS_NOT_NULL(allocated_row);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc_2(5, sizeof(uint32_t)))
        .SetReturn(allocated_row);

    //act
    row_result = TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED(uint32_t)(tdarr, 1);

    //assert
    ASSERT_IS_NOT_NULL(row_result);
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[1]);
    ASSERT_IS_TRUE(row_result == tdarr->row_arrays[1]);
    ASSERT_IS_TRUE(row_result == allocated_row);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}

/* Tests_SRS_TWO_D_ARRAY_88_003: [ If the array stored in row_index is non-NULL, TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED(T) shall return the existing row. ]*/
TEST_FUNCTION(TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED_returns_existing_row_when_row_is_allocated)
{
    //arrange
    uint32_t* row_result;
    uint32_t* first_row;
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    ASSERT_ARE_EQUAL(int, 0, TWO_D_ARRAY_ALLOCATE_NEW_ROW(uint32_t)(tdarr, 0));
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[0]);
    first_row = tdarr->row_arrays[0];
    umock_c_reset_all_calls();

    //act
    row_result = TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED(uint32_t)(tdarr, 0);

    //assert
    ASSERT_IS_NOT_NULL(row_result);
    ASSERT_IS_TRUE(row_result == first_row);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}

/* Tests_SRS_TWO_D_ARRAY_88_004: [ If the array stored in row_index is NULL, TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED(T) shall allocate a new row, store it in row_index, and return it. ]*/
TEST_FUNCTION(TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED_fails_when_malloc_fails)
{
    //arrange
    uint32_t* row_result;
    TWO_D_ARRAY(uint32_t) tdarr = TWO_D_ARRAY_CREATE(uint32_t)(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    ASSERT_IS_NULL(tdarr->row_arrays[3]);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc_2(5, sizeof(uint32_t)))
        .SetReturn(NULL);

    //act
    row_result = TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED(uint32_t)(tdarr, 3);

    //assert
    ASSERT_IS_NULL(row_result);
    ASSERT_IS_NULL(tdarr->row_arrays[3]);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    TWO_D_ARRAY_ASSIGN(uint32_t)(&tdarr, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
