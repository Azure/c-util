// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "thread_safe_paged_sparse_array_ut_pch.h"

THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(uint32_t)
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(uint32_t));
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(uint32_t));

THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(uint32_t);
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(uint32_t);

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

MU_DEFINE_ENUM_STRINGS(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES)

TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_RESULT_VALUES)

#define TEST_MAX_SIZE 100
#define TEST_PAGE_SIZE 16
#define TEST_PAGE_COUNT 7 /*(TEST_MAX_SIZE + TEST_PAGE_SIZE - 1) / TEST_PAGE_SIZE*/
#define TEST_LAST_PAGE_INDEX (TEST_PAGE_COUNT - 1)
#define TEST_LAST_PAGE_INDEX_COUNT 4 /*TEST_MAX_SIZE - TEST_LAST_PAGE_INDEX * TEST_PAGE_SIZE*/
#define TEST_INDEX_IN_FIRST_PAGE 5
#define TEST_OTHER_INDEX_IN_FIRST_PAGE 10
#define TEST_INDEX_ALLOCATED_DIRECTLY_IN_FIRST_PAGE 3
#define TEST_INDEX_IN_THIRD_PAGE 40
#define TEST_LAST_INDEX (TEST_MAX_SIZE - 1)
#define TEST_ITEM_VALUE 42

#define TEST_SLOT_TYPE THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(uint32_t)
#define TEST_PAGE_INDEX(index) ((index) / TEST_PAGE_SIZE)
#define TEST_ARRAY(psa) ((THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(uint32_t)*)(psa))
#define TEST_PAGE_GUARD(psa, page_index) (&TEST_ARRAY(psa)->page_guards[(page_index)])
#define TEST_PAGE_LOCK(psa, page_index) (&TEST_PAGE_GUARD(psa, page_index)->lock)
#define TEST_USE_COUNT(psa, page_index) interlocked_add_64(&TEST_PAGE_GUARD(psa, page_index)->use_count, 0)
#define TEST_PAGE_STATE(psa, page_index) interlocked_add(&TEST_PAGE_GUARD(psa, page_index)->page_state, 0)
#define TEST_WRAPPED_PAGE(psa, page_index) ((psa)->wrapped_array->pages[(page_index)])
#define TEST_SLOT(psa, index) (&TEST_WRAPPED_PAGE(psa, TEST_PAGE_INDEX(index))->items[(index) % TEST_PAGE_SIZE])
#define TEST_SLOT_STATE(psa, index) interlocked_add(&TEST_SLOT(psa, index)->state, 0)

#define TEST_BEGIN_PAGE_USE THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_BEGIN_PAGE_USE_INTERNAL_NAME(uint32_t)
#define TEST_END_PAGE_USE THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(uint32_t)
#define TEST_TRY_FREE_PAGE THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TRY_FREE_PAGE_INTERNAL_NAME(uint32_t)
#define TEST_ALLOCATE_PAGE THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(uint32_t)

/*used by the tests that check the state of the slot when item_dispose_func is called*/
static struct
{
    const TEST_SLOT_TYPE* slot;
    int32_t state_when_called;
} g_dispose_observer;

MOCK_FUNCTION_WITH_CODE(, void, test_item_dispose_func, uint32_t*, item)
    if (g_dispose_observer.slot == NULL)
    {
        /* do nothing, the test does not observe the state of the slot */
    }
    else
    {
        g_dispose_observer.state_when_called = interlocked_add((volatile_atomic int32_t*)&g_dispose_observer.slot->state, 0);
    }
MOCK_FUNCTION_END()

/*used by the tests where the page is being freed by another call when ALLOCATE needs to allocate it*/
static THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD* g_page_guard_freed_while_waiting;

static INTERLOCKED_HL_RESULT hook_InterlockedHL_WaitForNotValue(int32_t volatile_atomic* address_to_check, int32_t value_to_wait, uint32_t timeout_ms)
{
    if (g_page_guard_freed_while_waiting == NULL)
    {
        /* do nothing, no other call is freeing the page */
    }
    else
    {
        /* the other call finishes freeing the page */
        (void)interlocked_add_64(&g_page_guard_freed_while_waiting->use_count, -THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING);
        (void)interlocked_exchange(&g_page_guard_freed_while_waiting->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED);
        g_page_guard_freed_while_waiting = NULL;
    }
    return real_InterlockedHL_WaitForNotValue(address_to_check, value_to_wait, timeout_ms);
}

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

