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

#define ENABLE_MOCKS
#include "umock_c/umock_c.h"
#include "azure_c_util/gballoc.h"

#undef ENABLE_MOCKS

#include "azure_c_util/thandle.h"
#include "thandle_user.h"
#include "thandle_flex_user.h"

static TEST_MUTEX_HANDLE g_testByTest;

#define TEST_A_DEFINE 1
static const int TEST_A = TEST_A_DEFINE;

#define TEST_S_DEFINE "a"
static const char* TEST_S = TEST_S_DEFINE;

#define TEST_S2_DEFINE "aa"
static const char* TEST_S2 = TEST_S2_DEFINE;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

/*the following data structures etc are needed because THANDLE_FREE cannot be called with NULL from a THANDLER(T) holder*/
#define A_B_FIELDS          \
    int, a,                 \
    int, b

MU_DEFINE_STRUCT(A_B, A_B_FIELDS);


#define THANDLE_MALLOC_FUNCTION gballoc_malloc
#define THANDLE_FREE_FUNCTION gballoc_free
#ifdef __cplusplus
extern "C" {
#endif
    THANDLE_TYPE_DECLARE(A_B);
    THANDLE_TYPE_DEFINE(A_B);
#ifdef __cplusplus
    }
#endif
#undef THANDLE_MALLOC_FUNCTION
#undef THANDLE_FREE_FUNCTION

BEGIN_TEST_SUITE(thandle_unittests)

TEST_SUITE_INITIALIZE(setsBufferTempSize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);
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

/* THANDLE_MALLOC * /

/*Tests_SRS_THANDLE_02_013: [ THANDLE_MALLOC shall allocate memory. ]*/
/*Tests_SRS_THANDLE_02_014: [ THANDLE_MALLOC shall initialize the reference count to 1, store dispose and return a T* . ]*/
TEST_FUNCTION(thandle_user_create_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/

    ///act
    THANDLE(LL) ll = ll_create(TEST_A, TEST_S);

    ///assert
    ASSERT_IS_NOT_NULL(ll);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_DEC_REF(LL)(ll);
}

/*Tests_SRS_THANDLE_02_013: [ THANDLE_MALLOC shall allocate memory. ]*/
/*Tests_SRS_THANDLE_02_015: [ If malloc fails then THANDLE_MALLOC shall fail and return NULL. ]*/
TEST_FUNCTION(thandle_user_create_fails_when_thandle_malloc_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG))
        .SetReturn(NULL); /*this is THANDLE_MALLOC*/

    ///act
    THANDLE(LL) ll = ll_create(TEST_A, TEST_S);

    ///assert
    ASSERT_IS_NULL(ll);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/* THANDLE_MALLOC_WITH_EXTRA_SIZE */

/*Tests_SRS_THANDLE_02_020: [ THANDLE_MALLOC_WITH_EXTRA_SIZE shall allocate memory enough to hold T and extra_size. ]*/
/*Tests_SRS_THANDLE_02_021: [ THANDLE_MALLOC_WITH_EXTRA_SIZE shall initialize the reference count to 1, store dispose and return a T*. ]*/
TEST_FUNCTION(thandle_flex_user_create_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/

    ///act
    THANDLE(LL_FLEX) ll = ll_flex_create(TEST_A, TEST_S, 10);

    ///assert
    ASSERT_IS_NOT_NULL(ll);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_DEC_REF(LL_FLEX)(ll);
}

/*Tests_SRS_THANDLE_02_022: [ If malloc fails then THANDLE_MALLOC_WITH_EXTRA_SIZE shall fail and return NULL. ]*/
TEST_FUNCTION(thandle_flex_user_create_fails_when_thandle_malloc_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG))
        .SetReturn(NULL); /*this is THANDLE_MALLOC_WITH_EXTRA_SIZE*/

    ///act
    THANDLE(LL_FLEX) ll = ll_flex_create(TEST_A, TEST_S, 10);

    ///assert
    ASSERT_IS_NULL(ll);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/*Tests_SRS_THANDLE_02_019: [ If extra_size + sizeof(THANDLE_WRAPPER_TYPE_NAME(T)) would exceed SIZE_MAX then THANDLE_MALLOC_WITH_EXTRA_SIZE shall fail and return NULL. ]*/
