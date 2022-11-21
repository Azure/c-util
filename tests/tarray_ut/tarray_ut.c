// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdlib.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "umock_c/umock_c_negative_tests.h"

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#define ENABLE_MOCKS
#include "umock_c/umock_c.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_util/thandle.h"
#include "c_util/tarray.h"

static TEST_MUTEX_HANDLE g_testByTest;

/*TARRAY with regular types*/
TARRAY_DEFINE_STRUCT_TYPE(uint32_t)
THANDLE_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(uint32_t));
THANDLE_TYPE_DEFINE(TARRAY_TYPEDEF_NAME(uint32_t));

TARRAY_TYPE_DECLARE(uint32_t);
TARRAY_TYPE_DEFINE(uint32_t);

typedef struct A_TEST_TAG
{
    int a;
}A_TEST;

THANDLE_TYPE_DECLARE(A_TEST);
THANDLE_TYPE_DEFINE(A_TEST);

TARRAY_DEFINE_STRUCT_TYPE(THANDLE(A_TEST))
THANDLE_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(THANDLE(A_TEST)));
THANDLE_TYPE_DEFINE(TARRAY_TYPEDEF_NAME(THANDLE(A_TEST)));

/*TARRAY works with THANDLEs too*/
TARRAY_TYPE_DECLARE(THANDLE(A_TEST));
TARRAY_TYPE_DEFINE(THANDLE(A_TEST));


MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(it_does_something)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
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

    umock_c_negative_tests_init();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleans)
{
    umock_c_negative_tests_deinit();
    TEST_MUTEX_RELEASE(g_testByTest);
}

/* TARRAY_CREATE(T) */

/*Tests_SRS_TARRAY_02_001: [ TARRAY_CREATE(T) shall call THANDLE_MALLOC to allocate the result. ]*/
/*Tests_SRS_TARRAY_02_002: [ TARRAY_CREATE(T) shall call malloc to allocate result->arr. ]*/
/*Tests_SRS_TARRAY_02_003: [ TARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(uint32_t_can_be_created)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*struct*/
    STRICT_EXPECTED_CALL(malloc(1 * sizeof(uint32_t))); /*inner array*/

    ///act
    TARRAY(uint32_t) arr = TARRAY_CREATE(uint32_t)();

    arr->arr[0] = 4; /*assignment is possible*/

    ///assert
    ASSERT_IS_NOT_NULL(arr);
    ASSERT_IS_NOT_NULL(arr->arr);
    ASSERT_ARE_EQUAL(uint32_t, 1, arr->capacity);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    TARRAY_ASSIGN(uint32_t)(&arr, NULL);
}

