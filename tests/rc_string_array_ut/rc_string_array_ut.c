// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstdint>
#else
#include <stdlib.h>
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umock_c_negative_tests.h"

#include "real_gballoc_ll.h"
static void* my_gballoc_malloc(size_t size)
{
    return real_gballoc_ll_malloc(size);
}

static void* my_gballoc_malloc_2(size_t nmemb, size_t size)
{
    return real_gballoc_ll_malloc_2(nmemb, size);
}

static void my_gballoc_free(void* ptr)
{
     real_gballoc_ll_free(ptr);
}

#include "c_pal/interlocked.h" /*included for mocking reasons - it will prohibit creation of mocks belonging to interlocked.h - at the moment verified through int tests - this is porting legacy code, temporary solution*/

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_util/rc_string.h"
#undef ENABLE_MOCKS

// Must include umock_c_prod so mocks are not expanded in real_rc_string
#include "umock_c/umock_c_prod.h"

#include "real_gballoc_hl.h"
#include "real_rc_string.h"
#include "c_util/thandle.h"

#include "c_util/rc_string_array.h"

static TEST_MUTEX_HANDLE test_serialize_mutex;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}


BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));
    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types");

    REGISTER_RC_STRING_GLOBAL_MOCK_HOOKS();

    REGISTER_GLOBAL_MOCK_HOOK(malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(malloc_2, my_gballoc_malloc_2);
    REGISTER_GLOBAL_MOCK_HOOK(free, my_gballoc_free);

    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(RC_STRING), void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(test_serialize_mutex);

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }

    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

//
// rc_string_array_create
//

/*Tests_SRS_RC_STRING_ARRAY_42_001: [ rc_string_array_create shall allocate memory for RC_STRING_ARRAY. ]*/
/*Tests_SRS_RC_STRING_ARRAY_42_003: [ rc_string_array_create shall succeed and return the allocated RC_STRING_ARRAY. ]*/
TEST_FUNCTION(rc_string_array_create_with_0_elements_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    // act
    RC_STRING_ARRAY* result = rc_string_array_create(0);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    // cleanup
    rc_string_array_destroy(result);
}

/*Tests_SRS_RC_STRING_ARRAY_42_001: [ rc_string_array_create shall allocate memory for RC_STRING_ARRAY. ]*/
/*Tests_SRS_RC_STRING_ARRAY_42_008: [ rc_string_array_create shall allocate memory for count strings. ]*/
/*Tests_SRS_RC_STRING_ARRAY_42_007: [ rc_string_array_create shall initialize the strings in the array to NULL. ]*/
/*Tests_SRS_RC_STRING_ARRAY_42_003: [ rc_string_array_create shall succeed and return the allocated RC_STRING_ARRAY. ]*/
TEST_FUNCTION(rc_string_array_create_with_1_element_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc_2(1, sizeof(THANDLE(RC_STRING))));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, NULL));

    // act
    RC_STRING_ARRAY* result = rc_string_array_create(1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    // cleanup
    rc_string_array_destroy(result);
}

/*Tests_SRS_RC_STRING_ARRAY_42_001: [ rc_string_array_create shall allocate memory for RC_STRING_ARRAY. ]*/
/*Tests_SRS_RC_STRING_ARRAY_42_008: [ rc_string_array_create shall allocate memory for count strings. ]*/
/*Tests_SRS_RC_STRING_ARRAY_42_007: [ rc_string_array_create shall initialize the strings in the array to NULL. ]*/
/*Tests_SRS_RC_STRING_ARRAY_42_003: [ rc_string_array_create shall succeed and return the allocated RC_STRING_ARRAY. ]*/
TEST_FUNCTION(rc_string_array_create_with_10_elements_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc_2(10, sizeof(THANDLE(RC_STRING))));
    for (uint32_t i = 0; i < 10; i++)
    {
        STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, NULL));
    }

    // act
    RC_STRING_ARRAY* result = rc_string_array_create(10);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    // cleanup
    rc_string_array_destroy(result);
}

/*Tests_SRS_RC_STRING_ARRAY_42_002: [ If there are any errors then rc_string_array_create shall fail and return NULL. ]*/
TEST_FUNCTION(rc_string_array_create_with_10_elements_fails_when_underlying_functions_fail)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc_2(10, sizeof(THANDLE(RC_STRING))));
    for (uint32_t i = 0; i < 10; i++)
    {
        STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, NULL));
    }

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            RC_STRING_ARRAY* result = rc_string_array_create(10);

            // assert
            ASSERT_IS_NULL(result, "On failed call %zu", i);
        }
    }
}

//
// rc_string_array_destroy
//

/*Tests_SRS_RC_STRING_ARRAY_42_004: [ If rc_string_array is NULL then rc_string_array_destroy shall fail and return. ]*/
TEST_FUNCTION(rc_string_array_destroy_with_NULL_returns)
{
    // arrange

    // act
    rc_string_array_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_RC_STRING_ARRAY_42_006: [ rc_string_array_destroy shall free the memory allocated in rc_string_array. ]*/
TEST_FUNCTION(rc_string_array_destroy_with_0_elements_frees_everything)
{
    // arrange
    RC_STRING_ARRAY* rc_string_array = rc_string_array_create(0);
    ASSERT_IS_NOT_NULL(rc_string_array);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    rc_string_array_destroy(rc_string_array);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_RC_STRING_ARRAY_42_005: [ rc_string_array_destroy shall iterate over all of the elements in string_array and call THANDLE_DEC_REF(RC_STRING). ]*/
/*Tests_SRS_RC_STRING_ARRAY_42_006: [ rc_string_array_destroy shall free the memory allocated in rc_string_array. ]*/
TEST_FUNCTION(rc_string_array_destroy_with_10_elements_not_initialized_frees_everything)
{
    // arrange
    RC_STRING_ARRAY* rc_string_array = rc_string_array_create(10);
    ASSERT_IS_NOT_NULL(rc_string_array);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    rc_string_array_destroy(rc_string_array);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_RC_STRING_ARRAY_42_005: [ rc_string_array_destroy shall iterate over all of the elements in string_array and call THANDLE_DEC_REF(RC_STRING). ]*/
/*Tests_SRS_RC_STRING_ARRAY_42_006: [ rc_string_array_destroy shall free the memory allocated in rc_string_array. ]*/
TEST_FUNCTION(rc_string_array_destroy_with_10_elements_initialized_frees_everything)
{
    // arrange
    RC_STRING_ARRAY* rc_string_array = rc_string_array_create(10);
    ASSERT_IS_NOT_NULL(rc_string_array);

    for (uint32_t i = 0; i < rc_string_array->count; i++)
    {
        THANDLE(real_RC_STRING) temp = real_rc_string_create("foo");
        ASSERT_IS_NOT_NULL(temp);
        THANDLE_MOVE(real_RC_STRING)(&rc_string_array->string_array[i], &temp);
    }

    umock_c_reset_all_calls();

    for (uint32_t i = 0; i < 10; i++)
    {
        STRICT_EXPECTED_CALL(THANDLE_DEC_REF(RC_STRING)(rc_string_array->string_array[i]));
    }
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    rc_string_array_destroy(rc_string_array);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
