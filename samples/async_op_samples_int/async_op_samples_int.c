// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/sync.h"
#include "c_pal/thandle.h"
#include "c_pal/threadapi.h"

#include "c_util/async_op.h"

#include "common_async_op_module_interface.h"

#include "ll_async_op_module_fake_cancel.h"
#include "ll_async_op_module_real_cancel.h"

#include "ml_async_op_module.h"
#include "ml_async_op_module_with_async_chain.h"
#include "ml_async_op_module_with_retries.h"

#include "hl_async_op_module.h"
#include "hl_async_op_module_cancel_all.h"

TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_RESULT_VALUES)

typedef struct TEST_MODULES_TAG
{
    LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE ll_async_module_real_cancel;
    LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE ll_async_module_fake_cancel;
    ML_ASYNC_OP_MODULE_HANDLE ml_async_module;
    ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_HANDLE ml_async_module_with_async_chain;
    ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE ml_async_module_with_retries;
    HL_ASYNC_OP_MODULE_HANDLE hl_async_module;
    //HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE hl_async_module_cancel_all;
} TEST_MODULES;

static TEST_MODULES test_modules;

static void create_modules(void)
{
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);

    test_modules.ll_async_module_fake_cancel = ll_async_op_module_fake_cancel_create(execution_engine);
    ASSERT_IS_NOT_NULL(test_modules.ll_async_module_fake_cancel);

    test_modules.ll_async_module_real_cancel = ll_async_op_module_real_cancel_create(execution_engine);
    ASSERT_IS_NOT_NULL(test_modules.ll_async_module_real_cancel);

    test_modules.ml_async_module = ml_async_op_module_create(execution_engine, test_modules.ll_async_module_fake_cancel, test_modules.ll_async_module_real_cancel);
    ASSERT_IS_NOT_NULL(test_modules.ml_async_module);

    test_modules.ml_async_module_with_async_chain = ml_async_op_module_with_async_chain_create(execution_engine, test_modules.ll_async_module_fake_cancel, test_modules.ll_async_module_real_cancel);
    ASSERT_IS_NOT_NULL(test_modules.ml_async_module_with_async_chain);

    test_modules.ml_async_module_with_retries = ml_async_op_module_with_retries_create(execution_engine, test_modules.ll_async_module_fake_cancel, test_modules.ll_async_module_real_cancel);
    ASSERT_IS_NOT_NULL(test_modules.ml_async_module_with_retries);

    test_modules.hl_async_module = hl_async_op_module_create(execution_engine, test_modules.ml_async_module, test_modules.ml_async_module_with_async_chain, test_modules.ml_async_module_with_retries);
    ASSERT_IS_NOT_NULL(test_modules.hl_async_module);

    execution_engine_dec_ref(execution_engine);
}

static void open_modules(void)
{
    ASSERT_ARE_EQUAL(int, 0, ll_async_op_module_fake_cancel_open(test_modules.ll_async_module_fake_cancel));
    ASSERT_ARE_EQUAL(int, 0, ll_async_op_module_real_cancel_open(test_modules.ll_async_module_real_cancel));

    ASSERT_ARE_EQUAL(int, 0, ml_async_op_module_open(test_modules.ml_async_module));
    ASSERT_ARE_EQUAL(int, 0, ml_async_op_module_with_async_chain_open(test_modules.ml_async_module_with_async_chain));
    ASSERT_ARE_EQUAL(int, 0, ml_async_op_module_with_retries_open(test_modules.ml_async_module_with_retries));

    ASSERT_ARE_EQUAL(int, 0, hl_async_op_module_open(test_modules.hl_async_module));
}

static void close_modules(void)
{
    hl_async_op_module_close(test_modules.hl_async_module);

    ml_async_op_module_with_retries_close(test_modules.ml_async_module_with_retries);
    ml_async_op_module_with_async_chain_close(test_modules.ml_async_module_with_async_chain);
    ml_async_op_module_close(test_modules.ml_async_module);

    ll_async_op_module_real_cancel_close(test_modules.ll_async_module_real_cancel);
    ll_async_op_module_fake_cancel_close(test_modules.ll_async_module_fake_cancel);
}

static void destroy_modules(void)
{
    hl_async_op_module_destroy(test_modules.hl_async_module);
    test_modules.hl_async_module = NULL;

    ml_async_op_module_with_retries_destroy(test_modules.ml_async_module_with_retries);
    test_modules.ml_async_module_with_retries = NULL;

    ml_async_op_module_with_async_chain_destroy(test_modules.ml_async_module_with_async_chain);
    test_modules.ml_async_module_with_async_chain = NULL;

    ml_async_op_module_destroy(test_modules.ml_async_module);
    test_modules.ml_async_module = NULL;

    ll_async_op_module_real_cancel_destroy(test_modules.ll_async_module_real_cancel);
    test_modules.ll_async_module_real_cancel = NULL;

    ll_async_op_module_fake_cancel_destroy(test_modules.ll_async_module_fake_cancel);
    test_modules.ll_async_module_fake_cancel = NULL;
}

