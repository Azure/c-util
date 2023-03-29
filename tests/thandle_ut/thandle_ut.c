// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdlib.h>
#include <stddef.h>


#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

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
#include "thandle_user.h"
#include "thandle_user_33_characters.h"
#include "thandle_flex_user.h"

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


    THANDLE_TYPE_DECLARE(A_B);
    THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(A_B, gballoc_hl_malloc, gballoc_hl_malloc_flex, gballoc_hl_free);



typedef struct A_S_TAG
{
    int a;
    char* s;
}A_S;

static int copy_A_S(A_S* destination, const A_S* source)
{
    int result;
    destination->a = source->a;
    destination->s = malloc(strlen(source->s)+1);

    if (destination->s == NULL)
    {
        result = MU_FAILURE;
    }
    else
    {
        (void)memcpy(destination->s, source->s, strlen(source->s) + 1);
        result = 0;
    }

    return result;
}

static void dispose_A_S(A_S* a_s)
{
    free(a_s->s);
}


    THANDLE_TYPE_DECLARE(A_S);
    THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(A_S, gballoc_hl_malloc, gballoc_hl_malloc_flex, gballoc_hl_free);



typedef struct A_FLEX_TAG
{
    int n;
    int p[]; /*p has always "n" elements*/
}A_FLEX;


static size_t get_sizeof_A_FLEX(const A_FLEX* source)
{
    return sizeof(A_FLEX) + source->n * sizeof(int);
}


    THANDLE_TYPE_DECLARE(A_FLEX);
    THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(A_FLEX, gballoc_hl_malloc, gballoc_hl_malloc_flex, gballoc_hl_free);




/*a flex structure that has a non-default copy*/
typedef struct A_S_FLEX_TAG
{
    int n;
    char* s;
    int p[]; /*p has always "n" elements*/
}A_S_FLEX;

static int copy_A_S_FLEX(A_S_FLEX* destination, const A_S_FLEX* source)
{
    int result;
    destination->n = source->n;
    destination->s = malloc(strlen(source->s) + 1);

    if (destination->s == NULL)
    {
        result = MU_FAILURE;
    }
    else
    {
        (void)memcpy(destination->s, source->s, strlen(source->s) + 1);
        for (int i = 0; i < source->n; i++)
        {
            destination->p[i] = source->p[i];
        }
        result = 0;
    }

    return result;
}

static void dispose_A_S_FLEX(A_S_FLEX* a_s)
{
    free(a_s->s);
}

static size_t get_sizeof_A_S_FLEX(const A_S_FLEX* source)
{
    return sizeof(A_S_FLEX) + source->n * sizeof(int);
}


    THANDLE_TYPE_DECLARE(A_S_FLEX);
    THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(A_S_FLEX, gballoc_hl_malloc, gballoc_hl_malloc_flex, gballoc_hl_free);



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
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleans)
{
}

/* THANDLE_MALLOC * /

/*Tests_SRS_THANDLE_02_043: [ THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall allocate memory. ]*/
TEST_FUNCTION(thandle_user_create_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/

    ///act
    THANDLE(LL) ll = ll_create(TEST_A, TEST_S);

    ///assert
    ASSERT_IS_NOT_NULL(ll);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(LL)(&ll, NULL);
}

/*Tests_SRS_THANDLE_02_043: [ THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall allocate memory. ]*/
/*Tests_SRS_THANDLE_02_045: [ If allocating memory fails then THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/
TEST_FUNCTION(thandle_user_create_fails_when_thandle_malloc_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL); /*this is THANDLE_MALLOC*/

    ///act
    THANDLE(LL) ll = ll_create(TEST_A, TEST_S);

    ///assert
    ASSERT_IS_NULL(ll);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/* THANDLE_MALLOC_WITH_EXTRA_SIZE */

/*Tests_SRS_THANDLE_02_050: [ THANDLE_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS shall allocate memory. ]*/
/*Tests_SRS_THANDLE_02_051: [ THANDLE_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS shall initialize the reference count to 1, store dispose and free_function succeed and return a non-NULL T*. ]*/
TEST_FUNCTION(thandle_flex_user_create_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/

    ///act
    THANDLE(LL_FLEX) ll = ll_flex_create(TEST_A, TEST_S, 10);

    ///assert
    ASSERT_IS_NOT_NULL(ll);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(LL_FLEX)(&ll, NULL);
}