TEST_FUNCTION(thandle_flex_user_create_fails_when_SIZE_MAX_is_exceeded)
{
    ///arrange

    ///act
    THANDLE(LL_FLEX) ll = ll_flex_create(TEST_A, TEST_S, SIZE_MAX/sizeof(int));

    ///assert
    ASSERT_IS_NULL(ll);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/* THANDLE_DEC_REF */

/*Tests_SRS_THANDLE_02_001: [ If t is NULL then THANDLE_DEC_REF shall return. ]*/
TEST_FUNCTION(THANDLE_DEC_REF_with_t_NULL_returns)
{
    ///arrange
    THANDLE(LL) ll = NULL;

    ///act
    THANDLE_DEC_REF(LL)(ll);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/*Tests_SRS_THANDLE_02_002: [ THANDLE_DEC_REF shall decrement the ref count of t. ]*/
TEST_FUNCTION(THANDLE_DEC_REF_decrements)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
    THANDLE(LL) ll = ll_create(TEST_A, TEST_S);
    ASSERT_IS_NOT_NULL(ll);
    THANDLE_INC_REF(LL)(ll); /*intentionally setting the ref count to 2*/

    umock_c_reset_all_calls();

    ///act
    THANDLE_DEC_REF(LL)(ll);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_DEC_REF(LL)(ll);
}

/*Tests_SRS_THANDLE_02_003: [ If the ref count of t reaches 0 then THANDLE_DEC_REF shall call dispose (if not NULL) and free the used memory. ]*/
/*Tests_SRS_THANDLE_02_005: [ THANDLE_INC_REF shall increment the reference count of t. ]*/
TEST_FUNCTION(THANDLE_DEC_REF_decrements_and_frees_resources)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
    THANDLE(LL) ll = ll_create(TEST_A, TEST_S);
    ASSERT_IS_NOT_NULL(ll);
    THANDLE_INC_REF(LL)(ll); /*intentionally setting the ref count to 2*/
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_ARG)); /*this is the copy of s that is freed*/
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_ARG)); /*this is THANDLE_MALLOC's memory that gets freed*/

    ///act
    THANDLE_DEC_REF(LL)(ll);
    THANDLE_DEC_REF(LL)(ll); /*gets to 0*/

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup - nothing
}

/* THANDLE_INC_REF */

