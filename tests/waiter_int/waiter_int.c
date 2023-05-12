// Copyright (c) Microsoft. All rights reserved.

#include <stdint.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_pal/interlocked.h"
#include "c_pal/thandle.h"

#include "c_util/rc_ptr.h"

#include "c_util/waiter.h"

TEST_DEFINE_ENUM_TYPE(WAITER_RESULT, WAITER_RESULT_VALUES);

static void* test_data = (void*)0x0001;
static int32_t register_notification_canceled = 0x0002;
static int32_t notify_cancelled = 0x0003;
static int32_t register_notification_success = 0x0004;
static int32_t notify_success = 0x0005;
static int32_t register_notification_abandoned = 0x0006;
static int32_t notify_abandoned = 0x0007;
static void* test_data2 = (void*)0x0008;

static void test_on_notification_callback_cancelled(void* context, THANDLE(RC_PTR) data, WAITER_RESULT result)
{
    int32_t original_value = interlocked_exchange(context, register_notification_canceled);
    ASSERT_ARE_EQUAL(int32_t, 0, original_value);
    ASSERT_IS_NULL(data);
    ASSERT_ARE_EQUAL(WAITER_RESULT, WAITER_RESULT_CANCELLED, result);
}

static void test_on_notify_complete_callback_cancelled(void* context, WAITER_RESULT result)
{
    int32_t original_value = interlocked_exchange(context, notify_cancelled);
    ASSERT_ARE_EQUAL(int32_t, 0, original_value);
    ASSERT_ARE_EQUAL(WAITER_RESULT, WAITER_RESULT_CANCELLED, result);
}

static void test_on_notification_callback_abandoned(void* context, THANDLE(RC_PTR) data, WAITER_RESULT result)
{
    int32_t original_value = interlocked_exchange(context, register_notification_abandoned);
    ASSERT_ARE_EQUAL(int32_t, 0, original_value);
    ASSERT_IS_NULL(data);
    ASSERT_ARE_EQUAL(WAITER_RESULT, WAITER_RESULT_ABANDONED, result);
}

static void test_on_notify_complete_callback_abandoned(void* context, WAITER_RESULT result)
{
    int32_t original_value = interlocked_exchange(context, notify_abandoned);
    ASSERT_ARE_EQUAL(int32_t, 0, original_value);
    ASSERT_ARE_EQUAL(WAITER_RESULT, WAITER_RESULT_ABANDONED, result);
}

static void test_on_notification_callback_success(void* context, THANDLE(RC_PTR) data, WAITER_RESULT result)
{
    int32_t original_value = interlocked_exchange(context, register_notification_success);
    ASSERT_ARE_EQUAL(int32_t, 0, original_value);
    ASSERT_IS_NOT_NULL(data);
    ASSERT_ARE_EQUAL(void_ptr, test_data, RC_PTR_VALUE(data));
    ASSERT_ARE_EQUAL(WAITER_RESULT, WAITER_RESULT_OK, result);
}

static void test_on_notify_complete_callback_success(void* context, WAITER_RESULT result)
{
    int32_t original_value = interlocked_exchange(context, notify_success);
    ASSERT_ARE_EQUAL(int32_t, 0, original_value);
    ASSERT_ARE_EQUAL(WAITER_RESULT, WAITER_RESULT_OK, result);
}

static void test_free_waiter_data(void* data)
{
    ASSERT_ARE_EQUAL(void_ptr, test_data, data);
}

static void test_free_waiter_data2(void* data)
{
    ASSERT_ARE_EQUAL(void_ptr, test_data2, data);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
}

TEST_FUNCTION_INITIALIZE(method_init)
{
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}


TEST_FUNCTION(test_waiter_create)
{
    /// arrange
    WAITER_HANDLE waiter;

    /// act
    waiter = waiter_create();

    /// assert
    ASSERT_IS_NOT_NULL(waiter);

    // cleanup
    waiter_destroy(waiter);
}

TEST_FUNCTION(test_waiter_destroy)
{
    /// arrange
    WAITER_HANDLE waiter = waiter_create();

    /// act
    waiter_destroy(waiter);

    /// assert
    // no explicit assert
}

