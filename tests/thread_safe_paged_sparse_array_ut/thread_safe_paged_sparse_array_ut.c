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
#define TEST_INDEX_IN_THIRD_PAGE 40
#define TEST_LAST_INDEX (TEST_MAX_SIZE - 1)
#define TEST_ITEM_VALUE 42
#define TEST_OTHER_ITEM_VALUE 43

#define TEST_SLOT_TYPE THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(uint32_t)
#define TEST_PAGE_INDEX(index) ((index) / TEST_PAGE_SIZE)
#define TEST_ARRAY(psa) ((THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(uint32_t)*)(psa))
#define TEST_PAGE_GUARD(psa, page_index) (&TEST_ARRAY(psa)->page_guards[(page_index)])
#define TEST_PAGE_LOCK(psa, page_index) (&TEST_PAGE_GUARD(psa, page_index)->lock)
#define TEST_ELEMENT_COUNT(psa, page_index) interlocked_add_64(&TEST_PAGE_GUARD(psa, page_index)->element_count, 0)
#define TEST_PAGE_STATE(psa, page_index) interlocked_add(&TEST_PAGE_GUARD(psa, page_index)->page_state, 0)
#define TEST_WRAPPED_PAGE(psa, page_index) ((psa)->paged_sparse_array->pages[(page_index)])
#define TEST_SLOT(psa, index) (&TEST_WRAPPED_PAGE(psa, TEST_PAGE_INDEX(index))->items[(index) % TEST_PAGE_SIZE])
#define TEST_SLOT_STATE(psa, index) interlocked_add(&TEST_SLOT(psa, index)->state, 0)
#define TEST_SET_SLOT_STATE(psa, index, new_state) (void)interlocked_exchange(&TEST_SLOT(psa, index)->state, (new_state))

/*the state of a slot that RELEASE changed while no ACQUIRE_ITEM copies the item*/
#define TEST_SLOT_STATE_RELEASING_ALLOCATED (THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING + THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED)

#define TEST_TRY_FREE_PAGE THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TRY_FREE_PAGE_INTERNAL_NAME(uint32_t)
#define TEST_ALLOCATE_PAGE THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(uint32_t)

/*used by the tests that check the state of the slot when item_copy_func and item_dispose_func are called*/
static struct
{
    TEST_SLOT_TYPE* slot;
    int32_t state_when_called;
    /*added to the state of the slot when item_copy_func is called, to start a RELEASE while an ACQUIRE_ITEM copies the item*/
    int32_t state_to_add_when_called;
} g_slot_observer;

static void observe_slot(void)
{
    if (g_slot_observer.slot == NULL)
    {
        /* do nothing, the test does not observe the state of the slot */
    }
    else
    {
        g_slot_observer.state_when_called = interlocked_add(&g_slot_observer.slot->state, 0);
        (void)interlocked_add(&g_slot_observer.slot->state, g_slot_observer.state_to_add_when_called);
    }
}

MOCK_FUNCTION_WITH_CODE(, void, test_item_copy_func, uint32_t*, dst, uint32_t*, src)
    *dst = *src;
    observe_slot();
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, test_item_dispose_func, uint32_t*, item)
    observe_slot();
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
        (void)interlocked_exchange(&g_page_guard_freed_while_waiting->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED);
        g_page_guard_freed_while_waiting = NULL;
    }
    return real_InterlockedHL_WaitForNotValue(address_to_check, value_to_wait, timeout_ms);
}

/*used by the tests where an ACQUIRE_ITEM copies the item when RELEASE needs to dispose it*/
static TEST_SLOT_TYPE* g_slot_copied_while_waiting;

static INTERLOCKED_HL_RESULT hook_InterlockedHL_WaitForValue(int32_t volatile_atomic* address_to_check, int32_t value_to_wait, uint32_t timeout_ms)
{
    if (g_slot_copied_while_waiting == NULL)
    {
        /* do nothing, no ACQUIRE_ITEM copies the item */
    }
    else
    {
        /* the ACQUIRE_ITEM finishes copying the item */
        (void)interlocked_decrement(&g_slot_copied_while_waiting->state);
        g_slot_copied_while_waiting = NULL;
    }
    return real_InterlockedHL_WaitForValue(address_to_check, value_to_wait, timeout_ms);
}

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

