// Copyright (c) Microsoft. All rights reserved.

#include <stdint.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_pal/interlocked.h"
#include "c_pal/thandle.h"
#include "c_pal/threadpool.h"
#include "c_pal/sync.h"

#include "c_util/rc_ptr.h"
#include "c_util/channel.h"

TEST_DEFINE_ENUM_TYPE(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(CHANNEL_RESULT, CHANNEL_RESULT);

static struct
{
    THANDLE(THREADPOOL) g_threadpool;
}g;

static EXECUTION_ENGINE_HANDLE g_execution_engine = NULL;

static void* test_data = (void*)0x0001;
static int32_t pull_cancelled = 0x0002;
static int32_t push_cancelled = 0x0003;
static int32_t pull_success = 0x0004;
static int32_t push_success = 0x0005;
static int32_t pull_abandoned = 0x0006;
static int32_t push_abandoned = 0x0007;
static void* test_data2 = (void*)0x0008;

static void test_on_pull_callback_cancelled(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_PTR) data)
{
    int32_t original_value = interlocked_exchange(context, pull_cancelled);
    wake_by_address_single(context);
    ASSERT_ARE_EQUAL(int32_t, 0, original_value);
    ASSERT_IS_NULL(data);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_CANCELLED, result);
}

static void test_on_push_callback_cancelled(void* context, CHANNEL_CALLBACK_RESULT result)
{
    int32_t original_value = interlocked_exchange(context, push_cancelled);
    wake_by_address_single(context);
    ASSERT_ARE_EQUAL(int32_t, 0, original_value);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_CANCELLED, result);
}

static void test_on_pull_callback_abandoned(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_PTR) data)
{
    int32_t original_value = interlocked_exchange(context, pull_abandoned);
    wake_by_address_single(context);
    ASSERT_ARE_EQUAL(int32_t, 0, original_value);
    ASSERT_IS_NULL(data);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_ABANDONED, result);
}

static void test_on_push_callback_abandoned(void* context, CHANNEL_CALLBACK_RESULT result)
{
    int32_t original_value = interlocked_exchange(context, push_abandoned);
    wake_by_address_single(context);
    ASSERT_ARE_EQUAL(int32_t, 0, original_value);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_ABANDONED, result);
}

static void test_on_pull_callback_success(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_PTR) data)
{
    int32_t original_value = interlocked_exchange(context, pull_success);
    wake_by_address_single(context);
    ASSERT_ARE_EQUAL(int32_t, 0, original_value);
    ASSERT_IS_NOT_NULL(data);
    ASSERT_ARE_EQUAL(void_ptr, test_data, RC_PTR_VALUE(data));
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_OK, result);
}

static void test_on_push_callback_success(void* context, CHANNEL_CALLBACK_RESULT result)
{
    int32_t original_value = interlocked_exchange(context, push_success);
    wake_by_address_single(context);
    ASSERT_ARE_EQUAL(int32_t, 0, original_value);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_OK, result);
}

static void test_free_channel_data(void* data)
{
    ASSERT_ARE_EQUAL(void_ptr, test_data, data);
}

static void test_free_channel_data2(void* data)
{
    ASSERT_ARE_EQUAL(void_ptr, test_data2, data);
}

static THANDLE(THREADPOOL) setup_threadpool()
{
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);
    THANDLE(THREADPOOL) threadpool = threadpool_create(execution_engine);
    ASSERT_IS_NOT_NULL(threadpool);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open(threadpool));
    return threadpool;
}

static void cleanup_threadpool()
{

}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    g_execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(g_execution_engine);
    THANDLE(THREADPOOL) temp = threadpool_create(g_execution_engine);
    ASSERT_IS_NOT_NULL(temp);
    THANDLE_INITIALIZE_MOVE(THREADPOOL)(&g.g_threadpool, &temp);
    ASSERT_IS_NOT_NULL(g.g_threadpool);
    ASSERT_ARE_EQUAL(int, 0, threadpool_open(g.g_threadpool));
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    THANDLE_ASSIGN(THREADPOOL)(&g.g_threadpool, NULL);
    execution_engine_dec_ref(g_execution_engine);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}


TEST_FUNCTION(test_channel_create_and_destroy)
{
    /// arrange

    /// act
    THANDLE(CHANNEL) channel = channel_create(g.g_threadpool);

    /// assert
    ASSERT_IS_NOT_NULL(channel);

    // cleanup
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

TEST_FUNCTION(test_pull_and_cancel)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(g.g_threadpool);
    volatile_atomic int32_t context;
    (void)interlocked_exchange(&context, 0);

    /// act
    THANDLE(ASYNC_OP) async_op = NULL;
    CHANNEL_RESULT result = channel_pull(channel, test_on_pull_callback_cancelled, (void*)&context, &async_op);
    async_op_cancel(async_op);

    //wait for callback to execute
    wait_on_address(&context, 0, UINT32_MAX);

    /// assert
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, result);
    ASSERT_ARE_EQUAL(int32_t, pull_cancelled, interlocked_add(&context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);

}