typedef struct OPERATION_RESULT_CONTEXT_TAG
{
    COMMON_ASYNC_OP_MODULE_RESULT result;
    volatile_atomic int32_t callback_called;
} OPERATION_RESULT_CONTEXT;

static void test_ASYNC_OP_MODULE_CALLBACK(void* context, COMMON_ASYNC_OP_MODULE_RESULT result)
{
    OPERATION_RESULT_CONTEXT* result_context = context;
    result_context->result = result;
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_compare_exchange(&result_context->callback_called, 1, 0), "callback must only be called once");
    wake_by_address_all(&result_context->callback_called);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(setsBufferTempSize)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(function_init)
{
    create_modules();
    open_modules();
}

TEST_FUNCTION_CLEANUP(cleans)
{
    close_modules();
    destroy_modules();
}

// ll_async_op_module_fake_cancel

TEST_FUNCTION(ll_async_op_module_fake_cancel_execute_async_succeeds)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ll_async_op_module_fake_cancel_execute_async(test_modules.ll_async_module_fake_cancel, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ll_async_op_module_fake_cancel_execute_async_can_be_canceled)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ll_async_op_module_fake_cancel_execute_async(test_modules.ll_async_module_fake_cancel, 2000, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);

    // act
    (void)async_op_cancel(async_op);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 0 /* Callback should have come immediately */));
    // Note that the cleanup will block for 2 seconds because the real operation must still complete
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_CANCELED, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ll_async_op_module_fake_cancel_execute_async_succeeds_synchronously)
{
    // arrange
    ll_async_op_module_fake_cancel_next_call_completes_synchronously(test_modules.ll_async_module_fake_cancel, true);

    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ll_async_op_module_fake_cancel_execute_async(test_modules.ll_async_module_fake_cancel, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ll_async_op_module_fake_cancel_execute_async_succeeds_then_cancel_is_noop)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ll_async_op_module_fake_cancel_execute_async(test_modules.ll_async_module_fake_cancel, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // act
    (void)async_op_cancel(async_op);

    // assert
    // Nothing crashes

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

// ll_async_op_module_real_cancel

TEST_FUNCTION(ll_async_op_module_real_cancel_execute_async_succeeds)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ll_async_op_module_real_cancel_execute_async(test_modules.ll_async_module_real_cancel, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ll_async_op_module_real_cancel_execute_async_can_be_canceled)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ll_async_op_module_real_cancel_execute_async(test_modules.ll_async_module_real_cancel, 60000 /* would block for a very long time */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);

    // act
    (void)async_op_cancel(async_op);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_CANCELED, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ll_async_op_module_real_cancel_execute_async_succeeds_synchronously)
{
    // arrange
    ll_async_op_module_real_cancel_next_call_completes_synchronously(test_modules.ll_async_module_real_cancel, true);

    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ll_async_op_module_real_cancel_execute_async(test_modules.ll_async_module_real_cancel, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ll_async_op_module_real_cancel_execute_async_succeeds_then_cancel_is_noop)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ll_async_op_module_real_cancel_execute_async(test_modules.ll_async_module_real_cancel, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // act
    (void)async_op_cancel(async_op);

    // assert
    // Nothing crashes

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

// ml_async_op_module

TEST_FUNCTION(ml_async_op_module_execute_underlying_fake_cancel_async_succeeds)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ml_async_op_module_execute_underlying_fake_cancel_async(test_modules.ml_async_module, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_execute_underlying_fake_cancel_async_can_be_canceled)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_execute_underlying_fake_cancel_async(test_modules.ml_async_module, 2000, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);

    // act
    (void)async_op_cancel(async_op);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 0 /* Callback should have come immediately */));
    // Note that the cleanup will block for 2 seconds because the real operation must still complete
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_CANCELED, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_execute_underlying_fake_cancel_async_succeeds_synchronously)
{
    // arrange
    ll_async_op_module_fake_cancel_next_call_completes_synchronously(test_modules.ll_async_module_fake_cancel, true);

    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ml_async_op_module_execute_underlying_fake_cancel_async(test_modules.ml_async_module, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_execute_underlying_fake_cancel_async_succeeds_then_cancel_is_noop)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_execute_underlying_fake_cancel_async(test_modules.ml_async_module, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // act
    (void)async_op_cancel(async_op);

    // assert
    // Nothing crashes

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_execute_underlying_real_cancel_async_succeeds)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ml_async_op_module_execute_underlying_real_cancel_async(test_modules.ml_async_module, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_execute_underlying_real_cancel_async_can_be_canceled)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_execute_underlying_real_cancel_async(test_modules.ml_async_module, 60000 /* would block for a very long time */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);

    // act
    (void)async_op_cancel(async_op);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_CANCELED, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_execute_underlying_real_cancel_async_succeeds_synchronously)
{
    // arrange
    ll_async_op_module_real_cancel_next_call_completes_synchronously(test_modules.ll_async_module_real_cancel, true);

    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ml_async_op_module_execute_underlying_real_cancel_async(test_modules.ml_async_module, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_execute_underlying_real_cancel_async_succeeds_then_cancel_is_noop)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_execute_underlying_real_cancel_async(test_modules.ml_async_module, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // act
    (void)async_op_cancel(async_op);

    // assert
    // Nothing crashes

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

// ml_async_op_module_with_async_chain

TEST_FUNCTION(ml_async_op_module_with_async_chain_execute_underlying_fake_cancel_async_succeeds)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ml_async_op_module_with_async_chain_execute_underlying_fake_cancel_async(test_modules.ml_async_module_with_async_chain, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_async_chain_execute_underlying_fake_cancel_async_can_be_canceled)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_with_async_chain_execute_underlying_fake_cancel_async(test_modules.ml_async_module_with_async_chain, 2000, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);

    // act
    (void)async_op_cancel(async_op);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 0 /* Callback should have come immediately */));
    // Note that the cleanup will block for 2 seconds because the real operation must still complete
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_CANCELED, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_async_chain_execute_underlying_fake_cancel_async_succeeds_synchronously)
{
    // arrange
    ll_async_op_module_fake_cancel_next_call_completes_synchronously(test_modules.ll_async_module_fake_cancel, true);

    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ml_async_op_module_with_async_chain_execute_underlying_fake_cancel_async(test_modules.ml_async_module_with_async_chain, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_async_chain_execute_underlying_fake_cancel_async_succeeds_then_cancel_is_noop)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_with_async_chain_execute_underlying_fake_cancel_async(test_modules.ml_async_module_with_async_chain, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // act
    (void)async_op_cancel(async_op);

    // assert
    // Nothing crashes

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_async_chain_execute_underlying_real_cancel_async_succeeds)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ml_async_op_module_with_async_chain_execute_underlying_real_cancel_async(test_modules.ml_async_module_with_async_chain, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_async_chain_execute_underlying_real_cancel_async_can_be_canceled)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_with_async_chain_execute_underlying_real_cancel_async(test_modules.ml_async_module_with_async_chain, 60000 /* would block for a very long time */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);

    // act
    (void)async_op_cancel(async_op);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_CANCELED, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_async_chain_execute_underlying_real_cancel_async_succeeds_synchronously)
{
    // arrange
    ll_async_op_module_real_cancel_next_call_completes_synchronously(test_modules.ll_async_module_real_cancel, true);

    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ml_async_op_module_with_async_chain_execute_underlying_real_cancel_async(test_modules.ml_async_module_with_async_chain, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_async_chain_execute_underlying_real_cancel_async_succeeds_then_cancel_is_noop)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_with_async_chain_execute_underlying_real_cancel_async(test_modules.ml_async_module_with_async_chain, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // act
    (void)async_op_cancel(async_op);

    // assert
    // Nothing crashes

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

// ml_async_op_module_with_retries

TEST_FUNCTION(ml_async_op_module_with_retries_execute_underlying_fake_cancel_async_succeeds)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ml_async_op_module_with_retries_execute_underlying_fake_cancel_async(test_modules.ml_async_module_with_retries, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_retries_execute_underlying_fake_cancel_async_with_3_retries_succeeds)
{
    // arrange
    ll_async_op_module_fake_cancel_set_report_retry_result_count(test_modules.ll_async_module_fake_cancel, 3);

    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ml_async_op_module_with_retries_execute_underlying_fake_cancel_async(test_modules.ml_async_module_with_retries, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_retries_execute_underlying_fake_cancel_async_can_be_canceled)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_with_retries_execute_underlying_fake_cancel_async(test_modules.ml_async_module_with_retries, 2000, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);

    // act
    (void)async_op_cancel(async_op);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 0 /* Callback should have come immediately */));
    // Note that the cleanup will block for 2 seconds because the real operation must still complete
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_CANCELED, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_retries_execute_underlying_fake_cancel_async_with_many_retries_can_be_canceled)
{
    // arrange
    ll_async_op_module_fake_cancel_set_report_retry_result_count(test_modules.ll_async_module_fake_cancel, INT32_MAX); // more or less infinitely retry, but cancel will happen

    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_with_retries_execute_underlying_fake_cancel_async(test_modules.ml_async_module_with_retries, 1, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);

    ThreadAPI_Sleep(10); // give it a chance to retry

    // act
    (void)async_op_cancel(async_op);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 0 /* Callback should have come immediately */));
    // Note that the cleanup will block for 2 seconds because the real operation must still complete
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_CANCELED, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_retries_execute_underlying_fake_cancel_async_succeeds_synchronously)
{
    // arrange
    ll_async_op_module_fake_cancel_next_call_completes_synchronously(test_modules.ll_async_module_fake_cancel, true);

    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ml_async_op_module_with_retries_execute_underlying_fake_cancel_async(test_modules.ml_async_module_with_retries, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_retries_execute_underlying_fake_cancel_async_succeeds_synchronously_with_3_retries)
{
    // arrange
    ll_async_op_module_fake_cancel_set_report_retry_result_count(test_modules.ll_async_module_fake_cancel, 3);
    ll_async_op_module_fake_cancel_next_call_completes_synchronously(test_modules.ll_async_module_fake_cancel, true);

    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ml_async_op_module_with_retries_execute_underlying_fake_cancel_async(test_modules.ml_async_module_with_retries, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_retries_execute_underlying_fake_cancel_async_succeeds_then_cancel_is_noop)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_with_retries_execute_underlying_fake_cancel_async(test_modules.ml_async_module_with_retries, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // act
    (void)async_op_cancel(async_op);

    // assert
    // Nothing crashes

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_retries_execute_underlying_fake_cancel_async_succeeds_after_3_retries_then_cancel_is_noop)
{
    // arrange
    ll_async_op_module_fake_cancel_set_report_retry_result_count(test_modules.ll_async_module_fake_cancel, 3);

    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_with_retries_execute_underlying_fake_cancel_async(test_modules.ml_async_module_with_retries, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // act
    (void)async_op_cancel(async_op);

    // assert
    // Nothing crashes

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_retries_execute_underlying_real_cancel_async_succeeds)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ml_async_op_module_with_retries_execute_underlying_real_cancel_async(test_modules.ml_async_module_with_retries, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_retries_execute_underlying_real_cancel_async_with_3_retries_succeeds)
{
    // arrange
    ll_async_op_module_real_cancel_set_report_retry_result_count(test_modules.ll_async_module_real_cancel, 3);

    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ml_async_op_module_with_retries_execute_underlying_real_cancel_async(test_modules.ml_async_module_with_retries, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_retries_execute_underlying_real_cancel_async_can_be_canceled)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_with_retries_execute_underlying_real_cancel_async(test_modules.ml_async_module_with_retries, 60000 /* would block for a very long time */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);

    // act
    (void)async_op_cancel(async_op);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_CANCELED, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_retries_execute_underlying_real_cancel_async_with_many_retries_can_be_canceled)
{
    // arrange
    ll_async_op_module_real_cancel_set_report_retry_result_count(test_modules.ll_async_module_real_cancel, INT32_MAX); // more or less infinitely retry, but cancel will happen

    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_with_retries_execute_underlying_real_cancel_async(test_modules.ml_async_module_with_retries, 1, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);

    ThreadAPI_Sleep(10); // give it a chance to retry

    // act
    (void)async_op_cancel(async_op);

    // assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_CANCELED, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_retries_execute_underlying_real_cancel_async_succeeds_synchronously)
{
    // arrange
    ll_async_op_module_real_cancel_next_call_completes_synchronously(test_modules.ll_async_module_real_cancel, true);

    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    // act
    int result = ml_async_op_module_with_retries_execute_underlying_real_cancel_async(test_modules.ml_async_module_with_retries, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_retries_execute_underlying_real_cancel_async_succeeds_then_cancel_is_noop)
{
    // arrange
    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_with_retries_execute_underlying_real_cancel_async(test_modules.ml_async_module_with_retries, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // act
    (void)async_op_cancel(async_op);

    // assert
    // Nothing crashes

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

TEST_FUNCTION(ml_async_op_module_with_retries_execute_underlying_real_cancel_async_succeeds_after_3_retries_then_cancel_is_noop)
{
    // arrange
    ll_async_op_module_real_cancel_set_report_retry_result_count(test_modules.ll_async_module_real_cancel, 3);

    THANDLE(ASYNC_OP) async_op = NULL;
    OPERATION_RESULT_CONTEXT result_context;
    result_context.result = 0;
    (void)interlocked_exchange(&result_context.callback_called, 0);

    int result = ml_async_op_module_with_retries_execute_underlying_real_cancel_async(test_modules.ml_async_module_with_retries, 10, &async_op, test_ASYNC_OP_MODULE_CALLBACK, &result_context);

    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context.callback_called, 1, 1000));
    ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context.result);

    // act
    (void)async_op_cancel(async_op);

    // assert
    // Nothing crashes

    // cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
