// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdlib.h>
#include <stdint.h>


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

static void* my_gballoc_malloc_flex(size_t base, size_t nmemb, size_t size)
{
    return real_gballoc_ll_malloc_flex(base, nmemb, size);
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

#include "c_util/thandle_tuple_array.h"

static TEST_MUTEX_HANDLE test_serialize_mutex;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

#define TUPLE_ONE_FIELDS \
    RC_STRING, a

DECLARE_THANDLE_TUPLE_ARRAY(TUPLE_ONE, TUPLE_ONE_FIELDS);
DEFINE_THANDLE_TUPLE_ARRAY(TUPLE_ONE, TUPLE_ONE_FIELDS);

#define TUPLE_THREE_FIELDS \
    RC_STRING, foo, \
    RC_STRING, bar, \
    RC_STRING, baz

DECLARE_THANDLE_TUPLE_ARRAY(TUPLE_THREE, TUPLE_THREE_FIELDS);
DEFINE_THANDLE_TUPLE_ARRAY(TUPLE_THREE, TUPLE_THREE_FIELDS);


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
    REGISTER_GLOBAL_MOCK_HOOK(malloc_flex, my_gballoc_malloc_flex);
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
// THANDLE_TUPLE_ARRAY_CREATE(name)
//

/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_001: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall allocate memory for the array. ]*/
/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_005: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall succeed and return the allocated array. ]*/
TEST_FUNCTION(THANDLE_TUPLE_ARRAY_CREATE_with_1_field_0_elements_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, sizeof(TUPLE_ONE)));

    // act
    THANDLE_TUPLE_ARRAY_TYPE(TUPLE_ONE)* result = THANDLE_TUPLE_ARRAY_CREATE(TUPLE_ONE)(0);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(uint32_t, 0, result->count);

    // cleanup
    THANDLE_TUPLE_ARRAY_DESTROY(TUPLE_ONE)(result);
}

/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_001: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall allocate memory for the array. ]*/
/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_005: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall succeed and return the allocated array. ]*/
TEST_FUNCTION(THANDLE_TUPLE_ARRAY_CREATE_with_3_fields_0_elements_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, sizeof(TUPLE_THREE)));

    // act
    THANDLE_TUPLE_ARRAY_TYPE(TUPLE_THREE)* result = THANDLE_TUPLE_ARRAY_CREATE(TUPLE_THREE)(0);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(uint32_t, 0, result->count);

    // cleanup
    THANDLE_TUPLE_ARRAY_DESTROY(TUPLE_THREE)(result);
}

/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_001: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall allocate memory for the array. ]*/
/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_003: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall initialize the members of the tuples in the array to NULL. ]*/
/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_005: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall succeed and return the allocated array. ]*/
TEST_FUNCTION(THANDLE_TUPLE_ARRAY_CREATE_with_1_field_1_element_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, sizeof(TUPLE_ONE)));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, NULL));

    // act
    THANDLE_TUPLE_ARRAY_TYPE(TUPLE_ONE)* result = THANDLE_TUPLE_ARRAY_CREATE(TUPLE_ONE)(1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(uint32_t, 1, result->count);
    ASSERT_IS_NULL(result->tuple_array[0].a);

    // cleanup
    THANDLE_TUPLE_ARRAY_DESTROY(TUPLE_ONE)(result);
}

/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_001: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall allocate memory for the array. ]*/
/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_003: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall initialize the members of the tuples in the array to NULL. ]*/
/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_005: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall succeed and return the allocated array. ]*/
TEST_FUNCTION(THANDLE_TUPLE_ARRAY_CREATE_with_3_fields_1_element_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, sizeof(TUPLE_THREE)));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, NULL));

    // act
    THANDLE_TUPLE_ARRAY_TYPE(TUPLE_THREE)* result = THANDLE_TUPLE_ARRAY_CREATE(TUPLE_THREE)(1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(uint32_t, 1, result->count);
    ASSERT_IS_NULL(result->tuple_array[0].foo);
    ASSERT_IS_NULL(result->tuple_array[0].bar);
    ASSERT_IS_NULL(result->tuple_array[0].baz);

    // cleanup
    THANDLE_TUPLE_ARRAY_DESTROY(TUPLE_THREE)(result);
}

/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_001: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall allocate memory for the array. ]*/
/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_003: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall initialize the members of the tuples in the array to NULL. ]*/
/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_005: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall succeed and return the allocated array. ]*/
TEST_FUNCTION(THANDLE_TUPLE_ARRAY_CREATE_with_1_field_10_elements_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 10, sizeof(TUPLE_ONE)));
    for (uint32_t i = 0; i < 10; i++)
    {
        STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, NULL));
    }

    // act
    THANDLE_TUPLE_ARRAY_TYPE(TUPLE_ONE)* result = THANDLE_TUPLE_ARRAY_CREATE(TUPLE_ONE)(10);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(uint32_t, 10, result->count);
    for (uint32_t i = 0; i < 10; i++)
    {
        ASSERT_IS_NULL(result->tuple_array[i].a);
    }

    // cleanup
    THANDLE_TUPLE_ARRAY_DESTROY(TUPLE_ONE)(result);
}

