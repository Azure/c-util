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
#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h" /*THANDLE needs malloc/malloc_flex/free to exist*/
#include "c_pal/gballoc_hl_redirect.h" 
#include "malloc_mocks.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "g_off_t_off.h"
#include "g_on_t_off.h"
#include "g_on_t_on.h"

static TEST_MUTEX_HANDLE g_testByTest;

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

    REGISTER_GLOBAL_MOCK_HOOK(global_malloc, real_gballoc_hl_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(global_malloc_flex, real_gballoc_hl_malloc_flex);
    REGISTER_GLOBAL_MOCK_HOOK(global_free, real_gballoc_hl_free);

    REGISTER_GLOBAL_MOCK_HOOK(type_malloc, real_gballoc_hl_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(type_malloc_flex, real_gballoc_hl_malloc_flex);
    REGISTER_GLOBAL_MOCK_HOOK(type_free, real_gballoc_hl_free);

    REGISTER_GLOBAL_MOCK_HOOK(var_malloc, real_gballoc_hl_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(var_malloc_flex, real_gballoc_hl_malloc_flex);
    REGISTER_GLOBAL_MOCK_HOOK(var_free, real_gballoc_hl_free);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

    umock_c_init(on_umock_c_error);
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

/*Tests_SRS_THANDLE_02_041: [ If THANDLE_MALLOC_FUNCTION is not NULL then THANDLE_MALLOC_FUNCTION / THANDLE_FREE_FUNCTION shall be used to allocate/free memory. ]*/
/*Tests_SRS_THANDLE_02_044: [ THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall initialize the reference count to 1, store dispose and free_function and return a T* ]*/
TEST_FUNCTION(G_ON_T_OFF_create_calls_global_malloc)
{
    ///arrange
    STRICT_EXPECTED_CALL(global_malloc(IGNORED_ARG));

    ///act
    THANDLE(G_ON_T_OFF_DUMMY) dummy = G_ON_T_OFF_create(3);

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 3, dummy->x);
    ASSERT_ARE_EQUAL(uint32_t, 0, dummy->n);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_ON_T_OFF_DUMMY)(&dummy, NULL);
}

/*Tests_SRS_THANDLE_02_045: [ If allocating memory fails then THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/
TEST_FUNCTION(G_ON_T_OFF_create_calls_global_malloc_unhappy_path)
{
    ///arrange
    STRICT_EXPECTED_CALL(global_malloc(IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    THANDLE(G_ON_T_OFF_DUMMY) dummy = G_ON_T_OFF_create(3);

    ///assert
    ASSERT_IS_NULL(dummy);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_THANDLE_02_039: [ If malloc_function is not NULL then malloc_function and free_function shall be used to allocate/free memory. ]*/
/*Tests_SRS_THANDLE_02_044: [ THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall initialize the reference count to 1, store dispose and free_function and return a T* ]*/
TEST_FUNCTION(G_ON_T_OFF_create_calls_var_malloc)
{
    ///arrange
    STRICT_EXPECTED_CALL(var_malloc(IGNORED_ARG));

    ///act
    THANDLE(G_ON_T_OFF_DUMMY) dummy = G_ON_T_OFF_create_with_malloc_functions(4);

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 4, dummy->x);
    ASSERT_ARE_EQUAL(uint32_t, 0, dummy->n);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_ON_T_OFF_DUMMY)(&dummy, NULL);
}

/*Tests_SRS_THANDLE_02_045: [ If allocating memory fails then THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/
TEST_FUNCTION(G_ON_T_OFF_create_calls_var_malloc_unhappy_path)
{
    ///arrange
    STRICT_EXPECTED_CALL(var_malloc(IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    THANDLE(G_ON_T_OFF_DUMMY) dummy = G_ON_T_OFF_create_with_malloc_functions(4);

    ///assert
    ASSERT_IS_NULL(dummy);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_THANDLE_02_048: [ If THANDLE_MALLOC_FLEX_FUNCTION is not NULL then THANDLE_MALLOC_FLEX_FUNCTION / THANDLE_FREE_FUNCTION shall be used to allocate/free memory. ]*/
