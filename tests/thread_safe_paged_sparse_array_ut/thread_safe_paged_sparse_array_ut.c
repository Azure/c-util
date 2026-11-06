// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "thread_safe_paged_sparse_array_ut_pch.h"

/*THREAD_SAFE_PAGED_SPARSE_ARRAY with the default options (AUTO_FREE_EMPTY_PAGES, DO_NOT_ZERO_NEW_PAGES)*/
THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(uint32_t)
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(uint32_t));
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(uint32_t));

THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(uint32_t);
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(uint32_t);

/*the arrays of KEEP_AND_ZERO_PAGES_UINT32 are defined with KEEP_EMPTY_PAGES and ZERO_NEW_PAGES*/
typedef uint32_t KEEP_AND_ZERO_PAGES_UINT32;

THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(KEEP_AND_ZERO_PAGES_UINT32)
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(KEEP_AND_ZERO_PAGES_UINT32));
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(KEEP_AND_ZERO_PAGES_UINT32));

THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(KEEP_AND_ZERO_PAGES_UINT32);
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE_WITH_OPTIONS(KEEP_AND_ZERO_PAGES_UINT32, KEEP_EMPTY_PAGES, ZERO_NEW_PAGES);

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT_VALUES)

#define TEST_MAX_SIZE 100
#define TEST_PAGE_SIZE 16
#define TEST_PAGE_COUNT 7 /*(TEST_MAX_SIZE + TEST_PAGE_SIZE - 1) / TEST_PAGE_SIZE*/
#define TEST_EXACT_MULTIPLE_PAGE_COUNT 4
#define TEST_INDEX_IN_FIRST_PAGE 5
#define TEST_OTHER_INDEX_IN_FIRST_PAGE 10
#define TEST_LAST_INDEX_IN_FIRST_PAGE 15
#define TEST_INDEX_IN_SECOND_PAGE 20
#define TEST_INDEX_IN_THIRD_PAGE 40
#define TEST_INDEX_IN_FOURTH_PAGE 50
#define TEST_INDEX_IN_FIFTH_PAGE 70
#define TEST_LAST_INDEX (TEST_MAX_SIZE - 1)
#define TEST_ITEM_VALUE 42
#define TEST_GARBAGE_BYTE 0xFF
#define TEST_GARBAGE_ITEM_VALUE UINT32_MAX
#define TEST_PAGE_BITMAP_SIZE PAGED_SPARSE_ARRAY_BITMAP_SIZE(TEST_PAGE_SIZE)

/*all the array types used in the tests have the same layout*/
#define TEST_PAGE_SLOT(psa, index) ((THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT*)&(psa)->page_slots[(index) / TEST_PAGE_SIZE])
#define TEST_PAGE_LOCK(psa, index) (&TEST_PAGE_SLOT(psa, index)->lock)
#define TEST_PAGE(psa, index) ((uint32_t*)TEST_PAGE_SLOT(psa, index)->page)
#define TEST_USE_COUNT(psa, index) (TEST_PAGE_SLOT(psa, index)->use_count)
#define TEST_IS_ALLOCATED_IN_BITMAP(psa, index) \
    (PAGED_SPARSE_ARRAY_IS_ALLOCATED(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_BITMAP(TEST_PAGE(psa, index), TEST_PAGE_SIZE), (index) % TEST_PAGE_SIZE) != 0)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) test_create(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(uint32_t) item_dispose_func)
{
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, item_dispose_func);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();
    return psa;
}

static uint32_t* test_allocate(THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa, uint64_t index)
{
    uint32_t* ptr = NULL;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, index, &ptr));
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();
    return ptr;
}

static THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_AND_ZERO_PAGES_UINT32) test_create_keep_and_zero_pages(void)
{
    THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_AND_ZERO_PAGES_UINT32) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(KEEP_AND_ZERO_PAGES_UINT32)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL);
    ASSERT_IS_NOT_NULL(psa);
    umock_c_reset_all_calls();
    return psa;
}

static uint32_t* test_allocate_keep_and_zero_pages(THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_AND_ZERO_PAGES_UINT32) psa, uint64_t index)
{
    uint32_t* ptr = NULL;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(KEEP_AND_ZERO_PAGES_UINT32)(psa, index, &ptr));
    ASSERT_IS_NOT_NULL(ptr);
    umock_c_reset_all_calls();
    return ptr;
}

/*returns an array whose page of index is allocated and empty (no element in it is allocated)*/
static THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_AND_ZERO_PAGES_UINT32) test_create_keep_and_zero_pages_with_empty_page(uint64_t index)
{
    THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_AND_ZERO_PAGES_UINT32) psa = test_create_keep_and_zero_pages();
    (void)test_allocate_keep_and_zero_pages(psa, index);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(KEEP_AND_ZERO_PAGES_UINT32)(psa, index);
    ASSERT_IS_NOT_NULL(TEST_PAGE(psa, index));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, index));
    umock_c_reset_all_calls();
    return psa;
}