/*Tests_SRS_THANDLE_02_052: [ If allocating memory fails then THANDLE_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/
TEST_FUNCTION(thandle_flex_user_create_fails_when_thandle_malloc_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(NULL); /*this is THANDLE_MALLOC_WITH_EXTRA_SIZE*/

    ///act
    THANDLE(LL_FLEX) ll = ll_flex_create(TEST_A, TEST_S, 10);

    ///assert
    ASSERT_IS_NULL(ll);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
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
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
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
    THANDLE_ASSIGN(LL)(&t1, NULL);
    THANDLE_ASSIGN(LL)(&t2, NULL);
}

/*Tests_SRS_THANDLE_02_009: [ If *t1 is not NULL and t2 is NULL then THANDLE_ASSIGN shall decrement the reference count of *t1 and store NULL in *t1. ]*/
TEST_FUNCTION(THANDLE_ASSIGN_with_t1_not_NULL_t2_NULL)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
    THANDLE(LL) t1 = ll_create(TEST_A, TEST_S);
    ASSERT_IS_NOT_NULL(t1);

    THANDLE(LL) t2 = NULL;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*this is the copy of s*/
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*this is THANDLE_MALLOC's memory that gets freed*/

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
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
    THANDLE(LL) t1 = ll_create(TEST_A, TEST_S);
    ASSERT_IS_NOT_NULL(t1);

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(malloc(sizeof(TEST_S2_DEFINE))); /*this is the copy of s2*/
    THANDLE(LL) t2 = ll_create(TEST_A, TEST_S2);
    ASSERT_IS_NOT_NULL(t2);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*this is the copy of s that goes away*/
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*this is THANDLE_MALLOC's memory that gets freed*/

    ///act
    THANDLE_ASSIGN(LL)(&t1, t2);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(t1 == t2);

    ///cleanup
    THANDLE_ASSIGN(LL)(&t1, NULL);
    THANDLE_ASSIGN(LL)(&t2, NULL);
}

/* THANDLE_INITIALIZE */

/*Tests_SRS_THANDLE_02_011: [ If lvalue is NULL then THANDLE_INITIALIZE shall return. ]*/
TEST_FUNCTION(THANDLE_INITIALIZE_with_lvalue_NULL_returns)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
    THANDLE(LL) t2 = ll_create(TEST_A, TEST_S);
    ASSERT_IS_NOT_NULL(t2);
    umock_c_reset_all_calls();

    ///act
    THANDLE_INITIALIZE(LL)(NULL, t2);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(LL)(&t2, NULL);
}

/*Tests_SRS_THANDLE_02_018: [ If rvalue is NULL then THANDLE_INITIALIZE shall store NULL in *lvalue. ]*/
TEST_FUNCTION(THANDLE_INITIALIZE_with_rvalue_NULL_succeeds)
{
    ///arrange
    MU_SUPPRESS_WARNING(4197) // The cast would be perfectly fine for C, but teh C++ compiler will meow
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
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
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
    THANDLE_ASSIGN(LL)(&t1, NULL);
    THANDLE_ASSIGN(LL)(&t2, NULL);
}

/* THANDLE_FREE */

/*Tests_SRS_THANDLE_02_017: [ THANDLE_FREE shall free the allocated memory by THANDLE_MALLOC. ]*/
TEST_FUNCTION(THANDLE_FREE_frees_memory)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
    THANDLE(LL) t1 = ll_create(TEST_A, TEST_S);
    ASSERT_IS_NOT_NULL(t1);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*this is the copy of s that goes away*/
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*this is THANDLE_MALLOC's memory that gets freed*/
    ///act

    THANDLE_ASSIGN(LL)(&t1, NULL);

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
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(malloc(sizeof(TEST_S_DEFINE))); /*this is the copy of s*/
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
    THANDLE_ASSIGN(LL)(&ll, NULL);
}

/*returns a pointer to an array of 2 THANDLE(LL)*/
static void builds_out_arg(THANDLE(LL)** x)
{
    *x = my_gballoc_malloc(sizeof(THANDLE(LL)) * 2);
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
    THANDLE_ASSIGN(LL)(&temp, NULL);

    THANDLE(LL) temp2 = ll_create(2, "44");
    ASSERT_IS_NOT_NULL(temp2);
    THANDLE_INITIALIZE(LL)(&arr[1], temp2);
    THANDLE_ASSIGN(LL)(&temp2, NULL);

    THANDLE_ASSIGN(LL)(&arr[0], NULL);
    THANDLE_ASSIGN(LL)(&arr[1], NULL);

    my_gballoc_free((void*)arr);
}