TEST_FUNCTION(test_register_notification_and_cancel)
{
    /// arrange
    WAITER_HANDLE waiter = waiter_create();
    volatile_atomic int32_t context;
    (void)interlocked_exchange(&context, 0);

    /// act
    THANDLE(ASYNC_OP) async_op = waiter_register_notification(waiter, test_on_notification_callback_cancelled, (void*)&context);
    async_op_cancel(async_op);

    /// assert
    ASSERT_ARE_EQUAL(int32_t, register_notification_canceled, interlocked_add(&context, 0));

    // cleanup
   THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    waiter_destroy(waiter);
}

TEST_FUNCTION(test_notify_and_cancel)
{
    /// arrange
    WAITER_HANDLE waiter = waiter_create();
    volatile_atomic int32_t context;
    (void)interlocked_exchange(&context, 0);
    THANDLE(RC_PTR) waiter_data = rc_ptr_create_with_move_memory(test_data, test_free_waiter_data);

    /// act
    THANDLE(ASYNC_OP) async_op = waiter_notify(waiter, waiter_data, test_on_notify_complete_callback_cancelled, (void*)&context);
    async_op_cancel(async_op);

    /// assert
    ASSERT_ARE_EQUAL(int32_t, notify_cancelled, interlocked_add(&context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    THANDLE_ASSIGN(RC_PTR)(&waiter_data, NULL);
    waiter_destroy(waiter);
}

TEST_FUNCTION(test_register_notification_and_abandon)
{
    /// arrange
    WAITER_HANDLE waiter = waiter_create();
    volatile_atomic int32_t context;
    (void)interlocked_exchange(&context, 0);

    /// act
    THANDLE(ASYNC_OP) async_op = waiter_register_notification(waiter, test_on_notification_callback_abandoned, (void*)&context);
    waiter_destroy(waiter);

    /// assert
    ASSERT_ARE_EQUAL(int32_t, register_notification_abandoned, interlocked_add(&context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(test_notify_and_abandon)
{
    /// arrange
    WAITER_HANDLE waiter = waiter_create();
    volatile_atomic int32_t context;
    (void)interlocked_exchange(&context, 0);
    THANDLE(RC_PTR) waiter_data = rc_ptr_create_with_move_memory(test_data, test_free_waiter_data);

    /// act
    THANDLE(ASYNC_OP) async_op = waiter_notify(waiter, waiter_data, test_on_notify_complete_callback_abandoned, (void*)&context);
    waiter_destroy(waiter);


    /// assert
    ASSERT_ARE_EQUAL(int32_t, notify_abandoned, interlocked_add(&context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    THANDLE_ASSIGN(RC_PTR)(&waiter_data, NULL);
}

TEST_FUNCTION(test_register_and_then_notify)
{
    /// arrange
    WAITER_HANDLE waiter = waiter_create();
    volatile_atomic int32_t register_notification_context;
    (void)interlocked_exchange(&register_notification_context, 0);
    volatile_atomic int32_t notify_context;
    (void)interlocked_exchange(&notify_context, 0);
    THANDLE(RC_PTR) waiter_data = rc_ptr_create_with_move_memory(test_data, test_free_waiter_data);

    /// act
    THANDLE(ASYNC_OP) register_op = waiter_register_notification(waiter, test_on_notification_callback_success, (void*)&register_notification_context);
    THANDLE(ASYNC_OP) notify_op = waiter_notify(waiter, waiter_data, test_on_notify_complete_callback_success, (void*)&notify_context);

    /// assert
    ASSERT_ARE_EQUAL(int32_t, register_notification_success, interlocked_add(&register_notification_context, 0));
    ASSERT_ARE_EQUAL(int32_t, notify_success, interlocked_add(&notify_context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&notify_op, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&register_op, NULL);
    THANDLE_ASSIGN(RC_PTR)(&waiter_data, NULL);
    waiter_destroy(waiter);
}


TEST_FUNCTION(test_notify_and_then_register)
{
    /// arrange
    WAITER_HANDLE waiter = waiter_create();
    volatile_atomic int32_t register_notification_context;
    (void)interlocked_exchange(&register_notification_context, 0);
    volatile_atomic int32_t notify_context;
    (void)interlocked_exchange(&notify_context, 0);
    THANDLE(RC_PTR) waiter_data = rc_ptr_create_with_move_memory(test_data, test_free_waiter_data);

    /// act
    THANDLE(ASYNC_OP) notify_op = waiter_notify(waiter, waiter_data, test_on_notify_complete_callback_success, (void*)&notify_context);
    THANDLE(ASYNC_OP) register_op = waiter_register_notification(waiter, test_on_notification_callback_success, (void*)&register_notification_context);

    /// assert
    ASSERT_ARE_EQUAL(int32_t, register_notification_success, interlocked_add(&register_notification_context, 0));
    ASSERT_ARE_EQUAL(int32_t, notify_success, interlocked_add(&notify_context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&register_op, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&notify_op, NULL);
    THANDLE_ASSIGN(RC_PTR)(&waiter_data, NULL);
    waiter_destroy(waiter);
}

TEST_FUNCTION(test_register_after_register_fails)
{
    /// arrange
    WAITER_HANDLE waiter = waiter_create();
    volatile_atomic int32_t register_notification_context;
    (void)interlocked_exchange(&register_notification_context, 0);
    volatile_atomic int32_t notify_context;
    (void)interlocked_exchange(&notify_context, 0);
    THANDLE(RC_PTR) waiter_data = rc_ptr_create_with_move_memory(test_data, test_free_waiter_data);

    /// act
    THANDLE(ASYNC_OP) register_op1 = waiter_register_notification(waiter, test_on_notification_callback_success, (void*)&register_notification_context);
    THANDLE(ASYNC_OP) register_op2 = waiter_register_notification(waiter, test_on_notification_callback_cancelled, (void*)&register_notification_context);
    THANDLE(ASYNC_OP) notify_op = waiter_notify(waiter, waiter_data, test_on_notify_complete_callback_success, (void*)&notify_context);

    /// assert
    ASSERT_IS_NULL(register_op2);
    ASSERT_ARE_EQUAL(int32_t, register_notification_success, interlocked_add(&register_notification_context, 0));
    ASSERT_ARE_EQUAL(int32_t, notify_success, interlocked_add(&notify_context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&notify_op, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&register_op1, NULL);
    THANDLE_ASSIGN(RC_PTR)(&waiter_data, NULL);
    waiter_destroy(waiter);
}

TEST_FUNCTION(test_notify_after_notify_fails)
{
    /// arrange
    WAITER_HANDLE waiter = waiter_create();
    volatile_atomic int32_t register_notification_context;
    (void)interlocked_exchange(&register_notification_context, 0);
    volatile_atomic int32_t notify_context;
    (void)interlocked_exchange(&notify_context, 0);
    THANDLE(RC_PTR) waiter_data1 = rc_ptr_create_with_move_memory(test_data, test_free_waiter_data);
    THANDLE(RC_PTR) waiter_data2 = rc_ptr_create_with_move_memory(test_data2, test_free_waiter_data2);

    /// act
    THANDLE(ASYNC_OP) notify_op1 = waiter_notify(waiter, waiter_data1, test_on_notify_complete_callback_success, (void*)&notify_context);
    THANDLE(ASYNC_OP) notify_op2 = waiter_notify(waiter, waiter_data2, test_on_notify_complete_callback_success, (void*)&notify_context);
    THANDLE(ASYNC_OP) register_op = waiter_register_notification(waiter, test_on_notification_callback_success, (void*)&register_notification_context);

    /// assert
    ASSERT_IS_NULL(notify_op2);
    ASSERT_ARE_EQUAL(int32_t, register_notification_success, interlocked_add(&register_notification_context, 0));
    ASSERT_ARE_EQUAL(int32_t, notify_success, interlocked_add(&notify_context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&register_op, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&notify_op1, NULL);
    THANDLE_ASSIGN(RC_PTR)(&waiter_data2, NULL);
    THANDLE_ASSIGN(RC_PTR)(&waiter_data1, NULL);
    waiter_destroy(waiter);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
