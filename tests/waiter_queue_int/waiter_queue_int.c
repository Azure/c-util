// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/waiter_queue_ll.h"
#include "c_util/waiter_queue_hl.h"

static TEST_MUTEX_HANDLE g_testByTest;

static void* test_unblock_context = (void*)0x0001;
static void* test_data = (void*)0x0002;

TEST_DEFINE_ENUM_TYPE(WAITER_QUEUE_CALL_REASON, WAITER_QUEUE_CALL_REASON_VALUES);

static void test_unblock_callback(void* context, void* data, bool* remove, bool* continue_processing, WAITER_QUEUE_CALL_REASON reason)
{
    bool* callback_was_called = context;
    *callback_was_called = true;
    ASSERT_ARE_EQUAL(void_ptr, test_data, data);
    ASSERT_ARE_EQUAL(WAITER_QUEUE_CALL_REASON, WAITER_QUEUE_CALL_REASON_UNBLOCKED, reason);
    *continue_processing = false;
    *remove = true;
}

static void test_unblock_callback_continue(void* context, void* data, bool* remove, bool* continue_processing, WAITER_QUEUE_CALL_REASON reason)
{
    bool* callback_was_called = context;
    *callback_was_called = true;
    ASSERT_ARE_EQUAL(void_ptr, test_data, data);
    ASSERT_ARE_EQUAL(WAITER_QUEUE_CALL_REASON, WAITER_QUEUE_CALL_REASON_UNBLOCKED, reason);
    *continue_processing = true;
    *remove = true;
}

static void test_unblock_callback_abandoned(void* context, void* data, bool* remove, bool* continue_processing, WAITER_QUEUE_CALL_REASON reason)
{
    bool* callback_was_called = context;
    *callback_was_called = true;
    ASSERT_IS_NULL(data);
    ASSERT_ARE_EQUAL(WAITER_QUEUE_CALL_REASON, WAITER_QUEUE_CALL_REASON_ABANDONED, reason);
    *continue_processing = false;
    *remove = true;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(it_does_something)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(f)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }
}

TEST_FUNCTION_CLEANUP(cleans)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

TEST_FUNCTION(test_waiter_queue_hl_create)
{
    // arrange

    // act
    WAITER_QUEUE_HL_HANDLE result = waiter_queue_hl_create();

    // assert
    ASSERT_IS_NOT_NULL(result);

    // cleanup
    waiter_queue_hl_destroy(result);
}

TEST_FUNCTION(test_waiter_queue_unblock_and_push)
{
    // arrange
    WAITER_QUEUE_HL_HANDLE waiter_queue = waiter_queue_hl_create();
    bool callback_was_called = false;

    // act
    int push_result = waiter_queue_hl_add_waiter(waiter_queue, test_unblock_callback, &callback_was_called);
    int unblock_result = waiter_queue_hl_unblock_waiters(waiter_queue, test_data);

    // assert
    ASSERT_ARE_EQUAL(int, 0, push_result);
    ASSERT_ARE_EQUAL(int, 0, unblock_result);
    ASSERT_IS_TRUE(callback_was_called);

    // cleanup
    waiter_queue_hl_destroy(waiter_queue);
}

TEST_FUNCTION(test_unblocks_are_in_order)
{
    // arrange
    WAITER_QUEUE_HL_HANDLE waiter_queue = waiter_queue_hl_create();
    bool callback_was_called_1 = false;
    bool callback_was_called_2 = false;
    (void)waiter_queue_hl_add_waiter(waiter_queue, test_unblock_callback, &callback_was_called_1);

    // act
    int push_result_2 = waiter_queue_hl_add_waiter(waiter_queue, test_unblock_callback, &callback_was_called_2);
    int unblock_result = waiter_queue_hl_unblock_waiters(waiter_queue, test_data);

    // assert
    ASSERT_ARE_EQUAL(int, 0, push_result_2);
    ASSERT_ARE_EQUAL(int, 0, unblock_result);
    ASSERT_IS_TRUE(callback_was_called_1);
    ASSERT_IS_FALSE(callback_was_called_2);

    // cleanup
    (void)waiter_queue_hl_unblock_waiters(waiter_queue, test_data);
    waiter_queue_hl_destroy(waiter_queue);
}

TEST_FUNCTION(test_unblock_calls_multiple_callbacks)
{
    // arrange
    WAITER_QUEUE_HL_HANDLE waiter_queue = waiter_queue_hl_create();
    bool callback_was_called_1 = false;
    bool callback_was_called_2 = false;
    (void)waiter_queue_hl_add_waiter(waiter_queue, test_unblock_callback_continue, &callback_was_called_1);
    (void)waiter_queue_hl_add_waiter(waiter_queue, test_unblock_callback_continue, &callback_was_called_2);

    // act
    int unblock_result = waiter_queue_hl_unblock_waiters(waiter_queue, test_data);

    // assert
    ASSERT_ARE_EQUAL(int, 0, unblock_result);
    ASSERT_IS_TRUE(callback_was_called_1);
    ASSERT_IS_TRUE(callback_was_called_2);

    // cleanup
    waiter_queue_hl_destroy(waiter_queue);
}

TEST_FUNCTION(test_unblock_with_no_push)
{
    // arrange
    WAITER_QUEUE_HL_HANDLE waiter_queue = waiter_queue_hl_create();

    // act
    int unblock_result = waiter_queue_hl_unblock_waiters(waiter_queue, test_data);

    // assert
    ASSERT_ARE_EQUAL(int, 0, unblock_result);

    // cleanup
    waiter_queue_hl_destroy(waiter_queue);
}

TEST_FUNCTION(test_unblocks_get_abandoned)
{
    // arrange
    WAITER_QUEUE_HL_HANDLE waiter_queue = waiter_queue_hl_create();
    bool callback_was_called_1 = false;
    bool callback_was_called_2 = false;
    (void)waiter_queue_hl_add_waiter(waiter_queue, test_unblock_callback_abandoned, &callback_was_called_1);
    (void)waiter_queue_hl_add_waiter(waiter_queue, test_unblock_callback_abandoned, &callback_was_called_2);

    // act
    waiter_queue_hl_destroy(waiter_queue);

    // assert
    ASSERT_IS_TRUE(callback_was_called_1);
    ASSERT_IS_TRUE(callback_was_called_2);

    // cleanup
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)