TEST_FUNCTION(G_ON_T_OFF_create_with_extra_size_calls_global_malloc)
{
    ///arrange
    STRICT_EXPECTED_CALL(global_malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    ///act
    THANDLE(G_ON_T_OFF_DUMMY) dummy = G_ON_T_OFF_create_with_extra_size(5, "a");

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 5, dummy->x);
    ASSERT_ARE_EQUAL(uint32_t, 1, dummy->n);
    ASSERT_ARE_EQUAL(char_ptr, "a", dummy->s);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_ON_T_OFF_DUMMY)(&dummy, NULL);
}

/*Tests_SRS_THANDLE_02_046: [ If malloc_flex_function is not NULL then malloc_flex_function and free_function shall be used to allocate memory. ]*/
TEST_FUNCTION(G_ON_T_OFF_create_with_extra_size_with_malloc_functions_calls_var_malloc)
{
    ///arrange
    STRICT_EXPECTED_CALL(var_malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    ///act
    THANDLE(G_ON_T_OFF_DUMMY) dummy = G_ON_T_OFF_create_with_extra_size_with_malloc_functions(6, "ab");

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 6, dummy->x);
    ASSERT_ARE_EQUAL(uint32_t, 2, dummy->n);
    ASSERT_ARE_EQUAL(char_ptr, "ab", dummy->s);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_ON_T_OFF_DUMMY)(&dummy, NULL);
}

/*Tests_SRS_THANDLE_02_058: [ If THANDLE_MALLOC_FLEX_FUNCTION is not NULL then THANDLE_MALLOC_FLEX_FUNCTION / THANDLE_FREE_FUNCTION shall be used to allocate/free memory. ]*/
/*Tests_SRS_THANDLE_02_044: [ THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall initialize the reference count to 1, store dispose and free_function and return a T* ]*/
TEST_FUNCTION(G_ON_T_OFF_create_from_content_flex_calls_global_malloc)
{
    ///arrange
    STRICT_EXPECTED_CALL(global_malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    THANDLE(G_ON_T_OFF_DUMMY) origin = G_ON_T_OFF_create_with_extra_size(7, "abc");
    ASSERT_IS_NOT_NULL(origin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    STRICT_EXPECTED_CALL(global_malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    
    //act
    THANDLE(G_ON_T_OFF_DUMMY) dummy = G_ON_T_OFF_create_from_content_flex(origin);
    
    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 7, dummy->x);
    ASSERT_ARE_EQUAL(uint32_t, 3, dummy->n);
    ASSERT_ARE_EQUAL(char_ptr, "abc", dummy->s);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_ON_T_OFF_DUMMY)(&dummy, NULL);
    THANDLE_ASSIGN(G_ON_T_OFF_DUMMY)(&origin, NULL);
}

/*Tests_SRS_THANDLE_02_056: [ If malloc_flex_function is not NULL then malloc_flex_function and free_function shall be used to allocate memory. ]*/
TEST_FUNCTION(G_ON_T_OFF_create_from_content_flex_with_malloc_functions_calls_var_malloc)
{
    ///arrange
    STRICT_EXPECTED_CALL(global_malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    THANDLE(G_ON_T_OFF_DUMMY) origin = G_ON_T_OFF_create_with_extra_size(7, "abc");
    ASSERT_IS_NOT_NULL(origin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    STRICT_EXPECTED_CALL(var_malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    //act
    THANDLE(G_ON_T_OFF_DUMMY) dummy = G_ON_T_OFF_create_from_content_flex_with_malloc_functions(origin);

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 7, dummy->x);
    ASSERT_ARE_EQUAL(uint32_t, 3, dummy->n);
    ASSERT_ARE_EQUAL(char_ptr, "abc", dummy->s);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_ON_T_OFF_DUMMY)(&dummy, NULL);
    THANDLE_ASSIGN(G_ON_T_OFF_DUMMY)(&origin, NULL);
}

/*Tests_SRS_THANDLE_02_040: [ If malloc_function from THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS is not NULL then THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS's malloc_function and free_function shall be used to allocate/free memory. ]*/
TEST_FUNCTION(G_ON_T_ON_create_calls_type_malloc)
{
    ///arrange
    STRICT_EXPECTED_CALL(type_malloc(IGNORED_ARG));

    ///act
    THANDLE(G_ON_T_ON_DUMMY) dummy = G_ON_T_ON_create(3);

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 3, dummy->x);
    ASSERT_ARE_EQUAL(uint32_t, 0, dummy->n);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_ON_T_ON_DUMMY)(&dummy, NULL);
}