/*allocates the element at index with TEST_ITEM_VALUE and returns a pointer to the item of its slot*/
static uint32_t* test_allocate(THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa, uint64_t index)
{
    uint32_t item = TEST_ITEM_VALUE;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, index, &item));
    umock_c_reset_all_calls();
    return &TEST_SLOT(psa, index)->item;
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
    REGISTER_GLOBAL_MOCK_HOOK(InterlockedHL_WaitForValue, hook_InterlockedHL_WaitForValue);

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
    (void)memset(&g_slot_observer, 0, sizeof(g_slot_observer));
    g_page_guard_freed_while_waiting = NULL;
    g_slot_copied_while_waiting = NULL;

    umock_c_negative_tests_init();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_001: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call PAGED_SPARSE_ARRAY_CREATE(T) with max_size, page_size and NULL as item_dispose_func to create the paged sparse array of slots. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_003: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX to allocate result with one page guard in result->page_guards for each page of the paged sparse array. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_004: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall set element_count to 0 and page_state to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED in each page guard of result->page_guards. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_005: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call srw_lock_ll_init to initialize lock in each page guard of result->page_guards. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_007: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store item_dispose_func in result->item_dispose_func. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_110: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store item_copy_func in result->item_copy_func. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_008: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store the paged sparse array in result->paged_sparse_array. ]*/
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
        ASSERT_ARE_EQUAL(int64_t, 0, TEST_ELEMENT_COUNT(psa, i));
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
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD)))
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
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD)))
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

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_007: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store item_dispose_func in result->item_dispose_func. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_110: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store item_copy_func in result->item_copy_func. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_009: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE_with_NULL_item_copy_func_and_NULL_item_dispose_func_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(TEST_SLOT_TYPE)*)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, TEST_PAGE_COUNT, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD)));
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

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_012: [ If paged_sparse_array->item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call paged_sparse_array->item_dispose_func with a pointer to the item of each slot whose state is THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED in the pages whose page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_013: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call srw_lock_ll_deinit for lock in each page guard of paged_sparse_array->page_guards. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_014: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall release paged_sparse_array->paged_sparse_array. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE_disposes_the_allocated_items_deinitializes_the_locks_and_releases_the_paged_sparse_array)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
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

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_012: [ If paged_sparse_array->item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call paged_sparse_array->item_dispose_func with a pointer to the item of each slot whose state is THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED in the pages whose page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE_with_NULL_item_dispose_func_does_not_dispose_the_items)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
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

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_093: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall acquire page_guard->lock in shared mode, where page_guard is the page guard of the page with index index / page_size. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_045: [ If page_guard->page_state is not THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_097: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release page_guard->lock in shared mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_047: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) and return its result. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_030: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall acquire page_guard->lock in exclusive mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_033: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_034: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call PAGED_SPARSE_ARRAY_ALLOCATE(T) on array->paged_sparse_array for each index of the page. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_035: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall set the state of each slot of the page to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_038: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall set page_guard->page_state to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_099: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) and return its result. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [ In all cases, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall release page_guard->lock before returning. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_049: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall call PAGED_SPARSE_ARRAY_GET(T) with array->paged_sparse_array and index to get the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_112: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall change the state of the slot from THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATING using interlocked_compare_exchange. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_114: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall copy item to the item of the slot. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_098: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall increment page_guard->element_count. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_115: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall set the state of the slot to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_116: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall succeed and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_in_a_page_that_is_not_allocated_allocates_the_page_and_the_element_while_holding_the_lock_of_the_page_guard_in_exclusive_mode)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_THIRD_PAGE);
    uint32_t item = TEST_ITEM_VALUE;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)); /*the page of the paged sparse array*/
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_THIRD_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, TEST_SLOT(psa, TEST_INDEX_IN_THIRD_PAGE)->item);
    ASSERT_ARE_EQUAL(uint64_t, TEST_PAGE_SIZE, TEST_WRAPPED_PAGE(psa, page_index)->allocated_count);
    for (uint64_t index = page_index * TEST_PAGE_SIZE; index < (page_index + 1) * TEST_PAGE_SIZE; index++)
    {
        ASSERT_ARE_EQUAL(int32_t, (index == TEST_INDEX_IN_THIRD_PAGE) ? THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED : THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, TEST_SLOT_STATE(psa, index));
    }
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_ELEMENT_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_034: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call PAGED_SPARSE_ARRAY_ALLOCATE(T) on array->paged_sparse_array for each index of the page. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_of_the_last_index_allocates_only_the_indexes_of_the_last_page_that_are_lower_than_max_size)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint32_t item = TEST_ITEM_VALUE;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, TEST_LAST_PAGE_INDEX)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, TEST_LAST_PAGE_INDEX)));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_LAST_PAGE_INDEX)));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)); /*the page of the paged sparse array*/
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_LAST_PAGE_INDEX)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_LAST_INDEX, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, TEST_SLOT(psa, TEST_LAST_INDEX)->item);
    ASSERT_ARE_EQUAL(uint64_t, TEST_LAST_PAGE_INDEX_COUNT, TEST_WRAPPED_PAGE(psa, TEST_LAST_PAGE_INDEX)->allocated_count);

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_093: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall acquire page_guard->lock in shared mode, where page_guard is the page guard of the page with index index / page_size. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_094: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_095: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) and return its result. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_096: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release page_guard->lock in shared mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_049: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall call PAGED_SPARSE_ARRAY_GET(T) with array->paged_sparse_array and index to get the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_112: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall change the state of the slot from THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATING using interlocked_compare_exchange. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_114: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall copy item to the item of the slot. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_098: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall increment page_guard->element_count. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_115: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall set the state of the slot to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_116: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall succeed and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_in_an_allocated_page_copies_the_item_while_holding_the_lock_of_the_page_guard_in_shared_mode)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t item = TEST_OTHER_ITEM_VALUE;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, TEST_OTHER_ITEM_VALUE, TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE)->item);
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 2, TEST_ELEMENT_COUNT(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_112: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall change the state of the slot from THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATING using interlocked_compare_exchange. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_113: [ If array->item_copy_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall call array->item_copy_func with a pointer to the item of the slot as dst and item as src. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_115: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall set the state of the slot to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_calls_item_copy_func_while_the_state_of_the_slot_is_ALLOCATING)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t item = TEST_OTHER_ITEM_VALUE;
    g_slot_observer.slot = TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(test_item_copy_func(&TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE)->item, &item));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATING, g_slot_observer.state_when_called);
    ASSERT_ARE_EQUAL(uint32_t, TEST_OTHER_ITEM_VALUE, TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE)->item);
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 2, TEST_ELEMENT_COUNT(psa, page_index));

    //clean
    g_slot_observer.slot = NULL;
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_036: [ If PAGED_SPARSE_ARRAY_ALLOCATE(T) fails for the first index of the page, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [ In all cases, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall release page_guard->lock before returning. ]*/
TEST_FUNCTION(when_allocating_the_page_fails_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_releases_the_lock_of_the_page_guard_and_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    uint32_t item = TEST_ITEM_VALUE;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));
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
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_ELEMENT_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_031: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call InterlockedHL_WaitForNotValue to wait for page_guard->page_state to change. ]*/
TEST_FUNCTION(when_the_page_is_being_freed_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_waits_for_the_page_to_be_freed_and_allocates_it)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    uint32_t item = TEST_ITEM_VALUE;
    /* another call is freeing the page */
    (void)interlocked_exchange(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING);
    g_page_guard_freed_while_waiting = TEST_PAGE_GUARD(psa, page_index);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForNotValue(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING, UINT32_MAX));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)); /*the page of the paged sparse array*/
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->item);
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_ELEMENT_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_032: [ If InterlockedHL_WaitForNotValue fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/
TEST_FUNCTION(when_InterlockedHL_WaitForNotValue_fails_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_releases_the_lock_of_the_page_guard_and_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    uint32_t item = TEST_ITEM_VALUE;
    /* another call is freeing the page */
    (void)interlocked_exchange(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForNotValue(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING, UINT32_MAX))
        .SetReturn(INTERLOCKED_HL_ERROR);
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_ELEMENT_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING, TEST_PAGE_STATE(psa, page_index));

    //clean
    (void)interlocked_exchange(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_099: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) and return its result. ]*/
