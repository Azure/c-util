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
#include "c_pal/threadpool.h"
#include "c_pal/thandle_log_context_handle.h"

#include "c_util/async_op.h"
#include "c_util/rc_string.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_util/channel_internal.h"
#undef ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"


#include "real_gballoc_hl.h"
#include "real_channel_internal.h"
#include "real_threadpool.h"

#include "c_util/channel.h"

static EXECUTION_ENGINE_HANDLE g_execution_engine = NULL;
static struct G_TAG /*g comes from "global*/
{
    THANDLE(THREADPOOL) g_threadpool;
    THANDLE(PTR(LOG_CONTEXT_HANDLE)) g_log_context;
    THANDLE(RC_STRING) g_pull_correlation_id;
    THANDLE(RC_STRING) g_push_correlation_id;
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

static void* test_pull_context = (void*)0x1002;
static void* test_push_context = (void*)0x1004;


static ON_DATA_AVAILABLE_CB test_on_data_available_cb = (ON_DATA_AVAILABLE_CB)0x1005;
static ON_DATA_CONSUMED_CB test_on_data_consumed_cb = (ON_DATA_CONSUMED_CB)0x1006;
static THANDLE(RC_PTR) test_data_rc_ptr = (RC_PTR*)0x1007;
static THANDLE(CHANNEL) test_channel = (CHANNEL*)0x1008;
static void* test_data = (void*)0x1009;

static struct TEST_OUT_OP_TAG
{
    THANDLE(ASYNC_OP) out_op_pull;
    THANDLE(ASYNC_OP) out_op_push;
} test_out_op = { NULL, NULL };

static void do_nothing(void* context, void* data)
{
    (void)context;
    (void)data;
}

static void test_on_data_available_cb_abandoned(
    void* context,
    CHANNEL_CALLBACK_RESULT result,
    THANDLE(RC_STRING) pull_correlation_id,
    THANDLE(RC_STRING) push_correlation_id,
    THANDLE(RC_PTR) data
)
{
    ASSERT_ARE_EQUAL(void_ptr, test_pull_context, context);
    ASSERT_IS_NOT_NULL(pull_correlation_id);
    ASSERT_IS_NULL(push_correlation_id);
    ASSERT_IS_NULL(data);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_ABANDONED, result);
}

static void test_on_data_consumed_cb_abandoned(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id)
{
    ASSERT_ARE_EQUAL(void_ptr, test_push_context, context);
    ASSERT_IS_NULL(pull_correlation_id);
    ASSERT_IS_NOT_NULL(push_correlation_id);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_ABANDONED, result);
}

static void setup_channel_create_expectations(void)
{
    STRICT_EXPECTED_CALL(channel_internal_create(g.g_log_context, g.g_threadpool));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(CHANNEL_INTERNAL)(IGNORED_ARG, IGNORED_ARG));
}

static void setup_channel_open_expectations(void)
{
    STRICT_EXPECTED_CALL(channel_internal_open(IGNORED_ARG));
}

static THANDLE(CHANNEL) test_create_and_open_channel(void)
{
    setup_channel_create_expectations();
    THANDLE(CHANNEL) channel = channel_create(g.g_log_context, g.g_threadpool);
    ASSERT_IS_NOT_NULL(channel);
    setup_channel_open_expectations();
    ASSERT_ARE_EQUAL(int, 0, channel_open(channel));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    return channel;
}

