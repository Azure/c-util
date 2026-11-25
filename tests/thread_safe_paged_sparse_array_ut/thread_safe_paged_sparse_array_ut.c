// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "thread_safe_paged_sparse_array_ut_pch.h"

THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(uint32_t)
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(uint32_t));
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(uint32_t));

THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(uint32_t);
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(uint32_t);

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_RESULT_VALUES)

#define TEST_MAX_SIZE 100
#define TEST_PAGE_SIZE 16
#define TEST_PAGE_COUNT 7 /*(TEST_MAX_SIZE + TEST_PAGE_SIZE - 1) / TEST_PAGE_SIZE*/
#define TEST_INDEX_IN_FIRST_PAGE 5
#define TEST_OTHER_INDEX_IN_FIRST_PAGE 10
#define TEST_INDEX_IN_THIRD_PAGE 40
#define TEST_ITEM_VALUE 42
#define TEST_OTHER_ITEM_VALUE 43

#define TEST_SLOT_TYPE THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(uint32_t)
#define TEST_PAGE_INDEX(index) ((index) / TEST_PAGE_SIZE)
#define TEST_ARRAY(psa) ((THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(uint32_t)*)(psa))
#define TEST_PAGE_LOCK(psa, page_index) (&TEST_ARRAY(psa)->page_locks[(page_index)])
#define TEST_WRAPPED_PAGE(psa, page_index) ((psa)->paged_sparse_array->pages[(page_index)])
#define TEST_SLOT(psa, index) (&TEST_WRAPPED_PAGE(psa, TEST_PAGE_INDEX(index))->items[(index) % TEST_PAGE_SIZE])

MOCK_FUNCTION_WITH_CODE(, void, test_item_copy_func, uint32_t*, dst, uint32_t*, src)
    *dst = *src;
MOCK_FUNCTION_END()

/*RELEASE disposes a local copy of the item, so the tests check the value of the item that was disposed*/
static uint32_t g_disposed_item_value;

MOCK_FUNCTION_WITH_CODE(, void, test_item_dispose_func, uint32_t*, item)
    g_disposed_item_value = *item;
MOCK_FUNCTION_END()

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) test_create(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_COPY_FUNC(uint32_t) item_copy_func, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(uint32_t) item_dispose_func)
{
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, item_copy_func, item_dispose_func);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();
    return psa;
}

/*allocates the element at index with value and returns a pointer to the item of its slot*/
static uint32_t* test_allocate(THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa, uint64_t index, uint32_t value)
{
    uint32_t item = value;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, index, &item));
    umock_c_reset_all_calls();
    return &TEST_SLOT(psa, index)->item;
}