TEST_FUNCTION(test_push_and_cancel)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(g.g_threadpool);
    volatile_atomic int32_t context;
    (void)interlocked_exchange(&context, 0);
    THANDLE(RC_PTR) channel_data = rc_ptr_create_with_move_pointer(test_data, test_free_channel_data);

    /// act
    THANDLE(ASYNC_OP) async_op = NULL;
    CHANNEL_RESULT result = channel_push(channel, channel_data, test_on_push_callback_cancelled, (void*)&context,  &async_op);
    async_op_cancel(async_op);

    //wait for callback to execute
    wait_on_address(&context, 0, UINT32_MAX);

    /// assert
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, result);
    ASSERT_ARE_EQUAL(int32_t, push_cancelled, interlocked_add(&context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    THANDLE_ASSIGN(RC_PTR)(&channel_data, NULL);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

TEST_FUNCTION(test_pull_and_abandon)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(g.g_threadpool);
    volatile_atomic int32_t context;
    (void)interlocked_exchange(&context, 0);

    /// act
    THANDLE(ASYNC_OP) async_op = NULL;
    CHANNEL_RESULT result = channel_pull(channel, test_on_pull_callback_abandoned, (void*)&context, &async_op);
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);

    //wait for callback to execute
    wait_on_address(&context, 0, UINT32_MAX);

    /// assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, result);
    ASSERT_ARE_EQUAL(int32_t, pull_abandoned, interlocked_add(&context, 0));

    // cleanup
}

TEST_FUNCTION(test_push_and_abandon)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(g.g_threadpool);
    volatile_atomic int32_t context;
    (void)interlocked_exchange(&context, 0);
    THANDLE(RC_PTR) channel_data = rc_ptr_create_with_move_pointer(test_data, test_free_channel_data);

    /// act
    THANDLE(ASYNC_OP) async_op = NULL;
    CHANNEL_RESULT result = channel_push(channel, channel_data, test_on_push_callback_abandoned, (void*)&context, &async_op);
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);

    //wait for callback to execute
    wait_on_address(&context, 0, UINT32_MAX);

    /// assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, result);
    ASSERT_ARE_EQUAL(int32_t, push_abandoned, interlocked_add(&context, 0));

    // cleanup
    THANDLE_ASSIGN(RC_PTR)(&channel_data, NULL);
}