/*Tests_SRS_THANDLE_02_045: [ If allocating memory fails then THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/
TEST_FUNCTION(G_ON_T_ON_create_calls_type_malloc_unhappy_path)
{
    ///arrange
    STRICT_EXPECTED_CALL(type_malloc(IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    THANDLE(G_ON_T_ON_DUMMY) dummy = G_ON_T_ON_create(3);

    ///assert
    ASSERT_IS_NULL(dummy);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_THANDLE_02_044: [ THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall initialize the reference count to 1, store dispose and free_function and return a T* ]*/
/*Tests_SRS_THANDLE_02_039: [ If malloc_function is not NULL then malloc_function and free_function shall be used to allocate/free memory. ]*/
TEST_FUNCTION(G_ON_T_ON_create_calls_var_malloc)
{
    ///arrange
    STRICT_EXPECTED_CALL(var_malloc(IGNORED_ARG));

    ///act
    THANDLE(G_ON_T_ON_DUMMY) dummy = G_ON_T_ON_create_with_malloc_functions(4);

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 4, dummy->x);
    ASSERT_ARE_EQUAL(uint32_t, 0, dummy->n);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_ON_T_ON_DUMMY)(&dummy, NULL);
}

/*Tests_SRS_THANDLE_02_045: [ If allocating memory fails then THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/
TEST_FUNCTION(G_ON_T_ON_create_calls_var_malloc_unappy_path)
{
    ///arrange
    STRICT_EXPECTED_CALL(var_malloc(IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    THANDLE(G_ON_T_ON_DUMMY) dummy = G_ON_T_ON_create_with_malloc_functions(4);

    ///assert
    ASSERT_IS_NULL(dummy);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_THANDLE_02_047: [ If malloc_flex_function from THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS is not NULL then THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS's malloc_flex_function and free_function shall be used to allocate/free memory. ]*/
TEST_FUNCTION(G_ON_T_ON_create_with_extra_size_calls_type_malloc)
{
    ///arrange
    STRICT_EXPECTED_CALL(type_malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    ///act
    THANDLE(G_ON_T_ON_DUMMY) dummy = G_ON_T_ON_create_with_extra_size(5, "a");

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 5, dummy->x);
    ASSERT_ARE_EQUAL(uint32_t, 1, dummy->n);
    ASSERT_ARE_EQUAL(char_ptr, "a", dummy->s);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_ON_T_ON_DUMMY)(&dummy, NULL);
}

/*Tests_SRS_THANDLE_02_046: [ If malloc_flex_function is not NULL then malloc_flex_function and free_function shall be used to allocate memory. ]*/
TEST_FUNCTION(G_ON_T_ON_create_with_extra_size_with_malloc_functions_calls_var_malloc)
{
    ///arrange
    STRICT_EXPECTED_CALL(var_malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    ///act
    THANDLE(G_ON_T_ON_DUMMY) dummy = G_ON_T_ON_create_with_extra_size_with_malloc_functions(6, "ab");

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 6, dummy->x);
    ASSERT_ARE_EQUAL(uint32_t, 2, dummy->n);
    ASSERT_ARE_EQUAL(char_ptr, "ab", dummy->s);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_ON_T_ON_DUMMY)(&dummy, NULL);
}