/*marks the element at index as not allocated in the wrapped paged sparse array, behind the back of the thread safe paged sparse array*/
static void test_release_in_wrapped_array(THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa, uint64_t index)
{
    PAGED_SPARSE_ARRAY_RELEASE(TEST_SLOT_TYPE)(psa->wrapped_array, index);
    umock_c_reset_all_calls();
}

/*marks the element at index as allocated again in the wrapped paged sparse array, with a slot that is not allocated*/
static void test_allocate_in_wrapped_array(THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa, uint64_t index)
{
    TEST_SLOT_TYPE* slot;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(TEST_SLOT_TYPE)(psa->wrapped_array, index, &slot));
    (void)interlocked_exchange(&slot->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED);
    umock_c_reset_all_calls();
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

    REGISTER_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_SRW_LOCK_LL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_HL_GLOBAL_MOCK_HOOK();
    REGISTER_SYNC_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_HOOK(InterlockedHL_WaitForNotValue, hook_InterlockedHL_WaitForNotValue);

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
    (void)memset(&g_dispose_observer, 0, sizeof(g_dispose_observer));
    g_page_guard_freed_while_waiting = NULL;

    umock_c_negative_tests_init();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_001: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call PAGED_SPARSE_ARRAY_CREATE(T) with max_size, page_size and NULL as item_dispose_func to create the paged sparse array of slots. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_003: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX to allocate memory for the thread safe paged sparse array with one page guard for each page of the paged sparse array. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_004: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall set the use count of each page guard to 0 and the page state of each page guard to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_005: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call srw_lock_ll_init to initialize the lock of each page guard. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_007: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store item_dispose_func in the thread safe paged sparse array. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_008: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store the paged sparse array in the thread safe paged sparse array. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_009: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_succeeds)
{
    //arrange
    SRW_LOCK_LL* initialized_locks[TEST_PAGE_COUNT];
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(TEST_SLOT_TYPE)*)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD)));
    for (uint64_t i = 0; i < TEST_PAGE_COUNT; i++)
    {
        STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG))
            .CaptureArgumentValue_srw_lock_ll(&initialized_locks[i]);
    }

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, test_item_dispose_func);

    //assert
    ASSERT_IS_NOT_NULL(psa);
    ASSERT_IS_NOT_NULL(psa->wrapped_array);
    ASSERT_ARE_EQUAL(uint64_t, TEST_MAX_SIZE, psa->wrapped_array->max_size);
    ASSERT_ARE_EQUAL(uint64_t, TEST_PAGE_SIZE, psa->wrapped_array->page_size);
    ASSERT_ARE_EQUAL(uint64_t, TEST_PAGE_COUNT, psa->wrapped_array->page_count);
    ASSERT_IS_NULL(psa->wrapped_array->item_dispose_func);
    ASSERT_IS_TRUE(psa->item_dispose_func == test_item_dispose_func);
    for (uint64_t i = 0; i < TEST_PAGE_COUNT; i++)
    {
        ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, i));
        ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED, TEST_PAGE_STATE(psa, i));
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
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(0, TEST_PAGE_SIZE, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [ If there are any other errors: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_082: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall release the paged sparse array. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_083: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(when_THANDLE_MALLOC_FLEX_fails_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_releases_the_paged_sparse_array_and_fails)
{
    //arrange
    void* wrapped_array_memory;
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(TEST_SLOT_TYPE)*)))
        .CaptureReturn(&wrapped_array_memory);
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD)))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&wrapped_array_memory);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_006: [ If srw_lock_ll_init fails: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_080: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call srw_lock_ll_deinit for each lock that was initialized. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_081: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall free the memory of the thread safe paged sparse array. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [ If there are any other errors: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_082: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall release the paged sparse array. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_083: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(when_srw_lock_ll_init_fails_for_the_third_lock_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_deinitializes_the_initialized_locks_and_fails)
{
    //arrange
    void* wrapped_array_memory;
    void* thread_safe_array_memory;
    SRW_LOCK_LL* first_lock;
    SRW_LOCK_LL* second_lock;
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(TEST_SLOT_TYPE)*)))
        .CaptureReturn(&wrapped_array_memory);
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD)))
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
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_006: [ If srw_lock_ll_init fails: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_080: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call srw_lock_ll_deinit for each lock that was initialized. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_081: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall free the memory of the thread safe paged sparse array. ]*/
TEST_FUNCTION(when_srw_lock_ll_init_fails_for_the_first_lock_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_frees_the_arrays_and_fails)
{
    //arrange
    void* wrapped_array_memory;
    void* thread_safe_array_memory;
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(TEST_SLOT_TYPE)*)))
        .CaptureReturn(&wrapped_array_memory);
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD)))
        .CaptureReturn(&thread_safe_array_memory);
    STRICT_EXPECTED_CALL(srw_lock_ll_init(IGNORED_ARG))
        .SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&thread_safe_array_memory);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG))
        .ValidateArgumentValue_ptr(&wrapped_array_memory);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(uint32_t)(TEST_MAX_SIZE, TEST_PAGE_SIZE, NULL);

    //assert
    ASSERT_IS_NULL(psa);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [ If there are any other errors: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_082: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall release the paged sparse array. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_083: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_fails)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(TEST_SLOT_TYPE)*)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD)));
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

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_011: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE_with_NULL_returns)
{
    //arrange

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_NAME(uint32_t)(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_012: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call item_dispose_func with a pointer to the item of each slot whose state is THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED in the pages whose page state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_013: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call srw_lock_ll_deinit for the lock of each page guard. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_014: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall release the paged sparse array. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE_disposes_the_allocated_items_deinitializes_the_locks_and_releases_the_paged_sparse_array)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);
    uint32_t* item_in_first_page = test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* item_in_third_page = test_allocate(psa, TEST_INDEX_IN_THIRD_PAGE);
    void* first_page = TEST_WRAPPED_PAGE(psa, TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE));
    void* third_page = TEST_WRAPPED_PAGE(psa, TEST_PAGE_INDEX(TEST_INDEX_IN_THIRD_PAGE));
    for (uint64_t i = 0; i < TEST_PAGE_COUNT; i++)
    {
        if (i == TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE))
        {
            STRICT_EXPECTED_CALL(test_item_dispose_func(item_in_first_page));
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

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_012: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call item_dispose_func with a pointer to the item of each slot whose state is THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED in the pages whose page state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE_with_NULL_item_dispose_func_does_not_dispose_the_items)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
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
    uint32_t* ptr = NULL;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(NULL, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, result);
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_042: [ If allocated_ptr is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_with_NULL_allocated_ptr_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, NULL);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_043: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_with_index_equal_to_max_size_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint32_t* ptr = NULL;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_MAX_SIZE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, result);
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_045: [ If THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) returns false: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_046: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_047: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T). ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_030: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall acquire the lock of the page guard in exclusive mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_033: [ If the page state of the page guard is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_034: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call PAGED_SPARSE_ARRAY_ALLOCATE(T) for each index of the page. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_035: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall set the state of each slot of the page to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_037: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall increment the use count of the page guard using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_038: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall set the page state of the page guard to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [ In all cases, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall release the lock of the page guard before returning. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_040: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall succeed and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_in_a_page_that_is_not_allocated_allocates_the_page_while_holding_the_lock_of_the_page_guard)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_THIRD_PAGE);
    uint32_t* ptr = NULL;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)); /*the page of the paged sparse array*/
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_THIRD_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, &TEST_SLOT(psa, TEST_INDEX_IN_THIRD_PAGE)->item, ptr);
    ASSERT_ARE_EQUAL(uint64_t, TEST_PAGE_SIZE, TEST_WRAPPED_PAGE(psa, page_index)->allocated_count);
    for (uint64_t index = page_index * TEST_PAGE_SIZE; index < (page_index + 1) * TEST_PAGE_SIZE; index++)
    {
        ASSERT_ARE_EQUAL(int32_t, (index == TEST_INDEX_IN_THIRD_PAGE) ? THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED : THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, TEST_SLOT_STATE(psa, index));
    }
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_034: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call PAGED_SPARSE_ARRAY_ALLOCATE(T) for each index of the page. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_of_the_last_index_allocates_only_the_indexes_of_the_last_page_that_are_lower_than_max_size)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint32_t* ptr = NULL;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_LAST_PAGE_INDEX)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)); /*the page of the paged sparse array*/
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_LAST_PAGE_INDEX)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_LAST_INDEX, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, &TEST_SLOT(psa, TEST_LAST_INDEX)->item, ptr);
    ASSERT_ARE_EQUAL(uint64_t, TEST_LAST_PAGE_INDEX_COUNT, TEST_WRAPPED_PAGE(psa, TEST_LAST_PAGE_INDEX)->allocated_count);

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_044: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) for the page with index index / page_size. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_049: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call PAGED_SPARSE_ARRAY_GET(T) with the paged sparse array and index to get the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_051: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall change the state of the slot from THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED using interlocked_compare_exchange. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_053: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall store a pointer to the item of the slot in allocated_ptr and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/
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
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, &TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE)->item, ptr);
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 2, TEST_USE_COUNT(psa, TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE)));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_036: [ If PAGED_SPARSE_ARRAY_ALLOCATE(T) fails: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_078: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call PAGED_SPARSE_ARRAY_RELEASE(T) for each index of the page that was allocated. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_079: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_048: [ If THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/
TEST_FUNCTION(when_allocating_the_page_fails_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_releases_the_lock_of_the_page_guard_and_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* ptr = NULL;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)) /*the page of the paged sparse array*/
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ERROR, result);
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(TEST_WRAPPED_PAGE(psa, page_index));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_036: [ If PAGED_SPARSE_ARRAY_ALLOCATE(T) fails: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_078: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call PAGED_SPARSE_ARRAY_RELEASE(T) for each index of the page that was allocated. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_079: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/
TEST_FUNCTION(when_PAGED_SPARSE_ARRAY_ALLOCATE_fails_for_an_index_after_the_first_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_releases_the_indexes_allocated_before_and_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* ptr = NULL;
    test_allocate_in_wrapped_array(psa, TEST_INDEX_ALLOCATED_DIRECTLY_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ERROR, result);
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    /* only the index allocated directly in the paged sparse array is still allocated */
    ASSERT_ARE_EQUAL(uint64_t, 1, TEST_WRAPPED_PAGE(psa, page_index)->allocated_count);
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    test_release_in_wrapped_array(psa, TEST_INDEX_ALLOCATED_DIRECTLY_IN_FIRST_PAGE);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_031: [ While the page state of the page guard is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call InterlockedHL_WaitForNotValue to wait for the page state to change. ]*/
TEST_FUNCTION(when_the_page_is_being_freed_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_waits_for_the_page_to_be_freed_and_allocates_it)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* ptr = NULL;
    /* another call is freeing the page */
    (void)interlocked_exchange_64(&TEST_PAGE_GUARD(psa, page_index)->use_count, THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING);
    (void)interlocked_exchange(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING);
    g_page_guard_freed_while_waiting = TEST_PAGE_GUARD(psa, page_index);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForNotValue(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING, UINT32_MAX));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)); /*the page of the paged sparse array*/
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, &TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->item, ptr);
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_032: [ If InterlockedHL_WaitForNotValue fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/
TEST_FUNCTION(when_InterlockedHL_WaitForNotValue_fails_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_releases_the_lock_of_the_page_guard_and_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* ptr = NULL;
    /* another call is freeing the page */
    (void)interlocked_exchange_64(&TEST_PAGE_GUARD(psa, page_index)->use_count, THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING);
    (void)interlocked_exchange(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForNotValue(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING, UINT32_MAX))
        .SetReturn(INTERLOCKED_HL_ERROR);
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ERROR, result);
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING, TEST_USE_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING, TEST_PAGE_STATE(psa, page_index));

    //clean
    (void)interlocked_exchange_64(&TEST_PAGE_GUARD(psa, page_index)->use_count, 0);
    (void)interlocked_exchange(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_037: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall increment the use count of the page guard using interlocked operations. ]*/
