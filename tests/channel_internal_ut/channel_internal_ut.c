// Copyright (c) Microsoft. All rights reserved.

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/sync.h"
#include "c_pal/execution_engine.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/srw_lock.h"
#include "c_pal/threadpool.h"

#include "c_util/doublylinkedlist.h"
#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"
#undef ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"

#include "real_gballoc_hl.h"
#include "real_srw_lock.h"
#include "real_threadpool.h"
#include "real_doublylinkedlist.h"
#include "real_async_op.h"
#include "real_rc_ptr.h"

#include "c_util/channel_internal.h"

static EXECUTION_ENGINE_HANDLE g_execution_engine = NULL;
static struct G_TAG /*g comes from "global*/
{
    THANDLE(THREADPOOL) g_threadpool;
} g = { NULL };

TEST_DEFINE_ENUM_TYPE(CHANNEL_RESULT, CHANNEL_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(CHANNEL_RESULT, CHANNEL_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);


MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static void* test_threadpool = (void*)0x1000;
static void* test_pull_context = (void*)0x1002;
static void* test_push_context = (void*)0x1004;


static PULL_CALLBACK test_pull_callback = (PULL_CALLBACK)0x1005;
static PUSH_CALLBACK test_push_callback = (PUSH_CALLBACK)0x1006;
static THANDLE(RC_PTR) test_data_rc_ptr = (RC_PTR*)0x1007;
static void* test_data = (void*)0x1009;

static struct TEST_OUT_OP_TAG
{
    THANDLE(ASYNC_OP) out_op_pull;
    THANDLE(ASYNC_OP) out_op_push;
} test_out_op = { NULL, NULL };

static void do_nothing(void* data)
{
    (void)data;
}

static void test_pull_callback_abandoned(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_PTR) data)
{
    ASSERT_IS_NULL(data);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_ABANDONED, result);
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, InterlockedHL_SetAndWake(context, 1));
}

static void test_push_callback_abandoned(void* context, CHANNEL_CALLBACK_RESULT result)
{
    ASSERT_ARE_EQUAL(void_ptr, test_push_context, context);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_ABANDONED, result);
}

static void setup_channel_internal_create_and_open_expectations()
{
    STRICT_EXPECTED_CALL(srw_lock_create(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, g.g_threadpool));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_ARG));
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
    REGISTER_DOUBLYLINKEDLIST_GLOBAL_MOCK_HOOKS();
    REGISTER_ASYNC_OP_GLOBAL_MOCK_HOOKS();
    REGISTER_RC_PTR_GLOBAL_MOCK_HOOKS();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);

    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(THREADPOOL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREADPOOL_WORK_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SRW_LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(CHANNEL_INTERNAL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(RC_PTR), void*);
    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*)
    REGISTER_UMOCK_ALIAS_TYPE(const PDLIST_ENTRY, void*)
    REGISTER_UMOCK_ALIAS_TYPE(RC_PTR_FREE_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PULL_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PUSH_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ASYNC_OP_CANCEL_IMPL, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ASYNC_OP_DISPOSE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(ASYNC_OP), void*);


    REGISTER_TYPE(CHANNEL_RESULT, CHANNEL_RESULT);
    REGISTER_TYPE(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT);


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

/* channel_internal_create_and_open */

/*Codes_SRS_CHANNEL_INTERNAL_43_078: [ channel_create shall create a CHANNEL_INTERNAL object by calling THANDLE_MALLOC with channel_internal_dispose as dispose.]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_098: [ channel_create shall call srw_lock_create. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_080: [ channel_create shall store given threadpool in the created CHANNEL_INTERNAL. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_084: [ channel_create shall call DList_InitializeListHead. ]*/
TEST_FUNCTION(channel_internal_create_and_open_succeeds)
{
    //arrange
    setup_channel_internal_create_and_open_expectations();

    //act
    THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create_and_open(g.g_threadpool);

    //assert
    ASSERT_IS_NOT_NULL(channel_internal);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/*Codes_SRS_CHANNEL_43_002: [ If there are any failures, channel_create shall fail and return NULL. ]*/
TEST_FUNCTION(channel_internal_create_and_open_fails_when_underlying_functions_fail)
{
    setup_channel_internal_create_and_open_expectations();
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create_and_open(g.g_threadpool);

            // assert
            ASSERT_IS_NULL(channel_internal, "On failed call %zu", i);
        }
    }
}

/* channel_internal_close */

/*Codes_SRS_CHANNEL_INTERNAL_43_094: [ channel_dispose shall call srw_lock_acquire_exclusive. ]*/
TEST_FUNCTION(channel_internal_close_succeeds)
{
    //arrange
    volatile_atomic int32_t callback_counter = 0;
    THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create_and_open(g.g_threadpool);
    THANDLE(ASYNC_OP) pull_op = NULL;
    CHANNEL_RESULT pull_result = channel_internal_pull(channel_internal, test_pull_callback_abandoned, (void*)&callback_counter, &pull_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.g_threadpool, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL (THANDLE_ASSIGN(RC_PTR)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(ASYNC_OP)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(ASYNC_OP)(IGNORED_ARG, NULL));

    //act
    channel_internal_close(channel_internal);
    InterlockedHL_WaitForValue(&callback_counter, 1, UINT32_MAX);
    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