TEST_FUNCTION(test_pull_and_then_push)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(g.g_threadpool);
    volatile_atomic int32_t pull_context;
    (void)interlocked_exchange(&pull_context, 0);
    volatile_atomic int32_t push_context;
    (void)interlocked_exchange(&push_context, 0);
    THANDLE(RC_PTR) channel_data = rc_ptr_create_with_move_pointer(test_data, test_free_channel_data);

    /// act
    THANDLE(ASYNC_OP) pull_op = NULL;
    THANDLE(ASYNC_OP) push_op = NULL;
    CHANNEL_RESULT pull_result = channel_pull(channel, test_on_pull_callback_success, (void*)&pull_context, &pull_op);
    CHANNEL_RESULT push_result = channel_push(channel, channel_data, test_on_push_callback_success, (void*)&push_context, &push_op);

    //wait for callback to execute
    wait_on_address(&pull_context, 0, UINT32_MAX);
    wait_on_address(&push_context, 0, UINT32_MAX);

    /// assert
    ASSERT_IS_NOT_NULL(pull_op);
    ASSERT_IS_NOT_NULL(push_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result);
    ASSERT_ARE_EQUAL(int32_t, pull_success, interlocked_add(&pull_context, 0));
    ASSERT_ARE_EQUAL(int32_t, push_success, interlocked_add(&push_context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&push_op, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    THANDLE_ASSIGN(RC_PTR)(&channel_data, NULL);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

TEST_FUNCTION(test_push_and_then_pull)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(g.g_threadpool);
    volatile_atomic int32_t pull_context;
    (void)interlocked_exchange(&pull_context, 0);
    volatile_atomic int32_t push_context;
    (void)interlocked_exchange(&push_context, 0);
    THANDLE(RC_PTR) channel_data = rc_ptr_create_with_move_pointer(test_data, test_free_channel_data);

    /// act
    THANDLE(ASYNC_OP) push_op = NULL;
    THANDLE(ASYNC_OP) pull_op = NULL;
    CHANNEL_RESULT push_result = channel_push(channel, channel_data, test_on_push_callback_success, (void*)&push_context, &push_op);
    CHANNEL_RESULT pull_result = channel_pull(channel, test_on_pull_callback_success, (void*)&pull_context, &pull_op);

    //wait for callback to execute
    wait_on_address(&pull_context, 0, UINT32_MAX);
    wait_on_address(&push_context, 0, UINT32_MAX);

    /// assert
    ASSERT_IS_NOT_NULL(push_op);
    ASSERT_IS_NOT_NULL(pull_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_ARE_EQUAL(int32_t, pull_success, interlocked_add(&pull_context, 0));
    ASSERT_ARE_EQUAL(int32_t, push_success, interlocked_add(&push_context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&push_op, NULL);
    THANDLE_ASSIGN(RC_PTR)(&channel_data, NULL);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

TEST_FUNCTION(test_pull_after_pull_fails)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(g.g_threadpool);
    volatile_atomic int32_t pull_context_1;
    (void)interlocked_exchange(&pull_context_1, 0);
    volatile_atomic int32_t pull_context_2;
    (void)interlocked_exchange(&pull_context_2, 0);
    volatile_atomic int32_t push_context;
    (void)interlocked_exchange(&push_context, 0);
    THANDLE(RC_PTR) channel_data = rc_ptr_create_with_move_pointer(test_data, test_free_channel_data);

    /// act
    THANDLE(ASYNC_OP) pull_op1 = NULL;
    THANDLE(ASYNC_OP) pull_op2 = NULL;
    THANDLE(ASYNC_OP) push_op = NULL;
    CHANNEL_RESULT pull_result1 = channel_pull(channel, test_on_pull_callback_success, (void*)&pull_context_1, &pull_op1);
    CHANNEL_RESULT pull_result2 = channel_pull(channel, test_on_pull_callback_abandoned, (void*)&pull_context_2, &pull_op2);
    CHANNEL_RESULT push_result = channel_push(channel, channel_data, test_on_push_callback_success, (void*)&push_context, &push_op);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);

    //wait for callback to execute
    wait_on_address(&pull_context_1, 0, UINT32_MAX);
    wait_on_address(&pull_context_2, 0, UINT32_MAX);
    wait_on_address(&push_context, 0, UINT32_MAX);

    /// assert
    ASSERT_IS_NOT_NULL(pull_op1);
    ASSERT_IS_NOT_NULL(pull_op2);
    ASSERT_IS_NOT_NULL(push_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result1);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result2);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result);
    ASSERT_ARE_EQUAL(int32_t, pull_success, interlocked_add(&pull_context_1, 0));
    ASSERT_ARE_EQUAL(int32_t, pull_abandoned, interlocked_add(&pull_context_2, 0));
    ASSERT_ARE_EQUAL(int32_t, push_success, interlocked_add(&push_context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&push_op, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op1, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op2, NULL);
    THANDLE_ASSIGN(RC_PTR)(&channel_data, NULL);
}

TEST_FUNCTION(test_push_after_push_fails)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(g.g_threadpool);

    volatile_atomic int32_t push_context_1;
    (void)interlocked_exchange(&push_context_1, 0);

    volatile_atomic int32_t push_context_2;
    (void)interlocked_exchange(&push_context_2, 0);

    volatile_atomic int32_t pull_context;
    (void)interlocked_exchange(&pull_context, 0);

    THANDLE(RC_PTR) channel_data1 = rc_ptr_create_with_move_pointer(test_data, test_free_channel_data);
    THANDLE(RC_PTR) channel_data2 = rc_ptr_create_with_move_pointer(test_data2, test_free_channel_data2);

    /// act
    THANDLE(ASYNC_OP) push_op1 = NULL;
    THANDLE(ASYNC_OP) push_op2 = NULL;
    THANDLE(ASYNC_OP) pull_op = NULL;
    CHANNEL_RESULT push_result1 = channel_push(channel, channel_data1, test_on_push_callback_success, (void*)&push_context_1, &push_op1);
    CHANNEL_RESULT push_result2 = channel_push(channel, channel_data2, test_on_push_callback_abandoned, (void*)&push_context_2, &push_op2);
    CHANNEL_RESULT pull_result = channel_pull(channel, test_on_pull_callback_success, (void*)&pull_context, &pull_op);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);

    //wait for callback to execute
    wait_on_address(&push_context_1, 0, UINT32_MAX);
    wait_on_address(&push_context_2, 0, UINT32_MAX);
    wait_on_address(&pull_context, 0, UINT32_MAX);

    /// assert
    ASSERT_IS_NOT_NULL(push_op1);
    ASSERT_IS_NOT_NULL(push_op2);
    ASSERT_IS_NOT_NULL(pull_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result1);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result2);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_ARE_EQUAL(int32_t, pull_success, interlocked_add(&pull_context, 0));
    ASSERT_ARE_EQUAL(int32_t, push_success, interlocked_add(&push_context_1, 0));
    ASSERT_ARE_EQUAL(int32_t, push_abandoned, interlocked_add(&push_context_2, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&push_op1, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&push_op2, NULL);
    THANDLE_ASSIGN(RC_PTR)(&channel_data2, NULL);
    THANDLE_ASSIGN(RC_PTR)(&channel_data1, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