/*Tests_SRS_TARRAY_02_004: [ If there are any failures then TARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(uint32_t_can_be_created_unhappy_path_1)
{
    ///arrange

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*struct*/
    STRICT_EXPECTED_CALL(malloc(1 * sizeof(uint32_t)))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*struct*/

    ///act
    TARRAY(uint32_t) arr = TARRAY_CREATE(uint32_t)();
    
    ///assert
    ASSERT_IS_NULL(arr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_TARRAY_02_004: [ If there are any failures then TARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(uint32_t_can_be_created_unhappy_path_2)
{
    ///arrange

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)) /*struct*/
        .SetReturn(NULL);

    ///act
    TARRAY(uint32_t) arr = TARRAY_CREATE(uint32_t)();

    ///assert
    ASSERT_IS_NULL(arr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_TARRAY_02_001: [ TARRAY_CREATE(T) shall call THANDLE_MALLOC to allocate the result. ]*/
/*Tests_SRS_TARRAY_02_002: [ TARRAY_CREATE(T) shall call malloc to allocate result->arr. ]*/
/*Tests_SRS_TARRAY_02_003: [ TARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(TARRAY_of_THANDLE_can_be_created)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*struct*/
    STRICT_EXPECTED_CALL(malloc(1 * sizeof(THANDLE(A_TEST)))); /*inner array*/

    ///act
    TARRAY(THANDLE(A_TEST)) arr_of_a = TARRAY_CREATE(THANDLE(A_TEST))();

    ///assert
    ASSERT_IS_NOT_NULL(arr_of_a);
    ASSERT_IS_NOT_NULL(arr_of_a->arr);
    ASSERT_ARE_EQUAL(uint32_t, 1, arr_of_a->capacity);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    /*array element management is entirely in user's hands. TARRAY only knows about ensuring capacity for an array.*/
    THANDLE(A_TEST) a = THANDLE_MALLOC(A_TEST)(NULL); /*NULL = no special handling of the int inside the A_TEST*/
    ASSERT_IS_NOT_NULL(a);
    
    THANDLE_INITIALIZE(A_TEST)(&arr_of_a->arr[0], a);
    THANDLE_ASSIGN(A_TEST)(&arr_of_a->arr[0], NULL);
    THANDLE_ASSIGN(A_TEST)(&a, NULL);

    ///clean
    TARRAY_ASSIGN(THANDLE(A_TEST))(&arr_of_a, NULL);
}

/* TARRAY_CREATE_WITH_CAPACITY(T) */

/* Tests_SRS_TARRAY_01_001: [ If capacity is 0, TARRAY_CREATE_WITH_CAPACITY(T) shall fail and return NULL. ]*/
TEST_FUNCTION(TARRAY_CREATE_WITH_CAPACITY_with_0_capacity_fails)
{
    ///arrange

    ///act
    TARRAY(uint32_t) arr_of_uint32_t = TARRAY_CREATE_WITH_CAPACITY(uint32_t)(0);

    ///assert
    ASSERT_IS_NULL(arr_of_uint32_t);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_TARRAY_01_002: [ TARRAY_CREATE_WITH_CAPACITY(T) shall call THANDLE_MALLOC to allocate the result. ]*/
/* Tests_SRS_TARRAY_01_003: [ TARRAY_CREATE_WITH_CAPACITY(T) shall call malloc_2 to allocate capacity entries for result->arr. ]*/
/* Tests_SRS_TARRAY_01_004: [ TARRAY_CREATE_WITH_CAPACITY(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(TARRAY_CREATE_WITH_CAPACITY_with_1_uint32_t_TARRAY_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*struct*/
    STRICT_EXPECTED_CALL(malloc_2(1, sizeof(uint32_t))); /*inner array*/

    ///act
    TARRAY(uint32_t) arr_of_uint32_t = TARRAY_CREATE_WITH_CAPACITY(uint32_t)(1);

    ///assert
    ASSERT_IS_NOT_NULL(arr_of_uint32_t);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    arr_of_uint32_t->arr[0] = 4; /*assignment is possible*/

    // cleanup
    TARRAY_ASSIGN(uint32_t)(&arr_of_uint32_t, NULL);
}

#define N_ELEMENTS 1000

/* Tests_SRS_TARRAY_01_002: [ TARRAY_CREATE_WITH_CAPACITY(T) shall call THANDLE_MALLOC to allocate the result. ]*/
/* Tests_SRS_TARRAY_01_003: [ TARRAY_CREATE_WITH_CAPACITY(T) shall call malloc_2 to allocate capacity entries for result->arr. ]*/
/* Tests_SRS_TARRAY_01_004: [ TARRAY_CREATE_WITH_CAPACITY(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(MU_C3(TARRAY_CREATE_WITH_CAPACITY_with_,N_ELEMENTS,_uint32_t_TARRAY_succeeds))
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*struct*/
    STRICT_EXPECTED_CALL(malloc_2(N_ELEMENTS, sizeof(uint32_t))); /*inner array*/

    ///act
    TARRAY(uint32_t) arr_of_uint32_t = TARRAY_CREATE_WITH_CAPACITY(uint32_t)(N_ELEMENTS);

    ///assert
    ASSERT_IS_NOT_NULL(arr_of_uint32_t);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    for (uint32_t i = 0; i < N_ELEMENTS; i++)
    {
        arr_of_uint32_t->arr[i] = 4; /*assignment is possible*/
    }

    // cleanup
    TARRAY_ASSIGN(uint32_t)(&arr_of_uint32_t, NULL);
}

