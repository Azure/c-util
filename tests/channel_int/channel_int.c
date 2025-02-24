// Copyright (c) Microsoft. All rights reserved.

#include <stdint.h>
#include <stdlib.h>
#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#ifdef WIN32
#include "c_pal/execution_engine_win32.h"
#else
#include "c_pal/execution_engine_linux.h"
#endif // WIN32

#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/thandle.h"
#include "c_pal/threadpool.h"
#include "c_pal/threadapi.h"
#include "c_pal/sync.h"

#include "c_util/rc_ptr.h"
#include "c_util/channel.h"


TEST_DEFINE_ENUM_TYPE(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(CHANNEL_RESULT, CHANNEL_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

#define CHANNEL_ORDER_TEST_COUNT 10000

#define CHANNEL_CLOSE_TEST_THREAD_COUNT 100

#define TEST_ORIGINAL_VALUE 4242

static struct
{
    THANDLE(THREADPOOL) g_threadpool;
    THANDLE(RC_PTR) g_data;
    THANDLE(RC_PTR) g_data2;
    THANDLE(RC_STRING) g_pull_correlation_id;
    THANDLE(RC_STRING) g_push_correlation_id;
}g;

static EXECUTION_ENGINE_HANDLE g_execution_engine = NULL;

static volatile_atomic int32_t g_on_data_consumed_cb_count;

static void* test_data = (void*)0x0001;
static int32_t pull_cancelled = 0x0002;
static int32_t push_cancelled = 0x0003;
static int32_t pull_success = 0x0004;
static int32_t push_success = 0x0005;
static int32_t pull_abandoned = 0x0006;
static int32_t push_abandoned = 0x0007;
static void* test_data2 = (void*)0x0008;
static volatile_atomic int32_t test_signal;

static void test_on_data_available_cb_cancelled(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id, THANDLE(RC_PTR) data)
{
    ASSERT_IS_NOT_NULL(context);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_CANCELLED, result);
    ASSERT_IS_NOT_NULL(pull_correlation_id);
    (void)push_correlation_id; // Cannot make any assertions about push_correlation_id. May or may not be valid.
    ASSERT_IS_NULL(data);

    int32_t original_value = interlocked_exchange(context, pull_cancelled);
    wake_by_address_single(context);
    ASSERT_ARE_EQUAL(int32_t, TEST_ORIGINAL_VALUE, original_value);
}

static void test_on_data_consumed_cb_cancelled(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id)
{
    ASSERT_IS_NOT_NULL(context);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_CANCELLED, result);
    (void)pull_correlation_id; // Cannot make any assertions about pull_correlation_id. May or may not be valid.
    ASSERT_IS_NOT_NULL(push_correlation_id);

    int32_t original_value = interlocked_exchange(context, push_cancelled);
    wake_by_address_single(context);
    ASSERT_ARE_EQUAL(int32_t, TEST_ORIGINAL_VALUE, original_value);
}

static void test_on_data_available_cb_abandoned(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id, THANDLE(RC_PTR) data)
{
    ASSERT_IS_NOT_NULL(context);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_ABANDONED, result);
    ASSERT_IS_NOT_NULL(pull_correlation_id);
    ASSERT_IS_NULL(push_correlation_id);
    ASSERT_IS_NULL(data);

    int32_t original_value = interlocked_exchange(context, pull_abandoned);
    wake_by_address_single(context);
    ASSERT_ARE_EQUAL(int32_t, TEST_ORIGINAL_VALUE, original_value);
}

static void test_on_data_consumed_cb_abandoned(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id)
{
    ASSERT_IS_NOT_NULL(context);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_ABANDONED, result);
    ASSERT_IS_NULL(pull_correlation_id);
    ASSERT_IS_NOT_NULL(push_correlation_id);

    int32_t original_value = interlocked_exchange(context, push_abandoned);
    wake_by_address_single(context);
    ASSERT_ARE_EQUAL(int32_t, TEST_ORIGINAL_VALUE, original_value);
}