/*Tests_SRS_THANDLE_02_053: [ If source is NULL then THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/
/*Tests_SRS_THANDLE_02_032: [ THANDLE_CREATE_FROM_CONTENT returns what THANDLE_CREATE_FROM_CONTENT_FLEX(T)(source, dispose, copy, THANDLE_GET_SIZEOF(T)); returns. ]*/
TEST_FUNCTION(THANDLE_CREATE_FROM_CONTENT_FLEX_with_source_NULL_fails)
{
    ///arrange

    ///act
    THANDLE(A_B) result = THANDLE_CREATE_FROM_CONTENT(A_B)(NULL, NULL, NULL);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_THANDLE_02_060: [ THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall allocate memory. ]*/
/*Tests_SRS_THANDLE_02_055: [ If copy is NULL then THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall memcpy the content of source in allocated memory. ]*/
/*Tests_SRS_THANDLE_02_062: [ THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall initialize the ref count to 1, succeed and return a non-NULL value. ]*/
/*Tests_SRS_THANDLE_02_032: [ THANDLE_CREATE_FROM_CONTENT returns what THANDLE_CREATE_FROM_CONTENT_FLEX(T)(source, dispose, copy, THANDLE_GET_SIZEOF(T)); returns. ]*/
TEST_FUNCTION(THANDLE_CREATE_FROM_CONTENT_FLEX_with_copy_NULL_succeeds)
{
    ///arrange
    A_B a_b;
    a_b.a = 2;
    a_b.b = 3;

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, IGNORED_ARG)); /*this is THANDLE_MALLOC*/

    ///act
    THANDLE(A_B) result = THANDLE_CREATE_FROM_CONTENT(A_B)(&a_b, NULL, NULL);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(int, a_b.a, result->a);
    ASSERT_ARE_EQUAL(int, a_b.b, result->b);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(A_B)(&result, NULL);
}

/*Tests_SRS_THANDLE_02_060: [ THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall allocate memory. ]*/
/*Tests_SRS_THANDLE_02_061: [ If copy is not NULL then THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall call copy to copy source into allocated memory. ]*/
/*Tests_SRS_THANDLE_02_062: [ THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall initialize the ref count to 1, succeed and return a non-NULL value. ]*/
/*Tests_SRS_THANDLE_02_032: [ THANDLE_CREATE_FROM_CONTENT returns what THANDLE_CREATE_FROM_CONTENT_FLEX(T)(source, dispose, copy, THANDLE_GET_SIZEOF(T)); returns. ]*/
TEST_FUNCTION(THANDLE_CREATE_FROM_CONTENT_FLEX_DISPOSE_with_non_NULL_succeeds)
{
    ///arrange
    char copy[] = "HELLOWORLD";
    A_S a_s;
    a_s.a = 22;
    a_s.s = copy;

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*this is sprintf_char in copy_A_S, also known as "copy"*/

    ///act
    THANDLE(A_S) result = THANDLE_CREATE_FROM_CONTENT(A_S)(&a_s, dispose_A_S, copy_A_S);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(int, a_s.a, result->a);
    ASSERT_ARE_EQUAL(char_ptr, a_s.s, result->s);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(A_S)(&result, NULL);
}

