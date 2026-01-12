// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "paged_sparse_array_ut_pch.h"

/*PAGED_SPARSE_ARRAY with regular types*/
PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(uint32_t)

THANDLE_TYPE_DECLARE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(uint32_t));
THANDLE_TYPE_DEFINE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(uint32_t));

PAGED_SPARSE_ARRAY_TYPE_DECLARE(uint32_t);
PAGED_SPARSE_ARRAY_TYPE_DEFINE(uint32_t);

typedef struct B_TEST_TAG
{
    int b;
} B_TEST;

PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(B_TEST)
THANDLE_TYPE_DECLARE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(B_TEST));
THANDLE_TYPE_DEFINE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(B_TEST));

PAGED_SPARSE_ARRAY_TYPE_DECLARE(B_TEST);
PAGED_SPARSE_ARRAY_TYPE_DEFINE(B_TEST);

typedef struct A_TEST_TAG
{
    int a;
} A_TEST;

THANDLE_TYPE_DECLARE(A_TEST);
THANDLE_TYPE_DEFINE(A_TEST);

PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(THANDLE(A_TEST))
THANDLE_TYPE_DECLARE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(THANDLE(A_TEST)));
THANDLE_TYPE_DEFINE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(THANDLE(A_TEST)));

/*PAGED_SPARSE_ARRAY works with THANDLEs too*/
PAGED_SPARSE_ARRAY_TYPE_DECLARE(THANDLE(A_TEST));
PAGED_SPARSE_ARRAY_TYPE_DEFINE(THANDLE(A_TEST));

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

/*PAGED_SPARSE_ARRAY_CREATE(T)*/

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_003: [ PAGED_SPARSE_ARRAY_CREATE(T) shall compute the number of pages as (max_size + page_size - 1) / page_size. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_004: [ PAGED_SPARSE_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX to allocate memory for the paged sparse array with the number of pages. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_005: [ PAGED_SPARSE_ARRAY_CREATE(T) shall set all page pointers to NULL. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_006: [ PAGED_SPARSE_ARRAY_CREATE(T) shall store max_size and page_size in the structure. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_008: [ PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_CREATE_with_uint32_t_type_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 2, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)*)));

    //act
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 64);

    //assert
    ASSERT_IS_NOT_NULL(psa);
    ASSERT_ARE_EQUAL(uint32_t, 100, psa->max_size);
    ASSERT_ARE_EQUAL(uint32_t, 64, psa->page_size);
    ASSERT_ARE_EQUAL(uint32_t, 2, psa->page_count);
    for (uint32_t i = 0; i < psa->page_count; i++)
    {
        ASSERT_IS_NULL(psa->pages[i]);
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_003: [ PAGED_SPARSE_ARRAY_CREATE(T) shall compute the number of pages as (max_size + page_size - 1) / page_size. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_008: [ PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_CREATE_computes_page_count_correctly)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)*)));

    //act
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(10, 64);

    //assert
    ASSERT_IS_NOT_NULL(psa);
    ASSERT_ARE_EQUAL(uint32_t, 1, psa->page_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_001: [ If max_size is zero, PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_CREATE_fails_when_max_size_is_zero)
{
    //arrange

    //act
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(0, 64);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_002: [ If page_size is zero, PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_CREATE_fails_when_page_size_is_zero)
{
    //arrange

    //act
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 0);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_040: [ If max_size + page_size - 1 would overflow, PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_CREATE_fails_when_max_size_plus_page_size_would_overflow)
{
    //arrange

    //act
    // Use max_size=UINT32_MAX and page_size=2, so max_size + page_size - 1 = UINT32_MAX + 1 would overflow
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(UINT32_MAX, 2);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_007: [ If there are any errors, PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_CREATE_fails_when_malloc_flex_fails)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 2, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)*)))
        .SetReturn(NULL);

    //act
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 64);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_008: [ PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_CREATE_with_struct_type_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(B_TEST)*)));

    //act
    PAGED_SPARSE_ARRAY(B_TEST) psa = PAGED_SPARSE_ARRAY_CREATE(B_TEST)(10, 16);

    //assert
    ASSERT_IS_NOT_NULL(psa);
    ASSERT_ARE_EQUAL(uint32_t, 10, psa->max_size);
    ASSERT_ARE_EQUAL(uint32_t, 16, psa->page_size);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(B_TEST)(&psa, NULL);
}