static void test_on_data_available_cb_success(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id, THANDLE(RC_PTR) data)
{
    ASSERT_IS_NOT_NULL(context);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_OK, result);
    ASSERT_IS_NOT_NULL(pull_correlation_id);
    ASSERT_IS_NOT_NULL(push_correlation_id);
    ASSERT_IS_NOT_NULL(data);
    ASSERT_ARE_EQUAL(void_ptr, test_data, data->ptr);

    int32_t original_value = interlocked_exchange(context, pull_success);
    wake_by_address_single(context);
    ASSERT_ARE_EQUAL(int32_t, TEST_ORIGINAL_VALUE, original_value);
}

static void test_on_data_consumed_cb_success(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id)
{
    ASSERT_IS_NOT_NULL(context);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_OK, result);
    ASSERT_IS_NOT_NULL(pull_correlation_id);
    ASSERT_IS_NOT_NULL(push_correlation_id);

    int32_t original_value = interlocked_exchange(context, push_success);
    wake_by_address_single(context);
    ASSERT_ARE_EQUAL(int32_t, TEST_ORIGINAL_VALUE, original_value);
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
    return threadpool;
}

static void on_data_available_cb_order_checker(
    void* context,
    CHANNEL_CALLBACK_RESULT result,
    THANDLE(RC_STRING) pull_correlation_id,
    THANDLE(RC_STRING) push_correlation_id,
    THANDLE(RC_PTR) data
)
{
    ASSERT_IS_NOT_NULL(context);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_OK, result);
    ASSERT_IS_NOT_NULL(pull_correlation_id);
    ASSERT_IS_NOT_NULL(push_correlation_id);
    ASSERT_IS_NOT_NULL(data);


    //assert that n-th pull is matched with n-th push
     ASSERT_ARE_EQUAL(int32_t, *((int32_t*)context), (intptr_t)((void*)data->ptr)); //casts needed to suppress pointer truncation warning

    free(context);
}

static int pull_data(void* context)
{
    THANDLE(CHANNEL) channel = context;
    for (int32_t i = 1; i <= CHANNEL_ORDER_TEST_COUNT; i++)
    {
        int32_t* pull_id = malloc(sizeof(int32_t));
        *pull_id = i;

        THANDLE(RC_STRING) correlation_id = rc_string_create_with_format("pull_correlation_id_%" PRId32 "", i);
        ASSERT_IS_NOT_NULL(correlation_id);

        THANDLE(ASYNC_OP) async_op = NULL;
        CHANNEL_RESULT pull_result = channel_pull(channel, correlation_id, on_data_available_cb_order_checker, pull_id, &async_op);
        ASSERT_IS_NOT_NULL(async_op);

        ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);

        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        THANDLE_ASSIGN(RC_STRING)(&correlation_id, NULL);
    }
    return 0;
}

static int pull_once(void* context)
{
    THANDLE(CHANNEL) channel = context;

    THANDLE(RC_STRING) correlation_id = rc_string_create("pull_correlation_id");
    ASSERT_IS_NOT_NULL(correlation_id);

    volatile_atomic int32_t pull_context;
    (void)interlocked_exchange(&pull_context, TEST_ORIGINAL_VALUE);

    THANDLE(ASYNC_OP) async_op = NULL;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForNotValue(&test_signal, 0, UINT32_MAX));

    CHANNEL_RESULT pull_result = channel_pull(channel, correlation_id, test_on_data_available_cb_abandoned, (void*)&pull_context, &async_op);

    if (pull_result == CHANNEL_RESULT_OK)
    {
        ASSERT_IS_NOT_NULL(async_op);
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&pull_context, pull_abandoned, UINT32_MAX));
    }
    THANDLE_ASSIGN(RC_STRING)(&correlation_id, NULL);

    return 0;
}

static int close_channel(void* context)
{
    THANDLE(CHANNEL) channel = context;
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForNotValue(&test_signal, 0, UINT32_MAX));
    channel_close(channel);
    return 0;
}

static void dummy_free_func(void* ptr)
{
    (void)ptr;
}

