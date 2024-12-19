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

typedef struct TEST_MODULE_TAG
{
    COMMON_ASYNC_OP_MODULE_INTERFACE module_interface;
    const char* module_name;
    bool has_retry_support;
    bool exposes_async_op;
} TEST_MODULE;

static void create_all_modules_with_fake_cancel(EXECUTION_ENGINE_HANDLE execution_engine, uint32_t* module_count, TEST_MODULE** modules, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE* ll_async_module_fake_cancel)
{
    *module_count = 13;
    *modules = malloc_2(*module_count, sizeof(TEST_MODULE));
    ASSERT_IS_NOT_NULL(*modules);

    uint32_t i = 0;

    *ll_async_module_fake_cancel = ll_async_op_module_fake_cancel_create(execution_engine);
    ASSERT_IS_NOT_NULL(*ll_async_module_fake_cancel);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = ll_async_op_module_fake_cancel_get_interface(*ll_async_module_fake_cancel);
    COMMON_OP_MODULE_INTERFACE_HANDLE ll_async_op_module_fake_cancel_interface = &(*modules)[i].module_interface;
    (*modules)[i].module_name = "ll_async_op_module_fake_cancel";
    (*modules)[i].has_retry_support = false;
    (*modules)[i].exposes_async_op = true;
    i++;

    ML_ASYNC_OP_MODULE_HANDLE ml_async_module = ml_async_op_module_create(execution_engine, ll_async_op_module_fake_cancel_interface);
    ASSERT_IS_NOT_NULL(ml_async_module);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = ml_async_op_module_get_interface(ml_async_module);
    COMMON_OP_MODULE_INTERFACE_HANDLE ml_async_op_module_interface = &(*modules)[i].module_interface;
    (*modules)[i].module_name = "ml_async_op_module";
    (*modules)[i].has_retry_support = false;
    (*modules)[i].exposes_async_op = true;
    i++;

    ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_HANDLE ml_async_module_with_async_chain = ml_async_op_module_with_async_chain_create(execution_engine, ll_async_op_module_fake_cancel_interface);
    ASSERT_IS_NOT_NULL(ml_async_module_with_async_chain);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = ml_async_op_module_with_async_chain_get_interface(ml_async_module_with_async_chain);
    COMMON_OP_MODULE_INTERFACE_HANDLE ml_async_module_with_async_chain_interface = &(*modules)[i].module_interface;
    (*modules)[i].module_name = "ml_async_op_module_with_async_chain";
    (*modules)[i].has_retry_support = false;
    (*modules)[i].exposes_async_op = true;
    i++;

    ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE ml_async_module_with_retries = ml_async_op_module_with_retries_create(execution_engine, ll_async_op_module_fake_cancel_interface);
    ASSERT_IS_NOT_NULL(ml_async_module_with_retries);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = ml_async_op_module_with_retries_get_interface(ml_async_module_with_retries);
    COMMON_OP_MODULE_INTERFACE_HANDLE ml_async_op_module_with_retries_interface = &(*modules)[i].module_interface;
    (*modules)[i].module_name = "ml_async_op_module_with_retries";
    (*modules)[i].has_retry_support = true;
    (*modules)[i].exposes_async_op = true;
    i++;

    ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_HANDLE ml_async_module_with_async_chain_over_retries = ml_async_op_module_with_async_chain_create(execution_engine, ml_async_op_module_with_retries_interface);
    ASSERT_IS_NOT_NULL(ml_async_module_with_async_chain_over_retries);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = ml_async_op_module_with_async_chain_get_interface(ml_async_module_with_async_chain_over_retries);
    COMMON_OP_MODULE_INTERFACE_HANDLE ml_async_module_with_async_chain_over_retries_interface = &(*modules)[i].module_interface;
    (*modules)[i].module_name = "ml_async_module_with_async_chain_over_retries";
    (*modules)[i].has_retry_support = true;
    (*modules)[i].exposes_async_op = true;
    i++;

    HL_ASYNC_OP_MODULE_HANDLE hl_async_module1 = hl_async_op_module_create(execution_engine, ml_async_op_module_interface);
    ASSERT_IS_NOT_NULL(hl_async_module1);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_get_interface(hl_async_module1);
    (*modules)[i].module_name = "hl_async_module over ml_async_op_module";
    (*modules)[i].has_retry_support = false;
    (*modules)[i].exposes_async_op = true;
    i++;

    HL_ASYNC_OP_MODULE_HANDLE hl_async_module2 = hl_async_op_module_create(execution_engine, ml_async_module_with_async_chain_interface);
    ASSERT_IS_NOT_NULL(hl_async_module2);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_get_interface(hl_async_module2);
    (*modules)[i].module_name = "hl_async_module over ml_async_module_with_async_chain";
    (*modules)[i].has_retry_support = false;
    (*modules)[i].exposes_async_op = true;
    i++;

    HL_ASYNC_OP_MODULE_HANDLE hl_async_module3 = hl_async_op_module_create(execution_engine, ml_async_op_module_with_retries_interface);
    ASSERT_IS_NOT_NULL(hl_async_module3);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_get_interface(hl_async_module3);
    (*modules)[i].module_name = "hl_async_module over ml_async_op_module_with_retries";
    (*modules)[i].has_retry_support = true;
    (*modules)[i].exposes_async_op = true;
    i++;

    HL_ASYNC_OP_MODULE_HANDLE hl_async_module4 = hl_async_op_module_create(execution_engine, ml_async_module_with_async_chain_over_retries_interface);
    ASSERT_IS_NOT_NULL(hl_async_module4);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_get_interface(hl_async_module4);
    (*modules)[i].module_name = "hl_async_module over ml_async_module_with_async_chain_over_retries";
    (*modules)[i].has_retry_support = true;
    (*modules)[i].exposes_async_op = true;
    i++;

    HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE hl_async_module_cancel_all_1 = hl_async_op_module_cancel_all_create(execution_engine, ml_async_op_module_interface);
    ASSERT_IS_NOT_NULL(hl_async_module_cancel_all_1);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_cancel_all_get_interface(hl_async_module_cancel_all_1);
    (*modules)[i].module_name = "hl_async_module_cancel_all over ml_async_op_module";
    (*modules)[i].has_retry_support = false;
    (*modules)[i].exposes_async_op = false;
    i++;

    HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE hl_async_module_cancel_all_2 = hl_async_op_module_cancel_all_create(execution_engine, ml_async_module_with_async_chain_interface);
    ASSERT_IS_NOT_NULL(hl_async_module_cancel_all_2);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_cancel_all_get_interface(hl_async_module_cancel_all_2);
    (*modules)[i].module_name = "hl_async_module_cancel_all over ml_async_module_with_async_chain";
    (*modules)[i].has_retry_support = false;
    (*modules)[i].exposes_async_op = false;
    i++;

    HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE hl_async_module_cancel_all_3 = hl_async_op_module_cancel_all_create(execution_engine, ml_async_op_module_with_retries_interface);
    ASSERT_IS_NOT_NULL(hl_async_module_cancel_all_3);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_cancel_all_get_interface(hl_async_module_cancel_all_3);
    (*modules)[i].module_name = "hl_async_module_cancel_all over ml_async_op_module_with_retries";
    (*modules)[i].has_retry_support = true;
    (*modules)[i].exposes_async_op = false;
    i++;

    HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE hl_async_module_cancel_all_4 = hl_async_op_module_cancel_all_create(execution_engine, ml_async_module_with_async_chain_over_retries_interface);
    ASSERT_IS_NOT_NULL(hl_async_module_cancel_all_4);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_cancel_all_get_interface(hl_async_module_cancel_all_4);
    (*modules)[i].module_name = "hl_async_module_cancel_all over ml_async_module_with_async_chain_over_retries";
    (*modules)[i].has_retry_support = true;
    (*modules)[i].exposes_async_op = false;
    i++;

    ASSERT_ARE_EQUAL(uint32_t, *module_count, i);
}