static void* test_allocate_page_memory_filled_with_garbage(void)
{
    size_t page_memory_size = TEST_PAGE_SIZE * sizeof(uint32_t) + TEST_PAGE_BITMAP_SIZE;
    void* page_memory = real_gballoc_hl_malloc_flex(TEST_PAGE_BITMAP_SIZE, TEST_PAGE_SIZE, sizeof(uint32_t));
    ASSERT_IS_NOT_NULL(page_memory);
    (void)memset(page_memory, TEST_GARBAGE_BYTE, page_memory_size);
    return page_memory;
}

/*used by tests that simulate another thread calling into the array right before the lock of a page slot is acquired in exclusive mode*/
typedef enum TEST_CONCURRENT_CALL_TAG
{
    TEST_CONCURRENT_CALL_NONE,
    /*another thread allocates the element at index*/
    TEST_CONCURRENT_CALL_ALLOCATE,
    /*another thread frees the page of index*/
    TEST_CONCURRENT_CALL_FREE_EMPTY_PAGES,
    /*another thread begins to use the page of index (e.g. it is in a call to GET)*/
    TEST_CONCURRENT_CALL_BEGIN_PAGE_USE,
    /*the thread that is freeing the page of index finishes: it sets the page pointer to NULL and removes THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING from the use count*/
    TEST_CONCURRENT_CALL_FINISH_FREEING_PAGE
} TEST_CONCURRENT_CALL;

static struct
{
    TEST_CONCURRENT_CALL call;
    const THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(uint32_t)* psa;
    uint64_t index;
    bool was_called;
} g_concurrent_call;

static void hook_srw_lock_ll_acquire_exclusive_calls_concurrently(SRW_LOCK_LL* srw_lock_ll)
{
    if ((g_concurrent_call.call == TEST_CONCURRENT_CALL_NONE) || g_concurrent_call.was_called)
    {
        /* do nothing, the test does not simulate a concurrent call or the only one was already simulated */
    }
    else
    {
        g_concurrent_call.was_called = true;
        if (g_concurrent_call.call == TEST_CONCURRENT_CALL_ALLOCATE)
        {
            uint32_t* other_ptr;
            ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(g_concurrent_call.psa, g_concurrent_call.index, &other_ptr));
        }
        else if (g_concurrent_call.call == TEST_CONCURRENT_CALL_FREE_EMPTY_PAGES)
        {
            ASSERT_ARE_EQUAL(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(uint32_t)(g_concurrent_call.psa, g_concurrent_call.index, g_concurrent_call.index));
        }
        else if (g_concurrent_call.call == TEST_CONCURRENT_CALL_BEGIN_PAGE_USE)
        {
            (void)interlocked_increment_64(&TEST_USE_COUNT(g_concurrent_call.psa, g_concurrent_call.index));
        }
        else
        {
            (void)interlocked_exchange_pointer(&TEST_PAGE_SLOT(g_concurrent_call.psa, g_concurrent_call.index)->page, NULL);
            (void)interlocked_add_64(&TEST_USE_COUNT(g_concurrent_call.psa, g_concurrent_call.index), -THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING);
        }
    }
    real_srw_lock_ll_acquire_exclusive(srw_lock_ll);
}

static void setup_concurrent_call_on_acquire_exclusive(TEST_CONCURRENT_CALL call, THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa, uint64_t index)
{
    g_concurrent_call.call = call;
    g_concurrent_call.psa = psa;
    g_concurrent_call.index = index;
    g_concurrent_call.was_called = false;
}

/*used by tests that check the state of the array when item_dispose_func is called, optionally simulating another caller that releases the same element at that moment*/
static struct
{
    const THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(uint32_t)* psa;
    uint64_t index;
    bool release_concurrently;
    bool was_called;
    bool was_allocated_when_called;
    int64_t use_count_when_called;
} g_dispose_observer;

MOCK_FUNCTION_WITH_CODE(, void, test_item_dispose_func, uint32_t*, item)
    if (g_dispose_observer.psa == NULL)
    {
        /* do nothing, the observer is not set up (e.g. the array is being disposed) */
    }
    else
    {
        uint8_t* bitmap = THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_BITMAP(TEST_PAGE(g_dispose_observer.psa, g_dispose_observer.index), TEST_PAGE_SIZE);
        g_dispose_observer.was_called = true;
        g_dispose_observer.was_allocated_when_called = THREAD_SAFE_PAGED_SPARSE_ARRAY_IS_ALLOCATED(bitmap, g_dispose_observer.index % TEST_PAGE_SIZE);
        g_dispose_observer.use_count_when_called = interlocked_add_64(&TEST_USE_COUNT(g_dispose_observer.psa, g_dispose_observer.index), 0);
        if (g_dispose_observer.release_concurrently)
        {
            /* the other caller marks the element as not allocated and ends the use of the page by the element */
            ASSERT_IS_TRUE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TEST_AND_CLEAR_ALLOCATED(bitmap, g_dispose_observer.index % TEST_PAGE_SIZE));
            (void)interlocked_decrement_64(&TEST_USE_COUNT(g_dispose_observer.psa, g_dispose_observer.index));
        }
    }
MOCK_FUNCTION_END()