/*Tests_SRS_THANDLE_02_063: [ If there are any failures then THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/
/*Tests_SRS_THANDLE_02_032: [ THANDLE_CREATE_FROM_CONTENT returns what THANDLE_CREATE_FROM_CONTENT_FLEX(T)(source, dispose, copy, THANDLE_GET_SIZEOF(T)); returns. ]*/
TEST_FUNCTION(THANDLE_CREATE_FROM_CONTENT_FLEX_when_THANDLE_MALLOC_FUNCTION_fails_it_fails)
{
    ///arrange
    char copy[] = "HELLOWORLD";
    A_S a_s;
    a_s.a = 22;
    a_s.s = copy;

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, IGNORED_ARG)) /*this is THANDLE_MALLOC*/
        .SetReturn(NULL);

    ///act
    THANDLE(A_S) result = THANDLE_CREATE_FROM_CONTENT(A_S)(&a_s, dispose_A_S, copy_A_S);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_THANDLE_02_063: [ If there are any failures then THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/
/*Tests_SRS_THANDLE_02_032: [ THANDLE_CREATE_FROM_CONTENT returns what THANDLE_CREATE_FROM_CONTENT_FLEX(T)(source, dispose, copy, THANDLE_GET_SIZEOF(T)); returns. ]*/
TEST_FUNCTION(THANDLE_CREATE_FROM_CONTENT_FLEX_when_copy_fails_it_fails)
{
    ///arrange
    char copy[] = "HELLOWORLD";
    A_S a_s;
    a_s.a = 22;
    a_s.s = copy;

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)) /*this is malloc in copy_A_S, aslo known as "copy"*/
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*this is THANDLE_MALLOC*/

    ///act
    THANDLE(A_S) result = THANDLE_CREATE_FROM_CONTENT(A_S)(&a_s, dispose_A_S, copy_A_S);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_THANDLE_02_064: [ THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall call get_sizeof to get the needed size to store T ]*/
TEST_FUNCTION(THANDLE_CREATE_FROM_CONTENT_FLEX_calls_get_sizeof)
{
    ///arrange
    int n = 4; /*number of elements in the flexible array*/
    A_FLEX* source = my_gballoc_malloc(sizeof(A_FLEX) + n * sizeof(int));
    ASSERT_IS_NOT_NULL(source);

    source->n = 4;
    for (int i = 0; i < n; i++)
    {
        source->p[i] = i * i;
    }

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, IGNORED_ARG)); /*this is THANDLE_MALLOC*/

    ///act
    THANDLE(A_FLEX) copy = THANDLE_CREATE_FROM_CONTENT_FLEX(A_FLEX)(source, NULL, NULL, get_sizeof_A_FLEX);

    ///assert
    ASSERT_ARE_EQUAL(int, source->n, copy->n);
    for (int i = 0; i < n; i++)
    {
        ASSERT_ARE_EQUAL(int, source->p[i], copy->p[i]);
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    my_gballoc_free(source);
    THANDLE_ASSIGN(A_FLEX)(&copy, NULL);
}

/*Tests_SRS_THANDLE_02_064: [ THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall call get_sizeof to get the needed size to store T ]*/
/*Tests_SRS_THANDLE_02_061: [ If copy is not NULL then THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall call copy to copy source into allocated memory. ]*/
TEST_FUNCTION(THANDLE_CREATE_FROM_CONTENT_FLEX_calls_get_sizeof_2)
{
    ///arrange
    int n = 4; /*number of elements in the flexible array*/
    A_S_FLEX* source = my_gballoc_malloc(sizeof(A_S_FLEX) + n * sizeof(int));
    ASSERT_IS_NOT_NULL(source);

    source->s = "abc";

    source->n = 4;
    for (int i = 0; i < n; i++)
    {
        source->p[i] = i * i;
    }

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, IGNORED_ARG)); /*this is THANDLE_MALLOC*/
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); /*this is malloc for the string*/

    ///act
    THANDLE(A_S_FLEX) copy = THANDLE_CREATE_FROM_CONTENT_FLEX(A_S_FLEX)(source, dispose_A_S_FLEX, copy_A_S_FLEX, get_sizeof_A_S_FLEX);

    ///assert
    ASSERT_IS_NOT_NULL(copy);
    ASSERT_ARE_EQUAL(int, source->n, copy->n);
    for (int i = 0; i < n; i++)
    {
        ASSERT_ARE_EQUAL(int, source->p[i], copy->p[i]);
    }
    ASSERT_ARE_EQUAL(char_ptr, source->s, copy->s);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    my_gballoc_free(source);
    THANDLE_ASSIGN(A_S_FLEX)(&copy, NULL);
}

/*Tests_SRS_THANDLE_02_033: [ If t1 is NULL then THANDLE_MOVE shall return. ]*/
TEST_FUNCTION(THANDLE_MOVE_with_t1_NULL_returns)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    THANDLE(LL) ll2 = ll_create(2, "2");
    ASSERT_IS_NOT_NULL(ll2);
    umock_c_reset_all_calls();

    ///act
    THANDLE_MOVE(LL)(NULL, &ll2);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(LL)(&ll2, NULL);
}