static void create_all_modules_with_real_cancel(EXECUTION_ENGINE_HANDLE execution_engine, uint32_t* module_count, TEST_MODULE** modules, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE* ll_async_module_real_cancel)
{
    *module_count = 13;
    *modules = malloc_2(*module_count, sizeof(TEST_MODULE));
    ASSERT_IS_NOT_NULL(*modules);

    uint32_t i = 0;

    *ll_async_module_real_cancel = ll_async_op_module_real_cancel_create(execution_engine);
    ASSERT_IS_NOT_NULL(*ll_async_module_real_cancel);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = ll_async_op_module_real_cancel_get_interface(*ll_async_module_real_cancel);
    COMMON_OP_MODULE_INTERFACE_HANDLE ll_async_op_module_real_cancel_interface = &(*modules)[i].module_interface;
    (*modules)[i].module_name = "ll_async_op_module_real_cancel";
    (*modules)[i].has_retry_support = false;
    (*modules)[i].exposes_async_op = true;
    i++;

    ML_ASYNC_OP_MODULE_HANDLE ml_async_module = ml_async_op_module_create(execution_engine, ll_async_op_module_real_cancel_interface);
    ASSERT_IS_NOT_NULL(ml_async_module);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = ml_async_op_module_get_interface(ml_async_module);
    COMMON_OP_MODULE_INTERFACE_HANDLE ml_async_op_module_interface = &(*modules)[i].module_interface;
    (*modules)[i].module_name = "ml_async_op_module";
    (*modules)[i].has_retry_support = false;
    (*modules)[i].exposes_async_op = true;
    i++;

    ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_HANDLE ml_async_module_with_async_chain = ml_async_op_module_with_async_chain_create(execution_engine, ll_async_op_module_real_cancel_interface);
    ASSERT_IS_NOT_NULL(ml_async_module_with_async_chain);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = ml_async_op_module_with_async_chain_get_interface(ml_async_module_with_async_chain);
    COMMON_OP_MODULE_INTERFACE_HANDLE ml_async_module_with_async_chain_interface = &(*modules)[i].module_interface;
    (*modules)[i].module_name = "ml_async_op_module_with_async_chain";
    (*modules)[i].has_retry_support = false;
    (*modules)[i].exposes_async_op = true;
    i++;

    ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE ml_async_module_with_retries = ml_async_op_module_with_retries_create(execution_engine, ll_async_op_module_real_cancel_interface);
    ASSERT_IS_NOT_NULL(ml_async_module_with_retries);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = ml_async_op_module_with_retries_get_interface(ml_async_module_with_retries);
    COMMON_OP_MODULE_INTERFACE_HANDLE ml_async_op_module_with_retries_interface = &(*modules)[i].module_interface;
    (*modules)[i].module_name = "ml_async_op_module_with_retries";
    (*modules)[i].has_retry_support = true;
    (*modules)[i].exposes_async_op = true;
    i++;

    ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_HANDLE ml_async_module_with_async_chain_over_retries = ml_async_op_module_with_async_chain_create(execution_engine, ml_async_op_module_with_retries_interface);
    ASSERT_IS_NOT_NULL(ml_async_module_with_async_chain_over_retries);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = ml_async_op_module_with_async_chain_get_interface(ml_async_module_with_async_chain_over_retries);
    COMMON_OP_MODULE_INTERFACE_HANDLE ml_async_module_with_async_chain_over_retries_interface = &(*modules)[i].module_interface;
    (*modules)[i].module_name = "ml_async_module_with_async_chain_over_retries";
    (*modules)[i].has_retry_support = true;
    (*modules)[i].exposes_async_op = true;
    i++;

    HL_ASYNC_OP_MODULE_HANDLE hl_async_module1 = hl_async_op_module_create(execution_engine, ml_async_op_module_interface);
    ASSERT_IS_NOT_NULL(hl_async_module1);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_get_interface(hl_async_module1);
    (*modules)[i].module_name = "hl_async_module over ml_async_op_module";
    (*modules)[i].has_retry_support = false;
    (*modules)[i].exposes_async_op = true;
    i++;

    HL_ASYNC_OP_MODULE_HANDLE hl_async_module2 = hl_async_op_module_create(execution_engine, ml_async_module_with_async_chain_interface);
    ASSERT_IS_NOT_NULL(hl_async_module2);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_get_interface(hl_async_module2);
    (*modules)[i].module_name = "hl_async_module over ml_async_module_with_async_chain";
    (*modules)[i].has_retry_support = false;
    (*modules)[i].exposes_async_op = true;
    i++;

    HL_ASYNC_OP_MODULE_HANDLE hl_async_module3 = hl_async_op_module_create(execution_engine, ml_async_op_module_with_retries_interface);
    ASSERT_IS_NOT_NULL(hl_async_module3);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_get_interface(hl_async_module3);
    (*modules)[i].module_name = "hl_async_module over ml_async_op_module_with_retries";
    (*modules)[i].has_retry_support = true;
    (*modules)[i].exposes_async_op = true;
    i++;

    HL_ASYNC_OP_MODULE_HANDLE hl_async_module4 = hl_async_op_module_create(execution_engine, ml_async_module_with_async_chain_over_retries_interface);
    ASSERT_IS_NOT_NULL(hl_async_module4);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_get_interface(hl_async_module4);
    (*modules)[i].module_name = "hl_async_module over ml_async_module_with_async_chain_over_retries";
    (*modules)[i].has_retry_support = true;
    (*modules)[i].exposes_async_op = true;
    i++;

    HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE hl_async_module_cancel_all_1 = hl_async_op_module_cancel_all_create(execution_engine, ml_async_op_module_interface);
    ASSERT_IS_NOT_NULL(hl_async_module_cancel_all_1);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_cancel_all_get_interface(hl_async_module_cancel_all_1);
    (*modules)[i].module_name = "hl_async_module_cancel_all over ml_async_op_module";
    (*modules)[i].has_retry_support = false;
    (*modules)[i].exposes_async_op = false;
    i++;

    HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE hl_async_module_cancel_all_2 = hl_async_op_module_cancel_all_create(execution_engine, ml_async_module_with_async_chain_interface);
    ASSERT_IS_NOT_NULL(hl_async_module_cancel_all_2);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_cancel_all_get_interface(hl_async_module_cancel_all_2);
    (*modules)[i].module_name = "hl_async_module_cancel_all over ml_async_module_with_async_chain";
    (*modules)[i].has_retry_support = false;
    (*modules)[i].exposes_async_op = false;
    i++;

    HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE hl_async_module_cancel_all_3 = hl_async_op_module_cancel_all_create(execution_engine, ml_async_op_module_with_retries_interface);
    ASSERT_IS_NOT_NULL(hl_async_module_cancel_all_3);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_cancel_all_get_interface(hl_async_module_cancel_all_3);
    (*modules)[i].module_name = "hl_async_module_cancel_all over ml_async_op_module_with_retries";
    (*modules)[i].has_retry_support = true;
    (*modules)[i].exposes_async_op = false;
    i++;

    HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE hl_async_module_cancel_all_4 = hl_async_op_module_cancel_all_create(execution_engine, ml_async_module_with_async_chain_over_retries_interface);
    ASSERT_IS_NOT_NULL(hl_async_module_cancel_all_4);
    ASSERT_IS_TRUE(i < *module_count);
    (*modules)[i].module_interface = hl_async_op_module_cancel_all_get_interface(hl_async_module_cancel_all_4);
    (*modules)[i].module_name = "hl_async_module_cancel_all over ml_async_module_with_async_chain_over_retries";
    (*modules)[i].has_retry_support = true;
    (*modules)[i].exposes_async_op = false;
    i++;

    ASSERT_ARE_EQUAL(uint32_t, *module_count, i);
}


