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

TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT_VALUES)

#define TEST_MAX_SIZE 100
#define TEST_PAGE_SIZE 16
#define TEST_PAGE_COUNT 7 /*(TEST_MAX_SIZE + TEST_PAGE_SIZE - 1) / TEST_PAGE_SIZE*/
#define TEST_INDEX_IN_FIRST_PAGE 5
#define TEST_OTHER_INDEX_IN_FIRST_PAGE 10
#define TEST_ITEM_VALUE 42
#define TEST_GARBAGE_BYTE 0xFF
#define TEST_GARBAGE_ITEM_VALUE UINT32_MAX

#define TEST_PSA_LOCK(psa) ((SRW_LOCK_LL*)&(psa)->lock)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static PAGED_SPARSE_ARRAY_OPTIONS test_make_options(bool thread_safe, bool auto_free_empty_pages, bool zero_new_pages)
{
    PAGED_SPARSE_ARRAY_OPTIONS options;
    options.thread_safe = thread_safe;
    options.auto_free_empty_pages = auto_free_empty_pages;
    options.zero_new_pages = zero_new_pages;
    return options;
}

static PAGED_SPARSE_ARRAY(uint32_t) test_create_with_options(bool thread_safe, bool auto_free_empty_pages, bool zero_new_pages, PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(uint32_t) item_dispose_func)
{
    PAGED_SPARSE_ARRAY_OPTIONS options = test_make_options(thread_safe, auto_free_empty_pages, zero_new_pages);
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, item_dispose_func, &options);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();
    return psa;
}

static uint32_t* test_allocate(PAGED_SPARSE_ARRAY(uint32_t) psa, uint64_t index)
{
    uint32_t* ptr = NULL;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, index, &ptr));
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();
    return ptr;
}

static bool test_is_allocated_in_bitmap(PAGED_SPARSE_ARRAY(uint32_t) psa, uint64_t index)
{
    PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)* page = psa->pages[index / TEST_PAGE_SIZE];
    ASSERT_IS_NOT_NULL(page);
    return PAGED_SPARSE_ARRAY_IS_ALLOCATED(PAGED_SPARSE_ARRAY_GET_BITMAP(page, TEST_PAGE_SIZE), index % TEST_PAGE_SIZE) != 0;
}

/*used by tests that simulate another thread calling into the array while the lock is not held*/
static struct
{
    const PAGED_SPARSE_ARRAY_TYPEDEF_NAME(uint32_t)* psa;
    uint64_t index_to_allocate;
    bool was_called;
} g_concurrent_allocate;

static void hook_srw_lock_ll_acquire_exclusive_allocates_concurrently(SRW_LOCK_LL* srw_lock_ll)
{
    if (g_concurrent_allocate.was_called)
    {
        /* do nothing, only one concurrent allocation is simulated */
    }
    else
    {
        uint32_t* other_ptr;
        g_concurrent_allocate.was_called = true;
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(g_concurrent_allocate.psa, g_concurrent_allocate.index_to_allocate, &other_ptr));
    }
    real_srw_lock_ll_acquire_exclusive(srw_lock_ll);
}

static void setup_concurrent_allocate_on_acquire_exclusive(PAGED_SPARSE_ARRAY(uint32_t) psa, uint64_t index_to_allocate)
{
    g_concurrent_allocate.psa = psa;
    g_concurrent_allocate.index_to_allocate = index_to_allocate;
    g_concurrent_allocate.was_called = false;
    REGISTER_GLOBAL_MOCK_HOOK(srw_lock_ll_acquire_exclusive, hook_srw_lock_ll_acquire_exclusive_allocates_concurrently);
}

static void cleanup_concurrent_allocate_on_acquire_exclusive(void)
{
    REGISTER_GLOBAL_MOCK_HOOK(srw_lock_ll_acquire_exclusive, real_srw_lock_ll_acquire_exclusive);
    g_concurrent_allocate.psa = NULL;
}


BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(it_does_something)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    umock_c_init(on_umock_c_error);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_SRW_LOCK_LL_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_flex, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(srw_lock_ll_init, MU_FAILURE);
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
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_006: [ PAGED_SPARSE_ARRAY_CREATE(T) shall store max_size, page_size, and item_dispose_func in the structure.]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_008: [ PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_CREATE_with_uint32_t_type_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 2, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)*)));

    //act
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 64, NULL);

    //assert
    ASSERT_IS_NOT_NULL(psa);
    ASSERT_ARE_EQUAL(uint64_t, 100, psa->max_size);
    ASSERT_ARE_EQUAL(uint64_t, 64, psa->page_size);
    ASSERT_ARE_EQUAL(uint64_t, 2, psa->page_count);
    for (uint64_t i = 0; i < psa->page_count; i++)
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
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(10, 64, NULL);

    //assert
    ASSERT_IS_NOT_NULL(psa);
    ASSERT_ARE_EQUAL(uint64_t, 1, psa->page_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_001: [ If max_size is zero, PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_CREATE_fails_when_max_size_is_zero)
{
    //arrange

    //act
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(0, 64, NULL);

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
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 0, NULL);

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
    // Use max_size=UINT64_MAX and page_size=2, so max_size + page_size - 1 = UINT64_MAX + 1 would overflow
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(UINT64_MAX, 2, NULL);

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
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 64, NULL);

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
    PAGED_SPARSE_ARRAY(B_TEST) psa = PAGED_SPARSE_ARRAY_CREATE(B_TEST)(10, 16, NULL);

    //assert
    ASSERT_IS_NOT_NULL(psa);
    ASSERT_ARE_EQUAL(uint64_t, 10, psa->max_size);
    ASSERT_ARE_EQUAL(uint64_t, 16, psa->page_size);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(B_TEST)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_046: [ PAGED_SPARSE_ARRAY_CREATE(T) shall create the paged sparse array with thread_safe set to false, auto_free_empty_pages set to true and zero_new_pages set to false. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_CREATE_uses_default_options_and_does_not_create_a_lock)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)*)));

    //act
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL);

    //assert
    ASSERT_IS_NOT_NULL(psa);
    ASSERT_IS_FALSE(psa->options.thread_safe);
    ASSERT_IS_TRUE(psa->options.auto_free_empty_pages);
    ASSERT_IS_FALSE(psa->options.zero_new_pages);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T)*/

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_047: [ If options is NULL, PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T) shall fail and return NULL. ]*/
TEST_FUNCTION(when_options_is_NULL_then_PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS_fails)
{
    //arrange

    //act
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_048: [ PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T) shall validate max_size and page_size, and allocate and initialize the paged sparse array, in the same way as PAGED_SPARSE_ARRAY_CREATE(T). ]*/
TEST_FUNCTION(when_max_size_is_zero_then_PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS_fails)
{
    //arrange
    PAGED_SPARSE_ARRAY_OPTIONS options = test_make_options(true, true, false);

    //act
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(uint32_t)(0, TEST_PAGE_SIZE, NULL, &options);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_048: [ PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T) shall validate max_size and page_size, and allocate and initialize the paged sparse array, in the same way as PAGED_SPARSE_ARRAY_CREATE(T). ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_049: [ PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T) shall store a copy of *options in the structure. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS_without_thread_safe_succeeds_and_does_not_create_a_lock)
{
    //arrange
    PAGED_SPARSE_ARRAY_OPTIONS options = test_make_options(false, false, true);
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)*)));

    //act
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL, &options);

    //assert
    ASSERT_IS_NOT_NULL(psa);
    ASSERT_ARE_EQUAL(uint64_t, TEST_MAX_SIZE, psa->max_size);
    ASSERT_ARE_EQUAL(uint64_t, TEST_PAGE_SIZE, psa->page_size);
    ASSERT_ARE_EQUAL(uint64_t, TEST_PAGE_COUNT, psa->page_count);
    for (uint64_t i = 0; i < psa->page_count; i++)
    {
        ASSERT_IS_NULL(psa->pages[i]);
    }
    ASSERT_IS_FALSE(psa->options.thread_safe);
    ASSERT_IS_FALSE(psa->options.auto_free_empty_pages);
    ASSERT_IS_TRUE(psa->options.zero_new_pages);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_049: [ PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T) shall store a copy of *options in the structure. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_050: [ If options->thread_safe is true, PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T) shall call srw_lock_ll_init to initialize the lock. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS_with_thread_safe_succeeds_and_initializes_the_lock)
{
    //arrange
    PAGED_SPARSE_ARRAY_OPTIONS options = test_make_options(true, true, false);
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)*)));
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG));

    //act
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL, &options);

    //assert
    ASSERT_IS_NOT_NULL(psa);
    ASSERT_IS_TRUE(psa->options.thread_safe);
    ASSERT_IS_TRUE(psa->options.auto_free_empty_pages);
    ASSERT_IS_FALSE(psa->options.zero_new_pages);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_051: [ If srw_lock_ll_init fails, PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T) shall free the paged sparse array and return NULL. ]*/