/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_001: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall allocate memory for the array. ]*/
/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_003: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall initialize the members of the tuples in the array to NULL. ]*/
/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_005: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall succeed and return the allocated array. ]*/
TEST_FUNCTION(THANDLE_TUPLE_ARRAY_CREATE_with_3_fields_10_elements_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 10, sizeof(TUPLE_THREE)));
    for (uint32_t i = 0; i < 10; i++)
    {
        STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, NULL));
        STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, NULL));
        STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, NULL));
    }

    // act
    THANDLE_TUPLE_ARRAY_TYPE(TUPLE_THREE)* result = THANDLE_TUPLE_ARRAY_CREATE(TUPLE_THREE)(10);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(uint32_t, 10, result->count);
    for (uint32_t i = 0; i < 10; i++)
    {
        ASSERT_IS_NULL(result->tuple_array[i].foo);
        ASSERT_IS_NULL(result->tuple_array[i].bar);
        ASSERT_IS_NULL(result->tuple_array[i].baz);
    }

    // cleanup
    THANDLE_TUPLE_ARRAY_DESTROY(TUPLE_THREE)(result);
}

/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_004: [ If there are any errors then THANDLE_TUPLE_ARRAY_CREATE(name) shall fail and return NULL. ]*/
TEST_FUNCTION(THANDLE_TUPLE_ARRAY_CREATE_with_3_fields_10_elements_fails_when_underlying_functions_fail)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 10, sizeof(TUPLE_THREE)));
    for (uint32_t i = 0; i < 10; i++)
    {
        STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, NULL));
        STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, NULL));
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
            THANDLE_TUPLE_ARRAY_TYPE(TUPLE_THREE)* result = THANDLE_TUPLE_ARRAY_CREATE(TUPLE_THREE)(10);

            // assert
            ASSERT_IS_NULL(result, "On failed call %zu", i);
        }
    }
}

/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_004: [ If there are any errors then THANDLE_TUPLE_ARRAY_CREATE(name) shall fail and return NULL. ]*/
TEST_FUNCTION(THANDLE_TUPLE_ARRAY_CREATE_with_3_fields_0_elements_fails_when_underlying_functions_fail)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, sizeof(TUPLE_THREE)));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE_TUPLE_ARRAY_TYPE(TUPLE_THREE)* result = THANDLE_TUPLE_ARRAY_CREATE(TUPLE_THREE)(0);

            // assert
            ASSERT_IS_NULL(result, "On failed call %zu", i);
        }
    }
}

//
// THANDLE_TUPLE_ARRAY_DESTROY(name)
//