TEST_FUNCTION(when_another_call_allocated_the_page_before_the_lock_is_acquired_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE_only_allocates_the_element)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    void* page = TEST_WRAPPED_PAGE(psa, page_index);
    uint32_t item = TEST_OTHER_ITEM_VALUE;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = TEST_ALLOCATE_PAGE(TEST_ARRAY(psa), page_index, TEST_OTHER_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(void_ptr, page, TEST_WRAPPED_PAGE(psa, page_index));
    ASSERT_ARE_EQUAL(uint32_t, TEST_OTHER_ITEM_VALUE, TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE)->item);
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, TEST_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 2, TEST_ELEMENT_COUNT(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_099: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) and return its result. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_052: [ If the state of the slot was not THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED. ]*/
TEST_FUNCTION(when_another_call_allocated_the_page_and_the_element_before_the_lock_is_acquired_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE_fails_with_ALREADY_ALLOCATED)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t item = TEST_OTHER_ITEM_VALUE;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = TEST_ALLOCATE_PAGE(TEST_ARRAY(psa), page_index, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->item);
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_ELEMENT_COUNT(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_095: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) and return its result. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_096: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release page_guard->lock in shared mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_052: [ If the state of the slot was not THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_of_an_allocated_element_fails_without_copying_the_item)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t item = TEST_OTHER_ITEM_VALUE;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->item);
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, TEST_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_ELEMENT_COUNT(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_052: [ If the state of the slot was not THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_of_an_element_that_another_call_is_allocating_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    /* another call changed the state of the slot and copies its item */
    TEST_SET_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATING);
    uint32_t item = TEST_OTHER_ITEM_VALUE;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATING, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_ELEMENT_COUNT(psa, page_index));

    //clean
    TEST_SET_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_052: [ If the state of the slot was not THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_of_an_element_that_another_call_is_releasing_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    /* a RELEASE of the element changed the state of the slot and did not dispose the item yet */
    TEST_SET_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_SLOT_STATE_RELEASING_ALLOCATED);
    uint32_t item = TEST_OTHER_ITEM_VALUE;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->item);
    ASSERT_ARE_EQUAL(int32_t, TEST_SLOT_STATE_RELEASING_ALLOCATED, TEST_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_ELEMENT_COUNT(psa, page_index));

    //clean
    TEST_SET_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED);
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

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_058: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call PAGED_SPARSE_ARRAY_GET(T) with paged_sparse_array->paged_sparse_array and index to get the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_119: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall add THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING to the state of the slot using interlocked_compare_exchange if the state is greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED and lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_121: [ If the state of the slot was greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED and lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [ If paged_sparse_array->item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call paged_sparse_array->item_dispose_func with a pointer to the item of the slot. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_063: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall set the state of the slot to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_101: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall decrement page_guard->element_count, where page_guard is the page guard of the page with index index / page_size. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_an_element_disposes_the_item_without_holding_the_lock_of_the_page_guard_and_does_not_free_the_page_that_has_other_elements)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* item = test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    g_slot_observer.slot = TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(test_item_dispose_func(item));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, TEST_SLOT_STATE_RELEASING_ALLOCATED, g_slot_observer.state_when_called);
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, TEST_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_ELEMENT_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    g_slot_observer.slot = NULL;
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [ If paged_sparse_array->item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call paged_sparse_array->item_dispose_func with a pointer to the item of the slot. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_with_NULL_item_dispose_func_releases_the_element)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_ELEMENT_COUNT(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_101: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall decrement page_guard->element_count, where page_guard is the page guard of the page with index index / page_size. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_103: [ If the element_count became 0, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T). ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_021: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall acquire page_guard->lock in exclusive mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_022: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED and page_guard->element_count is 0: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_024: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall set page_guard->page_state to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release page_guard->lock. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_026: [ If page_guard->page_state was set to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_027: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall call PAGED_SPARSE_ARRAY_RELEASE(T) on array->paged_sparse_array for each index of the page. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_077: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall call InterlockedHL_SetAndWakeAll to set page_guard->page_state to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED and wake the calls waiting for it to change. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_the_last_element_of_a_page_frees_the_page_after_releasing_the_lock_of_the_page_guard)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_THIRD_PAGE);
    uint32_t* item = test_allocate(psa, TEST_INDEX_IN_THIRD_PAGE);
    void* page = TEST_WRAPPED_PAGE(psa, page_index);
    STRICT_EXPECTED_CALL(test_item_dispose_func(item));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(free(page));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWakeAll(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_THIRD_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(TEST_WRAPPED_PAGE(psa, page_index));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_ELEMENT_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_027: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall call PAGED_SPARSE_ARRAY_RELEASE(T) on array->paged_sparse_array for each index of the page. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_the_last_element_of_the_last_page_frees_the_last_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint32_t* item = test_allocate(psa, TEST_LAST_INDEX);
    void* page = TEST_WRAPPED_PAGE(psa, TEST_LAST_PAGE_INDEX);
    STRICT_EXPECTED_CALL(test_item_dispose_func(item));
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, TEST_LAST_PAGE_INDEX)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, TEST_LAST_PAGE_INDEX)));
    STRICT_EXPECTED_CALL(free(page));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWakeAll(&TEST_PAGE_GUARD(psa, TEST_LAST_PAGE_INDEX)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_LAST_INDEX);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(TEST_WRAPPED_PAGE(psa, TEST_LAST_PAGE_INDEX));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_ELEMENT_COUNT(psa, TEST_LAST_PAGE_INDEX));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED, TEST_PAGE_STATE(psa, TEST_LAST_PAGE_INDEX));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_124: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_an_element_that_is_not_allocated_in_an_allocated_page_does_nothing)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_ELEMENT_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_124: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_an_element_that_was_released_does_nothing)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    umock_c_reset_all_calls();

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, TEST_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_ELEMENT_COUNT(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_124: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_an_element_that_another_call_is_allocating_does_nothing)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    /* another call changed the state of the slot and copies its item */
    TEST_SET_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATING);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATING, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_ELEMENT_COUNT(psa, page_index));

    //clean
    TEST_SET_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_124: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_an_element_that_another_call_is_releasing_does_nothing)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    /* another RELEASE of the element changed the state of the slot and did not dispose the item yet */
    TEST_SET_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_SLOT_STATE_RELEASING_ALLOCATED);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, TEST_SLOT_STATE_RELEASING_ALLOCATED, TEST_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_ELEMENT_COUNT(psa, page_index));

    //clean
    TEST_SET_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_119: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall add THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING to the state of the slot using interlocked_compare_exchange if the state is greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED and lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_121: [ If the state of the slot was greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED and lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_122: [ If the state of the slot was not THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call InterlockedHL_WaitForValue to wait for the state of the slot to be THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING + THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [ If paged_sparse_array->item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call paged_sparse_array->item_dispose_func with a pointer to the item of the slot. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_063: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall set the state of the slot to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_101: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall decrement page_guard->element_count, where page_guard is the page guard of the page with index index / page_size. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_an_element_whose_item_an_ACQUIRE_ITEM_copies_waits_for_the_ACQUIRE_ITEM_before_disposing_the_item)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t* item = test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    /* an ACQUIRE_ITEM copies the item, it ends while RELEASE waits */
    TEST_SET_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED + 1);
    g_slot_copied_while_waiting = TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    g_slot_observer.slot = TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(&TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE)->state, TEST_SLOT_STATE_RELEASING_ALLOCATED, UINT32_MAX));
    STRICT_EXPECTED_CALL(test_item_dispose_func(item));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(g_slot_copied_while_waiting);
    ASSERT_ARE_EQUAL(int32_t, TEST_SLOT_STATE_RELEASING_ALLOCATED, g_slot_observer.state_when_called);
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_ELEMENT_COUNT(psa, page_index));

    //clean
    g_slot_observer.slot = NULL;
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_123: [ If InterlockedHL_WaitForValue fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall subtract THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING from the state of the slot and return. ]*/
TEST_FUNCTION(when_InterlockedHL_WaitForValue_fails_then_THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_keeps_the_element_allocated)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);
    /* an ACQUIRE_ITEM copies the item */
    TEST_SET_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED + 1);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(&TEST_SLOT(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE)->state, TEST_SLOT_STATE_RELEASING_ALLOCATED, UINT32_MAX))
        .SetReturn(INTERLOCKED_HL_ERROR);

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED + 1, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 2, TEST_ELEMENT_COUNT(psa, page_index));

    //clean
    TEST_SET_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_067: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_with_NULL_paged_sparse_array_fails)
{
    //arrange
    uint32_t item = 0;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(NULL, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(uint32_t, 0, item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_068: [ If item is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_with_NULL_item_fails)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);

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
    uint32_t item = 0;

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_MAX_SIZE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(uint32_t, 0, item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_104: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall acquire page_guard->lock in shared mode, where page_guard is the page guard of the page with index index / page_size. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_071: [ If page_guard->page_state is not THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_131: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall release page_guard->lock in shared mode before returning. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_in_a_page_that_is_not_allocated_releases_the_lock_of_the_page_guard_and_returns_NOT_ALLOCATED)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    uint32_t item = 0;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_ARE_EQUAL(uint32_t, 0, item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_071: [ If page_guard->page_state is not THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_in_a_page_that_is_being_freed_releases_the_lock_of_the_page_guard_and_returns_NOT_ALLOCATED)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    /* another call is freeing the page */
    (void)interlocked_exchange(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING);
    uint32_t item = 0;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_ARE_EQUAL(uint32_t, 0, item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    (void)interlocked_exchange(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_072: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall call PAGED_SPARSE_ARRAY_GET(T) with paged_sparse_array->paged_sparse_array and index to get the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_125: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall add 1 to the state of the slot using interlocked_compare_exchange if the state is greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED and lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_074: [ If the state of the slot is lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED or greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_131: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall release page_guard->lock in shared mode before returning. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_of_an_element_that_is_not_allocated_in_an_allocated_page_releases_the_lock_of_the_page_guard_and_returns_NOT_ALLOCATED)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t item = 0;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_ARE_EQUAL(uint32_t, 0, item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_074: [ If the state of the slot is lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED or greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_of_an_element_that_another_call_is_allocating_returns_NOT_ALLOCATED)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    /* another call changed the state of the slot and copies its item */
    TEST_SET_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATING);
    uint32_t item = 0;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_ARE_EQUAL(uint32_t, 0, item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATING, TEST_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE));

    //clean
    TEST_SET_SLOT_STATE(psa, TEST_OTHER_INDEX_IN_FIRST_PAGE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_074: [ If the state of the slot is lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED or greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_of_an_element_that_another_call_is_releasing_returns_NOT_ALLOCATED)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    /* a RELEASE of the element changed the state of the slot and did not dispose the item yet */
    TEST_SET_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE, TEST_SLOT_STATE_RELEASING_ALLOCATED);
    uint32_t item = 0;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, result);
    ASSERT_ARE_EQUAL(uint32_t, 0, item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, TEST_SLOT_STATE_RELEASING_ALLOCATED, TEST_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE));

    //clean
    TEST_SET_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_104: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall acquire page_guard->lock in shared mode, where page_guard is the page guard of the page with index index / page_size. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_072: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall call PAGED_SPARSE_ARRAY_GET(T) with paged_sparse_array->paged_sparse_array and index to get the slot of the element. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_125: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall add 1 to the state of the slot using interlocked_compare_exchange if the state is greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED and lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_127: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall copy the item of the slot to item. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_128: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall subtract 1 from the state of the slot. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_130: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall succeed and return PAGED_SPARSE_ARRAY_GET_OK. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_131: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall release page_guard->lock in shared mode before returning. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_of_an_allocated_element_copies_the_item_while_holding_the_lock_of_the_page_guard_in_shared_mode)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t item = 0;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, result);
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, TEST_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_ELEMENT_COUNT(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_125: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall add 1 to the state of the slot using interlocked_compare_exchange if the state is greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED and lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_126: [ If paged_sparse_array->item_copy_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall call paged_sparse_array->item_copy_func with item as dst and a pointer to the item of the slot as src. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_128: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall subtract 1 from the state of the slot. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_130: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall succeed and return PAGED_SPARSE_ARRAY_GET_OK. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_calls_item_copy_func_while_the_state_of_the_slot_counts_the_ACQUIRE_ITEM)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t item = 0;
    g_slot_observer.slot = TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(test_item_copy_func(&item, &TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->item));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, result);
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED + 1, g_slot_observer.state_when_called);
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, TEST_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE));

    //clean
    g_slot_observer.slot = NULL;
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_128: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall subtract 1 from the state of the slot. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_129: [ If the state of the slot became THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING + THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall call wake_by_address_single on the state of the slot. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_that_ends_the_last_copy_of_an_item_that_RELEASE_waits_for_wakes_RELEASE)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t item = 0;
    /* a RELEASE of the element starts while the item is copied */
    g_slot_observer.slot = TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE);
    g_slot_observer.state_to_add_when_called = THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(test_item_copy_func(&item, &TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->item));
    STRICT_EXPECTED_CALL(wake_by_address_single(&TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->state));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, result);
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, TEST_SLOT_STATE_RELEASING_ALLOCATED, TEST_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE));

    //clean
    g_slot_observer.slot = NULL;
    TEST_SET_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_128: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall subtract 1 from the state of the slot. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_129: [ If the state of the slot became THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING + THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall call wake_by_address_single on the state of the slot. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM_that_ends_while_another_ACQUIRE_ITEM_copies_the_item_that_RELEASE_waits_for_does_not_wake_RELEASE)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(test_item_copy_func, test_item_dispose_func);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    uint32_t item = 0;
    /* another ACQUIRE_ITEM copies the item, and a RELEASE of the element starts while the item is copied */
    TEST_SET_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED + 1);
    g_slot_observer.slot = TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE);
    g_slot_observer.state_to_add_when_called = THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING;
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_shared(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(test_item_copy_func(&item, &TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->item));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_shared(TEST_PAGE_LOCK(psa, page_index)));

    //act
    PAGED_SPARSE_ARRAY_GET_RESULT result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(uint32_t)(psa, TEST_INDEX_IN_FIRST_PAGE, &item);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, result);
    ASSERT_ARE_EQUAL(uint32_t, TEST_ITEM_VALUE, item);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED + 2, g_slot_observer.state_when_called);
    ASSERT_ARE_EQUAL(int32_t, TEST_SLOT_STATE_RELEASING_ALLOCATED + 1, TEST_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE));

    //clean
    g_slot_observer.slot = NULL;
    TEST_SET_SLOT_STATE(psa, TEST_INDEX_IN_FIRST_PAGE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)*/

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_021: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall acquire page_guard->lock in exclusive mode. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_022: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED and page_guard->element_count is 0: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release page_guard->lock. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE_of_a_page_that_another_call_freed_does_nothing)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    TEST_TRY_FREE_PAGE(TEST_ARRAY(psa), page_index);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_ELEMENT_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_022: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED and page_guard->element_count is 0: ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE_of_a_page_that_another_call_is_freeing_does_nothing)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    /* another call is freeing the page */
    (void)interlocked_exchange(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    TEST_TRY_FREE_PAGE(TEST_ARRAY(psa), page_index);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING, TEST_PAGE_STATE(psa, page_index));

    //clean
    (void)interlocked_exchange(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_022: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED and page_guard->element_count is 0: ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE_of_a_page_in_which_another_call_allocated_an_element_does_not_free_the_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));

    //act
    TEST_TRY_FREE_PAGE(TEST_ARRAY(psa), page_index);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(TEST_WRAPPED_PAGE(psa, page_index));
    ASSERT_ARE_EQUAL(int64_t, 1, TEST_ELEMENT_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_022: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED and page_guard->element_count is 0: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_024: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall set page_guard->page_state to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release page_guard->lock. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_026: [ If page_guard->page_state was set to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING: ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_027: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall call PAGED_SPARSE_ARRAY_RELEASE(T) on array->paged_sparse_array for each index of the page. ]*/
/* Tests_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_077: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall call InterlockedHL_SetAndWakeAll to set page_guard->page_state to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED and wake the calls waiting for it to change. ]*/
TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE_of_an_allocated_page_without_elements_frees_the_page_without_holding_the_lock_of_the_page_guard)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(uint32_t) psa = test_create(NULL, NULL);
    uint64_t page_index = TEST_PAGE_INDEX(TEST_INDEX_IN_FIRST_PAGE);
    (void)test_allocate(psa, TEST_INDEX_IN_FIRST_PAGE);
    /* the last element of the page was released */
    (void)interlocked_exchange(&TEST_SLOT(psa, TEST_INDEX_IN_FIRST_PAGE)->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED);
    (void)interlocked_exchange_64(&TEST_PAGE_GUARD(psa, page_index)->element_count, 0);
    void* page = TEST_WRAPPED_PAGE(psa, page_index);
    STRICT_EXPECTED_CALL(srw_lock_ll_acquire_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(srw_lock_ll_release_exclusive(TEST_PAGE_LOCK(psa, page_index)));
    STRICT_EXPECTED_CALL(free(page));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWakeAll(&TEST_PAGE_GUARD(psa, page_index)->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED));

    //act
    TEST_TRY_FREE_PAGE(TEST_ARRAY(psa), page_index);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(TEST_WRAPPED_PAGE(psa, page_index));
    ASSERT_ARE_EQUAL(int64_t, 0, TEST_ELEMENT_COUNT(psa, page_index));
    ASSERT_ARE_EQUAL(int32_t, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED, TEST_PAGE_STATE(psa, page_index));

    //clean
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(uint32_t)(&psa, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