/*the wrapped paged sparse array tracks which elements are allocated*/
static bool test_is_allocated(THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa, uint64_t index)
{
    TEST_SLOT_TYPE* slot;
    return PAGED_SPARSE_ARRAY_LL_GET(TEST_SLOT_TYPE)(psa->paged_sparse_array, index, &slot) == PAGED_SPARSE_ARRAY_GET_OK;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_SRW_LOCK_LL_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_flex, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(srw_lock_ll_init, MU_FAILURE);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    g_disposed_item_value = 0;

    umock_c_negative_tests_init();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_001: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call PAGED_SPARSE_ARRAY_CREATE(T) with max_size, page_size and NULL as item_dispose_func to create the paged sparse array of slots. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_003: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX to allocate the array with a lock for each page. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_005: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call srw_lock_ll_init to initialize the lock of each page. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_007: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store item_dispose_func. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_110: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store item_copy_func. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_008: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store the paged sparse array of slots. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_009: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_succeeds)
{
    //arrange
    SRW_LOCK_LL* initialized_locks[TEST_PAGE_COUNT];
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(TEST_SLOT_TYPE)*)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(SRW_LOCK_LL)));
    for (uint64_t i = 0; i < TEST_PAGE_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG))
            .CaptureArgumentValue_srw_lock_ll(&initialized_locks[i]);
    }

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, test_item_copy_func, test_item_dispose_func);

    //assert
    ASSERT_IS_NOT_NULL(psa);
    ASSERT_IS_NOT_NULL(psa->paged_sparse_array);
    ASSERT_ARE_EQUAL(uint64_t, TEST_MAX_SIZE, psa->paged_sparse_array->max_size);
    ASSERT_ARE_EQUAL(uint64_t, TEST_PAGE_SIZE, psa->paged_sparse_array->page_size);
    ASSERT_ARE_EQUAL(uint64_t, TEST_PAGE_COUNT, psa->paged_sparse_array->page_count);
    ASSERT_IS_NULL(psa->paged_sparse_array->item_dispose_func);
    ASSERT_IS_TRUE(psa->item_dispose_func == test_item_dispose_func);
    ASSERT_IS_TRUE(psa->item_copy_func == test_item_copy_func);
    for (uint64_t i = 0; i < TEST_PAGE_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(void_ptr, TEST_PAGE_LOCK(psa, i), initialized_locks[i]);
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_002: [ If PAGED_SPARSE_ARRAY_CREATE(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(when_PAGED_SPARSE_ARRAY_CREATE_fails_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_fails)
{
    //arrange

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(0, TEST_PAGE_SIZE, NULL, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [ If there are any other errors, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(when_THANDLE_MALLOC_FLEX_fails_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_releases_the_paged_sparse_array_and_fails)
{
    //arrange
    void* wrapped_array_memory;
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(TEST_SLOT_TYPE)*)))
        .CaptureReturn(&wrapped_array_memory);
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(SRW_LOCK_LL)))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&wrapped_array_memory);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [ If there are any other errors, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(when_srw_lock_ll_init_fails_for_the_third_lock_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_deinitializes_the_initialized_locks_and_fails)
{
    //arrange
    void* wrapped_array_memory;
    void* thread_safe_array_memory;
    SRW_LOCK_LL* first_lock;
    SRW_LOCK_LL* second_lock;
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(TEST_SLOT_TYPE)*)))
        .CaptureReturn(&wrapped_array_memory);
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(SRW_LOCK_LL)))
        .CaptureReturn(&thread_safe_array_memory);
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG))
        .CaptureArgumentValue_srw_lock_ll(&first_lock);
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG))
        .CaptureArgumentValue_srw_lock_ll(&second_lock);
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG))
        .SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(srw_lock_ll_deinit(IGNORED_ARG))
        .ValidateArgumentValue_srw_lock_ll(&first_lock);
    STRICT_EXPECTED_CALL(srw_lock_ll_deinit(IGNORED_ARG))
        .ValidateArgumentValue_srw_lock_ll(&second_lock);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&thread_safe_array_memory);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&wrapped_array_memory);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [ If there are any other errors, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(when_srw_lock_ll_init_fails_for_the_first_lock_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_frees_the_arrays_and_fails)
{
    //arrange
    void* wrapped_array_memory;
    void* thread_safe_array_memory;
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(TEST_SLOT_TYPE)*)))
        .CaptureReturn(&wrapped_array_memory);
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(SRW_LOCK_LL)))
        .CaptureReturn(&thread_safe_array_memory);
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG))
        .SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&thread_safe_array_memory);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&wrapped_array_memory);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [ If there are any other errors, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_fails)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(TEST_SLOT_TYPE)*)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(SRW_LOCK_LL)));
    for (uint64_t i = 0; i < TEST_PAGE_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG));
    }

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //act
            THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL, NULL);

            //assert
            ASSERT_IS_NULL(psa, "On failed call %zu", i);
        }
    }
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_109: [ If only one of item_copy_func and item_dispose_func is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_with_NULL_item_copy_func_and_non_NULL_item_dispose_func_fails)
{
    //arrange

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL, test_item_dispose_func);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_109: [ If only one of item_copy_func and item_dispose_func is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_with_non_NULL_item_copy_func_and_NULL_item_dispose_func_fails)
{
    //arrange

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, test_item_copy_func, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_007: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store item_dispose_func. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_110: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store item_copy_func. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_009: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_with_NULL_item_copy_func_and_NULL_item_dispose_func_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(TEST_SLOT_TYPE)*)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(SRW_LOCK_LL)));
    for (uint64_t i = 0; i < TEST_PAGE_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG));
    }

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL, NULL);

    //assert
    ASSERT_IS_NOT_NULL(psa);
    ASSERT_IS_NULL(psa->item_copy_func);
    ASSERT_IS_NULL(psa->item_dispose_func);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_011: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE_with_NULL_returns)
{
    //arrange

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_NAME(uint32_t)(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_012: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call item_dispose_func with a pointer to the item of each allocated element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_013: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call srw_lock_ll_deinit for the lock of each page. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_014: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall release the paged sparse array of slots. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE_disposes_the_allocated_items_deinitializes_the_locks_and_releases_the_paged_sparse_array)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint32_t* item_in_first_page = test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_ITEM_VALUE);
    uint32_t* other_item_in_first_page = test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, TEST_OTHER_ITEM_VALUE);
    uint32_t* item_in_third_page = test_allocate(psa, TEST_INDEX_IN_THIRD_PAGE, TEST_ITEM_VALUE);
    void* first_page = TEST_WRAPPED_PAGE(psa, TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE));
    void* third_page = TEST_WRAPPED_PAGE(psa, TEST_PAGE_INDEX(TEST_INDEX_IN_THIRD_PAGE));
    for (uint64_t i = 0; i < TEST_PAGE_COUNT; i++)
    {
        if (i == TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE))
        {
            STRICT_EXPECTED_CALL(test_item_dispose_func(item_in_first_page));
            STRICT_EXPECTED_CALL(test_item_dispose_func(other_item_in_first_page));
        }
        else
        {
            if (i == TEST_PAGE_INDEX(TEST_INDEX_IN_THIRD_PAGE))
            {
                STRICT_EXPECTED_CALL(test_item_dispose_func(item_in_third_page));
            }
            else
            {
                /* no item is allocated in the page */
            }
        }
        STRICT_EXPECTED_CALL(srw_lock_ll_deinit(TEST_PAGE_LOCK(psa, i)));
    }
    STRICT_EXPECTED_CALL(free(first_page));
    STRICT_EXPECTED_CALL(free(third_page));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*the paged sparse array*/
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*the thread safe paged sparse array*/

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_012: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call item_dispose_func with a pointer to the item of each allocated element. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE_with_NULL_item_dispose_func_does_not_dispose_the_items)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_ITEM_VALUE);
    void* first_page = TEST_WRAPPED_PAGE(psa, TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE));
    for (uint64_t i = 0; i < TEST_PAGE_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(srw_lock_ll_deinit(TEST_PAGE_LOCK(psa, i)));
    }
    STRICT_EXPECTED_CALL(free(first_page));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*the paged sparse array*/
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); /*the thread safe paged sparse array*/

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_041: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_with_NULL_paged_sparse_array_fails)
{
    //arrange
    uint32_t item = TEST_ITEM_VALUE;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(NULL, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_111: [ If item is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_with_NULL_item_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, NULL);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(TEST_WRAPPED_PAGE(psa, TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE)));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_043: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_with_index_equal_to_max_size_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint32_t item = TEST_ITEM_VALUE;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_MAX_SIZE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_030: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall acquire the lock of the page of index in exclusive mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_132: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call PAGED_SPARSE_ARRAY_ALLOCATE(T) to allocate the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_114: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall copy item to the item of the slot. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_116: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall succeed and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the page lock in exclusive mode before returning. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_in_a_page_that_is_not_allocated_allocates_the_page_and_the_element_while_holding_the_lock_of_the_page_in_exclusive_mode)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_THIRD_PAGE);
    uint32_t item = TEST_ITEM_VALUE;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)); /*the page of the paged sparse array*/
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_THIRD_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, TEST_SLOT(psa, TEST_INDEX_IN_THIRD_PAGE)->item);
    ASSERT_IS_TRUE(test_is_allocated(psa, TEST_INDEX_IN_THIRD_PAGE));
    ASSERT_ARE_EQUAL(uint64_t, 1, TEST_WRAPPED_PAGE(psa, page_index)->allocated_count);

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_030: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall acquire the lock of the page of index in exclusive mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_132: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call PAGED_SPARSE_ARRAY_ALLOCATE(T) to allocate the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_114: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall copy item to the item of the slot. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_116: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall succeed and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the page lock in exclusive mode before returning. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_in_an_allocated_page_allocates_the_element_while_holding_the_lock_of_the_page_in_exclusive_mode)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_ITEM_VALUE);
    uint32_t item = TEST_OTHER_ITEM_VALUE;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, TEST_OTHER_ITEM_VALUE, TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE)->item);
    ASSERT_IS_TRUE(test_is_allocated(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(uint64_t, 2, TEST_WRAPPED_PAGE(psa, page_index)->allocated_count);

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_113: [ If item_copy_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call item_copy_func with a pointer to the item of the slot as dst and item as src. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_calls_item_copy_func_while_holding_the_lock_of_the_page_in_exclusive_mode)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_ITEM_VALUE);
    uint32_t item = TEST_OTHER_ITEM_VALUE;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(test_item_copy_func(&TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE)->item, &item));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, TEST_OTHER_ITEM_VALUE, TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE)->item);
    ASSERT_IS_TRUE(test_is_allocated(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_133: [ If PAGED_SPARSE_ARRAY_ALLOCATE(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return the result of PAGED_SPARSE_ARRAY_ALLOCATE(T). ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the page lock in exclusive mode before returning. ]*/
TEST_FUNCTION(when_allocating_the_page_fails_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_releases_the_lock_of_the_page_and_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    uint32_t item = TEST_ITEM_VALUE;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)) /*the page of the paged sparse array*/
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(TEST_WRAPPED_PAGE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_133: [ If PAGED_SPARSE_ARRAY_ALLOCATE(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return the result of PAGED_SPARSE_ARRAY_ALLOCATE(T). ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the page lock in exclusive mode before returning. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_of_an_allocated_element_releases_the_lock_of_the_page_and_fails_without_copying_the_item)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_ITEM_VALUE);
    uint32_t item = TEST_OTHER_ITEM_VALUE;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->item);
    ASSERT_ARE_EQUAL(uint64_t, 1, TEST_WRAPPED_PAGE(psa, page_index)->allocated_count);

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_054: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_with_NULL_paged_sparse_array_returns)
{
    //arrange

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(NULL, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_055: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_with_index_equal_to_max_size_returns)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_MAX_SIZE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_021: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall acquire the lock of the page of index in exclusive mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_058: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call PAGED_SPARSE_ARRAY_GET(T) to get the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_135: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall copy the item of the slot to a local item. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_136: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call PAGED_SPARSE_ARRAY_RELEASE(T) to release the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall release the page lock. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call item_dispose_func with a pointer to the local item. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_an_element_releases_it_while_holding_the_lock_of_the_page_and_disposes_the_item_after_releasing_the_lock)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_ITEM_VALUE);
    (void)test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, TEST_OTHER_ITEM_VALUE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(test_item_dispose_func(IGNORED_ARG));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, TEST_OTHER_ITEM_VALUE, g_disposed_item_value);
    ASSERT_IS_FALSE(test_is_allocated(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_IS_TRUE(test_is_allocated(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(uint64_t, 1, TEST_WRAPPED_PAGE(psa, page_index)->allocated_count);

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_136: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call PAGED_SPARSE_ARRAY_RELEASE(T) to release the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall release the page lock. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_with_NULL_item_dispose_func_releases_the_element)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_ITEM_VALUE);
    (void)test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, TEST_OTHER_ITEM_VALUE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_FALSE(test_is_allocated(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(uint64_t, 1, TEST_WRAPPED_PAGE(psa, page_index)->allocated_count);

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_136: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call PAGED_SPARSE_ARRAY_RELEASE(T) to release the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall release the page lock. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call item_dispose_func with a pointer to the local item. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_the_last_element_of_a_page_frees_the_page_while_holding_the_lock_of_the_page_and_disposes_the_item_after_releasing_the_lock)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_THIRD_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_THIRD_PAGE, TEST_ITEM_VALUE);
    void* page = TEST_WRAPPED_PAGE(psa, page_index);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(free(page));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(test_item_dispose_func(IGNORED_ARG));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_THIRD_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, g_disposed_item_value);
    ASSERT_IS_NULL(TEST_WRAPPED_PAGE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_134: [ If PAGED_SPARSE_ARRAY_GET(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall release the page lock and return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_an_element_that_is_not_allocated_in_an_allocated_page_releases_the_lock_of_the_page_and_returns)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_ITEM_VALUE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(test_is_allocated(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(uint64_t, 1, TEST_WRAPPED_PAGE(psa, page_index)->allocated_count);

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_134: [ If PAGED_SPARSE_ARRAY_GET(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall release the page lock and return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_in_a_page_that_is_not_allocated_releases_the_lock_of_the_page_and_returns)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_THIRD_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_THIRD_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(TEST_WRAPPED_PAGE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_067: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_with_NULL_paged_sparse_array_fails)
{
    //arrange
    uint32_t item;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(NULL, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_068: [ If item is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_with_NULL_item_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_ITEM_VALUE);

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, NULL);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_069: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_with_index_equal_to_max_size_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint32_t item;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_MAX_SIZE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_104: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall acquire the lock of the page of index in shared mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_072: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall call PAGED_SPARSE_ARRAY_GET(T) to get the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_137: [ If PAGED_SPARSE_ARRAY_GET(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall fail and return the result of PAGED_SPARSE_ARRAY_GET(T). ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_131: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall release the page lock in shared mode before returning. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_in_a_page_that_is_not_allocated_releases_the_lock_of_the_page_and_returns_NOT_ALLOCATED)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_THIRD_PAGE);
    uint32_t item;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_INDEX_IN_THIRD_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_137: [ If PAGED_SPARSE_ARRAY_GET(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall fail and return the result of PAGED_SPARSE_ARRAY_GET(T). ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_131: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall release the page lock in shared mode before returning. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_of_an_element_that_is_not_allocated_in_an_allocated_page_releases_the_lock_of_the_page_and_returns_NOT_ALLOCATED)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_ITEM_VALUE);
    uint32_t item;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_104: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall acquire the lock of the page of index in shared mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_072: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall call PAGED_SPARSE_ARRAY_GET(T) to get the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_127: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall copy the item of the slot to item. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_130: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall succeed and return PAGED_SPARSE_ARRAY_GET_OK. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_131: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall release the page lock in shared mode before returning. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_of_an_allocated_element_copies_the_item_while_holding_the_lock_of_the_page_in_shared_mode)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_ITEM_VALUE);
    uint32_t item = 0;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, item);
    ASSERT_IS_TRUE(test_is_allocated(psa, TEST_INDEX_IN_FIRST_PAGE));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_126: [ If item_copy_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall call item_copy_func with item as dst and a pointer to the item of the slot as src. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_130: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall succeed and return PAGED_SPARSE_ARRAY_GET_OK. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_calls_item_copy_func_while_holding_the_lock_of_the_page_in_shared_mode)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* slot_item = test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_ITEM_VALUE);
    uint32_t item = 0;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(test_item_copy_func(&item, slot_item));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, item);
    ASSERT_IS_TRUE(test_is_allocated(psa, TEST_INDEX_IN_FIRST_PAGE));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