static void setup_dispose_observer(THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa, uint64_t index, bool release_concurrently)
{
    g_dispose_observer.psa = psa;
    g_dispose_observer.index = index;
    g_dispose_observer.release_concurrently = release_concurrently;
    g_dispose_observer.was_called = false;
    g_dispose_observer.was_allocated_when_called = false;
    g_dispose_observer.use_count_when_called = -1;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_SRW_LOCK_LL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_HOOK(srw_lock_ll_acquire_exclusive, hook_srw_lock_ll_acquire_exclusive_calls_concurrently);

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
    (void)memset(&g_concurrent_call, 0, sizeof(g_concurrent_call));
    (void)memset(&g_dispose_observer, 0, sizeof(g_dispose_observer));

    umock_c_negative_tests_init();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_001: [ If max_size is zero, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_with_max_size_0_fails)
{
    //arrange

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(0, TEST_PAGE_SIZE, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_002: [ If page_size is zero, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_with_page_size_0_fails)
{
    //arrange

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, 0, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_003: [ If max_size + page_size - 1 would overflow, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_when_max_size_plus_page_size_would_overflow_fails)
{
    //arrange

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(UINT64_MAX, TEST_PAGE_SIZE, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_004: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall compute the number of pages as (max_size + page_size - 1) / page_size. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_005: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX to allocate memory for the paged sparse array with one page slot for each page. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_006: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store max_size, page_size, and item_dispose_func in the structure. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_007: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall set the page pointer of each page slot to NULL and its use count to 0. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_008: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call srw_lock_ll_init to initialize the lock of each page slot. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_succeeds)
{
    //arrange
    SRW_LOCK_LL* initialized_locks[TEST_PAGE_COUNT];
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT)));
    for (uint64_t i = 0; i < TEST_PAGE_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG))
            .CaptureArgumentValue_srw_lock_ll(&initialized_locks[i]);
    }

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, test_item_dispose_func);

    //assert
    ASSERT_IS_NOT_NULL(psa);
    ASSERT_ARE_EQUAL(uint64_t, TEST_MAX_SIZE, psa->max_size);
    ASSERT_ARE_EQUAL(uint64_t, TEST_PAGE_SIZE, psa->page_size);
    ASSERT_ARE_EQUAL(uint64_t, TEST_PAGE_COUNT, psa->page_count);
    ASSERT_IS_TRUE(psa->item_dispose_func == test_item_dispose_func);
    for (uint64_t i = 0; i < TEST_PAGE_COUNT; i++)
    {
        ASSERT_IS_NULL(TEST_PAGE(psa, i * TEST_PAGE_SIZE));
        ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, i * TEST_PAGE_SIZE));
        ASSERT_ARE_EQUAL(void_ptr, TEST_PAGE_LOCK(psa, i * TEST_PAGE_SIZE), initialized_locks[i]);
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_004: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall compute the number of pages as (max_size + page_size - 1) / page_size. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_with_max_size_multiple_of_page_size_computes_the_page_count)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_EXACT_MULTIPLE_PAGE_COUNT, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT)));
    for (uint64_t i = 0; i < TEST_EXACT_MULTIPLE_PAGE_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG));
    }

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_EXACT_MULTIPLE_PAGE_COUNT * TEST_PAGE_SIZE, TEST_PAGE_SIZE, NULL);

    //assert
    ASSERT_IS_NOT_NULL(psa);
    ASSERT_ARE_EQUAL(uint64_t, TEST_EXACT_MULTIPLE_PAGE_COUNT, psa->page_count);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_009: [ If srw_lock_ll_init fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call srw_lock_ll_deinit for the locks that were initialized, free the paged sparse array and return NULL. ]*/
