// Copyright (c) Microsoft. All rights reserved.


#include <stdlib.h>
#include <stddef.h>


#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_stdint.h"

#include "c_pal/interlocked.h" /*included for mocking reasons - it will prohibit creation of mocks belonging to this interlocked.h, temporary solution*/

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "test_ref_counted.h"
#include "test_ref_counted.c"
#undef ENABLE_MOCKS
#include "real_gballoc_hl.h"


#include "c_util/async_type_helper.h"
#include "test_ref_counted_async_type_helper_handler.h"

#include "c_util/async_type_helper_ref_counted_handler.h"

static TEST_REFCOUNTED_HANDLE test_ref_counted;

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

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_flex, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(test_refcounted_dec_ref, UMOCK_REAL(test_refcounted_dec_ref));
    REGISTER_GLOBAL_MOCK_HOOK(test_refcounted_inc_ref, UMOCK_REAL(test_refcounted_inc_ref));

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_UMOCK_ALIAS_TYPE(TEST_REFCOUNTED_HANDLE, void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    test_ref_counted = test_refcounted_create();
    ASSERT_IS_NOT_NULL(test_ref_counted);

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    test_refcounted_dec_ref(test_ref_counted);
}

/* DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER */

/* Tests_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_004: [ If dst is NULL, the copy handler shall fail and return a non-zero value. ]*/
TEST_FUNCTION(async_type_helper_ref_counted_copy_handler_with_NULL_dst_fails)
{
    // arrange

    // act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(TEST_REFCOUNTED_HANDLE)(NULL, test_ref_counted);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_006: [ Otherwise the copy handler shall increment the reference count for src by calling inc_ref_function. ]*/
/* Tests_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_007: [ The copy handler shall store src in dst and return 0. ]*/
TEST_FUNCTION(async_type_helper_ref_counted_copy_handler_succeeds)
{
    // arrange
    TEST_REFCOUNTED_HANDLE dst;

    STRICT_EXPECTED_CALL(test_refcounted_inc_ref(test_ref_counted));

    // act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(TEST_REFCOUNTED_HANDLE)(&dst, test_ref_counted);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(void_ptr, dst, test_ref_counted);

    // cleanup
    UMOCK_REAL(test_refcounted_dec_ref)(dst);
}

/* Tests_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_011: [ The copy handler shall increment the reference count for src only if src is not NULL. ]*/
TEST_FUNCTION(async_type_helper_ref_counted_copy_handler_with_NULL_does_not_increment_reference_count)
{
    // arrange
    TEST_REFCOUNTED_HANDLE dst;

    // act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(TEST_REFCOUNTED_HANDLE)(&dst, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NULL(dst);

    // cleanup
    UMOCK_REAL(test_refcounted_dec_ref)(dst);
}

/* Tests_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_009: [ If arg is NULL, the free handler shall return. ]*/
TEST_FUNCTION(async_type_helper_ref_counted_free_handler_returns)
{
    // arrange
    // act
    ASYNC_TYPE_HELPER_FREE_HANDLER(TEST_REFCOUNTED_HANDLE)(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_010: [ Otherwise the free handler shall decrement the reference count for arg by calling dec_ref_function. ]*/
TEST_FUNCTION(async_type_helper_ref_counted_free_handler_decrements_the_ref_count)
{
    // arrange
    TEST_REFCOUNTED_HANDLE dst;
    ASSERT_ARE_EQUAL(int, 0, ASYNC_TYPE_HELPER_COPY_HANDLER(TEST_REFCOUNTED_HANDLE)(&dst, test_ref_counted));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_refcounted_dec_ref(test_ref_counted));

    // act
    ASYNC_TYPE_HELPER_FREE_HANDLER(TEST_REFCOUNTED_HANDLE)(dst);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