/*Tests_SRS_THANDLE_02_004: [ If t is NULL then THANDLE_INC_REF shall return. ]*/
TEST_FUNCTION(THANDLE_INC_REF_with_t_NULL_returns)
{
    ///arrange

    ///act
    THANDLE_INC_REF(LL)(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup - nothing
}

/* THANDLE_ASSIGN */

/*Tests_SRS_THANDLE_02_006: [ If t1 is NULL then THANDLE_ASSIGN shall return. ]*/
TEST_FUNCTION(THANDLE_ASSIGN_with_t1_NULL_returns)
{
    ///arrange

    ///act
    THANDLE_ASSIGN(LL)(NULL, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup - nothing
}

/*Tests_SRS_THANDLE_02_007: [ If *t1 is NULL and t2 is NULL then THANDLE_ASSIGN shall return. ]*/
TEST_FUNCTION(THANDLE_ASSIGN_with_t1_NULL_t2_NULL)
{
    ///arrange
    THANDLE(LL) t1 = NULL;

    ///act
    THANDLE_ASSIGN(LL)(&t1, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(t1);

    ///cleanup - nothing
}

/*Tests_SRS_THANDLE_02_008: [ If *t1 is NULL and t2 is not NULL then THANDLE_ASSIGN shall increment the reference count of t2 and store t2 in *t1. ]*/
TEST_FUNCTION(THANDLE_ASSIGN_with_t1_NULL_t2_not_NULL)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
    THANDLE(LL) t2 = ll_create(TEST_A, TEST_S);
    ASSERT_IS_NOT_NULL(t2);

    THANDLE(LL) t1 = NULL;
    umock_c_reset_all_calls();
    
    ///act
    THANDLE_ASSIGN(LL)(&t1, t2);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(t1 == t2);

    ///cleanup
    THANDLE_DEC_REF(LL)(t1);
    THANDLE_DEC_REF(LL)(t2);
}

/*Tests_SRS_THANDLE_02_009: [ If *t1 is not NULL and t2 is NULL then THANDLE_ASSIGN shall decrement the reference count of *t1 and store NULL in *t1. ]*/
TEST_FUNCTION(THANDLE_ASSIGN_with_t1_not_NULL_t2_NULL)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
    THANDLE(LL) t1 = ll_create(TEST_A, TEST_S);
    ASSERT_IS_NOT_NULL(t1);

    THANDLE(LL) t2 = NULL;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_ARG)); /*this is the copy of s*/
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_ARG)); /*this is THANDLE_MALLOC's memory that gets freed*/

    ///act
    THANDLE_ASSIGN(LL)(&t1, t2);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(t1 == t2);

    ///cleanup -
}

/*Tests_SRS_THANDLE_02_010: [ If *t1 is not NULL and t2 is not NULL then THANDLE_ASSIGN shall increment the reference count of t2, shall decrement the reference count of *t1 and store t2 in *t1. ]*/
TEST_FUNCTION(THANDLE_ASSIGN_with_t1_not_NULL_t2_not_NULL)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
    THANDLE(LL) t1 = ll_create(TEST_A, TEST_S);
    ASSERT_IS_NOT_NULL(t1);

    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(TEST_S2_DEFINE))); /*this is the copy of s2*/
    THANDLE(LL) t2 = ll_create(TEST_A, TEST_S2);
    ASSERT_IS_NOT_NULL(t2);
   
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_ARG)); /*this is the copy of s that goes away*/
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_ARG)); /*this is THANDLE_MALLOC's memory that gets freed*/

    ///act
    THANDLE_ASSIGN(LL)(&t1, t2);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(t1 == t2);

    ///cleanup
    THANDLE_DEC_REF(LL)(t1);
    THANDLE_DEC_REF(LL)(t2);
}

/* THANDLE_INITIALIZE */

/*Tests_SRS_THANDLE_02_011: [ If lvalue is NULL then THANDLE_INITIALIZE shall return. ]*/
TEST_FUNCTION(THANDLE_INITIALIZE_with_lvalue_NULL_returns)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
    THANDLE(LL) t2 = ll_create(TEST_A, TEST_S);
    ASSERT_IS_NOT_NULL(t2);
    umock_c_reset_all_calls();

    ///act
    THANDLE_INITIALIZE(LL)(NULL, t2);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_DEC_REF(LL)(t2);
}