TEST_FUNCTION(when_another_call_allocated_the_page_before_the_lock_is_acquired_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE_only_begins_a_use_of_the_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    void* page = TEST_WRAPPED_PAGE(psa, page_index);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = TEST_ALLOCATE_PAGE(TEST_ARRAY(psa), page_index);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, page, TEST_WRAPPED_PAGE(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, TEST_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 2, TEST_USE_COUNT(psa, page_index));

    //clean
    TEST_END_PAGE_USE(TEST_ARRAY(psa), page_index, 1);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_050: [ If PAGED_SPARSE_ARRAY_GET(T) fails: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_084: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_085: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/
TEST_FUNCTION(when_PAGED_SPARSE_ARRAY_GET_fails_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ends_the_use_of_the_page_and_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    test_release_in_wrapped_array(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    uint32_t* ptr = NULL;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ERROR, result);
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));

    //clean
    test_allocate_in_wrapped_array(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_052: [ If the state of the slot was not THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_086: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_087: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_of_an_allocated_element_fails_and_ends_the_use_of_the_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* allocated = test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* ptr = NULL;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED, result);
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, &TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->item, allocated);
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_052: [ If the state of the slot was not THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_086: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_087: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_of_an_element_that_is_being_released_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    (void)interlocked_exchange(&TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE)->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING);
    uint32_t* ptr = NULL;

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &ptr);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED, result);
    ASSERT_IS_NULL(ptr);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));

    //clean
    (void)interlocked_exchange(&TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE)->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED);
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
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_MAX_SIZE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_056: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) for the page with index index / page_size. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_066: [ If THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) returns false, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_in_a_page_that_is_not_allocated_returns)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(TEST_WRAPPED_PAGE(psa, page_index));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_065: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_an_element_that_is_not_allocated_in_an_allocated_page_returns)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_065: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_an_element_that_is_being_released_by_another_call_does_not_dispose_the_item)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    (void)interlocked_exchange(&TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING, TEST_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));

    //clean
    (void)interlocked_exchange(&TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_057: [ If THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) returns true: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_058: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call PAGED_SPARSE_ARRAY_GET(T) with the paged sparse array and index to get the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_060: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall change the state of the slot from THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING using interlocked_compare_exchange. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_061: [ If the state of the slot was THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call item_dispose_func with a pointer to the item of the slot. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_063: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall set the state of the slot to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_064: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 2. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_an_element_disposes_the_item_while_the_slot_is_releasing_and_does_not_free_the_page_that_has_other_elements)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* item = test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    g_dispose_observer.slot = TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(test_item_dispose_func(item));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING, g_dispose_observer.state_when_called);
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, TEST_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call item_dispose_func with a pointer to the item of the slot. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_with_NULL_item_dispose_func_releases_the_element)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_064: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 2. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_020: [ If the use count became 0 and the page state of the page guard is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T). ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_026: [ If the use count was changed to THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_027: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall call PAGED_SPARSE_ARRAY_RELEASE(T) for each index of the page. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_the_last_element_of_a_page_frees_the_page_after_releasing_the_lock_of_the_page_guard)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_THIRD_PAGE);
    uint32_t* item = test_allocate(psa, TEST_INDEX_IN_THIRD_PAGE);
    void* page = TEST_WRAPPED_PAGE(psa, page_index);
    STRICT_EXPECTED_CALL(test_item_dispose_func(item));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(free(page));
    STRICT_EXPECTED_CALL(wake_by_address_all(&TEST_PAGE_GUARD(psa, page_index)->page_state));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_THIRD_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(TEST_WRAPPED_PAGE(psa, page_index));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_027: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall call PAGED_SPARSE_ARRAY_RELEASE(T) for each index of the page. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_the_last_element_of_the_last_page_frees_the_last_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);
    uint32_t* item = test_allocate(psa, TEST_LAST_INDEX);
    void* page = TEST_WRAPPED_PAGE(psa, TEST_LAST_PAGE_INDEX);
    STRICT_EXPECTED_CALL(test_item_dispose_func(item));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_LAST_PAGE_INDEX)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_LAST_PAGE_INDEX)));
    STRICT_EXPECTED_CALL(free(page));
    STRICT_EXPECTED_CALL(wake_by_address_all(&TEST_PAGE_GUARD(psa, TEST_LAST_PAGE_INDEX)->page_state));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_LAST_INDEX);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(TEST_WRAPPED_PAGE(psa, TEST_LAST_PAGE_INDEX));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, TEST_LAST_PAGE_INDEX));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED, TEST_PAGE_STATE(psa, TEST_LAST_PAGE_INDEX));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_059: [ If PAGED_SPARSE_ARRAY_GET(T) fails: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_088: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_089: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/
TEST_FUNCTION(when_PAGED_SPARSE_ARRAY_GET_fails_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_ends_the_use_of_the_page_and_returns)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    test_release_in_wrapped_array(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));

    //clean
    test_allocate_in_wrapped_array(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_067: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_with_NULL_paged_sparse_array_fails)
{
    //arrange
    uint32_t* item = NULL;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(NULL, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, result);
    ASSERT_IS_NULL(item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_068: [ If item is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_with_NULL_item_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, NULL);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_069: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_with_index_equal_to_max_size_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint32_t* item = NULL;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_MAX_SIZE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, result);
    ASSERT_IS_NULL(item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_070: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) for the page with index index / page_size. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_071: [ If THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) returns false, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_076: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 before returning, also when THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) returns false. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_in_a_page_that_is_not_allocated_returns_NOT_ALLOCATED)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* item = NULL;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_IS_NULL(item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_071: [ If THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) returns false, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_in_a_page_that_is_being_freed_returns_NOT_ALLOCATED)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    /* another call is freeing the page and did not yet set the page state to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING */
    int64_t use_count = interlocked_exchange_64(&TEST_PAGE_GUARD(psa, page_index)->use_count, THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING);
    uint32_t* item = NULL;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_IS_NULL(item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING, TEST_USE_COUNT(psa, page_index));

    //clean
    (void)interlocked_exchange_64(&TEST_PAGE_GUARD(psa, page_index)->use_count, use_count);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_073: [ If PAGED_SPARSE_ARRAY_GET(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall return the result of PAGED_SPARSE_ARRAY_GET(T). ]*/
TEST_FUNCTION(when_PAGED_SPARSE_ARRAY_GET_fails_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_returns_its_result)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    test_release_in_wrapped_array(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    uint32_t* item = NULL;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_IS_NULL(item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));

    //clean
    test_allocate_in_wrapped_array(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_074: [ If the state of the slot is not THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_of_an_element_that_is_not_allocated_in_an_allocated_page_returns_NOT_ALLOCATED)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* item = NULL;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_IS_NULL(item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_074: [ If the state of the slot is not THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_of_an_element_that_is_being_released_returns_NOT_ALLOCATED)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    (void)interlocked_exchange(&TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING);
    uint32_t* item = NULL;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_IS_NULL(item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));

    //clean
    (void)interlocked_exchange(&TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_072: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call PAGED_SPARSE_ARRAY_GET(T) with the paged sparse array and index to get the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_075: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall store a pointer to the item of the slot in item and return PAGED_SPARSE_ARRAY_GET_OK. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_076: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 before returning, also when THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) returns false. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_GET_of_an_allocated_element_succeeds_without_acquiring_any_lock)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* allocated = test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    *allocated = TEST_ITEM_VALUE;
    uint32_t* item = NULL;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, allocated, item);
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, *item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_015: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall increment the use count of the page guard using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_017: [ If the page state of the page guard is not THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall return false. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE_of_a_page_that_is_not_allocated_returns_false)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);

    //act
    bool result = TEST_BEGIN_PAGE_USE(TEST_ARRAY(psa), page_index);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));

    //clean
    TEST_END_PAGE_USE(TEST_ARRAY(psa), page_index, 1);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_016: [ If the use count is greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING, THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall return false. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE_of_a_page_that_is_being_freed_returns_false)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, TEST_ALLOCATE_PAGE(TEST_ARRAY(psa), page_index));
    /* another call is freeing the page and did not yet set the page state to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING */
    (void)interlocked_exchange_64(&TEST_PAGE_GUARD(psa, page_index)->use_count, THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING);
    umock_c_reset_all_calls();

    //act
    bool result = TEST_BEGIN_PAGE_USE(TEST_ARRAY(psa), page_index);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING + 1, TEST_USE_COUNT(psa, page_index));

    //clean
    (void)interlocked_exchange_64(&TEST_PAGE_GUARD(psa, page_index)->use_count, 0);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_017: [ If the page state of the page guard is not THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall return false. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE_of_a_page_whose_page_state_is_still_FREEING_returns_false)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    /* another call freed the page and subtracted THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING, but did not yet set the page state to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED */
    (void)interlocked_exchange(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING);

    //act
    bool result = TEST_BEGIN_PAGE_USE(TEST_ARRAY(psa), page_index);

    //assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));

    //clean
    TEST_END_PAGE_USE(TEST_ARRAY(psa), page_index, 1);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    (void)interlocked_exchange(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_018: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall return true. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE_of_an_allocated_page_returns_true)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);

    //act
    bool result = TEST_BEGIN_PAGE_USE(TEST_ARRAY(psa), page_index);

    //assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 2, TEST_USE_COUNT(psa, page_index));

    //clean
    TEST_END_PAGE_USE(TEST_ARRAY(psa), page_index, 1);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_019: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) shall subtract ended_use_count from the use count of the page guard using interlocked operations. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE_of_a_page_that_is_still_used_does_not_free_the_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    ASSERT_IS_TRUE(TEST_BEGIN_PAGE_USE(TEST_ARRAY(psa), page_index));

    //act
    TEST_END_PAGE_USE(TEST_ARRAY(psa), page_index, 1);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));
    ASSERT_IS_NOT_NULL(TEST_WRAPPED_PAGE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_019: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) shall subtract ended_use_count from the use count of the page guard using interlocked operations. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE_of_the_last_use_of_a_page_that_is_not_allocated_does_not_free_the_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    ASSERT_IS_FALSE(TEST_BEGIN_PAGE_USE(TEST_ARRAY(psa), page_index));

    //act
    TEST_END_PAGE_USE(TEST_ARRAY(psa), page_index, 1);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_020: [ If the use count became 0 and the page state of the page guard is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T). ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE_of_the_last_use_of_an_allocated_page_frees_the_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, TEST_ALLOCATE_PAGE(TEST_ARRAY(psa), page_index));
    void* page = TEST_WRAPPED_PAGE(psa, page_index);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(free(page));
    STRICT_EXPECTED_CALL(wake_by_address_all(&TEST_PAGE_GUARD(psa, page_index)->page_state));

    //act
    TEST_END_PAGE_USE(TEST_ARRAY(psa), page_index, 1);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(TEST_WRAPPED_PAGE(psa, page_index));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_021: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall acquire the lock of the page guard in exclusive mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_022: [ If the page state of the page guard is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release the lock of the page guard. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE_of_a_page_that_another_call_freed_does_nothing)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    TEST_TRY_FREE_PAGE(TEST_ARRAY(psa), page_index);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_022: [ If the page state of the page guard is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED: ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE_of_a_page_that_another_call_is_freeing_does_nothing)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    /* another call is freeing the page */
    (void)interlocked_exchange_64(&TEST_PAGE_GUARD(psa, page_index)->use_count, THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING);
    (void)interlocked_exchange(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    TEST_TRY_FREE_PAGE(TEST_ARRAY(psa), page_index);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING, TEST_USE_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING, TEST_PAGE_STATE(psa, page_index));

    //clean
    (void)interlocked_exchange_64(&TEST_PAGE_GUARD(psa, page_index)->use_count, 0);
    (void)interlocked_exchange(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_023: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall change the use count of the page guard from 0 to THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING using interlocked_compare_exchange_64. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE_of_a_page_that_another_call_began_to_use_does_not_free_the_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    TEST_TRY_FREE_PAGE(TEST_ARRAY(psa), page_index);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(TEST_WRAPPED_PAGE(psa, page_index));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_USE_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_023: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall change the use count of the page guard from 0 to THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING using interlocked_compare_exchange_64. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_024: [ If the use count was 0, THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall set the page state of the page guard to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release the lock of the page guard. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_026: [ If the use count was changed to THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_027: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall call PAGED_SPARSE_ARRAY_RELEASE(T) for each index of the page. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_028: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall subtract THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING from the use count of the page guard using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_077: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall set the page state of the page guard to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED using interlocked operations. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_029: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall call wake_by_address_all for the page state of the page guard. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE_of_an_unused_allocated_page_frees_the_page_without_holding_the_lock_of_the_page_guard)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, TEST_ALLOCATE_PAGE(TEST_ARRAY(psa), page_index));
    /* the use that allocated the page ended */
    (void)interlocked_decrement_64(&TEST_PAGE_GUARD(psa, page_index)->use_count);
    void* page = TEST_WRAPPED_PAGE(psa, page_index);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(free(page));
    STRICT_EXPECTED_CALL(wake_by_address_all(&TEST_PAGE_GUARD(psa, page_index)->page_state));

    //act
    TEST_TRY_FREE_PAGE(TEST_ARRAY(psa), page_index);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(TEST_WRAPPED_PAGE(psa, page_index));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_USE_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