TEST_FUNCTION(when_srw_lock_ll_init_fails_for_the_third_page_slot_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_deinitializes_the_first_2_locks_and_fails)
{
    //arrange
    SRW_LOCK_LL* first_lock;
    SRW_LOCK_LL* second_lock;
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT)));
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
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_009: [ If srw_lock_ll_init fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call srw_lock_ll_deinit for the locks that were initialized, free the paged sparse array and return NULL. ]*/
TEST_FUNCTION(when_srw_lock_ll_init_fails_for_the_first_page_slot_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_frees_the_array_and_fails)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT)));
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG))
        .SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_009: [ If srw_lock_ll_init fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call srw_lock_ll_deinit for the locks that were initialized, free the paged sparse array and return NULL. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_011: [ If there are any other errors, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_fails)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT)));
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
            THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL);

            //assert
            ASSERT_IS_NULL(psa, "On failed call %zu", i);
        }
    }
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_012: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE_with_NULL_returns)
{
    //arrange

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_NAME(uint32_t)(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_014: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall free all pages that are non-NULL. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_015: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call srw_lock_ll_deinit for the lock of each page slot. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE_frees_the_allocated_pages_and_deinitializes_the_lock_of_each_page_slot)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_THIRD_PAGE);
    for (uint64_t i = 0; i < TEST_PAGE_COUNT; i++)
    {
        uint32_t* page = TEST_PAGE(psa, i * TEST_PAGE_SIZE);
        if (page != NULL)
        {
            STRICT_EXPECTED_CALL(free(page));
        }
        STRICT_EXPECTED_CALL(srw_lock_ll_deinit(TEST_PAGE_LOCK(psa, i * TEST_PAGE_SIZE)));
    }
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_015: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call srw_lock_ll_deinit for the lock of each page slot. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE_with_no_allocated_pages_only_deinitializes_the_locks)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);
    for (uint64_t i = 0; i < TEST_PAGE_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(srw_lock_ll_deinit(TEST_PAGE_LOCK(psa, i * TEST_PAGE_SIZE)));
    }
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_013: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call item_dispose_func for each element that is still allocated. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_014: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall free all pages that are non-NULL. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE_calls_item_dispose_func_for_each_allocated_element)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);
    uint32_t* ptr_first = test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* ptr_other = test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    uint32_t* ptr_third_page = test_allocate(psa, TEST_INDEX_IN_THIRD_PAGE);
    STRICT_EXPECTED_CALL(test_item_dispose_func(ptr_first));
    STRICT_EXPECTED_CALL(test_item_dispose_func(ptr_other));
    STRICT_EXPECTED_CALL(free(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(srw_lock_ll_deinit(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(srw_lock_ll_deinit(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_SECOND_PAGE)));
    STRICT_EXPECTED_CALL(test_item_dispose_func(ptr_third_page));
    STRICT_EXPECTED_CALL(free(TEST_PAGE(psa, TEST_INDEX_IN_THIRD_PAGE)));
    for (uint64_t i = TEST_INDEX_IN_THIRD_PAGE / TEST_PAGE_SIZE; i < TEST_PAGE_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(srw_lock_ll_deinit(TEST_PAGE_LOCK(psa, i * TEST_PAGE_SIZE)));
    }
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_029: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_with_NULL_paged_sparse_array_fails)
{
    //arrange
    uint32_t* ptr;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(NULL, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_030: [ If allocated_ptr is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_with_NULL_allocated_ptr_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, NULL);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, result);
    ASSERT_IS_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_031: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_with_index_equal_to_max_size_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint32_t* ptr;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_MAX_SIZE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_031: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_with_index_greater_than_max_size_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint32_t* ptr;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, UINT64_MAX, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_032: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall compute the page index as index / page_size. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_033: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T). ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_034: [ If the page is not allocated: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_035: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_036: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall acquire the lock of the page slot in exclusive mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_037: [ If the page is still not allocated: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_038: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call malloc_flex to allocate memory for page_size elements followed by the allocation bitmap. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_042: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall store the page pointer in the page slot using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_043: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall increment the use count of the page slot using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_044: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the lock of the page slot. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_045: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall mark the element at index as allocated using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_047: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall store a pointer to the element at index in allocated_ptr and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_in_a_page_that_is_not_allocated_allocates_the_page_while_holding_the_lock_of_the_page_slot)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint32_t* ptr = NULL;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(malloc_flex(TEST_PAGE_BITMAP_SIZE, TEST_PAGE_SIZE, sizeof(uint32_t)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_IS_NOT_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(void_ptr, &TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE)[TEST_INDEX_IN_FIRST_PAGE], ptr);
    ASSERT_IS_TRUE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    for (uint64_t i = 1; i < TEST_PAGE_COUNT; i++)
    {
        ASSERT_IS_NULL(TEST_PAGE(psa, i * TEST_PAGE_SIZE));
        ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, i * TEST_PAGE_SIZE));
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_032: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall compute the page index as index / page_size. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_of_the_last_index_allocates_the_last_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint32_t* ptr = NULL;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_LAST_INDEX)));
    STRICT_EXPECTED_CALL(malloc_flex(TEST_PAGE_BITMAP_SIZE, TEST_PAGE_SIZE, sizeof(uint32_t)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_LAST_INDEX)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_LAST_INDEX, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, &TEST_PAGE(psa, TEST_LAST_INDEX)[TEST_LAST_INDEX % TEST_PAGE_SIZE], ptr);
    ASSERT_IS_TRUE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_LAST_INDEX));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_LAST_INDEX));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_033: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T). ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_045: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall mark the element at index as allocated using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_047: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall store a pointer to the element at index in allocated_ptr and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_in_an_allocated_page_does_not_acquire_any_lock)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* ptr = NULL;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, &TEST_PAGE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE)[TEST_OTHER_INDEX_IN_FIRST_PAGE], ptr);
    ASSERT_IS_TRUE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_IS_TRUE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 2, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_040: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall set all the bits of the allocation bitmap to 0. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_041: [ If new_pages is ZERO_NEW_PAGES, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall set all the items of the page to zero. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_with_DO_NOT_ZERO_NEW_PAGES_initializes_the_bitmap_and_does_not_zero_the_items)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint32_t* ptr = NULL;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(malloc_flex(TEST_PAGE_BITMAP_SIZE, TEST_PAGE_SIZE, sizeof(uint32_t)))
        .SetReturn(test_allocate_page_memory_filled_with_garbage());
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    for (uint64_t i = 0; i < TEST_PAGE_SIZE; i++)
    {
        ASSERT_ARE_EQUAL(bool, i == TEST_INDEX_IN_FIRST_PAGE, TEST_IS_ALLOCATED_IN_BITMAP(psa, i), "index %" PRIu64 "", i);
        ASSERT_ARE_EQUAL(uint32_t, TEST_GARBAGE_ITEM_VALUE, TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE)[i], "index %" PRIu64 "", i);
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_040: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall set all the bits of the allocation bitmap to 0. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_041: [ If new_pages is ZERO_NEW_PAGES, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall set all the items of the page to zero. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_with_ZERO_NEW_PAGES_sets_all_the_items_of_a_new_page_to_zero)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_AND_ZERO_PAGES_UINT32) psa = test_create_keep_and_zero_pages();
    uint32_t* ptr = NULL;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(malloc_flex(TEST_PAGE_BITMAP_SIZE, TEST_PAGE_SIZE, sizeof(uint32_t)))
        .SetReturn(test_allocate_page_memory_filled_with_garbage());
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(KEEP_AND_ZERO_PAGES_UINT32)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    for (uint64_t i = 0; i < TEST_PAGE_SIZE; i++)
    {
        ASSERT_ARE_EQUAL(bool, i == TEST_INDEX_IN_FIRST_PAGE, TEST_IS_ALLOCATED_IN_BITMAP(psa, i), "index %" PRIu64 "", i);
        ASSERT_ARE_EQUAL(uint32_t, 0, TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE)[i], "index %" PRIu64 "", i);
    }
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(KEEP_AND_ZERO_PAGES_UINT32)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_041: [ If new_pages is ZERO_NEW_PAGES, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall set all the items of the page to zero. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_with_ZERO_NEW_PAGES_does_not_zero_the_items_of_a_page_that_was_kept)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_AND_ZERO_PAGES_UINT32) psa = test_create_keep_and_zero_pages();
    uint32_t* ptr = test_allocate_keep_and_zero_pages(psa, TEST_INDEX_IN_FIRST_PAGE);
    *ptr = TEST_ITEM_VALUE;
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(KEEP_AND_ZERO_PAGES_UINT32)(psa, TEST_INDEX_IN_FIRST_PAGE);
    umock_c_reset_all_calls();
    uint32_t* ptr_again = NULL;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(KEEP_AND_ZERO_PAGES_UINT32)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr_again);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, ptr, ptr_again);
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, *ptr_again);
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(KEEP_AND_ZERO_PAGES_UINT32)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [ If malloc_flex fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the lock of the page slot and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/
TEST_FUNCTION(when_malloc_flex_fails_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_releases_the_lock_of_the_page_slot_and_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint32_t* ptr = NULL;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(malloc_flex(TEST_PAGE_BITMAP_SIZE, TEST_PAGE_SIZE, sizeof(uint32_t)))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ERROR, result);
    ASSERT_IS_NULL(ptr);
    ASSERT_IS_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_037: [ If the page is still not allocated: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_043: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall increment the use count of the page slot using interlocked operations. ]*/
TEST_FUNCTION(when_another_thread_allocates_the_page_before_the_lock_is_acquired_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_uses_that_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint32_t* ptr = NULL;
    setup_concurrent_call_on_acquire_exclusive(TEST_CONCURRENT_CALL_ALLOCATE, psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    /* the other thread allocates the page */
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(malloc_flex(TEST_PAGE_BITMAP_SIZE, TEST_PAGE_SIZE, sizeof(uint32_t)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    /* back to the call under test */
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_IS_TRUE(g_concurrent_call.was_called);
    ASSERT_ARE_EQUAL(void_ptr, &TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE)[TEST_INDEX_IN_FIRST_PAGE], ptr);
    ASSERT_IS_TRUE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_IS_TRUE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 2, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_017: [ If the use count is greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING, THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall return NULL. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_034: [ If the page is not allocated: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_035: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_036: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall acquire the lock of the page slot in exclusive mode. ]*/
TEST_FUNCTION(when_the_page_is_being_freed_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_does_not_use_it_and_allocates_a_new_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* page_being_freed = TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE);
    /* another thread released the element and is freeing the page: it changed the use count from 0 to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING */
    ASSERT_IS_TRUE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TEST_AND_CLEAR_ALLOCATED(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_BITMAP(page_being_freed, TEST_PAGE_SIZE), TEST_INDEX_IN_FIRST_PAGE));
    (void)interlocked_exchange_64(&TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE), THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING);
    /* the other thread finishes freeing the page while holding the lock of the page slot */
    setup_concurrent_call_on_acquire_exclusive(TEST_CONCURRENT_CALL_FINISH_FREEING_PAGE, psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* ptr = NULL;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(malloc_flex(TEST_PAGE_BITMAP_SIZE, TEST_PAGE_SIZE, sizeof(uint32_t)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_IS_TRUE(g_concurrent_call.was_called);
    ASSERT_IS_NOT_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_NOT_EQUAL(void_ptr, page_being_freed, TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(void_ptr, &TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE)[TEST_OTHER_INDEX_IN_FIRST_PAGE], ptr);
    ASSERT_IS_TRUE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    /* the page that was being freed was not changed */
    ASSERT_IS_FALSE(THREAD_SAFE_PAGED_SPARSE_ARRAY_IS_ALLOCATED(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_BITMAP(page_being_freed, TEST_PAGE_SIZE), TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    real_gballoc_hl_free(page_being_freed);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_045: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall mark the element at index as allocated using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_046: [ If the element at index was already marked as allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 and return PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_of_an_allocated_element_fails_and_ends_the_use_of_the_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint32_t* ptr = test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* ptr_again = NULL;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr_again);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED, result);
    ASSERT_IS_NULL(ptr_again);
    ASSERT_IS_TRUE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(void_ptr, &TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE)[TEST_INDEX_IN_FIRST_PAGE], ptr);
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_034: [ If the page is not allocated: ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_after_the_page_was_freed_allocates_the_page_again)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);
    ASSERT_IS_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    umock_c_reset_all_calls();
    uint32_t* ptr = NULL;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(malloc_flex(TEST_PAGE_BITMAP_SIZE, TEST_PAGE_SIZE, sizeof(uint32_t)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, &TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE)[TEST_INDEX_IN_FIRST_PAGE], ptr);
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_034: [ If the page is not allocated: ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_after_FREE_EMPTY_PAGES_allocates_the_page_again)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_AND_ZERO_PAGES_UINT32) psa = test_create_keep_and_zero_pages_with_empty_page(TEST_INDEX_IN_FIRST_PAGE);
    ASSERT_ARE_EQUAL(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(KEEP_AND_ZERO_PAGES_UINT32)(psa, 0, TEST_LAST_INDEX));
    ASSERT_IS_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    umock_c_reset_all_calls();
    uint32_t* ptr = NULL;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(malloc_flex(TEST_PAGE_BITMAP_SIZE, TEST_PAGE_SIZE, sizeof(uint32_t)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(KEEP_AND_ZERO_PAGES_UINT32)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, &TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE)[TEST_INDEX_IN_FIRST_PAGE], ptr);
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(KEEP_AND_ZERO_PAGES_UINT32)(&psa, NULL);
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_048: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_with_NULL_paged_sparse_array_returns)
{
    //arrange

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(NULL, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_049: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_with_index_equal_to_max_size_returns)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_MAX_SIZE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_050: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall compute the page index as index / page_size. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_051: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T). ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_052: [ If the page is not allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 and return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_in_a_page_that_is_not_allocated_returns_without_acquiring_any_lock)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);
    (void)test_allocate(psa, TEST_INDEX_IN_THIRD_PAGE);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_THIRD_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_053: [ If the element at index is not allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 and return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_an_element_that_is_not_allocated_returns)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);
    (void)test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_NOT_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_IS_TRUE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_016: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall increment the use count of the page slot using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_051: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T). ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_054: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call item_dispose_func for the element being released before marking the element as not allocated. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_calls_item_dispose_func_while_using_the_page_before_marking_the_element_as_not_allocated)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);
    uint32_t* ptr = test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    setup_dispose_observer(psa, TEST_INDEX_IN_FIRST_PAGE, false);
    STRICT_EXPECTED_CALL(test_item_dispose_func(ptr));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_TRUE(g_dispose_observer.was_called);
    ASSERT_IS_TRUE(g_dispose_observer.was_allocated_when_called);
    /* 2 allocated elements and the call to RELEASE */
    ASSERT_ARE_EQUAL(int64_t, 3, g_dispose_observer.use_count_when_called);
    ASSERT_IS_FALSE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    g_dispose_observer.psa = NULL;
    umock_c_reset_all_calls();
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_019: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) shall subtract ended_use_count from the use count of the page slot using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_055: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall mark the element at index as not allocated using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_056: [ If the element was still marked as allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 2. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_an_element_when_the_page_has_other_allocated_elements_does_not_acquire_any_lock)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_NOT_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_IS_FALSE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_IS_TRUE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_020: [ If empty_pages is AUTO_FREE_EMPTY_PAGES, the use count became 0 and the page is allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T). ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_021: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall acquire the lock of the page slot in exclusive mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_023: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall change the use count of the page slot from 0 to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall set the page pointer of the page slot to NULL using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_026: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall subtract THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING from the use count of the page slot using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_027: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release the lock of the page slot. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_028: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall free the page. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_056: [ If the element was still marked as allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 2. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_the_last_element_frees_the_page_after_releasing_the_lock_of_the_page_slot)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* page = TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(free(page));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_020: [ If empty_pages is AUTO_FREE_EMPTY_PAGES, the use count became 0 and the page is allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T). ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_the_last_element_with_KEEP_EMPTY_PAGES_keeps_the_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_AND_ZERO_PAGES_UINT32) psa = test_create_keep_and_zero_pages();
    (void)test_allocate_keep_and_zero_pages(psa, TEST_INDEX_IN_FIRST_PAGE);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(KEEP_AND_ZERO_PAGES_UINT32)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_NOT_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_IS_FALSE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(KEEP_AND_ZERO_PAGES_UINT32)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_055: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall mark the element at index as not allocated using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_057: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/