/*PAGED_SPARSE_ARRAY_DISPOSE(T)*/

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_009: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_DISPOSE(T) shall return. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_DISPOSE_with_null_returns)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = NULL;

    //act - assigning NULL to a NULL handle should not crash
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);

    //assert
    // No crash means success - the FREE function handles NULL gracefully
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_010: [ PAGED_SPARSE_ARRAY_DISPOSE(T) shall free all pages that are non-NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_DISPOSE_frees_all_allocated_pages)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);

    // Allocate elements in multiple pages
    uint32_t* ptr0 = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 0);   // Page 0
    uint32_t* ptr20 = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 20); // Page 1
    uint32_t* ptr40 = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 40); // Page 2
    ASSERT_IS_NOT_NULL(ptr0);
    ASSERT_IS_NOT_NULL(ptr20);
    ASSERT_IS_NOT_NULL(ptr40);
    ASSERT_IS_NOT_NULL(psa->pages[0]);
    ASSERT_IS_NOT_NULL(psa->pages[1]);
    ASSERT_IS_NOT_NULL(psa->pages[2]);

    umock_c_reset_all_calls();

    // Expect free for each page (3 pages x 1 free each = 3 frees)
    // Plus free for the THANDLE wrapper
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // page 0
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // page 1
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // page 2
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // THANDLE wrapper

    //act
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/*PAGED_SPARSE_ARRAY_ALLOCATE(T)*/

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_013: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall compute the page index as index / page_size. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_014: [ If the page is not allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall allocate memory for the page containing page_size elements and an allocation bitmap, and initialize all elements as not allocated. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_016: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall mark the element at index as allocated. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_017: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall return a pointer to the element at index. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_succeeds_first_element)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, 1)); // page with embedded bitmap

    //act
    uint32_t* ptr = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 0);

    //assert
    ASSERT_IS_NOT_NULL(ptr);
    ASSERT_IS_NOT_NULL(psa->pages[0]);
    ASSERT_ARE_EQUAL(uint32_t, 1, psa->pages[0]->allocated_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_014: [ If the page is not allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall allocate memory for the page containing page_size elements and an allocation bitmap, and initialize all elements as not allocated. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_017: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall return a pointer to the element at index. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_succeeds_second_page)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, 1)); // page with embedded bitmap

    //act
    uint32_t* ptr = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 20);

    //assert
    ASSERT_IS_NOT_NULL(ptr);
    ASSERT_IS_NULL(psa->pages[0]); // First page should not be allocated
    ASSERT_IS_NOT_NULL(psa->pages[1]); // Second page should be allocated
    ASSERT_ARE_EQUAL(uint32_t, 1, psa->pages[1]->allocated_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_017: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall return a pointer to the element at index. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_multiple_elements_in_same_page)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, 1)); // page with embedded bitmap

    //act
    uint32_t* ptr0 = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 0);
    uint32_t* ptr5 = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5);
    uint32_t* ptr10 = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 10);

    //assert
    ASSERT_IS_NOT_NULL(ptr0);
    ASSERT_IS_NOT_NULL(ptr5);
    ASSERT_IS_NOT_NULL(ptr10);
    ASSERT_ARE_EQUAL(uint32_t, 3, psa->pages[0]->allocated_count);
    ASSERT_ARE_NOT_EQUAL(void_ptr, ptr0, ptr5);
    ASSERT_ARE_NOT_EQUAL(void_ptr, ptr5, ptr10);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_011: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_fails_when_handle_is_null)
{
    //arrange

    //act
    uint32_t* ptr = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(NULL, 0);

    //assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_012: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_fails_when_index_equals_max_size)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();

    //act
    uint32_t* ptr = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 100);

    //assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_012: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_fails_when_index_greater_than_max_size)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();

    //act
    uint32_t* ptr = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 150);

    //assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_015: [ If the element at index is already allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_fails_when_already_allocated)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    uint32_t* ptr1 = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5);
    ASSERT_IS_NOT_NULL(ptr1);
    umock_c_reset_all_calls();

    //act
    uint32_t* ptr2 = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5);

    //assert
    ASSERT_IS_NULL(ptr2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_018: [ If there are any errors, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_fails_when_page_malloc_fails)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, 1)) // page with embedded bitmap
        .SetReturn(NULL);

    //act
    uint32_t* ptr = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 0);

    //assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*PAGED_SPARSE_ARRAY_RELEASE(T)*/

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_021: [ PAGED_SPARSE_ARRAY_RELEASE(T) shall compute the page index as index / page_size. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_024: [ PAGED_SPARSE_ARRAY_RELEASE(T) shall mark the element at index as not allocated. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_025: [ If all elements in the page are now not allocated, PAGED_SPARSE_ARRAY_RELEASE(T) shall free the page and set the page pointer to NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_succeeds_and_frees_page_when_last_element)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    uint32_t* ptr = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5);
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // page (includes embedded bitmap)

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, 5);

    //assert
    ASSERT_IS_NULL(psa->pages[0]);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_024: [ PAGED_SPARSE_ARRAY_RELEASE(T) shall mark the element at index as not allocated. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_succeeds_but_keeps_page_when_other_elements_allocated)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    uint32_t* ptr1 = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5);
    uint32_t* ptr2 = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 10);
    ASSERT_IS_NOT_NULL(ptr1);
    ASSERT_IS_NOT_NULL(ptr2);
    ASSERT_ARE_EQUAL(uint32_t, 2, psa->pages[0]->allocated_count);
    umock_c_reset_all_calls();

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, 5);

    //assert
    ASSERT_IS_NOT_NULL(psa->pages[0]); // Page should still be allocated
    ASSERT_ARE_EQUAL(uint32_t, 1, psa->pages[0]->allocated_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_019: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_with_null_handle_returns)
{
    //arrange

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(NULL, 0);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_020: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_with_index_out_of_bounds_returns)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, 100);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_022: [ If the page is not allocated, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_with_page_not_allocated_returns)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, 5);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_023: [ If the element at index is not allocated, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_with_element_not_allocated_returns)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    uint32_t* ptr = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5);
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, 10); // Different element in same page

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*PAGED_SPARSE_ARRAY_GET(T)*/

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_036: [ PAGED_SPARSE_ARRAY_GET(T) shall compute the page index as index / page_size. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_039: [ PAGED_SPARSE_ARRAY_GET(T) shall return a pointer to the element at index. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_GET_succeeds)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    uint32_t* ptr1 = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5);
    ASSERT_IS_NOT_NULL(ptr1);
    *ptr1 = 42;
    umock_c_reset_all_calls();

    //act
    uint32_t* ptr2 = PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, 5);

    //assert
    ASSERT_IS_NOT_NULL(ptr2);
    ASSERT_ARE_EQUAL(void_ptr, ptr1, ptr2);
    ASSERT_ARE_EQUAL(uint32_t, 42, *ptr2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_034: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_GET(T) shall fail and return NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_GET_fails_when_handle_is_null)
{
    //arrange

    //act
    uint32_t* ptr = PAGED_SPARSE_ARRAY_GET(uint32_t)(NULL, 0);

    //assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_035: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_GET(T) shall fail and return NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_GET_fails_when_index_out_of_bounds)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();

    //act
    uint32_t* ptr = PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, 100);

    //assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_037: [ If the page is not allocated, PAGED_SPARSE_ARRAY_GET(T) shall fail and return NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_GET_fails_when_page_not_allocated)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();

    //act
    uint32_t* ptr = PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, 5);

    //assert
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_038: [ If the element at index is not allocated, PAGED_SPARSE_ARRAY_GET(T) shall fail and return NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_GET_fails_when_element_not_allocated)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16);
    ASSERT_IS_NOT_NULL(psa);
    uint32_t* ptr = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5);
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();

    //act
    uint32_t* result = PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, 10); // Different element in same page

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* PAGED_SPARSE_ARRAY_CREATE */