static void on_data_consumed_cb_order_checker(
    void* context,
    CHANNEL_CALLBACK_RESULT result,
    THANDLE(RC_STRING) pull_correlation_id,
    THANDLE(RC_STRING) push_correlation_id
)
{
    ASSERT_IS_NOT_NULL(context);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_OK, result);
    ASSERT_IS_NOT_NULL(pull_correlation_id);
    ASSERT_IS_NOT_NULL(push_correlation_id);

    interlocked_increment(&g_on_data_consumed_cb_count);
    wake_by_address_single(&g_on_data_consumed_cb_count);
}

static int push_data(void* context)
{
    THANDLE(CHANNEL) channel = context;
    for (int32_t i = 1; i <= CHANNEL_ORDER_TEST_COUNT; i++)
    {
        THANDLE(RC_STRING) correlation_id = rc_string_create_with_format("push_correlation_id_%" PRId32 "", i);
        THANDLE(RC_PTR) data = rc_ptr_create_with_move_pointer((void*)(intptr_t)i, dummy_free_func);
        ASSERT_IS_NOT_NULL(data);

        THANDLE(ASYNC_OP) async_op = NULL;
        CHANNEL_RESULT push_result = channel_push(channel, correlation_id, data, on_data_consumed_cb_order_checker, (void*)(int64_t)i, &async_op);
        ASSERT_IS_NOT_NULL(async_op);

        ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result);

        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        THANDLE_ASSIGN(RC_PTR)(&data, NULL);
        THANDLE_ASSIGN(RC_STRING)(&correlation_id, NULL);
    }
    return 0;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    void* execution_engine_parameters = NULL;

    execution_engine_parameters = &(EXECUTION_ENGINE_PARAMETERS) { .min_thread_count = 1, .max_thread_count = 1 };
    g_execution_engine = execution_engine_create(execution_engine_parameters);
    ASSERT_IS_NOT_NULL(g_execution_engine);
    THANDLE(THREADPOOL) temp = threadpool_create(g_execution_engine);
    ASSERT_IS_NOT_NULL(temp);
    THANDLE_INITIALIZE_MOVE(THREADPOOL)(&g.g_threadpool, &temp);
    ASSERT_IS_NOT_NULL(g.g_threadpool);

    THANDLE_INITIALIZE_MOVE(RC_PTR)(&g.g_data, &(THANDLE(RC_PTR)){ rc_ptr_create_with_move_pointer(test_data, test_free_channel_data) });
    ASSERT_IS_NOT_NULL(g.g_data);
    THANDLE_INITIALIZE_MOVE(RC_PTR)(&g.g_data2, &(THANDLE(RC_PTR)){ rc_ptr_create_with_move_pointer(test_data2, test_free_channel_data2) });
    ASSERT_IS_NOT_NULL(g.g_data2);

    THANDLE_INITIALIZE_MOVE(RC_STRING)(&g.g_pull_correlation_id, &(THANDLE(RC_STRING)){ rc_string_create("pull_correlation_id")});
    ASSERT_IS_NOT_NULL(g.g_pull_correlation_id);

    THANDLE_INITIALIZE_MOVE(RC_STRING)(&g.g_push_correlation_id, &(THANDLE(RC_STRING)){ rc_string_create("push_correlation_id")});
    ASSERT_IS_NOT_NULL(g.g_push_correlation_id);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    THANDLE_ASSIGN(RC_STRING)(&g.g_push_correlation_id, NULL);
    THANDLE_ASSIGN(RC_STRING)(&g.g_pull_correlation_id, NULL);
    THANDLE_ASSIGN(RC_PTR)(&g.g_data2, NULL);
    THANDLE_ASSIGN(RC_PTR)(&g.g_data, NULL);
    THANDLE_ASSIGN(THREADPOOL)(&g.g_threadpool, NULL);
    execution_engine_dec_ref(g_execution_engine);
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    // Do Nothing.
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    // Do Nothing.
}