/*Tests_SRS_THANDLE_02_034: [ If t2 is NULL then THANDLE_MOVE shall return. ]*/
TEST_FUNCTION(THANDLE_MOVE_with_t2_NULL_returns)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    THANDLE(LL) ll1 = ll_create(1, "1");
    ASSERT_IS_NOT_NULL(ll1);
    umock_c_reset_all_calls();

    ///act
    THANDLE_MOVE(LL)(&ll1, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(LL)(&ll1, NULL);
}

/*Tests_SRS_THANDLE_02_035: [ If *t1 is NULL and *t2 is NULL then THANDLE_MOVE shall return. ]*/
TEST_FUNCTION(THANDLE_MOVE_with_star_t1_NULL_and_star_t2_NULL_returns)
{
    ///arrange
    THANDLE(LL) ll1 = NULL;
    THANDLE(LL) ll2 = NULL;

    ///act
    THANDLE_MOVE(LL)(&ll1, &ll2);

    ///assert
    ASSERT_IS_NULL(ll1);
    ASSERT_IS_NULL(ll2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_THANDLE_02_036: [ If *t1 is NULL and *t2 is not NULL then THANDLE_MOVE shall move *t2 under t1, set *t2 to NULL and return. ]*/
TEST_FUNCTION(THANDLE_MOVE_with_star_t1_NULL_and_star_t2_not_NULL)
{
    ///arrange
    THANDLE(LL) ll1 = NULL;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    THANDLE(LL) ll2 = ll_create(2, "2");
    ASSERT_IS_NOT_NULL(ll2);
    umock_c_reset_all_calls();

    ///act
    THANDLE_MOVE(LL)(&ll1, &ll2);

    ///assert
    ASSERT_IS_NOT_NULL(ll1);
    ASSERT_IS_NULL(ll2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(LL)(&ll1, NULL);
}

/*Tests_SRS_THANDLE_02_037: [ If *t1 is not NULL and *t2 is NULL then THANDLE_MOVE shall THANDLE_DEC_REF *t1, set *t1 to NULL and return. ]*/
TEST_FUNCTION(THANDLE_MOVE_with_star_t1_not_NULL_and_star_t2_NULL)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    THANDLE(LL) ll1 = ll_create(1, "1");
    ASSERT_IS_NOT_NULL(ll1);

    THANDLE(LL) ll2 = NULL;

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*ll1 goes poof!*/
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /**/

    ///act
    THANDLE_MOVE(LL)(&ll1, &ll2);

    ///assert
    ASSERT_IS_NULL(ll1);
    ASSERT_IS_NULL(ll2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_THANDLE_02_038: [ If *t1 is not NULL and *t2 is not NULL then THANDLE_MOVE shall THANDLE_DEC_REF *t1, set *t1 to *t2, set *t2 to NULL and return. ]*/
TEST_FUNCTION(THANDLE_MOVE_with_star_t1_not_NULL_and_star_t2_not_NULL)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    THANDLE(LL) ll1 = ll_create(1, "1");
    ASSERT_IS_NOT_NULL(ll1);

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    THANDLE(LL) ll2 = ll_create(2, "2");
    ASSERT_IS_NOT_NULL(ll2);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*ll1 string*/
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*ll1 goes poof!*/

    ///act
    THANDLE_MOVE(LL)(&ll1, &ll2);

    ///assert
    ASSERT_IS_NOT_NULL(ll1);
    ASSERT_IS_NULL(ll2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(LL)(&ll1, NULL);
}

/* THANDLE_INITIALIZE_MOVE_MACRO */

/*Tests_SRS_THANDLE_01_001: [ If t1 is NULL then THANDLE_INITIALIZE_MOVE shall return. ]*/
TEST_FUNCTION(THANDLE_INITIALIZE_MOVE_with_t1_NULL_returns)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    THANDLE(LL) ll2 = ll_create(2, "2");
    ASSERT_IS_NOT_NULL(ll2);
    umock_c_reset_all_calls();

    ///act
    THANDLE_INITIALIZE_MOVE(LL)(NULL, &ll2);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(LL)(&ll2, NULL);
}

/*Tests_SRS_THANDLE_01_002: [ If t2 is NULL then THANDLE_INITIALIZE_MOVE shall return. ]*/
TEST_FUNCTION(THANDLE_INITIALIZE_MOVE_with_t2_NULL_returns)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    THANDLE(LL) ll1 = ll_create(1, "1");
    ASSERT_IS_NOT_NULL(ll1);
    umock_c_reset_all_calls();

    ///act
    THANDLE_INITIALIZE_MOVE(LL)(&ll1, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(LL)(&ll1, NULL);
}

/*Tests_SRS_THANDLE_01_003: [ If *t2 is NULL then THANDLE_INITIALIZE_MOVE shall set *t1 to NULL and return. ]*/
TEST_FUNCTION(THANDLE_INITIALIZE_MOVE_with_star_t1_NULL_and_star_t2_NULL_returns)
{
    ///arrange
    THANDLE(LL) ll1 = NULL;
    THANDLE(LL) ll2 = NULL;

    ///act
    THANDLE_INITIALIZE_MOVE(LL)(&ll1, &ll2);

    ///assert
    ASSERT_IS_NULL(ll1);
    ASSERT_IS_NULL(ll2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_THANDLE_01_004: [ If *t2 is not NULL then THANDLE_INITIALIZE_MOVE shall set *t1 to *t2, set *t2 to NULL and return. ]*/
TEST_FUNCTION(THANDLE_INITIALIZE_MOVE_with_star_t1_NULL_and_star_t2_not_NULL)
{
    ///arrange
    THANDLE(LL) ll1 = NULL;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    THANDLE(LL) ll2 = ll_create(2, "2");
    ASSERT_IS_NOT_NULL(ll2);
    umock_c_reset_all_calls();

    ///act
    THANDLE_INITIALIZE_MOVE(LL)(&ll1, &ll2);

    ///assert
    ASSERT_IS_NOT_NULL(ll1);
    ASSERT_IS_NULL(ll2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(LL)(&ll1, NULL);
}

/*Tests_SRS_THANDLE_01_003: [ If *t2 is NULL then THANDLE_INITIALIZE_MOVE shall set *t1 to NULL and return. ]*/
TEST_FUNCTION(THANDLE_INITIALIZE_MOVE_with_star_t1_not_NULL_and_star_t2_NULL)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    THANDLE(LL) ll1 = (LL*)0x4242; /* some dummy value */
    ASSERT_IS_NOT_NULL(ll1);

    THANDLE(LL) ll2 = NULL;

    umock_c_reset_all_calls();

    ///act
    THANDLE_INITIALIZE_MOVE(LL)(&ll1, &ll2);

    ///assert
    ASSERT_IS_NULL(ll1);
    ASSERT_IS_NULL(ll2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* Tests_SRS_THANDLE_01_004: [ If *t2 is not NULL then THANDLE_INITIALIZE_MOVE shall set *t1 to *t2, set *t2 to NULL and return. ]*/
TEST_FUNCTION(THANDLE_INITIALIZE_MOVE_with_star_t1_not_NULL_and_star_t2_not_NULL)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    THANDLE(LL) ll1 = (LL*)0x4242; // some dummy value
    ASSERT_IS_NOT_NULL(ll1);

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    THANDLE(LL) ll2 = ll_create(2, "2");
    ASSERT_IS_NOT_NULL(ll2);
    umock_c_reset_all_calls();

    ///act
    THANDLE_INITIALIZE_MOVE(LL)(&ll1, &ll2);

    ///assert
    ASSERT_IS_NOT_NULL(ll1);
    ASSERT_IS_NULL(ll2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(LL)(&ll1, NULL);
}

#if defined(_DEBUG) || defined (DEBUG)
TEST_FUNCTION(THANDLE_can_be_build_from_a_33_character_type)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    THANDLE(LL3456789012345678901234567890123) ll1 = ll33_create(2);
    ASSERT_IS_NOT_NULL(ll1);
    umock_c_reset_all_calls();

    ///act
    const char* name = ll33_get_name(ll1);

    ///assert
    /* the type is              LL3456789012345678901234567890123                                                                                                                          */
    ASSERT_ARE_EQUAL(char_ptr, "LL34567890123456789012345678901", name); /*note how the name comes truncated due to only 32 characters existing in name. Of which 1 is '\0', the last one  */

    ///clean
    THANDLE_ASSIGN(LL3456789012345678901234567890123)(&ll1, NULL);
}
#endif



END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