/*Tests_SRS_THANDLE_02_018: [ If rvalue is NULL then THANDLE_INITIALIZE shall store NULL in *lvalue. ]*/
TEST_FUNCTION(THANDLE_INITIALIZE_with_rvalue_NULL_succeeds)
{
    ///arrange
    THANDLE(LL) t2 = (THANDLE(LL))(0x444);
    umock_c_reset_all_calls();

    ///act
    THANDLE_INITIALIZE(LL)(&t2, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(t2);

    ///cleanup
}

/*Tests_SRS_THANDLE_02_012: [ THANDLE_INITIALIZE shall increment the reference count of rvalue and store it in *lvalue. ]*/
TEST_FUNCTION(THANDLE_INITIALIZE_with_lvalue_non_NULL_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
    THANDLE(LL) t1 = ll_create(TEST_A, TEST_S);
    THANDLE(LL) t2 = NULL;
    ASSERT_IS_NOT_NULL(t1);
    umock_c_reset_all_calls();

    ///act
    THANDLE_INITIALIZE(LL)(&t2, t1);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(t2);
    ASSERT_ARE_EQUAL(void_ptr, t2, t1);

    ///cleanup
    THANDLE_DEC_REF(LL)(t1);
    THANDLE_DEC_REF(LL)(t2);
}

/* THANDLE_FREE */

/*Tests_SRS_THANDLE_02_017: [ THANDLE_FREE shall free the allocated memory by THANDLE_MALLOC. ]*/
TEST_FUNCTION(THANDLE_FREE_frees_memory)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
    THANDLE(LL) t1 = ll_create(TEST_A, TEST_S);
    ASSERT_IS_NOT_NULL(t1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_ARG)); /*this is the copy of s that goes away*/
    STRICT_EXPECTED_CALL(gballoc_free(IGNORED_ARG)); /*this is THANDLE_MALLOC's memory that gets freed*/
    ///act

    THANDLE_DEC_REF(LL)(t1);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/*Tests_SRS_THANDLE_02_016: [ If t is NULL then THANDLE_FREE shall return. ]*/
TEST_FUNCTION(THANDLE_FREE_with_t_NULL_returns)
{
    ///arrange

    ///act
    THANDLE_FREE(A_B)(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/*Tests_SRS_THANDLE_02_023: [ If t is NULL then THANDLE_GET_T(T) shall return NULL. ]*/
TEST_FUNCTION(THANDLE_GET_T_with_t_NULL_returns_NULL)
{
    ///arrange

    ///act
    A_B* result = THANDLE_GET_T(A_B)(NULL);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    
    ///cleanup
}

/*Tests_SRS_THANDLE_02_024: [ THANDLE_GET_T(T) shall return the same pointer as THANDLE_MALLOC/THANDLE_MALLOC_WITH_EXTRA_SIZE returned at the handle creation time. ]*/
TEST_FUNCTION(THANDLE_GET_T_with_t_not_NULL_returns_original_pointer) /*direct testing is not really possible (GET_T is static) but shall be inferred by the actions of _increment_a and _get_a*/
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(gballoc_malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
    THANDLE(LL) ll = ll_create(TEST_A, TEST_S);
    ASSERT_IS_NOT_NULL(ll);
    int incremented;

    ///act
    ll_increment_a(ll, 5);
    incremented = ll_get_a(ll);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, TEST_A + 5, incremented);

    ///cleanup
    THANDLE_DEC_REF(LL)(ll);
}

/*returns a pointer to an array of 2 THANDLE(LL)*/
static void builds_out_arg(THANDLE(LL)** x)
{
    *x = (THANDLE(LL)*)gballoc_malloc(sizeof(THANDLE(LL)) * 2);
    ASSERT_IS_NOT_NULL(*x);
}

/*this test wants to see that an array of THANDLE(LL) can be returned from some constructor as out argument*/
TEST_FUNCTION(THANDLE_T_can_build_an_array)
{
    THANDLE(LL)* arr;
    builds_out_arg(&arr); /*arr points to an array of 2 THANDLE(LL)s*/

    THANDLE(LL) temp = ll_create(1, "4");
    ASSERT_IS_NOT_NULL(temp);
    THANDLE_INITIALIZE(LL)(&arr[0], temp);
    THANDLE_DEC_REF(LL)(temp);

    THANDLE(LL) temp2 = ll_create(2, "44");
    ASSERT_IS_NOT_NULL(temp2);
    THANDLE_INITIALIZE(LL)(&arr[1], temp2);
    THANDLE_DEC_REF(LL)(temp2);

    THANDLE_DEC_REF(LL)(arr[0]);
    THANDLE_DEC_REF(LL)(arr[1]);

    gballoc_free((void*)arr);
}


END_TEST_SUITE(thandle_unittests)