/*Tests_SRS_THANDLE_02_044: [ THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall initialize the reference count to 1, store dispose and free_function and return a T* ]*/
/*Tests_SRS_THANDLE_02_057: [ If malloc_flex_function from THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS is not NULL then THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS's malloc_flex_function and free_function shall be used to allocate/free memory. ]*/
TEST_FUNCTION(G_ON_T_ON_create_from_content_flex_calls_type_malloc)
{
    ///arrange
    STRICT_EXPECTED_CALL(type_malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    THANDLE(G_ON_T_ON_DUMMY) origin = G_ON_T_ON_create_with_extra_size(7, "abc");
    ASSERT_IS_NOT_NULL(origin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    STRICT_EXPECTED_CALL(type_malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    //act
    THANDLE(G_ON_T_ON_DUMMY) dummy = G_ON_T_ON_create_from_content_flex(origin);

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 7, dummy->x);
    ASSERT_ARE_EQUAL(uint32_t, 3, dummy->n);
    ASSERT_ARE_EQUAL(char_ptr, "abc", dummy->s);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_ON_T_ON_DUMMY)(&dummy, NULL);
    THANDLE_ASSIGN(G_ON_T_ON_DUMMY)(&origin, NULL);
}

/*Tests_SRS_THANDLE_02_056: [ If malloc_flex_function is not NULL then malloc_flex_function and free_function shall be used to allocate memory. ]*/
/*Tests_SRS_THANDLE_02_057: [ If malloc_flex_function from THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS is not NULL then THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS's malloc_flex_function and free_function shall be used to allocate/free memory. ]*/
TEST_FUNCTION(G_ON_T_ON_create_from_content_flex_with_malloc_functions_calls_var_malloc)
{
    ///arrange
    STRICT_EXPECTED_CALL(type_malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    THANDLE(G_ON_T_ON_DUMMY) origin = G_ON_T_ON_create_with_extra_size(7, "abc");
    ASSERT_IS_NOT_NULL(origin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    STRICT_EXPECTED_CALL(var_malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    //act
    THANDLE(G_ON_T_ON_DUMMY) dummy = G_ON_T_ON_create_from_content_flex_with_malloc_functions(origin);

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 7, dummy->x);
    ASSERT_ARE_EQUAL(uint32_t, 3, dummy->n);
    ASSERT_ARE_EQUAL(char_ptr, "abc", dummy->s);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_ON_T_ON_DUMMY)(&dummy, NULL);
    THANDLE_ASSIGN(G_ON_T_ON_DUMMY)(&origin, NULL);
}

/*Tests_SRS_THANDLE_02_042: [ If no function can be found to allocate/free memory then THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall use malloc and free. ]*/
TEST_FUNCTION(G_OFF_T_OFF_uses_malloc_when_no_function_is_specified_1)
{
    ///arrange
    
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    //act
    THANDLE(G_OFF_T_OFF_DUMMY) dummy = G_OFF_T_OFF_create(8);

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 8, dummy->x);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_OFF_T_OFF_DUMMY)(&dummy, NULL);
}

/*Tests_SRS_THANDLE_02_049: [ If no function can be found to allocate/free memory then THANDLE_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS shall use malloc_flex and free. ]*/
TEST_FUNCTION(G_OFF_T_OFF_uses_malloc_flex_when_no_function_is_specified_2)
{
    ///arrange

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));


    //act
    THANDLE(G_OFF_T_OFF_DUMMY) dummy = G_OFF_T_OFF_create_with_extra_size(9, "abcde");

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 9, dummy->x);
    ASSERT_ARE_EQUAL(uint32_t, 5, dummy->n);
    ASSERT_ARE_EQUAL(char_ptr, "abcde", dummy->s);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_OFF_T_OFF_DUMMY)(&dummy, NULL);
}

/*Tests_SRS_THANDLE_02_054: [ If get_sizeof is NULL then THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/
TEST_FUNCTION(THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS_with_get_sizeof_NULL_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(var_malloc(IGNORED_ARG));
    THANDLE(G_OFF_T_OFF_DUMMY) origin = G_OFF_T_OFF_create_with_malloc_functions(9);
    ASSERT_IS_NOT_NULL(origin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //act
    THANDLE(G_OFF_T_OFF_DUMMY) dummy = G_OFF_T_OFF_create_from_content_flex_with_getsizeof_NULL(origin);

    ///assert
    ASSERT_IS_NULL(dummy);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_OFF_T_OFF_DUMMY)(&origin, NULL);
}

/*Tests_SRS_THANDLE_02_059: [ If no function can be found to allocate/free memory then THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/
TEST_FUNCTION(THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS_with_no_explicit_malloc_functions_calls_malloc_flex)
{
    ///arrange
    STRICT_EXPECTED_CALL(var_malloc(IGNORED_ARG));
    THANDLE(G_OFF_T_OFF_DUMMY) origin = G_OFF_T_OFF_create_with_malloc_functions(9);
    ASSERT_IS_NOT_NULL(origin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    //act
    THANDLE(G_OFF_T_OFF_DUMMY) dummy = G_OFF_T_OFF_create_from_content_flex(origin);

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 9, dummy->x);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_OFF_T_OFF_DUMMY)(&origin, NULL);
    THANDLE_ASSIGN(G_OFF_T_OFF_DUMMY)(&dummy, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