static void setup_channel_close_expectations(void)
{
    STRICT_EXPECTED_CALL(channel_internal_close(IGNORED_ARG));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_CHANNEL_INTERNAL_GLOBAL_MOCK_HOOKS();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(channel_internal_create, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(channel_internal_open, MU_FAILURE);

    REGISTER_CHANNEL_INTERNAL_GLOBAL_MOCK_HOOKS();


    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(THREADPOOL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(CHANNEL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(CHANNEL_INTERNAL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(RC_PTR), void*);
    REGISTER_UMOCK_ALIAS_TYPE(RC_PTR_FREE_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DATA_AVAILABLE_CB, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DATA_CONSUMED_CB, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(PTR(LOG_CONTEXT_HANDLE)), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(RC_STRING), void*);

    REGISTER_TYPE(CHANNEL_RESULT, CHANNEL_RESULT);
    REGISTER_TYPE(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT);

    g_execution_engine = execution_engine_create(NULL);
    ASSERT_IS_NOT_NULL(g_execution_engine);
    THANDLE(THREADPOOL) temp = threadpool_create(g_execution_engine);
    ASSERT_IS_NOT_NULL(temp);
    THANDLE_INITIALIZE_MOVE(THREADPOOL)(&g.g_threadpool, &temp);
    ASSERT_IS_NOT_NULL(g.g_threadpool);

    {
        LOG_CONTEXT_HANDLE raw_log_context;
        LOG_CONTEXT_CREATE(
            raw_log_context,
            NULL,
            LOG_CONTEXT_NAME(test_context),
            LOG_CONTEXT_STRING_PROPERTY(test_property, "test_property_value")
        );
        ASSERT_IS_NOT_NULL(raw_log_context);
        THANDLE(PTR(LOG_CONTEXT_HANDLE)) log_context = THANDLE_PTR_CREATE_WITH_MOVE_LOG_CONTEXT_HANDLE(&raw_log_context, log_context_destroy);
        THANDLE_INITIALIZE_MOVE(PTR(LOG_CONTEXT_HANDLE))(&g.g_log_context, &log_context);
    }

    {
        THANDLE(RC_STRING) pull_correlation_id = rc_string_create("pull_correlation_id");
        ASSERT_IS_NOT_NULL(pull_correlation_id);
        THANDLE_INITIALIZE_MOVE(RC_STRING)(&g.g_pull_correlation_id, &pull_correlation_id);
    }

    {
        THANDLE(RC_STRING) push_correlation_id = rc_string_create("push_correlation_id");
        ASSERT_IS_NOT_NULL(push_correlation_id);
        THANDLE_INITIALIZE_MOVE(RC_STRING)(&g.g_push_correlation_id, &push_correlation_id);
    }

}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    THANDLE_ASSIGN(RC_STRING)(&g.g_push_correlation_id, NULL);
    THANDLE_ASSIGN(RC_STRING)(&g.g_pull_correlation_id, NULL);
    THANDLE_ASSIGN(PTR(LOG_CONTEXT_HANDLE))(&g.g_log_context, NULL);
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

/* Tests_SRS_CHANNEL_43_077: [ If threadpool is NULL, channel_create shall fail and return NULL. ] */
TEST_FUNCTION(channel_create_fails_with_null_threadpool)
{
    //arrange

    //act
    THANDLE(CHANNEL) channel = channel_create(g.g_log_context, NULL);

    //assert
    ASSERT_IS_NULL(channel);
}

/*Tests_SRS_CHANNEL_43_001: [ channel_create shall create a CHANNEL object by calling THANDLE_MALLOC with channel_dispose as dispose. ]*/
/*Tests_SRS_CHANNEL_43_078: [ channel_create shall create a CHANNEL_INTERNAL object by calling THANDLE_MALLOC with channel_internal_dispose as dispose.]*/
/*Tests_SRS_CHANNEL_43_078: [ channel_create shall call create a THANDLE(CHANNEL_INTERNAL) by calling channel_internal_create.]*/
/*Tests_SRS_CHANNEL_43_079: [ channel_create shall store the created THANDLE(CHANNEL_INTERNAL) in the THANDLE(CHANNEL). ]*/
/*Tests_SRS_CHANNEL_43_086: [ channel_create shall succeed and return the created THANDLE(CHANNEL). ]*/
TEST_FUNCTION(channel_create_succeeds)
{
    //arrange
    setup_channel_create_expectations();

    //act
    THANDLE(CHANNEL) channel = channel_create(g.g_log_context, g.g_threadpool);

    //assert
    ASSERT_IS_NOT_NULL(channel);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

/*Tests_SRS_CHANNEL_43_002: [ If there are any failures, channel_create shall fail and return NULL. ]*/
TEST_FUNCTION(channel_create_fails_when_underlying_functions_fail)
{
    setup_channel_create_expectations();
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE(CHANNEL) channel = channel_create(g.g_log_context, g.g_threadpool);

            // assert
            ASSERT_IS_NULL(channel, "On failed call %zu", i);
        }
    }
}

/* channel_dispose */

/*Tests_SRS_CHANNEL_43_092: [ channel_dispose shall release the reference to THANDLE(CHANNEL_INTERNAL). ]*/
TEST_FUNCTION(channel_dispose_succeeds)
{
    //arrange
    THANDLE(CHANNEL) channel = channel_create(g.g_log_context, g.g_threadpool);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(CHANNEL_INTERNAL)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);

    //assert
    ASSERT_IS_NULL(channel);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

/* channel_open */

/*Tests_SRS_CHANNEL_43_095: [If channel is NULL, channel_open shall fail and return a non - zero value.]*/
TEST_FUNCTION(channel_open_fails_with_null_channel)
{
    //arrange

    //act
    int result = channel_open(NULL);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CHANNEL_43_096: [channel_open shall call channel_internal_open.]*/
TEST_FUNCTION(channel_open_calls_underlying_functions)
{
    //arrange
    setup_channel_create_expectations();
    THANDLE(CHANNEL) channel = channel_create(g.g_log_context, g.g_threadpool);
    setup_channel_open_expectations();;

    //act
    int result = channel_open(channel);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    channel_close(channel);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

/*Tests_SRS_CHANNEL_43_099: [ If there are any failures, channel_open shall fail and return a non-zero value. ]*/
TEST_FUNCTION(channel_open_fails_when_underlying_functions_fail)
{
    //arrange
    setup_channel_create_expectations();
    THANDLE(CHANNEL) channel = channel_create(g.g_log_context, g.g_threadpool);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    setup_channel_open_expectations();
    umock_c_negative_tests_snapshot();
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);
            // act
            int result = channel_open(channel);
            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
        }
    }
    //cleanup
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

/*Tests_SRS_CHANNEL_43_097: [If channel is NULL, channel_close shall return immediately.] */
TEST_FUNCTION(channel_close_returns_immediately_with_null_channel)
{
    //arrange

    //act
    channel_close(NULL);
    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CHANNEL_43_098: [ channel_close shall call channel_internal_close. ]*/
TEST_FUNCTION(channel_close_calls_underlying_functions)
{
    //arrange
    THANDLE(CHANNEL) channel = test_create_and_open_channel();
    umock_c_reset_all_calls();
    setup_channel_close_expectations();

    //act
    channel_close(channel);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

/* channel_pull */

/*Tests_SRS_CHANNEL_43_007: [ If channel is NULL, channel_pull shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
TEST_FUNCTION(channel_pull_fails_with_null_channel)
{
    //arrange

    //act
    CHANNEL_RESULT result = channel_pull(NULL, g.g_pull_correlation_id, test_on_data_available_cb, test_pull_context, &test_out_op.out_op_pull);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CHANNEL_43_008: [ If on_data_available_cb is NULL, channel_pull shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
TEST_FUNCTION(channel_pull_fails_with_null_on_data_available_cb)
{
    //arrange

    //act
    CHANNEL_RESULT result = channel_pull(test_channel, g.g_pull_correlation_id, NULL, test_pull_context, &test_out_op.out_op_pull);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CHANNEL_43_009: [ If out_op_pull is NULL, channel_pull shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
TEST_FUNCTION(channel_pull_fails_with_null_out_op_pull)
{
    //arrange

    //act
    CHANNEL_RESULT result = channel_pull(test_channel, g.g_pull_correlation_id, test_on_data_available_cb, test_pull_context, NULL);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CHANNEL_43_011: [ channel_pull shall call channel_internal_pull and return as it returns. ]*/
TEST_FUNCTION(channel_pull_calls_channel_internal_pull)
{
    //arrange
    THANDLE(CHANNEL) channel = test_create_and_open_channel();
    THANDLE(ASYNC_OP) pull_op = NULL;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(channel_internal_pull(IGNORED_ARG, g.g_pull_correlation_id, test_on_data_available_cb_abandoned, test_pull_context, &pull_op));

    //act
    CHANNEL_RESULT result = channel_pull(channel, g.g_pull_correlation_id, test_on_data_available_cb_abandoned, test_pull_context, &pull_op);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(pull_op);

    //cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    channel_close(channel);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

/* channel_push */

/*Tests_SRS_CHANNEL_43_024: [ If channel is NULL, channel_push shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
TEST_FUNCTION(channel_push_fails_with_null_channel)
{
    //arrange

    //act
    CHANNEL_RESULT result = channel_push(NULL, g.g_push_correlation_id, test_data_rc_ptr, test_on_data_consumed_cb, test_push_context, &test_out_op.out_op_push);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CHANNEL_43_025: [ If on_data_consumed_cb is NULL, channel_push shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
TEST_FUNCTION(channel_push_fails_with_null_on_data_consumed_cb)
{
    //arrange

    //act
    CHANNEL_RESULT result = channel_push(test_channel, g.g_push_correlation_id, test_data_rc_ptr, NULL, test_push_context, &test_out_op.out_op_push);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CHANNEL_43_026: [ If out_op_push is NULL, channel_push shall fail and return CHANNEL_RESULT_INVALID_ARGS. ]*/
TEST_FUNCTION(channel_push_fails_with_null_out_op_push)
{
    //arrange

    //act
    CHANNEL_RESULT result = channel_push(test_channel, g.g_push_correlation_id, test_data_rc_ptr, test_on_data_consumed_cb, test_push_context, NULL);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CHANNEL_43_041: [ channel_push shall call channel_internal_push and return as it returns. ]*/
TEST_FUNCTION(channel_push_calls_channel_internal_push)
{
    //arrange
    THANDLE(CHANNEL) channel = test_create_and_open_channel();
    THANDLE(ASYNC_OP) push_op = NULL;
    THANDLE(RC_PTR) data_rc_ptr = rc_ptr_create_with_move_pointer(test_data, do_nothing, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(channel_internal_push(IGNORED_ARG, g.g_push_correlation_id, data_rc_ptr, test_on_data_consumed_cb_abandoned, test_push_context, &push_op));

    //act
    CHANNEL_RESULT result = channel_push(channel, g.g_push_correlation_id, data_rc_ptr, test_on_data_consumed_cb_abandoned, test_push_context, &push_op);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(push_op);

    //cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&push_op, NULL);
    THANDLE_ASSIGN(RC_PTR)(&data_rc_ptr, NULL);
    channel_close(channel);
    THANDLE_ASSIGN(CHANNEL)(&channel, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