/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_006: [ If tuple_array is NULL then THANDLE_TUPLE_ARRAY_DESTROY(name) shall fail and return. ]*/
TEST_FUNCTION(THANDLE_TUPLE_ARRAY_DESTROY_with_NULL_returns)
{
    // arrange

    // act
    THANDLE_TUPLE_ARRAY_DESTROY(TUPLE_ONE)(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_008: [ THANDLE_TUPLE_ARRAY_DESTROY(name) shall free the memory allocated in tuple_array. ]*/
TEST_FUNCTION(THANDLE_TUPLE_ARRAY_DESTROY_with_1_field_0_elements_frees_everything)
{
    // arrange
    THANDLE_TUPLE_ARRAY_TYPE(TUPLE_ONE)* tuple_array = THANDLE_TUPLE_ARRAY_CREATE(TUPLE_ONE)(0);
    ASSERT_IS_NOT_NULL(tuple_array);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_TUPLE_ARRAY_DESTROY(TUPLE_ONE)(tuple_array);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_008: [ THANDLE_TUPLE_ARRAY_DESTROY(name) shall free the memory allocated in tuple_array. ]*/
TEST_FUNCTION(THANDLE_TUPLE_ARRAY_DESTROY_with_3_fields_0_elements_frees_everything)
{
    // arrange
    THANDLE_TUPLE_ARRAY_TYPE(TUPLE_THREE)* tuple_array = THANDLE_TUPLE_ARRAY_CREATE(TUPLE_THREE)(0);
    ASSERT_IS_NOT_NULL(tuple_array);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_TUPLE_ARRAY_DESTROY(TUPLE_THREE)(tuple_array);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_007: [ THANDLE_TUPLE_ARRAY_DESTROY(name) shall iterate over all of the elements in tuple_array and call THANDLE_ASSIGN(type) with NULL for each field. ]*/
/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_008: [ THANDLE_TUPLE_ARRAY_DESTROY(name) shall free the memory allocated in tuple_array. ]*/
TEST_FUNCTION(THANDLE_TUPLE_ARRAY_DESTROY_with_1_field_10_elements_not_initialized_frees_everything)
{
    // arrange
    THANDLE_TUPLE_ARRAY_TYPE(TUPLE_ONE)* tuple_array = THANDLE_TUPLE_ARRAY_CREATE(TUPLE_ONE)(10);
    ASSERT_IS_NOT_NULL(tuple_array);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_TUPLE_ARRAY_DESTROY(TUPLE_ONE)(tuple_array);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_007: [ THANDLE_TUPLE_ARRAY_DESTROY(name) shall iterate over all of the elements in tuple_array and call THANDLE_ASSIGN(type) with NULL for each field. ]*/
/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_008: [ THANDLE_TUPLE_ARRAY_DESTROY(name) shall free the memory allocated in tuple_array. ]*/
TEST_FUNCTION(THANDLE_TUPLE_ARRAY_DESTROY_with_3_fields_10_elements_not_initialized_frees_everything)
{
    // arrange
    THANDLE_TUPLE_ARRAY_TYPE(TUPLE_THREE)* tuple_array = THANDLE_TUPLE_ARRAY_CREATE(TUPLE_THREE)(10);
    ASSERT_IS_NOT_NULL(tuple_array);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_TUPLE_ARRAY_DESTROY(TUPLE_THREE)(tuple_array);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_007: [ THANDLE_TUPLE_ARRAY_DESTROY(name) shall iterate over all of the elements in tuple_array and call THANDLE_ASSIGN(type) with NULL for each field. ]*/
/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_008: [ THANDLE_TUPLE_ARRAY_DESTROY(name) shall free the memory allocated in tuple_array. ]*/
TEST_FUNCTION(THANDLE_TUPLE_ARRAY_DESTROY_with_1_field_10_elements_initialized_frees_everything)
{
    // arrange
    THANDLE_TUPLE_ARRAY_TYPE(TUPLE_ONE)* tuple_array = THANDLE_TUPLE_ARRAY_CREATE(TUPLE_ONE)(10);
    ASSERT_IS_NOT_NULL(tuple_array);

    for (uint32_t i = 0; i < tuple_array->count; i++)
    {
        THANDLE(real_RC_STRING) temp = real_rc_string_create("foo");
        ASSERT_IS_NOT_NULL(temp);
        THANDLE_MOVE(real_RC_STRING)(&tuple_array->tuple_array[i].a, &temp);
    }

    umock_c_reset_all_calls();

    for (uint32_t i = 0; i < 10; i++)
    {
        STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_STRING)(&tuple_array->tuple_array[i].a, NULL));
    }
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_TUPLE_ARRAY_DESTROY(TUPLE_ONE)(tuple_array);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_007: [ THANDLE_TUPLE_ARRAY_DESTROY(name) shall iterate over all of the elements in tuple_array and call THANDLE_ASSIGN(type) with NULL for each field. ]*/
/*Tests_SRS_THANDLE_TUPLE_ARRAY_42_008: [ THANDLE_TUPLE_ARRAY_DESTROY(name) shall free the memory allocated in tuple_array. ]*/
TEST_FUNCTION(THANDLE_TUPLE_ARRAY_DESTROY_with_3_fields_10_elements_initialized_frees_everything)
{
    // arrange
    THANDLE_TUPLE_ARRAY_TYPE(TUPLE_THREE)* tuple_array = THANDLE_TUPLE_ARRAY_CREATE(TUPLE_THREE)(10);
    ASSERT_IS_NOT_NULL(tuple_array);

    for (uint32_t i = 0; i < tuple_array->count; i++)
    {
        THANDLE(real_RC_STRING) temp = real_rc_string_create("string1");
        ASSERT_IS_NOT_NULL(temp);
        THANDLE_MOVE(real_RC_STRING)(&tuple_array->tuple_array[i].foo, &temp);
        THANDLE(real_RC_STRING) temp2 = real_rc_string_create("string2");
        ASSERT_IS_NOT_NULL(temp2);
        THANDLE_MOVE(real_RC_STRING)(&tuple_array->tuple_array[i].bar, &temp2);
        THANDLE(real_RC_STRING) temp3 = real_rc_string_create("string3");
        ASSERT_IS_NOT_NULL(temp3);
        THANDLE_MOVE(real_RC_STRING)(&tuple_array->tuple_array[i].baz, &temp3);
    }

    umock_c_reset_all_calls();

    for (uint32_t i = 0; i < 10; i++)
    {
        STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_STRING)(&tuple_array->tuple_array[i].foo, NULL));
        STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_STRING)(&tuple_array->tuple_array[i].bar, NULL));
        STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_STRING)(&tuple_array->tuple_array[i].baz, NULL));
    }
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_TUPLE_ARRAY_DESTROY(TUPLE_THREE)(tuple_array);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