/* Tests_SRS_TARRAY_01_005: [ If there are any failures then TARRAY_CREATE_WITH_CAPACITY(T) shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_TARRAY_CREATE_WITH_CAPACITY_also_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*struct*/
    STRICT_EXPECTED_CALL(malloc_2(1, sizeof(uint32_t))); /*inner array*/

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            ///act
            TARRAY(uint32_t) arr_of_uint32_t = TARRAY_CREATE_WITH_CAPACITY(uint32_t)(1);

            // assert
            ASSERT_IS_NULL(arr_of_uint32_t, "On failed call %zu", i);
        }
    }
}

#define N_ELEMENTS 1000

/* Tests_SRS_TARRAY_01_002: [ TARRAY_CREATE_WITH_CAPACITY(T) shall call THANDLE_MALLOC to allocate the result. ]*/
/* Tests_SRS_TARRAY_01_003: [ TARRAY_CREATE_WITH_CAPACITY(T) shall call malloc_2 to allocate capacity entries for result->arr. ]*/
/* Tests_SRS_TARRAY_01_004: [ TARRAY_CREATE_WITH_CAPACITY(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(MU_C3(TARRAY_CREATE_WITH_CAPACITY_with_, N_ELEMENTS, _THANDLE_A_TEST_TARRAY_succeeds))
{
    ///arrange

    /*array element management is entirely in user's hands. TARRAY only knows about ensuring capacity for an array.*/
    THANDLE(A_TEST) a = THANDLE_MALLOC(A_TEST)(NULL); /*NULL = no special handling of the int inside the A_TEST*/
    ASSERT_IS_NOT_NULL(a);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*struct*/
    STRICT_EXPECTED_CALL(malloc_2(N_ELEMENTS, sizeof(THANDLE(A_TEST)))); /*inner array*/

    ///act
    TARRAY(THANDLE(A_TEST)) arr_of_a = TARRAY_CREATE_WITH_CAPACITY(THANDLE(A_TEST))(N_ELEMENTS);

    ///assert
    ASSERT_IS_NOT_NULL(arr_of_a);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    for (uint32_t i = 0; i < N_ELEMENTS; i++)
    {
        THANDLE_INITIALIZE(A_TEST)(&arr_of_a->arr[i], a);
        THANDLE_ASSIGN(A_TEST)(&arr_of_a->arr[i], NULL);
        THANDLE_ASSIGN(A_TEST)(&a, NULL);
    }

    // cleanup
    TARRAY_ASSIGN(THANDLE(A_TEST))(&arr_of_a, NULL);
}

/* TARRAY_ENSURE_CAPACITY(T) */

/*Tests_SRS_TARRAY_02_005: [ If tarray is NULL then TARRAY_ENSURE_CAPACITY(T) shall fail and return a non-zero value. ]*/
TEST_FUNCTION(TARRAY_ENSURE_CAPACITY_with_tarray_NULL_fails)
{
    ///arrange
    int result;

    ///act
    result = TARRAY_ENSURE_CAPACITY(uint32_t)(NULL, 2);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_TARRAY_02_006: [ If tarray->capacity is greater than or equal to capacity then TARRAY_ENSURE_CAPACITY(T) shall succeed and return 0. ]*/
/*Tests_SRS_TARRAY_02_008: [ TARRAY_ENSURE_CAPACITY(T) shall succeed, record the new computed capacity, and return 0. ]*/
TEST_FUNCTION(TARRAY_ENSURE_CAPACITY_with_same_capacity_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*struct*/
    STRICT_EXPECTED_CALL(malloc(1 * sizeof(uint32_t))); /*inner array*/
    TARRAY(uint32_t) arr = TARRAY_CREATE(uint32_t)();

    int result;

    ///act
    result = TARRAY_ENSURE_CAPACITY(uint32_t)(arr, 1);/*capacity is already at 1*/
    
    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint32_t, 1, arr->capacity);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    TARRAY_ASSIGN(uint32_t)(&arr, NULL);
}