TEST_FUNCTION(when_srw_lock_ll_init_fails_then_PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS_frees_the_array_and_fails)
{
    //arrange
    PAGED_SPARSE_ARRAY_OPTIONS options = test_make_options(true, true, false);
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)*)));
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG))
        .SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL, &options);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_048: [ PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T) shall validate max_size and page_size, and allocate and initialize the paged sparse array, in the same way as PAGED_SPARSE_ARRAY_CREATE(T). ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_051: [ If srw_lock_ll_init fails, PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T) shall free the paged sparse array and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_then_PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS_fails)
{
    //arrange
    PAGED_SPARSE_ARRAY_OPTIONS options = test_make_options(true, true, false);
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)*)));
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL, &options);

            //assert
            ASSERT_IS_NULL(psa, "On failed call %zu", i);
        }
    }
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
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);

    // Allocate elements in multiple pages
    uint32_t* ptr0;
    uint32_t* ptr20;
    uint32_t* ptr40;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 0, &ptr0));   // Page 0
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 20, &ptr20)); // Page 1
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 40, &ptr40)); // Page 2
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

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_052: [ If thread_safe is true, PAGED_SPARSE_ARRAY_DISPOSE(T) shall call srw_lock_ll_deinit. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_DISPOSE_with_thread_safe_frees_pages_and_deinitializes_the_lock)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, true, false, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);

    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // page 0
    STRICT_EXPECTED_CALL(srw_lock_ll_deinit(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // THANDLE wrapper

    //act
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*PAGED_SPARSE_ARRAY_ALLOCATE(T)*/

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_013: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall compute the page index as index / page_size. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_014: [ If the page is not allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call malloc_flex to allocate memory for the page structure containing page_size elements of type T plus a uint8_t allocation bitmap of (page_size + 7) / 8 bytes, and initialize all bitmap bits to 0 (not allocated).]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_016: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall mark the element at index as allocated. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_017: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall store a pointer to the element at index in *allocated_ptr and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_succeeds_first_element)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, 1)); // page with embedded bitmap

    //act
    uint32_t* ptr;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 0, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_IS_NOT_NULL(ptr);
    ASSERT_IS_NOT_NULL(psa->pages[0]);
    ASSERT_ARE_EQUAL(uint64_t, 1, psa->pages[0]->allocated_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_014: [ If the page is not allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call malloc_flex to allocate memory for the page structure containing page_size elements of type T plus a uint8_t allocation bitmap of (page_size + 7) / 8 bytes, and initialize all bitmap bits to 0 (not allocated).]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_017: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall store a pointer to the element at index in *allocated_ptr and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_succeeds_second_page)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, 1)); // page with embedded bitmap

    //act
    uint32_t* ptr;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 20, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_IS_NOT_NULL(ptr);
    ASSERT_IS_NULL(psa->pages[0]); // First page should not be allocated
    ASSERT_IS_NOT_NULL(psa->pages[1]); // Second page should be allocated
    ASSERT_ARE_EQUAL(uint64_t, 1, psa->pages[1]->allocated_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_017: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall store a pointer to the element at index in *allocated_ptr and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_multiple_elements_in_same_page)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, 1)); // page with embedded bitmap

    //act
    uint32_t* ptr0;
    uint32_t* ptr5;
    uint32_t* ptr10;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result0 = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 0, &ptr0);
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result5 = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5, &ptr5);
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result10 = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 10, &ptr10);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result0);
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result5);
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result10);
    ASSERT_IS_NOT_NULL(ptr0);
    ASSERT_IS_NOT_NULL(ptr5);
    ASSERT_IS_NOT_NULL(ptr10);
    ASSERT_ARE_EQUAL(uint64_t, 3, psa->pages[0]->allocated_count);
    ASSERT_ARE_NOT_EQUAL(void_ptr, ptr0, ptr5);
    ASSERT_ARE_NOT_EQUAL(void_ptr, ptr5, ptr10);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_011: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_fails_when_handle_is_null)
{
    //arrange
    uint32_t* ptr;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(NULL, 0, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_041: [ If allocated_ptr is NULL, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_fails_when_allocated_ptr_is_null)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 0, NULL);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_012: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_fails_when_index_equals_max_size)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();
    uint32_t* ptr;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 100, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_012: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_fails_when_index_greater_than_max_size)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();
    uint32_t* ptr;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 150, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_015: [ If the element at index is already allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_fails_when_already_allocated)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    uint32_t* ptr1;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5, &ptr1));
    ASSERT_IS_NOT_NULL(ptr1);
    umock_c_reset_all_calls();

    //act
    uint32_t* ptr2;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5, &ptr2);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_018: [ If malloc_flex fails, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_fails_when_page_malloc_fails)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, 1)) // page with embedded bitmap
        .SetReturn(NULL);

    //act
    uint32_t* ptr;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 0, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

static PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)* test_allocate_page_memory_filled_with_garbage(void)
{
    size_t items_and_bitmap_size = TEST_PAGE_SIZE * sizeof(uint32_t) + PAGED_SPARSE_ARRAY_BITMAP_SIZE(TEST_PAGE_SIZE);
    PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)* page_memory = real_gballoc_hl_malloc_flex(sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)), items_and_bitmap_size, 1);
    ASSERT_IS_NOT_NULL(page_memory);
    (void)memset(page_memory, TEST_GARBAGE_BYTE, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)) + items_and_bitmap_size);
    return page_memory;
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_053: [ If zero_new_pages is true, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall set all the items of a newly allocated page to zero. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_with_zero_new_pages_sets_all_items_of_the_new_page_to_zero)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(false, true, true, NULL);
    PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)* page_memory = test_allocate_page_memory_filled_with_garbage();
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, 1))
        .SetReturn(page_memory);

    //act
    uint32_t* ptr;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, page_memory, psa->pages[0]);
    for (uint64_t i = 0; i < TEST_PAGE_SIZE; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, 0, psa->pages[0]->items[i]);
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_053: [ If zero_new_pages is true, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall set all the items of a newly allocated page to zero. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_without_zero_new_pages_leaves_the_items_of_the_new_page_untouched)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL);
    ASSERT_IS_NOT_NULL(psa);
    PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(uint32_t)* page_memory = test_allocate_page_memory_filled_with_garbage();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, 1))
        .SetReturn(page_memory);

    //act
    uint32_t* ptr;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, page_memory, psa->pages[0]);
    for (uint64_t i = 0; i < TEST_PAGE_SIZE; i++)
    {
        ASSERT_ARE_EQUAL(uint32_t, TEST_GARBAGE_ITEM_VALUE, psa->pages[0]->items[i]);
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_054: [ If thread_safe is true, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall acquire the lock in shared mode. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_055: [ If thread_safe is true and the page is not allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the shared lock and acquire the lock in exclusive mode. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_056: [ After acquiring the lock in exclusive mode, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall check again whether the page is allocated, since another thread may have allocated it in the meantime. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_057: [ If thread_safe is true, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall mark the element at index as allocated and increment the allocated count of the page using interlocked operations. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_058: [ If thread_safe is true, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the lock (shared or exclusive, whichever is held) before returning. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_014: [ If the page is not allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call malloc_flex to allocate memory for the page structure containing page_size elements of type T plus a uint8_t allocation bitmap of (page_size + 7) / 8 bytes, and initialize all bitmap bits to 0 (not allocated). ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_017: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall store a pointer to the element at index in *allocated_ptr and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_with_thread_safe_allocates_the_page_under_the_exclusive_lock)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, true, false, NULL);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PSA_LOCK(psa)));

    //act
    uint32_t* ptr;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_IS_NOT_NULL(psa->pages[0]);
    ASSERT_ARE_EQUAL(void_ptr, &psa->pages[0]->items[TEST_INDEX_IN_FIRST_PAGE], ptr);
    ASSERT_IS_TRUE(test_is_allocated_in_bitmap(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(uint64_t, 1, psa->pages[0]->allocated_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_054: [ If thread_safe is true, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall acquire the lock in shared mode. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_057: [ If thread_safe is true, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall mark the element at index as allocated and increment the allocated count of the page using interlocked operations. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_058: [ If thread_safe is true, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the lock (shared or exclusive, whichever is held) before returning. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_ALLOCATE_with_thread_safe_in_an_allocated_page_only_takes_the_shared_lock)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, true, false, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));

    //act
    uint32_t* ptr;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, &psa->pages[0]->items[TEST_OTHER_INDEX_IN_FIRST_PAGE], ptr);
    ASSERT_IS_TRUE(test_is_allocated_in_bitmap(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(uint64_t, 2, psa->pages[0]->allocated_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_015: [ If the element at index is already allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_058: [ If thread_safe is true, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the lock (shared or exclusive, whichever is held) before returning. ]*/
TEST_FUNCTION(when_element_is_already_allocated_then_PAGED_SPARSE_ARRAY_ALLOCATE_with_thread_safe_fails)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, true, false, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));

    //act
    uint32_t* ptr;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED, result);
    ASSERT_ARE_EQUAL(uint64_t, 1, psa->pages[0]->allocated_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_018: [ If malloc_flex fails, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_058: [ If thread_safe is true, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the lock (shared or exclusive, whichever is held) before returning. ]*/
TEST_FUNCTION(when_malloc_flex_fails_then_PAGED_SPARSE_ARRAY_ALLOCATE_with_thread_safe_fails)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, true, false, NULL);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, 1))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PSA_LOCK(psa)));

    //act
    uint32_t* ptr;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ERROR, result);
    ASSERT_IS_NULL(psa->pages[0]);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_056: [ After acquiring the lock in exclusive mode, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall check again whether the page is allocated, since another thread may have allocated it in the meantime. ]*/
TEST_FUNCTION(when_another_thread_allocates_the_page_before_the_exclusive_lock_then_PAGED_SPARSE_ARRAY_ALLOCATE_with_thread_safe_uses_that_page)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, true, false, NULL);
    setup_concurrent_allocate_on_acquire_exclusive(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PSA_LOCK(psa)));
    // the "other thread" allocates the page
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PSA_LOCK(psa)));
    // no second page allocation
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PSA_LOCK(psa)));

    //act
    uint32_t* ptr;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_IS_TRUE(g_concurrent_allocate.was_called);
    ASSERT_ARE_EQUAL(void_ptr, &psa->pages[0]->items[TEST_INDEX_IN_FIRST_PAGE], ptr);
    ASSERT_IS_TRUE(test_is_allocated_in_bitmap(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_IS_TRUE(test_is_allocated_in_bitmap(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(uint64_t, 2, psa->pages[0]->allocated_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    cleanup_concurrent_allocate_on_acquire_exclusive();
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*helper for item_dispose_func tests*/

MOCK_FUNCTION_WITH_CODE(, void, test_item_dispose_func, uint32_t*, item)
MOCK_FUNCTION_END()

/*PAGED_SPARSE_ARRAY_RELEASE(T)*/

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_044: [ If item_dispose_func is not NULL, PAGED_SPARSE_ARRAY_RELEASE(T) shall call item_dispose_func for the element being released. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_024: [ PAGED_SPARSE_ARRAY_RELEASE(T) shall mark the element at index as not allocated. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_calls_item_dispose_func)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, test_item_dispose_func);
    ASSERT_IS_NOT_NULL(psa);
    uint32_t* ptr;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5, &ptr));
    ASSERT_IS_NOT_NULL(ptr);
    *ptr = 42;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_item_dispose_func(ptr));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // page freed since last element

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, 5);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_044: [ If item_dispose_func is not NULL, PAGED_SPARSE_ARRAY_RELEASE(T) shall call item_dispose_func for the element being released. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_with_NULL_item_dispose_func_does_not_call_dispose)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    uint32_t* ptr;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5, &ptr));
    ASSERT_IS_NOT_NULL(ptr);
    *ptr = 42;
    umock_c_reset_all_calls();

    // No test_item_dispose_func call expected since item_dispose_func is NULL
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // page freed since last element

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, 5);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_021: [ PAGED_SPARSE_ARRAY_RELEASE(T) shall compute the page index as index / page_size. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_024: [ PAGED_SPARSE_ARRAY_RELEASE(T) shall mark the element at index as not allocated. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_025: [ If auto_free_empty_pages is true and all elements in the page are now not allocated, PAGED_SPARSE_ARRAY_RELEASE(T) shall free the page and set the page pointer to NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_succeeds_and_frees_page_when_last_element)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    uint32_t* ptr;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5, &ptr));
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
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    uint32_t* ptr1;
    uint32_t* ptr2;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5, &ptr1));
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 10, &ptr2));
    ASSERT_IS_NOT_NULL(ptr1);
    ASSERT_IS_NOT_NULL(ptr2);
    ASSERT_ARE_EQUAL(uint64_t, 2, psa->pages[0]->allocated_count);
    umock_c_reset_all_calls();

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, 5);

    //assert
    ASSERT_IS_NOT_NULL(psa->pages[0]); // Page should still be allocated
    ASSERT_ARE_EQUAL(uint64_t, 1, psa->pages[0]->allocated_count);
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
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
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
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
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
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    uint32_t* ptr;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5, &ptr));
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, 10); // Different element in same page

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_025: [ If auto_free_empty_pages is true and all elements in the page are now not allocated, PAGED_SPARSE_ARRAY_RELEASE(T) shall free the page and set the page pointer to NULL. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_with_auto_free_empty_pages_false_keeps_the_page_when_last_element)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(false, false, false, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_NOT_NULL(psa->pages[0]);
    ASSERT_IS_FALSE(test_is_allocated_in_bitmap(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(uint64_t, 0, psa->pages[0]->allocated_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_059: [ If thread_safe is true, PAGED_SPARSE_ARRAY_RELEASE(T) shall acquire the lock in shared mode. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_060: [ If thread_safe is true, PAGED_SPARSE_ARRAY_RELEASE(T) shall mark the element at index as not allocated using interlocked operations and, only if the element was still marked as allocated, decrement the allocated count of the page using interlocked operations. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_061: [ If thread_safe is true, PAGED_SPARSE_ARRAY_RELEASE(T) shall release the shared lock. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_with_thread_safe_keeps_the_page_when_other_elements_are_allocated)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, true, false, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_NOT_NULL(psa->pages[0]);
    ASSERT_IS_FALSE(test_is_allocated_in_bitmap(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_IS_TRUE(test_is_allocated_in_bitmap(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(uint64_t, 1, psa->pages[0]->allocated_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_062: [ If thread_safe is true, auto_free_empty_pages is true and the allocated count of the page became 0, PAGED_SPARSE_ARRAY_RELEASE(T) shall acquire the lock in exclusive mode, free the page only if it is still allocated and its allocated count is still 0, and release the exclusive lock. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_with_thread_safe_frees_the_page_under_the_exclusive_lock_when_last_element)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, true, false, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // page
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PSA_LOCK(psa)));

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_NULL(psa->pages[0]);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_062: [ If thread_safe is true, auto_free_empty_pages is true and the allocated count of the page became 0, PAGED_SPARSE_ARRAY_RELEASE(T) shall acquire the lock in exclusive mode, free the page only if it is still allocated and its allocated count is still 0, and release the exclusive lock. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_with_thread_safe_and_auto_free_empty_pages_false_keeps_the_page_when_last_element)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, false, false, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_NOT_NULL(psa->pages[0]);
    ASSERT_ARE_EQUAL(uint64_t, 0, psa->pages[0]->allocated_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_062: [ If thread_safe is true, auto_free_empty_pages is true and the allocated count of the page became 0, PAGED_SPARSE_ARRAY_RELEASE(T) shall acquire the lock in exclusive mode, free the page only if it is still allocated and its allocated count is still 0, and release the exclusive lock. ]*/
TEST_FUNCTION(when_another_thread_allocates_in_the_page_before_the_exclusive_lock_then_PAGED_SPARSE_ARRAY_RELEASE_with_thread_safe_keeps_the_page)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, true, false, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    setup_concurrent_allocate_on_acquire_exclusive(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PSA_LOCK(psa)));
    // the "other thread" allocates an element in the (still allocated) page
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));
    // no free of the page
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PSA_LOCK(psa)));

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_TRUE(g_concurrent_allocate.was_called);
    ASSERT_IS_NOT_NULL(psa->pages[0]);
    ASSERT_IS_FALSE(test_is_allocated_in_bitmap(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_IS_TRUE(test_is_allocated_in_bitmap(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(uint64_t, 1, psa->pages[0]->allocated_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    cleanup_concurrent_allocate_on_acquire_exclusive();
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_044: [ If item_dispose_func is not NULL, PAGED_SPARSE_ARRAY_RELEASE(T) shall call item_dispose_func for the element being released. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_059: [ If thread_safe is true, PAGED_SPARSE_ARRAY_RELEASE(T) shall acquire the lock in shared mode. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_061: [ If thread_safe is true, PAGED_SPARSE_ARRAY_RELEASE(T) shall release the shared lock. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_with_thread_safe_calls_item_dispose_func_while_holding_the_shared_lock)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, true, false, test_item_dispose_func);
    uint32_t* ptr = test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(test_item_dispose_func(ptr));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    umock_c_reset_all_calls();
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_022: [ If the page is not allocated, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_061: [ If thread_safe is true, PAGED_SPARSE_ARRAY_RELEASE(T) shall release the shared lock. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_with_thread_safe_and_page_not_allocated_returns)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, true, false, test_item_dispose_func);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_023: [ If the element at index is not allocated, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_061: [ If thread_safe is true, PAGED_SPARSE_ARRAY_RELEASE(T) shall release the shared lock. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_RELEASE_with_thread_safe_and_element_not_allocated_returns)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, true, false, test_item_dispose_func);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));

    //act
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_TRUE(test_is_allocated_in_bitmap(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(uint64_t, 1, psa->pages[0]->allocated_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    umock_c_reset_all_calls();
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*PAGED_SPARSE_ARRAY_GET(T)*/

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_036: [ PAGED_SPARSE_ARRAY_GET(T) shall compute the page index as index / page_size. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_039: [ PAGED_SPARSE_ARRAY_GET(T) shall store in item a pointer to the element at index and return PAGED_SPARSE_ARRAY_GET_OK. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_GET_succeeds)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    uint32_t* ptr1;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5, &ptr1));
    ASSERT_IS_NOT_NULL(ptr1);
    *ptr1 = 42;
    umock_c_reset_all_calls();

    //act
    uint32_t* ptr2 = NULL;
    PAGED_SPARSE_ARRAY_GET_RESULT result = PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, 5, &ptr2);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, result);
    ASSERT_IS_NOT_NULL(ptr2);
    ASSERT_ARE_EQUAL(void_ptr, ptr1, ptr2);
    ASSERT_ARE_EQUAL(uint32_t, 42, *ptr2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_034: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_GET_fails_when_handle_is_null)
{
    //arrange

    //act
    uint32_t* item = NULL;
    PAGED_SPARSE_ARRAY_GET_RESULT result = PAGED_SPARSE_ARRAY_GET(uint32_t)(NULL, 0, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_045: [ If item is NULL, PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_GET_fails_when_item_is_null)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, 0, NULL);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_035: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_GET_fails_when_index_out_of_bounds)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();

    //act
    uint32_t* item = NULL;
    PAGED_SPARSE_ARRAY_GET_RESULT result = PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, 100, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_037: [ If the page is not allocated, PAGED_SPARSE_ARRAY_GET(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_GET_fails_when_page_not_allocated)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();

    //act
    uint32_t* item = NULL;
    PAGED_SPARSE_ARRAY_GET_RESULT result = PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, 5, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_038: [ If the element at index is not allocated, PAGED_SPARSE_ARRAY_GET(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_GET_fails_when_element_not_allocated)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);
    uint32_t* ptr;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5, &ptr));
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();

    //act
    uint32_t* item = NULL;
    PAGED_SPARSE_ARRAY_GET_RESULT result = PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, 10, &item); // Different element in same page

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_063: [ If thread_safe is true, PAGED_SPARSE_ARRAY_GET(T) shall acquire the lock in shared mode. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_064: [ If thread_safe is true, PAGED_SPARSE_ARRAY_GET(T) shall check whether the element at index is allocated using interlocked operations. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_065: [ If thread_safe is true, PAGED_SPARSE_ARRAY_GET(T) shall release the shared lock before returning. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_039: [ PAGED_SPARSE_ARRAY_GET(T) shall store in item a pointer to the element at index and return PAGED_SPARSE_ARRAY_GET_OK. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_GET_with_thread_safe_succeeds_under_the_shared_lock)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, true, false, NULL);
    uint32_t* allocated_ptr = test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    *allocated_ptr = TEST_ITEM_VALUE;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));

    //act
    uint32_t* item = NULL;
    PAGED_SPARSE_ARRAY_GET_RESULT result = PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, allocated_ptr, item);
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, *item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_037: [ If the page is not allocated, PAGED_SPARSE_ARRAY_GET(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_065: [ If thread_safe is true, PAGED_SPARSE_ARRAY_GET(T) shall release the shared lock before returning. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_GET_with_thread_safe_and_page_not_allocated_returns_NOT_ALLOCATED)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, true, false, NULL);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));

    //act
    uint32_t* item = NULL;
    PAGED_SPARSE_ARRAY_GET_RESULT result = PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_038: [ If the element at index is not allocated, PAGED_SPARSE_ARRAY_GET(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_064: [ If thread_safe is true, PAGED_SPARSE_ARRAY_GET(T) shall check whether the element at index is allocated using interlocked operations. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_GET_with_thread_safe_and_element_not_allocated_returns_NOT_ALLOCATED)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, true, false, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PSA_LOCK(psa)));

    //act
    uint32_t* item = NULL;
    PAGED_SPARSE_ARRAY_GET_RESULT result = PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T)*/

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_066: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall fail and return PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_INVALID_ARGS. ]*/
TEST_FUNCTION(when_paged_sparse_array_is_NULL_then_PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_fails)
{
    //arrange

    //act
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(uint32_t)(NULL, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_067: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall fail and return PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_INVALID_ARGS. ]*/
TEST_FUNCTION(when_index_is_max_size_then_PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_fails)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, false, false, NULL);

    //act
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(uint32_t)(psa, TEST_MAX_SIZE);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_069: [ PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall compute the page index as index / page_size. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_070: [ If the page is not allocated, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall return PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_ALLOCATED. ]*/
TEST_FUNCTION(when_page_is_not_allocated_then_PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_returns_NOT_ALLOCATED)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(false, false, false, NULL);

    //act
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_ALLOCATED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_071: [ If the allocated count of the page is not 0, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall return PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_EMPTY. ]*/
TEST_FUNCTION(when_page_is_not_empty_then_PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_returns_NOT_EMPTY)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(false, false, false, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);

    //act
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_EMPTY, result);
    ASSERT_IS_NOT_NULL(psa->pages[0]);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_069: [ PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall compute the page index as index / page_size. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_072: [ Otherwise, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall free the page, set the page pointer to NULL and return PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_FREED. ]*/
TEST_FUNCTION(when_page_is_empty_then_PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_frees_it)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(false, false, false, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);
    ASSERT_IS_NOT_NULL(psa->pages[0]);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // page

    //act
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_FREED, result);
    ASSERT_IS_NULL(psa->pages[0]);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_068: [ If thread_safe is true, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall acquire the lock in exclusive mode. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_072: [ Otherwise, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall free the page, set the page pointer to NULL and return PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_FREED. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_073: [ If thread_safe is true, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall release the exclusive lock before returning. ]*/
TEST_FUNCTION(when_page_is_empty_then_PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_with_thread_safe_frees_it_under_the_exclusive_lock)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, false, false, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);
    ASSERT_IS_NOT_NULL(psa->pages[0]);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // page
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PSA_LOCK(psa)));

    //act
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_FREED, result);
    ASSERT_IS_NULL(psa->pages[0]);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_068: [ If thread_safe is true, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall acquire the lock in exclusive mode. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_071: [ If the allocated count of the page is not 0, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall return PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_EMPTY. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_073: [ If thread_safe is true, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall release the exclusive lock before returning. ]*/
TEST_FUNCTION(when_page_is_not_empty_then_PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_with_thread_safe_returns_NOT_EMPTY)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = test_create_with_options(true, false, false, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PSA_LOCK(psa)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PSA_LOCK(psa)));

    //act
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_EMPTY, result);
    ASSERT_IS_NOT_NULL(psa->pages[0]);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* PAGED_SPARSE_ARRAY_CREATE */

/*Tests_SRS_PAGED_SPARSE_ARRAY_88_008: [ PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
/*Tests_SRS_PAGED_SPARSE_ARRAY_88_017: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall store a pointer to the element at index in *allocated_ptr and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_with_THANDLE_type_works)
{
    //arrange
    PAGED_SPARSE_ARRAY(THANDLE(A_TEST)) psa = PAGED_SPARSE_ARRAY_CREATE(THANDLE(A_TEST))(50, 10, NULL);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();

    //act
    THANDLE(A_TEST)* ptr;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(THANDLE(A_TEST))(psa, 0, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
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
/*Tests_SRS_PAGED_SPARSE_ARRAY_88_017: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall store a pointer to the element at index in *allocated_ptr and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/
/*Tests_SRS_PAGED_SPARSE_ARRAY_88_034: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS.]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_with_struct_type_works)
{
    //arrange
    PAGED_SPARSE_ARRAY(B_TEST) psa = PAGED_SPARSE_ARRAY_CREATE(B_TEST)(50, 10, NULL);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();

    //act
    B_TEST* ptr;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = PAGED_SPARSE_ARRAY_ALLOCATE(B_TEST)(psa, 0, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_IS_NOT_NULL(ptr);
    ptr->b = 42;
    
    B_TEST* ptr_get = NULL;
    PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_GET(B_TEST)(psa, 0, &ptr_get);
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, get_result);
    ASSERT_IS_NOT_NULL(ptr_get);
    ASSERT_ARE_EQUAL(int, 42, ptr_get->b);

    //clean
    PAGED_SPARSE_ARRAY_ASSIGN(B_TEST)(&psa, NULL);
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_006: [ PAGED_SPARSE_ARRAY_CREATE(T) shall store max_size, page_size, and item_dispose_func in the structure. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_043: [ If item_dispose_func is not NULL, PAGED_SPARSE_ARRAY_DISPOSE(T) shall call item_dispose_func for each element that is still allocated. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_DISPOSE_calls_item_dispose_func_for_allocated_elements)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, test_item_dispose_func);
    ASSERT_IS_NOT_NULL(psa);

    // Allocate some elements and set values
    uint32_t* ptr0;
    uint32_t* ptr5;
    uint32_t* ptr20;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 0, &ptr0));
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5, &ptr5));
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 20, &ptr20));
    *ptr0 = 100;
    *ptr5 = 105;
    *ptr20 = 120;
    umock_c_reset_all_calls();

    // Items are disposed in order within each page, then pages in order
    // Page 0: index 0 first, then index 5
    STRICT_EXPECTED_CALL(test_item_dispose_func(ptr0));
    STRICT_EXPECTED_CALL(test_item_dispose_func(ptr5));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // page 0
    // Page 1: index 20 (which is index 4 in page 1)
    STRICT_EXPECTED_CALL(test_item_dispose_func(ptr20));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // page 1
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // THANDLE wrapper

    //act
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_043: [ If item_dispose_func is not NULL, PAGED_SPARSE_ARRAY_DISPOSE(T) shall call item_dispose_func for each element that is still allocated. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_DISPOSE_with_NULL_item_dispose_func_does_not_call_dispose)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, NULL);
    ASSERT_IS_NOT_NULL(psa);

    // Allocate some elements
    uint32_t* ptr0;
    uint32_t* ptr5;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 0, &ptr0));
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5, &ptr5));
    *ptr0 = 100;
    *ptr5 = 105;
    umock_c_reset_all_calls();

    // No test_item_dispose_func calls expected since item_dispose_func is NULL
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // page 0
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // THANDLE wrapper

    //act
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_043: [ If item_dispose_func is not NULL, PAGED_SPARSE_ARRAY_DISPOSE(T) shall call item_dispose_func for each element that is still allocated. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_044: [ If item_dispose_func is not NULL, PAGED_SPARSE_ARRAY_RELEASE(T) shall call item_dispose_func for the element being released. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_DISPOSE_with_item_dispose_func_only_disposes_allocated_elements)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, test_item_dispose_func);
    ASSERT_IS_NOT_NULL(psa);

    // Allocate elements at indices 0, 5, and 10
    uint32_t* ptr0;
    uint32_t* ptr5;
    uint32_t* ptr10;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 0, &ptr0));
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 5, &ptr5));
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, 10, &ptr10));
    *ptr0 = 200;
    *ptr5 = 205;
    *ptr10 = 210;
    umock_c_reset_all_calls();

    // Release index 5 - item_dispose_func is called during release
    // Note: page 0 is NOT freed because elements 0 and 10 are still allocated (page_size=16, so all in page 0)
    STRICT_EXPECTED_CALL(test_item_dispose_func(ptr5));

    //act - Release first, then dispose
    PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, 5);

    //assert release call
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    umock_c_reset_all_calls();

    // Only indices 0 and 10 should be disposed (not 5 which was already released)
    STRICT_EXPECTED_CALL(test_item_dispose_func(ptr0));
    STRICT_EXPECTED_CALL(test_item_dispose_func(ptr10));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // page 0
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // THANDLE wrapper

    //act
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_PAGED_SPARSE_ARRAY_88_010: [ PAGED_SPARSE_ARRAY_DISPOSE(T) shall free all pages that are non-NULL. ]*/
/* Tests_SRS_PAGED_SPARSE_ARRAY_88_043: [ If item_dispose_func is not NULL, PAGED_SPARSE_ARRAY_DISPOSE(T) shall call item_dispose_func for each element that is still allocated. ]*/
TEST_FUNCTION(PAGED_SPARSE_ARRAY_DISPOSE_with_no_allocated_elements_does_not_call_dispose)
{
    //arrange
    PAGED_SPARSE_ARRAY(uint32_t) psa = PAGED_SPARSE_ARRAY_CREATE(uint32_t)(100, 16, test_item_dispose_func);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();

    // No test_item_dispose_func calls expected since nothing was allocated
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // THANDLE wrapper only

    //act
    PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