TEST_FUNCTION(when_another_caller_releases_the_same_element_concurrently_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_ends_only_the_use_of_the_call)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);
    uint32_t* ptr = test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    setup_dispose_observer(psa, TEST_INDEX_IN_FIRST_PAGE, true);
    STRICT_EXPECTED_CALL(test_item_dispose_func(ptr));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_TRUE(g_dispose_observer.was_called);
    ASSERT_IS_NOT_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_IS_FALSE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_IS_TRUE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    g_dispose_observer.psa = NULL;
    umock_c_reset_all_calls();
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_017: [ If the use count is greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING, THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall return NULL. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_052: [ If the page is not allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 and return. ]*/
TEST_FUNCTION(when_the_page_is_being_freed_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_does_not_access_the_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    /* the bit of the element is still set, so accessing the page would call item_dispose_func */
    (void)interlocked_exchange_64(&TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE), THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_TRUE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    /* restore the use count of the page: the use of its 1 allocated element */
    (void)interlocked_exchange_64(&TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE), 1);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_023: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall change the use count of the page slot from 0 to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_024: [ If the use count was not 0, THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release the lock of the page slot and return. ]*/
TEST_FUNCTION(when_another_call_begins_to_use_the_page_before_the_lock_is_acquired_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_does_not_free_the_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    setup_concurrent_call_on_acquire_exclusive(TEST_CONCURRENT_CALL_BEGIN_PAGE_USE, psa, TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_TRUE(g_concurrent_call.was_called);
    ASSERT_IS_NOT_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_IS_FALSE(TEST_IS_ALLOCATED_IN_BITMAP(psa, TEST_INDEX_IN_FIRST_PAGE));
    /* the use of the other call */
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    (void)interlocked_decrement_64(&TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_022: [ If the page is not allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release the lock of the page slot and return. ]*/
TEST_FUNCTION(when_another_thread_frees_the_page_before_the_lock_is_acquired_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_does_not_free_it_again)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* page = TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE);
    setup_concurrent_call_on_acquire_exclusive(TEST_CONCURRENT_CALL_FREE_EMPTY_PAGES, psa, TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    /* the other thread frees the page */
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(free(page));
    /* back to the call under test */
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_IS_TRUE(g_concurrent_call.was_called);
    ASSERT_IS_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_058: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_with_NULL_paged_sparse_array_fails)
{
    //arrange
    uint32_t* item;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(NULL, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_059: [ If item is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_with_NULL_item_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, NULL);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_060: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_with_index_equal_to_max_size_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint32_t* item;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_MAX_SIZE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_020: [ If empty_pages is AUTO_FREE_EMPTY_PAGES, the use count became 0 and the page is allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T). ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_061: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall compute the page index as index / page_size. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T). ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_063: [ If the page is not allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 and return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_in_a_page_that_is_not_allocated_returns_NOT_ALLOCATED_without_acquiring_any_lock)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint32_t* item = NULL;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_IS_NULL(item);
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_064: [ If the element at index is not allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 and return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_of_an_element_that_is_not_allocated_returns_NOT_ALLOCATED)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    (void)test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    uint32_t* item = NULL;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_IS_NULL(item);
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_018: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall return the page pointer of the page slot, read using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_019: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) shall subtract ended_use_count from the use count of the page slot using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_065: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall store in item a pointer to the element at index. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_066: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 and return PAGED_SPARSE_ARRAY_GET_OK. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_returns_the_element_without_acquiring_any_lock)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint32_t* ptr = test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    *ptr = TEST_ITEM_VALUE;
    uint32_t* item = NULL;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, ptr, item);
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, *item);
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_061: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall compute the page index as index / page_size. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_of_the_last_index_returns_the_element)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint32_t* ptr = test_allocate(psa, TEST_LAST_INDEX);
    uint32_t* item = NULL;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_LAST_INDEX, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, ptr, item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_017: [ If the use count is greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING, THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall return NULL. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_063: [ If the page is not allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 and return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
TEST_FUNCTION(when_the_page_is_being_freed_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_returns_NOT_ALLOCATED_without_accessing_the_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    /* the bit of the element is still set, so accessing the page would return the element */
    (void)interlocked_exchange_64(&TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE), THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING);
    uint32_t* item = NULL;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_IS_NULL(item);
    ASSERT_ARE_EQUAL(int64_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    /* restore the use count of the page: the use of its 1 allocated element */
    (void)interlocked_exchange_64(&TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE), 1);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_067: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T) shall fail and return THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_with_NULL_paged_sparse_array_fails)
{
    //arrange

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(uint32_t)(NULL, 0, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_068: [ If start_index is greater than end_index, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T) shall fail and return THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_with_start_index_greater_than_end_index_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_AND_ZERO_PAGES_UINT32) psa = test_create_keep_and_zero_pages_with_empty_page(TEST_INDEX_IN_FIRST_PAGE);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(KEEP_AND_ZERO_PAGES_UINT32)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS, result);
    ASSERT_IS_NOT_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(KEEP_AND_ZERO_PAGES_UINT32)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_069: [ If end_index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T) shall fail and return THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_with_end_index_equal_to_max_size_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_AND_ZERO_PAGES_UINT32) psa = test_create_keep_and_zero_pages_with_empty_page(TEST_INDEX_IN_FIRST_PAGE);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(KEEP_AND_ZERO_PAGES_UINT32)(psa, 0, TEST_MAX_SIZE);

    //assert
    ASSERT_ARE_EQUAL(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS, result);
    ASSERT_IS_NOT_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(KEEP_AND_ZERO_PAGES_UINT32)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_021: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall acquire the lock of the page slot in exclusive mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_022: [ If the page is not allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release the lock of the page slot and return. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_023: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall change the use count of the page slot from 0 to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_024: [ If the use count was not 0, THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release the lock of the page slot and return. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall set the page pointer of the page slot to NULL using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_026: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall subtract THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING from the use count of the page slot using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_027: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release the lock of the page slot. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_028: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall free the page. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_070: [ For each page from start_index / page_size to end_index / page_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T). ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_071: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T) shall succeed and return THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_frees_the_empty_pages_of_the_range_and_skips_the_others)
{
    //arrange
    /* page 0: empty, page 1: not allocated, page 2: empty, page 3: has an allocated element, page 4: empty but outside of the range */
    THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_AND_ZERO_PAGES_UINT32) psa = test_create_keep_and_zero_pages_with_empty_page(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate_keep_and_zero_pages(psa, TEST_INDEX_IN_THIRD_PAGE);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(KEEP_AND_ZERO_PAGES_UINT32)(psa, TEST_INDEX_IN_THIRD_PAGE);
    (void)test_allocate_keep_and_zero_pages(psa, TEST_INDEX_IN_FOURTH_PAGE);
    (void)test_allocate_keep_and_zero_pages(psa, TEST_INDEX_IN_FIFTH_PAGE);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(KEEP_AND_ZERO_PAGES_UINT32)(psa, TEST_INDEX_IN_FIFTH_PAGE);
    umock_c_reset_all_calls();
    uint32_t* first_page = TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* third_page = TEST_PAGE(psa, TEST_INDEX_IN_THIRD_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(free(first_page));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_SECOND_PAGE)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_SECOND_PAGE)));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_THIRD_PAGE)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_THIRD_PAGE)));
    STRICT_EXPECTED_CALL(free(third_page));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FOURTH_PAGE)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FOURTH_PAGE)));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(KEEP_AND_ZERO_PAGES_UINT32)(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_INDEX_IN_FOURTH_PAGE);

    //assert
    ASSERT_ARE_EQUAL(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK, result);
    ASSERT_IS_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_IS_NULL(TEST_PAGE(psa, TEST_INDEX_IN_SECOND_PAGE));
    ASSERT_IS_NULL(TEST_PAGE(psa, TEST_INDEX_IN_THIRD_PAGE));
    ASSERT_IS_NOT_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FOURTH_PAGE));
    ASSERT_IS_NOT_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIFTH_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, TEST_INDEX_IN_SECOND_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, TEST_INDEX_IN_THIRD_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FOURTH_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(KEEP_AND_ZERO_PAGES_UINT32)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_070: [ For each page from start_index / page_size to end_index / page_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T). ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_with_start_index_equal_to_end_index_frees_only_the_page_of_that_index)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_AND_ZERO_PAGES_UINT32) psa = test_create_keep_and_zero_pages_with_empty_page(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate_keep_and_zero_pages(psa, TEST_INDEX_IN_SECOND_PAGE);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(KEEP_AND_ZERO_PAGES_UINT32)(psa, TEST_INDEX_IN_SECOND_PAGE);
    umock_c_reset_all_calls();
    uint32_t* first_page = TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(free(first_page));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(KEEP_AND_ZERO_PAGES_UINT32)(psa, TEST_LAST_INDEX_IN_FIRST_PAGE, TEST_LAST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK, result);
    ASSERT_IS_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_IS_NOT_NULL(TEST_PAGE(psa, TEST_INDEX_IN_SECOND_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(KEEP_AND_ZERO_PAGES_UINT32)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_069: [ If end_index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T) shall fail and return THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_070: [ For each page from start_index / page_size to end_index / page_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T). ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_with_end_index_equal_to_the_last_index_frees_the_last_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_AND_ZERO_PAGES_UINT32) psa = test_create_keep_and_zero_pages_with_empty_page(TEST_LAST_INDEX);
    uint32_t* last_page = TEST_PAGE(psa, TEST_LAST_INDEX);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_LAST_INDEX)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_LAST_INDEX)));
    STRICT_EXPECTED_CALL(free(last_page));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(KEEP_AND_ZERO_PAGES_UINT32)(psa, TEST_LAST_INDEX, TEST_LAST_INDEX);

    //assert
    ASSERT_ARE_EQUAL(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK, result);
    ASSERT_IS_NULL(TEST_PAGE(psa, TEST_LAST_INDEX));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(KEEP_AND_ZERO_PAGES_UINT32)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_024: [ If the use count was not 0, THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release the lock of the page slot and return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_does_not_free_a_page_that_another_call_uses)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_AND_ZERO_PAGES_UINT32) psa = test_create_keep_and_zero_pages_with_empty_page(TEST_INDEX_IN_FIRST_PAGE);
    setup_concurrent_call_on_acquire_exclusive(TEST_CONCURRENT_CALL_BEGIN_PAGE_USE, (THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t))(const void*)psa, TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_INDEX_IN_FIRST_PAGE)));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(KEEP_AND_ZERO_PAGES_UINT32)(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK, result);
    ASSERT_IS_TRUE(g_concurrent_call.was_called);
    ASSERT_IS_NOT_NULL(TEST_PAGE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    (void)interlocked_decrement_64(&TEST_USE_COUNT(psa, TEST_INDEX_IN_FIRST_PAGE));
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(KEEP_AND_ZERO_PAGES_UINT32)(&psa, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