/*Tests_SRS_PAGED_SPARSE_ARRAY_88_008: [ PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
/*Tests_SRS_PAGED_SPARSE_ARRAY_88_017: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall return a pointer to the element at index.]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_with_THANDLE_type_works)
{
    //arrange
    PAGED_SPARSE_ARRAY(THANDLE(A_TEST)) psa = PAGED_SPARSE_ARRAY_CREATE(THANDLE(A_TEST))(50, 10);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();

    //act
    THANDLE(A_TEST)* ptr = PAGED_SPARSE_ARRAY_ALLOCATE(THANDLE(A_TEST))(psa, 0);

    //assert
    ASSERT_IS_NOT_NULL(ptr);

    // Can use THANDLE operations on the stored value
    THANDLE(A_TEST) a = THANDLE_MALLOC(A_TEST)(NULL);
    ASSERT_IS_NOT_NULL(a);

    THANDLE_INITIALIZE(A_TEST)(ptr, a);
    THANDLE_ASSIGN(A_TEST)(ptr, NULL);
    THANDLE_ASSIGN(A_TEST)(&a, NULL);

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(THANDLE(A_TEST))(&psa, NULL);
}

/*Tests_SRS_PAGED_SPARSE_ARRAY_88_008: [ PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
/*Tests_SRS_PAGED_SPARSE_ARRAY_88_017: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall return a pointer to the element at index.]*/
/*Tests_SRS_PAGED_SPARSE_ARRAY_88_034: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_GET(T) shall fail and return NULL.]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_with_struct_type_works)
{
    //arrange
    PAGED_SPARSE_ARRAY(B_TEST) psa = PAGED_SPARSE_ARRAY_CREATE(B_TEST)(50, 10);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();

    //act
    B_TEST* ptr = PAGED_SPARSE_ARRAY_ALLOCATE(B_TEST)(psa, 0);

    //assert
    ASSERT_IS_NOT_NULL(ptr);
    ptr->b = 42;
    
    B_TEST* ptr_get = PAGED_SPARSE_ARRAY_GET(B_TEST)(psa, 0);
    ASSERT_IS_NOT_NULL(ptr_get);
    ASSERT_ARE_EQUAL(int, 42, ptr_get->b);

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(B_TEST)(&psa, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