/*Tests_SRS_TARRAY_02_007: [ TARRAY_ENSURE_CAPACITY(T) shall shall call realloc_2 to resize arr to the next multiple of 2 greater than or equal to capacity. ]*/
TEST_FUNCTION(TARRAY_ENSURE_CAPACITY_reallocs_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*struct*/
    STRICT_EXPECTED_CALL(malloc(1 * sizeof(uint32_t))); /*inner array*/
    TARRAY(uint32_t) arr = TARRAY_CREATE(uint32_t)();

    int result;

    STRICT_EXPECTED_CALL(realloc_2(arr->arr, 8, sizeof(uint32_t)));

    ///act
    result = TARRAY_ENSURE_CAPACITY(uint32_t)(arr, 5);/*capacity is already at 1, the next power of 2 after 5 is 8*/

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint32_t, 8, arr->capacity);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    TARRAY_ASSIGN(uint32_t)(&arr, NULL);
}

/*Tests_SRS_TARRAY_02_007: [ TARRAY_ENSURE_CAPACITY(T) shall shall call realloc_2 to resize arr to the next multiple of 2 greater than or equal to capacity. ]*/
TEST_FUNCTION(TARRAY_ENSURE_CAPACITY_reallocs_does_not_realloc_when_size_is_exactly_power_of_2)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*struct*/
    STRICT_EXPECTED_CALL(malloc(1 * sizeof(uint32_t))); /*inner array*/
    TARRAY(uint32_t) arr = TARRAY_CREATE(uint32_t)();

    int result;

    STRICT_EXPECTED_CALL(realloc_2(arr->arr, 8, sizeof(uint32_t)));
    result = TARRAY_ENSURE_CAPACITY(uint32_t)(arr, 5);/*capacity is already at 1, the next multiple of 2 of 5 is 8*/
    ASSERT_ARE_EQUAL(int, 0, result);

    ///act
    result = TARRAY_ENSURE_CAPACITY(uint32_t)(arr, 8);          /*capacity is already at 8. Note: no new realloc*/

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint32_t, 8, arr->capacity);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    TARRAY_ASSIGN(uint32_t)(&arr, NULL);
}

/*Tests_SRS_TARRAY_02_010: [ If capacity is greater than 2147483648 then TARRAY_ENSURE_CAPACITY(T) shall fail and return a non-zero value. ]*/
TEST_FUNCTION(TARRAY_ENSURE_CAPACITY_with_overflow_capacity_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*struct*/
    STRICT_EXPECTED_CALL(malloc(1 * sizeof(uint32_t))); /*inner array*/
    TARRAY(uint32_t) arr = TARRAY_CREATE(uint32_t)();

    int result;

    ///act
    result = TARRAY_ENSURE_CAPACITY(uint32_t)(arr, 1+(((uint32_t)1)<<31)); /*exceeds UINT32_MAX when it is doubled*/

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint32_t, 1, arr->capacity);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    TARRAY_ASSIGN(uint32_t)(&arr, NULL);
}

/*Tests_SRS_TARRAY_02_009: [ If there are any failures then TARRAY_ENSURE_CAPACITY(T) shall fail and return a non-zero value. ]*/
TEST_FUNCTION(TARRAY_ENSURE_CAPACITY_unhappy_path)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*struct*/
    STRICT_EXPECTED_CALL(malloc(1 * sizeof(uint32_t))); /*inner array*/
    TARRAY(uint32_t) arr = TARRAY_CREATE(uint32_t)();

    int result;

    STRICT_EXPECTED_CALL(realloc_2(arr->arr, 8, sizeof(uint32_t)))
        .SetReturn(NULL);

    ///act
    result = TARRAY_ENSURE_CAPACITY(uint32_t)(arr, 5);/*capacity is already at 1, the next multiple of 2 of 5 is 8*/
    
    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint32_t, 1, arr->capacity);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    TARRAY_ASSIGN(uint32_t)(&arr, NULL);
}


END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