static void open_modules(uint32_t module_count, TEST_MODULE* modules)
{
    for (uint32_t i = 0; i < module_count; i++)
    {
        ASSERT_ARE_EQUAL(int, 0, modules[i].module_interface.open(modules[i].module_interface.handle));
    }
}

static void close_modules(uint32_t module_count, TEST_MODULE* modules)
{
    for (uint32_t i = 0; i < module_count; i++)
    {
        modules[i].module_interface.close(modules[i].module_interface.handle);
    }
}

static void destroy_modules(uint32_t module_count, TEST_MODULE* modules)
{
    for (uint32_t i = 0; i < module_count; i++)
    {
        modules[i].module_interface.destroy(modules[i].module_interface.handle);
    }
}

static LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE ll_async_module_fake_cancel = NULL;
static LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE ll_async_module_real_cancel = NULL;

static TEST_MODULE* g_test_modules_with_fake_cancel = NULL;
static uint32_t g_test_module_count_with_fake_cancel = 0;

static TEST_MODULE* g_test_modules_with_real_cancel = NULL;
static uint32_t g_test_module_count_with_real_cancel = 0;


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

TEST_SUITE_INITIALIZE(TestClassInit)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(function_init)
{
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(execution_engine);

    create_all_modules_with_fake_cancel(execution_engine, &g_test_module_count_with_fake_cancel, &g_test_modules_with_fake_cancel, &ll_async_module_fake_cancel);
    create_all_modules_with_real_cancel(execution_engine, &g_test_module_count_with_real_cancel, &g_test_modules_with_real_cancel, &ll_async_module_real_cancel);

    open_modules(g_test_module_count_with_fake_cancel, g_test_modules_with_fake_cancel);
    open_modules(g_test_module_count_with_real_cancel, g_test_modules_with_real_cancel);

    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION_CLEANUP(cleans)
{
    LogInfo("Cleaning up test function...");

    close_modules(g_test_module_count_with_fake_cancel, g_test_modules_with_fake_cancel);
    close_modules(g_test_module_count_with_real_cancel, g_test_modules_with_real_cancel);

    destroy_modules(g_test_module_count_with_fake_cancel, g_test_modules_with_fake_cancel);
    destroy_modules(g_test_module_count_with_real_cancel, g_test_modules_with_real_cancel);

    free(g_test_modules_with_fake_cancel);
    free(g_test_modules_with_real_cancel);

    LogInfo("Done cleaning up test function");
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_with_fake_cancel)
{
    for (uint32_t i = 0; i < g_test_module_count_with_fake_cancel; i++)
    {
        // arrange
        LogInfo("Begin Testing module %s", g_test_modules_with_fake_cancel[i].module_name);
        THANDLE(ASYNC_OP) async_op = NULL;
        OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
        ASSERT_IS_NOT_NULL(result_context);
        result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
        (void)interlocked_exchange(&result_context->callback_called, 0);
        int result;

        // act
        if (g_test_modules_with_fake_cancel[i].exposes_async_op)
        {
            result = g_test_modules_with_fake_cancel[i].module_interface.execute_async(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }
        else
        {
            result = g_test_modules_with_fake_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }

        // assert
        ASSERT_ARE_EQUAL(int, 0, result);
        if (g_test_modules_with_fake_cancel[i].exposes_async_op)
        {
            ASSERT_IS_NOT_NULL(async_op);
        }
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
        ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

        // cleanup
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        free(result_context);
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_with_real_cancel)
{
    for (uint32_t i = 0; i < g_test_module_count_with_real_cancel; i++)
    {
        // arrange
        LogInfo("Begin Testing module %s", g_test_modules_with_real_cancel[i].module_name);
        THANDLE(ASYNC_OP) async_op = NULL;
        OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
        ASSERT_IS_NOT_NULL(result_context);
        result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
        (void)interlocked_exchange(&result_context->callback_called, 0);
        int result;

        // act
        if (g_test_modules_with_real_cancel[i].exposes_async_op)
        {
            result = g_test_modules_with_real_cancel[i].module_interface.execute_async(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }
        else
        {
            result = g_test_modules_with_real_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }

        // assert
        ASSERT_ARE_EQUAL(int, 0, result);
        if (g_test_modules_with_real_cancel[i].exposes_async_op)
        {
            ASSERT_IS_NOT_NULL(async_op);
        }
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
        ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

        // cleanup
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        free(result_context);
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_on_sync_path_with_fake_cancel)
{
    for (uint32_t i = 0; i < g_test_module_count_with_fake_cancel; i++)
    {
        // arrange
        LogInfo("Begin Testing module %s", g_test_modules_with_fake_cancel[i].module_name);
        ll_async_op_module_fake_cancel_next_call_completes_synchronously(ll_async_module_fake_cancel, true);
        THANDLE(ASYNC_OP) async_op = NULL;
        OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
        ASSERT_IS_NOT_NULL(result_context);
        result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
        (void)interlocked_exchange(&result_context->callback_called, 0);
        int result;

        // act
        if (g_test_modules_with_fake_cancel[i].exposes_async_op)
        {
            result = g_test_modules_with_fake_cancel[i].module_interface.execute_async(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }
        else
        {
            result = g_test_modules_with_fake_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }

        // assert
        ASSERT_ARE_EQUAL(int, 0, result);
        if (g_test_modules_with_fake_cancel[i].exposes_async_op)
        {
            ASSERT_IS_NOT_NULL(async_op);
        }
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
        ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

        // cleanup
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        free(result_context);
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_on_sync_path_with_real_cancel)
{
    for (uint32_t i = 0; i < g_test_module_count_with_real_cancel; i++)
    {
        // arrange
        LogInfo("Begin Testing module %s", g_test_modules_with_real_cancel[i].module_name);
        ll_async_op_module_real_cancel_next_call_completes_synchronously(ll_async_module_real_cancel, true);
        THANDLE(ASYNC_OP) async_op = NULL;
        OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
        ASSERT_IS_NOT_NULL(result_context);
        result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
        (void)interlocked_exchange(&result_context->callback_called, 0);
        int result;

        // act
        if (g_test_modules_with_real_cancel[i].exposes_async_op)
        {
            result = g_test_modules_with_real_cancel[i].module_interface.execute_async(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }
        else
        {
            result = g_test_modules_with_real_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }

        // assert
        ASSERT_ARE_EQUAL(int, 0, result);
        if (g_test_modules_with_real_cancel[i].exposes_async_op)
        {
            ASSERT_IS_NOT_NULL(async_op);
        }
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
        ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

        // cleanup
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        free(result_context);
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_on_sync_path_first_then_async_with_fake_cancel)
{
    for (uint32_t i = 0; i < g_test_module_count_with_fake_cancel; i++)
    {
        // arrange
        LogInfo("Begin Testing module %s", g_test_modules_with_fake_cancel[i].module_name);
        ll_async_op_module_fake_cancel_add_result_settings_to_queue(ll_async_module_fake_cancel, true, COMMON_ASYNC_OP_MODULE_OK);
        ll_async_op_module_fake_cancel_add_result_settings_to_queue(ll_async_module_fake_cancel, false, COMMON_ASYNC_OP_MODULE_OK);
        THANDLE(ASYNC_OP) async_op = NULL;
        OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
        ASSERT_IS_NOT_NULL(result_context);
        result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
        (void)interlocked_exchange(&result_context->callback_called, 0);
        int result;

        // act
        if (g_test_modules_with_fake_cancel[i].exposes_async_op)
        {
            result = g_test_modules_with_fake_cancel[i].module_interface.execute_async(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }
        else
        {
            result = g_test_modules_with_fake_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }

        // assert
        ASSERT_ARE_EQUAL(int, 0, result);
        if (g_test_modules_with_fake_cancel[i].exposes_async_op)
        {
            ASSERT_IS_NOT_NULL(async_op);
        }
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
        ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

        // cleanup
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        free(result_context);
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_on_sync_path_first_then_async_with_real_cancel)
{
    for (uint32_t i = 0; i < g_test_module_count_with_real_cancel; i++)
    {
        // arrange
        LogInfo("Begin Testing module %s", g_test_modules_with_real_cancel[i].module_name);
        ll_async_op_module_real_cancel_add_result_settings_to_queue(ll_async_module_real_cancel, true, COMMON_ASYNC_OP_MODULE_OK);
        ll_async_op_module_real_cancel_add_result_settings_to_queue(ll_async_module_real_cancel, false, COMMON_ASYNC_OP_MODULE_OK);
        THANDLE(ASYNC_OP) async_op = NULL;
        OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
        ASSERT_IS_NOT_NULL(result_context);
        result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
        (void)interlocked_exchange(&result_context->callback_called, 0);
        int result;

        // act
        if (g_test_modules_with_real_cancel[i].exposes_async_op)
        {
            result = g_test_modules_with_real_cancel[i].module_interface.execute_async(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }
        else
        {
            result = g_test_modules_with_real_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }

        // assert
        ASSERT_ARE_EQUAL(int, 0, result);
        if (g_test_modules_with_real_cancel[i].exposes_async_op)
        {
            ASSERT_IS_NOT_NULL(async_op);
        }
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
        ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

        // cleanup
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        free(result_context);
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_on_async_path_first_then_sync_with_fake_cancel)
{
    for (uint32_t i = 0; i < g_test_module_count_with_fake_cancel; i++)
    {
        // arrange
        LogInfo("Begin Testing module %s", g_test_modules_with_fake_cancel[i].module_name);
        ll_async_op_module_fake_cancel_add_result_settings_to_queue(ll_async_module_fake_cancel, false, COMMON_ASYNC_OP_MODULE_OK);
        ll_async_op_module_fake_cancel_add_result_settings_to_queue(ll_async_module_fake_cancel, true, COMMON_ASYNC_OP_MODULE_OK);
        THANDLE(ASYNC_OP) async_op = NULL;
        OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
        ASSERT_IS_NOT_NULL(result_context);
        result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
        (void)interlocked_exchange(&result_context->callback_called, 0);
        int result;

        // act
        if (g_test_modules_with_fake_cancel[i].exposes_async_op)
        {
            result = g_test_modules_with_fake_cancel[i].module_interface.execute_async(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }
        else
        {
            result = g_test_modules_with_fake_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }

        // assert
        ASSERT_ARE_EQUAL(int, 0, result);
        if (g_test_modules_with_fake_cancel[i].exposes_async_op)
        {
            ASSERT_IS_NOT_NULL(async_op);
        }
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
        ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

        // cleanup
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        free(result_context);
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_on_async_path_first_then_sync_with_real_cancel)
{
    for (uint32_t i = 0; i < g_test_module_count_with_real_cancel; i++)
    {
        // arrange
        LogInfo("Begin Testing module %s", g_test_modules_with_real_cancel[i].module_name);
        ll_async_op_module_real_cancel_add_result_settings_to_queue(ll_async_module_real_cancel, false, COMMON_ASYNC_OP_MODULE_OK);
        ll_async_op_module_real_cancel_add_result_settings_to_queue(ll_async_module_real_cancel, true, COMMON_ASYNC_OP_MODULE_OK);
        THANDLE(ASYNC_OP) async_op = NULL;
        OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
        ASSERT_IS_NOT_NULL(result_context);
        result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
        (void)interlocked_exchange(&result_context->callback_called, 0);
        int result;

        // act
        if (g_test_modules_with_real_cancel[i].exposes_async_op)
        {
            result = g_test_modules_with_real_cancel[i].module_interface.execute_async(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }
        else
        {
            result = g_test_modules_with_real_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
        }

        // assert
        ASSERT_ARE_EQUAL(int, 0, result);
        if (g_test_modules_with_real_cancel[i].exposes_async_op)
        {
            ASSERT_IS_NOT_NULL(async_op);
        }
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
        ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

        // cleanup
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        free(result_context);
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_then_cancel_is_noop_with_fake_cancel)
{
    for (uint32_t i = 0; i < g_test_module_count_with_fake_cancel; i++)
    {
        // arrange
        LogInfo("Begin Testing module %s", g_test_modules_with_fake_cancel[i].module_name);
        THANDLE(ASYNC_OP) async_op = NULL;
        OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
        ASSERT_IS_NOT_NULL(result_context);
        result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
        (void)interlocked_exchange(&result_context->callback_called, 0);

        if (g_test_modules_with_fake_cancel[i].exposes_async_op)
        {
            ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_fake_cancel[i].module_interface.execute_async(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context));
            ASSERT_IS_NOT_NULL(async_op);
        }
        else
        {
            ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_fake_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context));
        }

        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
        ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

        // act
        if (g_test_modules_with_fake_cancel[i].exposes_async_op)
        {
            (void)async_op_cancel(async_op);
        }
        else
        {
            g_test_modules_with_fake_cancel[i].module_interface.close(g_test_modules_with_fake_cancel[i].module_interface.handle);
            ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_fake_cancel[i].module_interface.open(g_test_modules_with_fake_cancel[i].module_interface.handle));
        }

        // assert
        // Nothing crashes

        // cleanup
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        free(result_context);
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_then_cancel_is_noop_with_real_cancel)
{
    for (uint32_t i = 0; i < g_test_module_count_with_real_cancel; i++)
    {
        // arrange
        LogInfo("Begin Testing module %s", g_test_modules_with_real_cancel[i].module_name);
        THANDLE(ASYNC_OP) async_op = NULL;
        OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
        ASSERT_IS_NOT_NULL(result_context);
        result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
        (void)interlocked_exchange(&result_context->callback_called, 0);

        if (g_test_modules_with_real_cancel[i].exposes_async_op)
        {
            ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_real_cancel[i].module_interface.execute_async(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context));
            ASSERT_IS_NOT_NULL(async_op);
        }
        else
        {
            ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_real_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context));
        }

        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
        ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

        // act
        if (g_test_modules_with_real_cancel[i].exposes_async_op)
        {
            (void)async_op_cancel(async_op);
        }
        else
        {
            g_test_modules_with_real_cancel[i].module_interface.close(g_test_modules_with_real_cancel[i].module_interface.handle);
            ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_real_cancel[i].module_interface.open(g_test_modules_with_real_cancel[i].module_interface.handle));
        }

        // assert
        // Nothing crashes

        // cleanup
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        free(result_context);
    }
}

TEST_FUNCTION(all_modules_can_be_canceled_successfully_with_fake_cancel)
{
    for (uint32_t i = 0; i < g_test_module_count_with_fake_cancel; i++)
    {
        // arrange
        LogInfo("Begin Testing module %s", g_test_modules_with_fake_cancel[i].module_name);
        THANDLE(ASYNC_OP) async_op = NULL;
        OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
        ASSERT_IS_NOT_NULL(result_context);
        result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
        (void)interlocked_exchange(&result_context->callback_called, 0);

        if (g_test_modules_with_fake_cancel[i].exposes_async_op)
        {
            ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_fake_cancel[i].module_interface.execute_async(g_test_modules_with_fake_cancel[i].module_interface.handle, 2000 /* long enough to not complete, but don't make test wait too long */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context));
            ASSERT_IS_NOT_NULL(async_op);
        }
        else
        {
            ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_fake_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_fake_cancel[i].module_interface.handle, 2000 /* long enough to not complete, but don't make test wait too long */, test_ASYNC_OP_MODULE_CALLBACK, result_context));
        }

        // act
        if (g_test_modules_with_fake_cancel[i].exposes_async_op)
        {
            (void)async_op_cancel(async_op);
        }
        else
        {
            g_test_modules_with_fake_cancel[i].module_interface.close(g_test_modules_with_fake_cancel[i].module_interface.handle);
            ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_fake_cancel[i].module_interface.open(g_test_modules_with_fake_cancel[i].module_interface.handle));
        }

        // assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 0 /* Callback should have come immediately */));
        // Note that the cleanup will block for 2 seconds because the real operation must still complete
        ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_CANCELED, result_context->result);

        // cleanup
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        free(result_context);
    }
}

TEST_FUNCTION(all_modules_can_be_canceled_successfully_with_real_cancel)
{
    for (uint32_t i = 0; i < g_test_module_count_with_real_cancel; i++)
    {
        // arrange
        LogInfo("Begin Testing module %s", g_test_modules_with_real_cancel[i].module_name);
        THANDLE(ASYNC_OP) async_op = NULL;
        OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
        ASSERT_IS_NOT_NULL(result_context);
        result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
        (void)interlocked_exchange(&result_context->callback_called, 0);

        if (g_test_modules_with_real_cancel[i].exposes_async_op)
        {
            ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_real_cancel[i].module_interface.execute_async(g_test_modules_with_real_cancel[i].module_interface.handle, UINT32_MAX /* will never complete */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context));
            ASSERT_IS_NOT_NULL(async_op);
        }
        else
        {
            ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_real_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_real_cancel[i].module_interface.handle, UINT32_MAX /* will never complete */, test_ASYNC_OP_MODULE_CALLBACK, result_context));
        }

        // act
        if (g_test_modules_with_real_cancel[i].exposes_async_op)
        {
            (void)async_op_cancel(async_op);
        }
        else
        {
            g_test_modules_with_real_cancel[i].module_interface.close(g_test_modules_with_real_cancel[i].module_interface.handle);
            ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_real_cancel[i].module_interface.open(g_test_modules_with_real_cancel[i].module_interface.handle));
        }

        // assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
        ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_CANCELED, result_context->result);

        // cleanup
        THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
        free(result_context);
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_with_fake_cancel_and_3_retries)
{
    for (uint32_t i = 0; i < g_test_module_count_with_fake_cancel; i++)
    {
        if (g_test_modules_with_fake_cancel[i].has_retry_support)
        {
            // arrange
            LogInfo("Begin Testing module %s", g_test_modules_with_fake_cancel[i].module_name);
            ll_async_op_module_fake_cancel_set_report_retry_result_count(ll_async_module_fake_cancel, 3);
            THANDLE(ASYNC_OP) async_op = NULL;
            OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
            ASSERT_IS_NOT_NULL(result_context);
            result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
            (void)interlocked_exchange(&result_context->callback_called, 0);
            int result;

            // act
            if (g_test_modules_with_fake_cancel[i].exposes_async_op)
            {
                result = g_test_modules_with_fake_cancel[i].module_interface.execute_async(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }
            else
            {
                result = g_test_modules_with_fake_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }

            // assert
            ASSERT_ARE_EQUAL(int, 0, result);
            if (g_test_modules_with_fake_cancel[i].exposes_async_op)
            {
                ASSERT_IS_NOT_NULL(async_op);
            }
            ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
            ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

            // cleanup
            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
            free(result_context);
        }
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_with_real_cancel_and_3_retries)
{
    for (uint32_t i = 0; i < g_test_module_count_with_real_cancel; i++)
    {
        if (g_test_modules_with_real_cancel[i].has_retry_support)
        {
            // arrange
            LogInfo("Begin Testing module %s", g_test_modules_with_real_cancel[i].module_name);
            ll_async_op_module_real_cancel_set_report_retry_result_count(ll_async_module_real_cancel, 3);
            THANDLE(ASYNC_OP) async_op = NULL;
            OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
            ASSERT_IS_NOT_NULL(result_context);
            result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
            (void)interlocked_exchange(&result_context->callback_called, 0);
            int result;

            // act
            if (g_test_modules_with_real_cancel[i].exposes_async_op)
            {
                result = g_test_modules_with_real_cancel[i].module_interface.execute_async(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }
            else
            {
                result = g_test_modules_with_real_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }

            // assert
            ASSERT_ARE_EQUAL(int, 0, result);
            if (g_test_modules_with_real_cancel[i].exposes_async_op)
            {
                ASSERT_IS_NOT_NULL(async_op);
            }
            ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
            ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

            // cleanup
            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
            free(result_context);
        }
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_then_cancel_is_noop_with_fake_cancel_and_3_retries)
{
    for (uint32_t i = 0; i < g_test_module_count_with_fake_cancel; i++)
    {
        if (g_test_modules_with_fake_cancel[i].has_retry_support)
        {
            // arrange
            LogInfo("Begin Testing module %s", g_test_modules_with_fake_cancel[i].module_name);
            ll_async_op_module_fake_cancel_set_report_retry_result_count(ll_async_module_fake_cancel, 3);
            THANDLE(ASYNC_OP) async_op = NULL;
            OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
            ASSERT_IS_NOT_NULL(result_context);
            result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
            (void)interlocked_exchange(&result_context->callback_called, 0);

            if (g_test_modules_with_fake_cancel[i].exposes_async_op)
            {
                ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_fake_cancel[i].module_interface.execute_async(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context));
                ASSERT_IS_NOT_NULL(async_op);
            }
            else
            {
                ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_fake_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context));
            }

            ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
            ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

            // act
            if (g_test_modules_with_fake_cancel[i].exposes_async_op)
            {
                (void)async_op_cancel(async_op);
            }
            else
            {
                g_test_modules_with_fake_cancel[i].module_interface.close(g_test_modules_with_fake_cancel[i].module_interface.handle);
                ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_fake_cancel[i].module_interface.open(g_test_modules_with_fake_cancel[i].module_interface.handle));
            }

            // assert
            // Nothing crashes

            // cleanup
            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
            free(result_context);
        }
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_then_cancel_is_noop_with_real_cancel_and_3_retries)
{
    for (uint32_t i = 0; i < g_test_module_count_with_real_cancel; i++)
    {
        if (g_test_modules_with_real_cancel[i].has_retry_support)
        {
            // arrange
            LogInfo("Begin Testing module %s", g_test_modules_with_real_cancel[i].module_name);
            ll_async_op_module_real_cancel_set_report_retry_result_count(ll_async_module_real_cancel, 3);
            THANDLE(ASYNC_OP) async_op = NULL;
            OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
            ASSERT_IS_NOT_NULL(result_context);
            result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
            (void)interlocked_exchange(&result_context->callback_called, 0);

            if (g_test_modules_with_real_cancel[i].exposes_async_op)
            {
                ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_real_cancel[i].module_interface.execute_async(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context));
                ASSERT_IS_NOT_NULL(async_op);
            }
            else
            {
                ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_real_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context));
            }

            ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
            ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

            // act
            if (g_test_modules_with_real_cancel[i].exposes_async_op)
            {
                (void)async_op_cancel(async_op);
            }
            else
            {
                g_test_modules_with_real_cancel[i].module_interface.close(g_test_modules_with_real_cancel[i].module_interface.handle);
                ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_real_cancel[i].module_interface.open(g_test_modules_with_real_cancel[i].module_interface.handle));
            }

            // assert
            // Nothing crashes

            // cleanup
            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
            free(result_context);
        }
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_on_sync_path_with_fake_cancel_and_3_retries)
{
    for (uint32_t i = 0; i < g_test_module_count_with_fake_cancel; i++)
    {
        if (g_test_modules_with_fake_cancel[i].has_retry_support)
        {
            // arrange
            LogInfo("Begin Testing module %s", g_test_modules_with_fake_cancel[i].module_name);
            ll_async_op_module_fake_cancel_set_report_retry_result_count(ll_async_module_fake_cancel, 3);
            ll_async_op_module_fake_cancel_next_call_completes_synchronously(ll_async_module_fake_cancel, true);
            THANDLE(ASYNC_OP) async_op = NULL;
            OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
            ASSERT_IS_NOT_NULL(result_context);
            result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
            (void)interlocked_exchange(&result_context->callback_called, 0);
            int result;

            // act
            if (g_test_modules_with_fake_cancel[i].exposes_async_op)
            {
                result = g_test_modules_with_fake_cancel[i].module_interface.execute_async(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }
            else
            {
                result = g_test_modules_with_fake_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }

            // assert
            ASSERT_ARE_EQUAL(int, 0, result);
            if (g_test_modules_with_fake_cancel[i].exposes_async_op)
            {
                ASSERT_IS_NOT_NULL(async_op);
            }
            ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
            ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

            // cleanup
            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
            free(result_context);
        }
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_on_sync_path_with_real_cancel_and_3_retries)
{
    for (uint32_t i = 0; i < g_test_module_count_with_real_cancel; i++)
    {
        if (g_test_modules_with_real_cancel[i].has_retry_support)
        {
            // arrange
            LogInfo("Begin Testing module %s", g_test_modules_with_real_cancel[i].module_name);
            ll_async_op_module_real_cancel_set_report_retry_result_count(ll_async_module_real_cancel, 3);
            ll_async_op_module_real_cancel_next_call_completes_synchronously(ll_async_module_real_cancel, true);
            THANDLE(ASYNC_OP) async_op = NULL;
            OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
            ASSERT_IS_NOT_NULL(result_context);
            result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
            (void)interlocked_exchange(&result_context->callback_called, 0);
            int result;

            // act
            if (g_test_modules_with_real_cancel[i].exposes_async_op)
            {
                result = g_test_modules_with_real_cancel[i].module_interface.execute_async(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }
            else
            {
                result = g_test_modules_with_real_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }

            // assert
            ASSERT_ARE_EQUAL(int, 0, result);
            if (g_test_modules_with_real_cancel[i].exposes_async_op)
            {
                ASSERT_IS_NOT_NULL(async_op);
            }
            ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
            ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

            // cleanup
            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
            free(result_context);
        }
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_on_sync_path_then_async_with_fake_cancel_and_3_retries)
{
    for (uint32_t i = 0; i < g_test_module_count_with_fake_cancel; i++)
    {
        if (g_test_modules_with_fake_cancel[i].has_retry_support)
        {
            // arrange
            LogInfo("Begin Testing module %s", g_test_modules_with_fake_cancel[i].module_name);
            ll_async_op_module_fake_cancel_add_result_settings_to_queue(ll_async_module_fake_cancel, true, COMMON_ASYNC_OP_MODULE_ERROR_CAN_RETRY);
            ll_async_op_module_fake_cancel_add_result_settings_to_queue(ll_async_module_fake_cancel, false, COMMON_ASYNC_OP_MODULE_ERROR_CAN_RETRY);
            ll_async_op_module_fake_cancel_add_result_settings_to_queue(ll_async_module_fake_cancel, false, COMMON_ASYNC_OP_MODULE_ERROR_CAN_RETRY);
            ll_async_op_module_fake_cancel_add_result_settings_to_queue(ll_async_module_fake_cancel, false, COMMON_ASYNC_OP_MODULE_OK);
            THANDLE(ASYNC_OP) async_op = NULL;
            OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
            ASSERT_IS_NOT_NULL(result_context);
            result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
            (void)interlocked_exchange(&result_context->callback_called, 0);
            int result;

            // act
            if (g_test_modules_with_fake_cancel[i].exposes_async_op)
            {
                result = g_test_modules_with_fake_cancel[i].module_interface.execute_async(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }
            else
            {
                result = g_test_modules_with_fake_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }

            // assert
            ASSERT_ARE_EQUAL(int, 0, result);
            if (g_test_modules_with_fake_cancel[i].exposes_async_op)
            {
                ASSERT_IS_NOT_NULL(async_op);
            }
            ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
            ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

            // cleanup
            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
            free(result_context);
        }
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_on_sync_path_then_async_with_real_cancel_and_3_retries)
{
    for (uint32_t i = 0; i < g_test_module_count_with_real_cancel; i++)
    {
        if (g_test_modules_with_real_cancel[i].has_retry_support)
        {
            // arrange
            LogInfo("Begin Testing module %s", g_test_modules_with_real_cancel[i].module_name);
            ll_async_op_module_real_cancel_add_result_settings_to_queue(ll_async_module_real_cancel, true, COMMON_ASYNC_OP_MODULE_ERROR_CAN_RETRY);
            ll_async_op_module_real_cancel_add_result_settings_to_queue(ll_async_module_real_cancel, false, COMMON_ASYNC_OP_MODULE_ERROR_CAN_RETRY);
            ll_async_op_module_real_cancel_add_result_settings_to_queue(ll_async_module_real_cancel, false, COMMON_ASYNC_OP_MODULE_ERROR_CAN_RETRY);
            ll_async_op_module_real_cancel_add_result_settings_to_queue(ll_async_module_real_cancel, false, COMMON_ASYNC_OP_MODULE_OK);
            THANDLE(ASYNC_OP) async_op = NULL;
            OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
            ASSERT_IS_NOT_NULL(result_context);
            result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
            (void)interlocked_exchange(&result_context->callback_called, 0);
            int result;

            // act
            if (g_test_modules_with_real_cancel[i].exposes_async_op)
            {
                result = g_test_modules_with_real_cancel[i].module_interface.execute_async(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }
            else
            {
                result = g_test_modules_with_real_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }

            // assert
            ASSERT_ARE_EQUAL(int, 0, result);
            if (g_test_modules_with_real_cancel[i].exposes_async_op)
            {
                ASSERT_IS_NOT_NULL(async_op);
            }
            ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
            ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

            // cleanup
            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
            free(result_context);
        }
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_on_async_path_then_sync_with_fake_cancel_and_3_retries)
{
    for (uint32_t i = 0; i < g_test_module_count_with_fake_cancel; i++)
    {
        if (g_test_modules_with_fake_cancel[i].has_retry_support)
        {
            // arrange
            LogInfo("Begin Testing module %s", g_test_modules_with_fake_cancel[i].module_name);
            ll_async_op_module_fake_cancel_add_result_settings_to_queue(ll_async_module_fake_cancel, false, COMMON_ASYNC_OP_MODULE_ERROR_CAN_RETRY);
            ll_async_op_module_fake_cancel_add_result_settings_to_queue(ll_async_module_fake_cancel, true, COMMON_ASYNC_OP_MODULE_ERROR_CAN_RETRY);
            ll_async_op_module_fake_cancel_add_result_settings_to_queue(ll_async_module_fake_cancel, true, COMMON_ASYNC_OP_MODULE_ERROR_CAN_RETRY);
            ll_async_op_module_fake_cancel_add_result_settings_to_queue(ll_async_module_fake_cancel, true, COMMON_ASYNC_OP_MODULE_OK);
            THANDLE(ASYNC_OP) async_op = NULL;
            OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
            ASSERT_IS_NOT_NULL(result_context);
            result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
            (void)interlocked_exchange(&result_context->callback_called, 0);
            int result;

            // act
            if (g_test_modules_with_fake_cancel[i].exposes_async_op)
            {
                result = g_test_modules_with_fake_cancel[i].module_interface.execute_async(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }
            else
            {
                result = g_test_modules_with_fake_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }

            // assert
            ASSERT_ARE_EQUAL(int, 0, result);
            if (g_test_modules_with_fake_cancel[i].exposes_async_op)
            {
                ASSERT_IS_NOT_NULL(async_op);
            }
            ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
            ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

            // cleanup
            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
            free(result_context);
        }
    }
}

TEST_FUNCTION(all_modules_can_execute_async_successfully_on_async_path_then_sync_with_real_cancel_and_3_retries)
{
    for (uint32_t i = 0; i < g_test_module_count_with_real_cancel; i++)
    {
        if (g_test_modules_with_real_cancel[i].has_retry_support)
        {
            // arrange
            LogInfo("Begin Testing module %s", g_test_modules_with_real_cancel[i].module_name);
            ll_async_op_module_real_cancel_add_result_settings_to_queue(ll_async_module_real_cancel, false, COMMON_ASYNC_OP_MODULE_ERROR_CAN_RETRY);
            ll_async_op_module_real_cancel_add_result_settings_to_queue(ll_async_module_real_cancel, true, COMMON_ASYNC_OP_MODULE_ERROR_CAN_RETRY);
            ll_async_op_module_real_cancel_add_result_settings_to_queue(ll_async_module_real_cancel, true, COMMON_ASYNC_OP_MODULE_ERROR_CAN_RETRY);
            ll_async_op_module_real_cancel_add_result_settings_to_queue(ll_async_module_real_cancel, true, COMMON_ASYNC_OP_MODULE_OK);
            THANDLE(ASYNC_OP) async_op = NULL;
            OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
            ASSERT_IS_NOT_NULL(result_context);
            result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
            (void)interlocked_exchange(&result_context->callback_called, 0);
            int result;

            // act
            if (g_test_modules_with_real_cancel[i].exposes_async_op)
            {
                result = g_test_modules_with_real_cancel[i].module_interface.execute_async(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }
            else
            {
                result = g_test_modules_with_real_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* Short timeout */, test_ASYNC_OP_MODULE_CALLBACK, result_context);
            }

            // assert
            ASSERT_ARE_EQUAL(int, 0, result);
            if (g_test_modules_with_real_cancel[i].exposes_async_op)
            {
                ASSERT_IS_NOT_NULL(async_op);
            }
            ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
            ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_OK, result_context->result);

            // cleanup
            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
            free(result_context);
        }
    }
}

TEST_FUNCTION(all_modules_can_be_canceled_successfully_with_fake_cancel_after_many_retries)
{
    for (uint32_t i = 0; i < g_test_module_count_with_fake_cancel; i++)
    {
        if (g_test_modules_with_fake_cancel[i].has_retry_support)
        {
            // arrange
            LogInfo("Begin Testing module %s", g_test_modules_with_fake_cancel[i].module_name);
            ll_async_op_module_fake_cancel_set_report_retry_result_count(ll_async_module_fake_cancel, INT32_MAX); // more or less infinitely retry, but cancel will happen
            THANDLE(ASYNC_OP) async_op = NULL;
            OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
            ASSERT_IS_NOT_NULL(result_context);
            result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
            (void)interlocked_exchange(&result_context->callback_called, 0);

            if (g_test_modules_with_fake_cancel[i].exposes_async_op)
            {
                ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_fake_cancel[i].module_interface.execute_async(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* each call completes quickly */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context));
                ASSERT_IS_NOT_NULL(async_op);
            }
            else
            {
                ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_fake_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_fake_cancel[i].module_interface.handle, 1 /* each call completes quickly */, test_ASYNC_OP_MODULE_CALLBACK, result_context));
            }

            ThreadAPI_Sleep(10); // give it a chance to retry

            // act
            if (g_test_modules_with_fake_cancel[i].exposes_async_op)
            {
                (void)async_op_cancel(async_op);
            }
            else
            {
                g_test_modules_with_fake_cancel[i].module_interface.close(g_test_modules_with_fake_cancel[i].module_interface.handle);
                ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_fake_cancel[i].module_interface.open(g_test_modules_with_fake_cancel[i].module_interface.handle));
            }

            // assert
            ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
            // Note that the cleanup will block for 2 seconds because the real operation must still complete
            ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_CANCELED, result_context->result);

            // cleanup
            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
            free(result_context);
        }
    }
}

TEST_FUNCTION(all_modules_can_be_canceled_successfully_with_real_cancel_after_many_retries)
{
    for (uint32_t i = 0; i < g_test_module_count_with_real_cancel; i++)
    {
        if (g_test_modules_with_real_cancel[i].has_retry_support)
        {
            // arrange
            LogInfo("Begin Testing module %s", g_test_modules_with_real_cancel[i].module_name);
            ll_async_op_module_real_cancel_set_report_retry_result_count(ll_async_module_real_cancel, INT32_MAX); // more or less infinitely retry, but cancel will happen
            THANDLE(ASYNC_OP) async_op = NULL;
            OPERATION_RESULT_CONTEXT* result_context = malloc(sizeof(OPERATION_RESULT_CONTEXT));
            ASSERT_IS_NOT_NULL(result_context);
            result_context->result = COMMON_ASYNC_OP_MODULE_RESULT_INVALID;
            (void)interlocked_exchange(&result_context->callback_called, 0);

            if (g_test_modules_with_real_cancel[i].exposes_async_op)
            {
                ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_real_cancel[i].module_interface.execute_async(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* each call completes quickly */, &async_op, test_ASYNC_OP_MODULE_CALLBACK, result_context));
                ASSERT_IS_NOT_NULL(async_op);
            }
            else
            {
                ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_real_cancel[i].module_interface.execute_async_no_async_op_out(g_test_modules_with_real_cancel[i].module_interface.handle, 1 /* each call completes quickly */, test_ASYNC_OP_MODULE_CALLBACK, result_context));
            }

            ThreadAPI_Sleep(10); // give it a chance to retry

            // act
            if (g_test_modules_with_real_cancel[i].exposes_async_op)
            {
                (void)async_op_cancel(async_op);
            }
            else
            {
                g_test_modules_with_real_cancel[i].module_interface.close(g_test_modules_with_real_cancel[i].module_interface.handle);
                ASSERT_ARE_EQUAL(int, 0, g_test_modules_with_real_cancel[i].module_interface.open(g_test_modules_with_real_cancel[i].module_interface.handle));
            }

            // assert
            ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_WaitForValue(&result_context->callback_called, 1, 1000));
            ASSERT_ARE_EQUAL(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_CANCELED, result_context->result);

            // cleanup
            THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
            free(result_context);
        }
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