TEST_FUNCTION(test_channel_create_and_destroy)
{
    /// arrange

    /// act
    THANDLE(CHANNEL) channel = channel_create(NULL, g.g_threadpool);

    /// assert
    ASSERT_IS_NOT_NULL(channel);

    // cleanup
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

TEST_FUNCTION(test_pull_and_cancel)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(NULL, g.g_threadpool);
    ASSERT_IS_NOT_NULL(channel);
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    volatile_atomic int32_t context;
    (void)interlocked_exchange(&context, TEST_ORIGINAL_VALUE);

    /// act
    THANDLE(ASYNC_OP) async_op = NULL;
    CHANNEL_RESULT result = channel_pull(channel, g.g_pull_correlation_id, test_on_data_available_cb_cancelled, (void*)&context, &async_op);
    ASSERT_IS_NOT_NULL(async_op);
    async_op_cancel(async_op);

    //wait for callback to execute
    InterlockedHL_WaitForNotValue(&context, TEST_ORIGINAL_VALUE, UINT32_MAX);

    /// assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, result);
    ASSERT_ARE_EQUAL(int32_t, pull_cancelled, interlocked_add(&context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    channel_close(channel);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

TEST_FUNCTION(test_push_and_cancel)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(NULL, g.g_threadpool);
    ASSERT_IS_NOT_NULL(channel);
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    volatile_atomic int32_t context;
    (void)interlocked_exchange(&context, TEST_ORIGINAL_VALUE);

    /// act
    THANDLE(ASYNC_OP) async_op = NULL;
    CHANNEL_RESULT result = channel_push(channel, g.g_push_correlation_id, g.g_data, test_on_data_consumed_cb_cancelled, (void*)&context,  &async_op);
    ASSERT_IS_NOT_NULL(async_op);
    async_op_cancel(async_op);

    //wait for callback to execute
    InterlockedHL_WaitForNotValue(&context, TEST_ORIGINAL_VALUE, UINT32_MAX);

    /// assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, result);
    ASSERT_ARE_EQUAL(int32_t, push_cancelled, interlocked_add(&context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    channel_close(channel);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

/*
* This test fails because there is an inherent limitation that prevents cancelling an abandoned operation.
* The test is disabled until the underlying issue is fixed. TODO Task 30251613
*/
DISABLED_TEST_FUNCTION(test_cancel_after_close)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(NULL, g.g_threadpool);
    ASSERT_IS_NOT_NULL(channel);
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    volatile_atomic int32_t context;
    (void)interlocked_exchange(&context, TEST_ORIGINAL_VALUE);

    /// act
    THANDLE(ASYNC_OP) async_op = NULL;
    CHANNEL_RESULT result = channel_pull(channel, g.g_pull_correlation_id, test_on_data_available_cb_abandoned, (void*)&context, &async_op);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, result);
    channel_close(channel);
    async_op_cancel(async_op);

    //wait for callback to execute
    InterlockedHL_WaitForNotValue(&context, TEST_ORIGINAL_VALUE, UINT32_MAX);

    /// assert
    ASSERT_ARE_EQUAL(int32_t, pull_abandoned, interlocked_add(&context, 0));

    /// cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

TEST_FUNCTION(test_pull_and_abandon)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(NULL, g.g_threadpool);
    ASSERT_IS_NOT_NULL(channel);
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    volatile_atomic int32_t context;
    (void)interlocked_exchange(&context, TEST_ORIGINAL_VALUE);

    /// act
    THANDLE(ASYNC_OP) async_op = NULL;
    CHANNEL_RESULT result = channel_pull(channel, g.g_pull_correlation_id, test_on_data_available_cb_abandoned, (void*)&context, &async_op);
    ASSERT_IS_NOT_NULL(async_op);
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    channel_close(channel);

    //wait for callback to execute
    InterlockedHL_WaitForNotValue(&context, TEST_ORIGINAL_VALUE, UINT32_MAX);

    /// assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, result);
    ASSERT_ARE_EQUAL(int32_t, pull_abandoned, interlocked_add(&context, 0));

    /// cleanup
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

TEST_FUNCTION(test_push_and_abandon)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(NULL, g.g_threadpool);
    ASSERT_IS_NOT_NULL(channel);
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    volatile_atomic int32_t context;
    (void)interlocked_exchange(&context, TEST_ORIGINAL_VALUE);

    /// act
    THANDLE(ASYNC_OP) async_op = NULL;
    CHANNEL_RESULT result = channel_push(channel, g.g_push_correlation_id, g.g_data, test_on_data_consumed_cb_abandoned, (void*)&context, &async_op);
    ASSERT_IS_NOT_NULL(async_op);
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    channel_close(channel);

    //wait for callback to execute
    InterlockedHL_WaitForNotValue(&context, TEST_ORIGINAL_VALUE, UINT32_MAX);

    /// assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, result);
    ASSERT_ARE_EQUAL(int32_t, push_abandoned, interlocked_add(&context, 0));

    /// cleanup
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

/*
 * This test verifies that a channel can be used, reopened, and used again.
 */
TEST_FUNCTION(test_channel_reopen_and_push_pull)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(NULL, g.g_threadpool);
    ASSERT_IS_NOT_NULL(channel);
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    volatile_atomic int32_t pull_context;
    (void)interlocked_exchange(&pull_context, TEST_ORIGINAL_VALUE);
    volatile_atomic int32_t push_context;
    (void)interlocked_exchange(&push_context, TEST_ORIGINAL_VALUE);

    /// act
    THANDLE(ASYNC_OP) pull_op = NULL;
    THANDLE(ASYNC_OP) push_op = NULL;
    CHANNEL_RESULT pull_result = channel_pull(channel, g.g_pull_correlation_id, test_on_data_available_cb_success, (void*)&pull_context, &pull_op);
    ASSERT_IS_NOT_NULL(pull_op);
    CHANNEL_RESULT push_result = channel_push(channel, g.g_push_correlation_id, g.g_data, test_on_data_consumed_cb_success, (void*)&push_context, &push_op);
    ASSERT_IS_NOT_NULL(push_op);

    //wait for callback to execute
    InterlockedHL_WaitForNotValue(&pull_context, TEST_ORIGINAL_VALUE, UINT32_MAX);
    InterlockedHL_WaitForNotValue(&push_context, TEST_ORIGINAL_VALUE, UINT32_MAX);

    /// assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result);
    ASSERT_ARE_EQUAL(int32_t, pull_success, interlocked_add(&pull_context, 0));
    ASSERT_ARE_EQUAL(int32_t, push_success, interlocked_add(&push_context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&push_op, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    channel_close(channel);

    /// act
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    volatile_atomic int32_t pull_context_2;
    (void)interlocked_exchange(&pull_context_2, TEST_ORIGINAL_VALUE);
    volatile_atomic int32_t push_context_2;
    (void)interlocked_exchange(&push_context_2, TEST_ORIGINAL_VALUE);
    THANDLE(ASYNC_OP) pull_op_2 = NULL;
    THANDLE(ASYNC_OP) push_op_2 = NULL;
    CHANNEL_RESULT pull_result_2 = channel_pull(channel, g.g_pull_correlation_id, test_on_data_available_cb_success, (void*)&pull_context_2, &pull_op_2);
    ASSERT_IS_NOT_NULL(pull_op_2);
    CHANNEL_RESULT push_result_2 = channel_push(channel, g.g_push_correlation_id, g.g_data, test_on_data_consumed_cb_success, (void*)&push_context_2, &push_op_2);
    ASSERT_IS_NOT_NULL(push_op_2);

    //wait for callback to execute
    InterlockedHL_WaitForNotValue(&pull_context_2, TEST_ORIGINAL_VALUE, UINT32_MAX);
    InterlockedHL_WaitForNotValue(&push_context_2, TEST_ORIGINAL_VALUE, UINT32_MAX);

    /// assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result_2);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result_2);
    ASSERT_ARE_EQUAL(int32_t, pull_success, interlocked_add(&pull_context, 0));
    ASSERT_ARE_EQUAL(int32_t, push_success, interlocked_add(&push_context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&push_op_2, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op_2, NULL);
    channel_close(channel);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}


/*
 * This test verifies that the channel is clean after closing.
 */
TEST_FUNCTION(test_channel_is_clean_after_closing)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(NULL, g.g_threadpool);
    ASSERT_IS_NOT_NULL(channel);
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    volatile_atomic int32_t push_context;
    (void)interlocked_exchange(&push_context, TEST_ORIGINAL_VALUE);
    volatile_atomic int32_t pull_context;
    (void)interlocked_exchange(&pull_context, TEST_ORIGINAL_VALUE);

    /// act
    THANDLE(ASYNC_OP) async_op = NULL;
    CHANNEL_RESULT pull_result = channel_pull(channel, g.g_pull_correlation_id, test_on_data_available_cb_abandoned, (void*)&pull_context, &async_op);
    ASSERT_IS_NOT_NULL(async_op);
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    channel_close(channel);

    /// assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_ARE_EQUAL(int, pull_abandoned, interlocked_add(&pull_context, 0));

    /// act
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    CHANNEL_RESULT push_result = channel_push(channel, g.g_push_correlation_id, g.g_data, test_on_data_consumed_cb_abandoned, (void*)&push_context, &async_op);
    ASSERT_IS_NOT_NULL(async_op);
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
    channel_close(channel);

    /// assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result);
    ASSERT_ARE_EQUAL(int32_t, push_abandoned, interlocked_add(&push_context, 0));

    /// cleanup
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

/*
 * This test verifies that a channel can be reopened after operations are cancelled.
 */
TEST_FUNCTION(test_channel_reopen_after_pull_cancel_and_push_cancel)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(NULL, g.g_threadpool);
    ASSERT_IS_NOT_NULL(channel);
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    volatile_atomic int32_t pull_context;
    (void)interlocked_exchange(&pull_context, TEST_ORIGINAL_VALUE);

    /// act
    THANDLE(ASYNC_OP) pull_op = NULL;
    CHANNEL_RESULT pull_result = channel_pull(channel, g.g_pull_correlation_id, test_on_data_available_cb_cancelled, (void*)&pull_context, &pull_op);
    ASSERT_IS_NOT_NULL(pull_op);
    async_op_cancel(pull_op);

    //wait for callback to execute
    InterlockedHL_WaitForNotValue(&pull_context, TEST_ORIGINAL_VALUE, UINT32_MAX);

    /// assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_ARE_EQUAL(int32_t, pull_cancelled, interlocked_add(&pull_context, 0));

    /// cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    channel_close(channel);

    /// act
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    volatile_atomic int32_t push_context;
    (void)interlocked_exchange(&push_context, TEST_ORIGINAL_VALUE);
    THANDLE(ASYNC_OP) push_op = NULL;
    CHANNEL_RESULT push_result = channel_push(channel, g.g_push_correlation_id, g.g_data, test_on_data_consumed_cb_cancelled, (void*)&push_context, &push_op);
    ASSERT_IS_NOT_NULL(push_op);
    async_op_cancel(push_op);

    //wait for callback to execute
    InterlockedHL_WaitForNotValue(&push_context, TEST_ORIGINAL_VALUE, UINT32_MAX);

    /// assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result);
    ASSERT_ARE_EQUAL(int32_t, push_cancelled, interlocked_add(&push_context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&push_op, NULL);
    channel_close(channel);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}


TEST_FUNCTION(test_pull_and_then_push)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(NULL, g.g_threadpool);
    ASSERT_IS_NOT_NULL(channel);
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    volatile_atomic int32_t pull_context;
    (void)interlocked_exchange(&pull_context, TEST_ORIGINAL_VALUE);
    volatile_atomic int32_t push_context;
    (void)interlocked_exchange(&push_context, TEST_ORIGINAL_VALUE);

    /// act
    THANDLE(ASYNC_OP) pull_op = NULL;
    THANDLE(ASYNC_OP) push_op = NULL;
    CHANNEL_RESULT pull_result = channel_pull(channel, g.g_pull_correlation_id, test_on_data_available_cb_success, (void*)&pull_context, &pull_op);
    ASSERT_IS_NOT_NULL(pull_op);
    CHANNEL_RESULT push_result = channel_push(channel, g.g_push_correlation_id, g.g_data, test_on_data_consumed_cb_success, (void*)&push_context, &push_op);
    ASSERT_IS_NOT_NULL(push_op);

    //wait for callback to execute
    InterlockedHL_WaitForNotValue(&pull_context, TEST_ORIGINAL_VALUE, UINT32_MAX);
    InterlockedHL_WaitForNotValue(&push_context, TEST_ORIGINAL_VALUE, UINT32_MAX);

    /// assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result);
    ASSERT_ARE_EQUAL(int32_t, pull_success, interlocked_add(&pull_context, 0));
    ASSERT_ARE_EQUAL(int32_t, push_success, interlocked_add(&push_context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&push_op, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    channel_close(channel);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

TEST_FUNCTION(test_push_and_then_pull)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(NULL, g.g_threadpool);
    ASSERT_IS_NOT_NULL(channel);
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    volatile_atomic int32_t pull_context;
    (void)interlocked_exchange(&pull_context, TEST_ORIGINAL_VALUE);
    volatile_atomic int32_t push_context;
    (void)interlocked_exchange(&push_context, TEST_ORIGINAL_VALUE);

    /// act
    THANDLE(ASYNC_OP) push_op = NULL;
    THANDLE(ASYNC_OP) pull_op = NULL;
    CHANNEL_RESULT push_result = channel_push(channel, g.g_push_correlation_id, g.g_data, test_on_data_consumed_cb_success, (void*)&push_context, &push_op);
    ASSERT_IS_NOT_NULL(push_op);
    CHANNEL_RESULT pull_result = channel_pull(channel, g.g_pull_correlation_id, test_on_data_available_cb_success, (void*)&pull_context, &pull_op);
    ASSERT_IS_NOT_NULL(pull_op);

    //wait for callback to execute
    InterlockedHL_WaitForNotValue(&pull_context, TEST_ORIGINAL_VALUE, UINT32_MAX);
    InterlockedHL_WaitForNotValue(&push_context, TEST_ORIGINAL_VALUE, UINT32_MAX);

    /// assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_ARE_EQUAL(int32_t, pull_success, interlocked_add(&pull_context, 0));
    ASSERT_ARE_EQUAL(int32_t, push_success, interlocked_add(&push_context, 0));

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&push_op, NULL);
    channel_close(channel);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

TEST_FUNCTION(test_pull_after_pull)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(NULL, g.g_threadpool);
    ASSERT_IS_NOT_NULL(channel);
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    volatile_atomic int32_t pull_context_1;
    (void)interlocked_exchange(&pull_context_1, TEST_ORIGINAL_VALUE);
    volatile_atomic int32_t pull_context_2;
    (void)interlocked_exchange(&pull_context_2, TEST_ORIGINAL_VALUE);
    volatile_atomic int32_t push_context;
    (void)interlocked_exchange(&push_context, TEST_ORIGINAL_VALUE);

    /// act
    THANDLE(ASYNC_OP) pull_op1 = NULL;
    THANDLE(ASYNC_OP) pull_op2 = NULL;
    THANDLE(ASYNC_OP) push_op = NULL;
    CHANNEL_RESULT pull_result1 = channel_pull(channel, g.g_pull_correlation_id, test_on_data_available_cb_success, (void*)&pull_context_1, &pull_op1);
    ASSERT_IS_NOT_NULL(pull_op1);
    CHANNEL_RESULT pull_result2 = channel_pull(channel, g.g_pull_correlation_id, test_on_data_available_cb_abandoned, (void*)&pull_context_2, &pull_op2);
    ASSERT_IS_NOT_NULL(pull_op2);
    CHANNEL_RESULT push_result = channel_push(channel, g.g_push_correlation_id, g.g_data, test_on_data_consumed_cb_success, (void*)&push_context, &push_op);
    ASSERT_IS_NOT_NULL(push_op);
    channel_close(channel);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);

    //wait for callback to execute
    InterlockedHL_WaitForNotValue(&pull_context_1, TEST_ORIGINAL_VALUE, UINT32_MAX);
    InterlockedHL_WaitForNotValue(&pull_context_2, TEST_ORIGINAL_VALUE, UINT32_MAX);
    InterlockedHL_WaitForNotValue(&push_context, TEST_ORIGINAL_VALUE, UINT32_MAX);

    /// assert
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
}

TEST_FUNCTION(test_push_after_push)
{
    /// arrange
    THANDLE(CHANNEL) channel = channel_create(NULL, g.g_threadpool);
    ASSERT_IS_NOT_NULL(channel);
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));

    volatile_atomic int32_t push_context_1;
    (void)interlocked_exchange(&push_context_1, TEST_ORIGINAL_VALUE);

    volatile_atomic int32_t push_context_2;
    (void)interlocked_exchange(&push_context_2, TEST_ORIGINAL_VALUE);

    volatile_atomic int32_t pull_context;
    (void)interlocked_exchange(&pull_context, TEST_ORIGINAL_VALUE);

    /// act
    THANDLE(ASYNC_OP) push_op1 = NULL;
    THANDLE(ASYNC_OP) push_op2 = NULL;
    THANDLE(ASYNC_OP) pull_op = NULL;
    CHANNEL_RESULT push_result1 = channel_push(channel, g.g_push_correlation_id, g.g_data, test_on_data_consumed_cb_success, (void*)&push_context_1, &push_op1);
    ASSERT_IS_NOT_NULL(push_op1);
    CHANNEL_RESULT push_result2 = channel_push(channel, g.g_push_correlation_id, g.g_data2, test_on_data_consumed_cb_abandoned, (void*)&push_context_2, &push_op2);
    ASSERT_IS_NOT_NULL(push_op2);
    CHANNEL_RESULT pull_result = channel_pull(channel, g.g_pull_correlation_id, test_on_data_available_cb_success, (void*)&pull_context, &pull_op);
    ASSERT_IS_NOT_NULL(pull_op);
    channel_close(channel);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);

    //wait for callback to execute
    InterlockedHL_WaitForNotValue(&push_context_1, TEST_ORIGINAL_VALUE, UINT32_MAX);
    InterlockedHL_WaitForNotValue(&push_context_2, TEST_ORIGINAL_VALUE, UINT32_MAX);
    InterlockedHL_WaitForNotValue(&pull_context, TEST_ORIGINAL_VALUE, UINT32_MAX);

    /// assert
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
}

// TODO: Fix threadpool_linux and then enable this test. Task 31404616
#ifdef WIN32
TEST_FUNCTION(test_channel_maintains_data_order)
{
    //arrange
    THANDLE(CHANNEL) channel = channel_create(NULL, g.g_threadpool);
    ASSERT_IS_NOT_NULL(channel);
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    (void)interlocked_exchange(&g_on_data_consumed_cb_count, 0);

    THREAD_HANDLE pull_thread;
    THREAD_HANDLE push_thread;
    int pull_result;
    int push_result;

    //act
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&pull_thread, pull_data, (void*)channel));
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&push_thread, push_data, (void*)channel));


    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(pull_thread, &pull_result));
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(push_thread, &push_result));
    ASSERT_ARE_EQUAL(int, 0, pull_result);
    ASSERT_ARE_EQUAL(int, 0, push_result);

    //assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&g_on_data_consumed_cb_count, CHANNEL_ORDER_TEST_COUNT, UINT32_MAX));

    //cleanup
    channel_close(channel);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}
#endif

TEST_FUNCTION(test_close_does_not_get_stuck)
{
    for (size_t run_iteration = 0; run_iteration < 100; run_iteration++)
    {
        //arrange
        THANDLE(CHANNEL) channel = channel_create(NULL, g.g_threadpool);
        ASSERT_IS_NOT_NULL(channel);
        ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
        (void)interlocked_exchange(&test_signal, 0);

        //act
        THREAD_HANDLE pull_threads[CHANNEL_CLOSE_TEST_THREAD_COUNT];

        for (size_t i = 0; i < CHANNEL_CLOSE_TEST_THREAD_COUNT; i++)
        {
            ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&pull_threads[i], pull_once, (void*)channel));
        }

        THREAD_HANDLE close_thread;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&close_thread, close_channel, (void*)channel));

        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWakeAll(&test_signal, 1));

        for (size_t i = 0; i < CHANNEL_CLOSE_TEST_THREAD_COUNT; i++)
        {
            int pull_result;
            ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(pull_threads[i], &pull_result));
            ASSERT_ARE_EQUAL(int, 0, pull_result);
        }
        int close_result;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(close_thread, &close_result));
        ASSERT_ARE_EQUAL(int, 0, close_result);

        //cleanup
        THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
