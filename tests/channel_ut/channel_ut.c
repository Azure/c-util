// Copyright (c) Microsoft. All rights reserved.

#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "c_pal/interlocked.h"
#include "c_pal/execution_engine.h"
#include "c_pal/thandle.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/srw_lock.h"
#include "c_pal/threadpool.h"

#include "c_util/async_op.h"
#include "c_util/doublylinkedlist.h"
#include "c_util/rc_ptr.h"
#include "c_util/channel_internal.h"

#undef ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"

#include "real_gballoc_hl.h"
#include "real_srw_lock.h"
#include "real_threadpool.h"
#include "real_async_op.h"
#include "real_doublylinkedlist.h"
#include "real_rc_ptr.h"
#include "real_channel_internal.h"

#include "c_util/channel.h"

static EXECUTION_ENGINE_HANDLE g_execution_engine = NULL;
static struct G_TAG /*g comes from "global*/
{
    THANDLE(THREADPOOL) g_threadpool;
} g = { NULL };

TEST_DEFINE_ENUM_TYPE(CHANNEL_RESULT, CHANNEL_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(CHANNEL_RESULT, CHANNEL_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_VALUES);

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static void* test_threadpool = (void*)0x1000;
static PULL_CALLBACK test_pull_callback = (PULL_CALLBACK)0x1001;
static void* test_pull_context = (void*)0x1002;
static PUSH_CALLBACK test_push_callback = (PUSH_CALLBACK)0x1003;
static void* test_push_context = (void*)0x1004;
static THANDLE(ASYNC_OP) test_out_op_pull = (ASYNC_OP*)0x1005;
static THANDLE(ASYNC_OP) test_out_op_push = (ASYNC_OP*)0x1006;
static THANDLE(RC_PTR) test_data = (RC_PTR*)0x1007;
static THANDLE(CHANNEL) test_channel = (CHANNEL*)0x1008;

static void setup_channel_create_expectations(void)
{
    CHANNEL* channel;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .CaptureReturn(&channel);
    STRICT_EXPECTED_CALL(channel_internal_create_and_open(g.g_threadpool));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(CHANNEL_INTERNAL)(IGNORED_ARG, IGNORED_ARG));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_SRW_LOCK_GLOBAL_MOCK_HOOK();
    REGISTER_THREADPOOL_GLOBAL_MOCK_HOOK();

    REGISTER_ASYNC_OP_GLOBAL_MOCK_HOOKS();
    REGISTER_DOUBLYLINKEDLIST_GLOBAL_MOCK_HOOKS();
    REGISTER_RC_PTR_GLOBAL_MOCK_HOOKS();
    REGISTER_CHANNEL_INTERNAL_GLOBAL_MOCK_HOOKS();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(srw_lock_create, NULL);

    REGISTER_CHANNEL_INTERNAL_GLOBAL_MOCK_HOOKS();

    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(THREADPOOL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(SRW_LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(CHANNEL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(CHANNEL_INTERNAL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(RC_PTR), void*);
    REGISTER_TYPE(CHANNEL_RESULT, CHANNEL_RESULT);
    REGISTER_TYPE(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT);

    g_execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(g_execution_engine);
    THANDLE(THREADPOOL) temp = real_threadpool_create(g_execution_engine);
    ASSERT_IS_NOT_NULL(temp);
    THANDLE_INITIALIZE_MOVE(THREADPOOL)(&g.g_threadpool, &temp);
    ASSERT_IS_NOT_NULL(g.g_threadpool);
    ASSERT_ARE_EQUAL(int, 0, real_threadpool_open(g.g_threadpool));

}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    THANDLE_ASSIGN(THREADPOOL)(&g.g_threadpool, NULL);
    execution_engine_dec_ref(g_execution_engine);
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_negative_tests_init();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

/* channel_create */


/* Codes_SRS_CHANNEL_43_077: [ If threadpool is NULL, channel_create shall fail and return NULL. ] */
TEST_FUNCTION(channel_create_fails_with_null_threadpool)
{
    //arrange

    //act
    THANDLE(CHANNEL) channel = channel_create(NULL);

    //assert
    ASSERT_IS_NULL(channel);
}

/*Codes_SRS_CHANNEL_43_001: [ channel_create shall create a CHANNEL object by calling THANDLE_MALLOC with channel_dispose as dispose. ]*/
/*Codes_SRS_CHANNEL_43_078: [ channel_create shall create a CHANNEL_INTERNAL object by calling THANDLE_MALLOC with channel_internal_dispose as dispose.]*/
/*Codes_SRS_CHANNEL_43_079: [ channel_create shall store the created THANDLE(CHANNEL_INTERNAL) in the THANDLE(CHANNEL). ]*/
/*Codes_SRS_CHANNEL_43_080: [ channel_create shall store given threadpool in the created CHANNEL_INTERNAL. ]*/
/*Codes_SRS_CHANNEL_43_098: [ channel_create shall call srw_lock_create. ]*/
/*Codes_SRS_CHANNEL_43_084: [ channel_create shall call DList_InitializeListHead. ]*/
/*Codes_SRS_CHANNEL_43_086: [ channel_create shall succeed and return the created THANDLE(CHANNEL). ]*/
TEST_FUNCTION(channel_create_succeeds)
{
    //arrange
    setup_channel_create_expectations();

    //act
    THANDLE(CHANNEL) channel = channel_create(g.g_threadpool);

    //assert
    ASSERT_IS_NOT_NULL(channel);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

TEST_FUNCTION(channel_pull_fails_with_null_channel)
{
    //arrange

    //act
    CHANNEL_RESULT result = channel_pull(NULL, test_pull_callback, test_pull_context, &test_out_op_pull);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
