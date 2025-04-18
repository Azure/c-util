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

#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/srw_lock.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle_log_context_handle.h"
#include "c_pal/sm.h"
#include "c_pal/log_critical_and_terminate.h"

#include "c_util/doublylinkedlist.h"
#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"
#include "c_util/rc_string.h"
#undef ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"

#include "real_gballoc_hl.h"
#include "real_interlocked.h"
#include "real_srw_lock.h"
#include "real_thandle_helper.h"
#include "real_thandle_log_context_handle.h"
#include "real_sm.h"

#include "real_doublylinkedlist.h"
#include "real_async_op.h"
#include "real_rc_ptr.h"
#include "real_rc_string.h"


#include "c_pal/thandle.h"

#include "c_util/channel_internal.h"

#define DISABLE_TEST_FUNCTION(x) static void x(void)


TEST_DEFINE_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS(SM_RESULT, SM_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(CHANNEL_RESULT, CHANNEL_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(CHANNEL_RESULT, CHANNEL_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_VALUES);


MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

typedef struct THREADPOOL_TAG
{
    uint8_t dummy;
} THREADPOOL;

#include "real_interlocked_renames.h" // IWYU pragma: keep
REAL_THANDLE_DECLARE(THREADPOOL);
REAL_THANDLE_DEFINE(THREADPOOL);
#include "real_interlocked_undo_rename.h" // IWYU pragma: keep

static void dispose_REAL_THREADPOOL_do_nothing(REAL_THREADPOOL* nothing)
{
    (void)nothing;
}

static struct G_TAG /*g comes from "global*/
{
    THANDLE(PTR(LOG_CONTEXT_HANDLE)) g_log_context;
    THANDLE(RC_STRING) g_pull_correlation_id;
    THANDLE(RC_STRING) g_push_correlation_id;
    THANDLE(THREADPOOL) g_threadpool;
} g = { NULL };

static void* test_data = (void*)0x1001;
static void* test_data_2 = (void*)0x1002;

static void test_data_dispose(void* context, void* data)
{
    (void)context;
    (void)data;
}

static void test_on_data_available_cb_abandoned(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id, THANDLE(RC_PTR) data)
{
    ASSERT_ARE_EQUAL(int, 0, (*(int32_t*)context));
    (*(int32_t*)context)++;
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_ABANDONED, result);
    ASSERT_ARE_EQUAL(void_ptr, g.g_pull_correlation_id, pull_correlation_id);
    ASSERT_IS_NULL(push_correlation_id);
    ASSERT_IS_NULL(data);
}

static void test_on_data_consumed_cb_abandoned(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id)
{
    ASSERT_ARE_EQUAL(int, 0, (*(int32_t*)context));
    (*(int32_t*)context)++;
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_ABANDONED, result);
    ASSERT_IS_NULL(pull_correlation_id);
    ASSERT_ARE_EQUAL(void_ptr, g.g_push_correlation_id, push_correlation_id);
}

static void test_on_data_available_cb_ok(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id, THANDLE(RC_PTR) data)
{
    ASSERT_ARE_EQUAL(int, 0, (*(int32_t*)context));
    (*(int32_t*)context)++;
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, g.g_pull_correlation_id, pull_correlation_id);
    ASSERT_ARE_EQUAL(void_ptr, g.g_push_correlation_id, push_correlation_id);
    ASSERT_ARE_EQUAL(void_ptr, data->ptr, test_data);
}

static void test_on_data_consumed_cb_ok(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id)
{
    ASSERT_ARE_EQUAL(int, 0, (*(int32_t*)context));
    (*(int32_t*)context)++;
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, g.g_pull_correlation_id, pull_correlation_id);
    ASSERT_ARE_EQUAL(void_ptr, g.g_push_correlation_id, push_correlation_id);
}

static void test_on_data_available_cb_cancelled(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id, THANDLE(RC_PTR) data)
{
    ASSERT_ARE_EQUAL(int, 0, (*(int32_t*)context));
    (*(int32_t*)context)++;
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_CANCELLED, result);
    ASSERT_ARE_EQUAL(void_ptr, g.g_pull_correlation_id, pull_correlation_id);
    (void)push_correlation_id; // Cannot make any assertions about push_correlation_id. May or may not be NULL
    ASSERT_IS_NULL(data);
}

static void test_on_data_available_cb_matched_cancelled(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id, THANDLE(RC_PTR) data)
{
    ASSERT_ARE_EQUAL(int, 0, (*(int32_t*)context));
    (*(int32_t*)context)++;
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_CANCELLED, result);
    ASSERT_ARE_EQUAL(void_ptr, g.g_pull_correlation_id, pull_correlation_id);
    ASSERT_ARE_EQUAL(void_ptr, g.g_push_correlation_id, push_correlation_id);
    ASSERT_IS_NOT_NULL(data);
}

static void test_on_data_consumed_cb_cancelled(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id)
{
    ASSERT_ARE_EQUAL(int, 0, (*(int32_t*)context));
    (*(int32_t*)context)++;
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_CANCELLED, result);
    (void)pull_correlation_id; // Cannot make any assertions about pull_correlation_id. May or may not be NULL
    ASSERT_ARE_EQUAL(void_ptr, g.g_push_correlation_id, push_correlation_id);
}

static void setup_channel_internal_create_expectations(void)
{
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_create(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(PTR(LOG_CONTEXT_HANDLE))(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_ARG));
}

static void setup_channel_internal_open_expectations(void)
{
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));
}

static THANDLE(CHANNEL_INTERNAL) test_create_and_open_channel_internal(void)
{
    setup_channel_internal_create_expectations();
    THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create(g.g_log_context, g.g_threadpool);
    ASSERT_IS_NOT_NULL(channel_internal);
    setup_channel_internal_open_expectations();
    ASSERT_ARE_EQUAL(int, 0, channel_internal_open(channel_internal));

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    return channel_internal;
}

static void setup_first_op_expectations(void)
{
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(async_op_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_PTR)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(ASYNC_OP)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(ASYNC_OP)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));
}

static void setup_second_op_expectations(bool is_pull, bool expected_fail, THREADPOOL_WORK_FUNCTION* captured_work_function, void** captured_work_context)
{
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, IGNORED_ARG));
    is_pull ? (void)0 : (void)STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_PTR)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(ASYNC_OP)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_schedule_work(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_work_function(captured_work_function)
        .CaptureArgumentValue_work_function_context(captured_work_context)
        .SetReturn(expected_fail ? 1 : 0);
    if (expected_fail)
    {
        STRICT_EXPECTED_CALL(THANDLE_ASSIGN(ASYNC_OP)(IGNORED_ARG, NULL));
        STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));
        STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    }
    else
    {
        STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(ASYNC_OP)(IGNORED_ARG, IGNORED_ARG));
        STRICT_EXPECTED_CALL(THANDLE_ASSIGN(ASYNC_OP)(IGNORED_ARG, NULL));
        STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));
    }
}

static void setup_op_cleanup_expectations(bool completed)
{
    if(completed)
    {
        STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_STRING)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_STRING)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_PTR)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(ASYNC_OP)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(ASYNC_OP)(IGNORED_ARG, NULL));
}

static void setup_channel_internal_close_expectations(size_t num_ops)
{
    STRICT_EXPECTED_CALL(sm_close_begin_with_cb(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_ForEach(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    for (size_t i = 0; i < num_ops; ++i)
    {
        STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_ARG));
    for (size_t i = 0; i < num_ops; ++i)
    {
        setup_op_cleanup_expectations(false);
        STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));
}

static void setup_cancel_op_expectations(bool completed, THREADPOOL_WORK_FUNCTION* work_function, void** work_context)
{
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    if(!completed)
    {
        STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));
    if(!completed)
    {
        STRICT_EXPECTED_CALL(threadpool_schedule_work(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
            .CaptureArgumentValue_work_function(work_function)
            .CaptureArgumentValue_work_function_context(work_context);
        setup_op_cleanup_expectations(completed);
    }
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
}

static void cleanup_pending_ops(THANDLE(CHANNEL_INTERNAL) channel_internal, size_t num_ops)
{
    setup_channel_internal_close_expectations(num_ops);
    channel_internal_close(channel_internal);
}

static int hook_threadpool_schedule_work(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_FUNCTION work_func, void* work_func_context)
{
    (void)threadpool;
    work_func(work_func_context);
    return 0;
}

static int hook_threadpool_schedule_work_do_nothing(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_FUNCTION work_func, void* work_func_context)
{
    (void)threadpool;
    (void)work_func;
    (void)work_func_context;
    return 0;
}
BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_SRW_LOCK_GLOBAL_MOCK_HOOK();
    REGISTER_RC_STRING_GLOBAL_MOCK_HOOKS();
    REGISTER_SM_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_HOOK(threadpool_schedule_work, hook_threadpool_schedule_work);


    REGISTER_DOUBLYLINKEDLIST_GLOBAL_MOCK_HOOKS();
    REGISTER_ASYNC_OP_GLOBAL_MOCK_HOOKS();
    REGISTER_RC_PTR_GLOBAL_MOCK_HOOKS();
    REGISTER_THANDLE_LOG_CONTEXT_HANDLE_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(srw_lock_create, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(threadpool_schedule_work, 1);

    REGISTER_REAL_THANDLE_MOCK_HOOK(THREADPOOL);

    REGISTER_UMOCK_ALIAS_TYPE(SM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_SM_CLOSING_COMPLETE_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_SM_CLOSING_WHILE_OPENING_CALLBACK, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(THREADPOOL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREADPOOL_WORK_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SRW_LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(CHANNEL_INTERNAL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(RC_PTR), void*);
    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*)
    REGISTER_UMOCK_ALIAS_TYPE(const PDLIST_ENTRY, void*)
    REGISTER_UMOCK_ALIAS_TYPE(RC_PTR_FREE_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DATA_AVAILABLE_CB, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DATA_CONSUMED_CB, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ASYNC_OP_CANCEL_IMPL, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ASYNC_OP_DISPOSE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(ASYNC_OP), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(RC_STRING), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(PTR(LOG_CONTEXT_HANDLE)), void*);


    REGISTER_TYPE(CHANNEL_RESULT, CHANNEL_RESULT);
    REGISTER_TYPE(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT);

    {
        LOG_CONTEXT_HANDLE raw_log_context;
        LOG_CONTEXT_CREATE(
            raw_log_context,
            NULL,
            LOG_CONTEXT_NAME(test_context),
            LOG_CONTEXT_STRING_PROPERTY(test_property, "test_property_value")
        );
        ASSERT_IS_NOT_NULL(raw_log_context);
        THANDLE(PTR(real_LOG_CONTEXT_HANDLE)) log_context = THANDLE_PTR_CREATE_WITH_MOVE_real_LOG_CONTEXT_HANDLE(&raw_log_context, log_context_destroy);
        THANDLE_INITIALIZE_MOVE(PTR(real_LOG_CONTEXT_HANDLE))((THANDLE(PTR(real_LOG_CONTEXT_HANDLE))*) & g.g_log_context, &log_context);
    }
    {
        THANDLE(real_RC_STRING) pull_correlation_id = real_rc_string_create("pull_correlation_id");
        ASSERT_IS_NOT_NULL(pull_correlation_id);
        THANDLE_INITIALIZE_MOVE(real_RC_STRING)((THANDLE(RC_STRING)*) & g.g_pull_correlation_id, &pull_correlation_id);
    }
    {
        THANDLE(real_RC_STRING) push_correlation_id = real_rc_string_create("push_correlation_id");
        ASSERT_IS_NOT_NULL(push_correlation_id);
        THANDLE_INITIALIZE_MOVE(real_RC_STRING)((THANDLE(RC_STRING)*) & g.g_push_correlation_id, &push_correlation_id);
    }
    {
        THANDLE(THREADPOOL) temp = THANDLE_MALLOC(REAL_THREADPOOL)(dispose_REAL_THREADPOOL_do_nothing);
        ASSERT_IS_NOT_NULL(temp);
        THANDLE_MOVE(REAL_THREADPOOL)(&g.g_threadpool, &temp);
    }
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    THANDLE_ASSIGN(REAL_THREADPOOL)(&g.g_threadpool, NULL);
    THANDLE_ASSIGN(real_RC_STRING)((THANDLE(RC_STRING)*) & g.g_push_correlation_id, NULL);
    THANDLE_ASSIGN(real_RC_STRING)((THANDLE(RC_STRING)*) & g.g_pull_correlation_id, NULL);
    THANDLE_ASSIGN(PTR(real_LOG_CONTEXT_HANDLE))((THANDLE(PTR(real_LOG_CONTEXT_HANDLE))*) & g.g_log_context, NULL);
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

/* channel_internal_create */


/*Tests_SRS_CHANNEL_INTERNAL_43_151: [ channel_intenral_create shall call sm_create. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_098: [ channel_intenral_create shall call srw_lock_create. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_078: [ channel_intenral_create shall create a CHANNEL_INTERNAL object by calling THANDLE_MALLOC with channel_internal_dispose as dispose.]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_080: [ channel_intenral_create shall store given threadpool in the created CHANNEL_INTERNAL. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_149: [ channel_internal_create shall store the given log_context in the created CHANNEL_INTERNAL. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_084: [ channel_intenral_create shall call DList_InitializeListHead. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_086: [ channel_intenral_create shall succeed and return the created THANDLE(CHANNEL). ]*/
TEST_FUNCTION(channel_internal_create_succeeds)
{
    //arrange
    setup_channel_internal_create_expectations();

    //act
    THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create(g.g_log_context, g.g_threadpool);

    //assert
    ASSERT_IS_NOT_NULL(channel_internal);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/*Tests_SRS_CHANNEL_INTERNAL_43_002: [ If there are any failures, channel_internal_create shall fail and return NULL. ]*/
TEST_FUNCTION(channel_internal_create_fails_when_underlying_functions_fail)
{
    setup_channel_internal_create_expectations();
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create(g.g_log_context, g.g_threadpool);

            // assert
            ASSERT_IS_NULL(channel_internal, "On failed call %zu", i);
        }
    }
}

/* channel_internal_open */

/*Tests_SRS_CHANNEL_INTERNAL_43_159: [channel_internal_open shall call sm_open_begin.]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_172: [ channel_internal_open shall call srw_lock_acquire_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_166: [ channel_internal_open shall set is_open to true. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_173: [ channel_internal_open shall call srw_lock_release_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_160: [ channel_internal_open shall call sm_open_end. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_162: [ channel_internal_open shall succeed and return 0. ]*/
TEST_FUNCTION(channel_internal_open_succeeds)
{
    //arrange
    setup_channel_internal_create_expectations();
    THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create(g.g_log_context, g.g_threadpool);
    setup_channel_internal_open_expectations();

    //act
    int result = channel_internal_open(channel_internal);

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    channel_internal_close(channel_internal);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/*Tests_SRS_CHANNEL_INTERNAL_43_161: [If there are any failures, channel_internal_open shall fail and return a non-zero value.]*/
TEST_FUNCTION(channel_internal_open_fails_when_underlying_functions_fail)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create(g.g_log_context, g.g_threadpool);
    umock_c_reset_all_calls();

    setup_channel_internal_open_expectations();
    umock_c_negative_tests_snapshot();
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);
            // act
            int result = channel_internal_open(channel_internal);
            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
        }
    }
    //cleanup
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/* channel_internal_close */

/*Tests_SRS_CHANNEL_INTERNAL_43_094: [ channel_internal_close shall call sm_close_begin_with_cb with abandon_pending_operation as the callback. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_167: [ abandon_pending_operations shall call srw_lock_acquire_exclusive.]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_168: [ abandon_pending_operations shall set is_open to false.]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_174: [abandon_pending_operations shall make a local copy of the list of pending operations.]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_175: [abandon_pending_operations shall set the list of pending operations to an empty list by calling DList_InitializeListHead.]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_169: [ abandon_pending_operations shall call srw_lock_release_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_095: [ abandon_pending_operations shall iterate over the local copy and do the following: ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_096: [ set the result of the operation to CHANNEL_CALLBACK_RESULT_ABANDONED. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_097: [ call execute_callbacks with the operation as context.]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_100: [ channel_internal_close shall call sm_close_end. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_145: [ execute_callbacks shall call the stored callback(s) with the result of the operation. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_157: [ execute_callbacks shall call sm_exec_end for each callback that is called. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_147: [ execute_callbacks shall perform cleanup of the operation. ]*/
TEST_FUNCTION(channel_internal_close_calls_underlying_functions)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = test_create_and_open_channel_internal();
    THANDLE(ASYNC_OP) pull_op = NULL;
    int32_t pull_context = 0;
    CHANNEL_RESULT pull_result = channel_internal_pull(channel_internal, g.g_pull_correlation_id, test_on_data_available_cb_abandoned, &pull_context, &pull_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    umock_c_reset_all_calls();

    setup_channel_internal_close_expectations(1);

    //act
    channel_internal_close(channel_internal);

    //assert
    ASSERT_ARE_EQUAL(int, 1, pull_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/* channel_internal_dispose */

/*Tests_SRS_CHANNEL_INTERNAL_43_150: [ channel_internal_dispose shall release the reference to the log_context ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_091: [ channel_internal_dispose shall release the reference to THANDLE(THREADPOOL). ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_099: [ channel_internal_dispose shall call srw_lock_destroy. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_165: [ channel_internal_dispose shall call sm_destroy. ]*/
TEST_FUNCTION(channel_internal_dispose_calls_underlying_functions)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create(g.g_log_context, g.g_threadpool);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(PTR(LOG_CONTEXT_HANDLE))(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(THREADPOOL)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(srw_lock_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* channel_internal_pull */

/*Tests_SRS_CHANNEL_INTERNAL_43_152: [ channel_internal_pull shall call sm_exec_begin. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_010: [ channel_internal_pull shall call srw_lock_acquire_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_170: [ channel_internal_pull shall check if is_open is true. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_101: [ If the list of pending operations is empty or the first operation in the list of pending operations contains a non-NULL on_data_available_cb: ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_103: [ channel_internal_pull shall create a THANDLE(ASYNC_OP) by calling async_op_create with cancel_op as cancel. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_104: [ channel_internal_pull shall store the correlation_id, on_data_available_cb and pull_context in the THANDLE(ASYNC_OP). ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_111: [ channel_internal_pull shall set the result of the created operation to CHANNEL_CALLBACK_RESULT_OK. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_105: [ channel_internal_pull shall insert the created THANDLE(ASYNC_OP) in the list of pending operations by calling DList_InsertTailList. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_107: [ channel_internal_pull shall set *out_op_pull to the created THANDLE(ASYNC_OP). ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_115: [ channel_internal_pull shall call srw_lock_release_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_011: [ channel_internal_pull shall succeeds and return CHANNEL_RESULT_OK. ]*/
TEST_FUNCTION(channel_internal_pull_on_empty_succeeds)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = test_create_and_open_channel_internal();
    THANDLE(ASYNC_OP) pull_op = NULL;
    int32_t pull_context = 0;
    setup_first_op_expectations();

    //act
    CHANNEL_RESULT pull_result = channel_internal_pull(channel_internal, g.g_pull_correlation_id, test_on_data_available_cb_abandoned, &pull_context, &pull_op);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_IS_NOT_NULL(pull_op);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    channel_internal_close(channel_internal);
    ASSERT_ARE_EQUAL(int, 1, pull_context);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/*Tests_SRS_CHANNEL_INTERNAL_43_152: [ channel_internal_pull shall call sm_exec_begin. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_010: [ channel_internal_pull shall call srw_lock_acquire_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_170: [ channel_internal_pull shall check if is_open is true. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_101: [ If the list of pending operations is empty or the first operation in the list of pending operations contains a non-NULL on_data_available_cb: ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_103: [ channel_internal_pull shall create a THANDLE(ASYNC_OP) by calling async_op_create with cancel_op as cancel. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_104: [ channel_internal_pull shall store the correlation_id, on_data_available_cb and pull_context in the THANDLE(ASYNC_OP). ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_111: [ channel_internal_pull shall set the result of the created operation to CHANNEL_CALLBACK_RESULT_OK. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_105: [ channel_internal_pull shall insert the created THANDLE(ASYNC_OP) in the list of pending operations by calling DList_InsertTailList. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_107: [ channel_internal_pull shall set *out_op_pull to the created THANDLE(ASYNC_OP). ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_115: [ channel_internal_pull shall call srw_lock_release_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_011: [ channel_internal_pull shall succeeds and return CHANNEL_RESULT_OK. ]*/
TEST_FUNCTION(channel_internal_pull_after_pull_succeeds)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = test_create_and_open_channel_internal();
    int32_t pull_context_1 = 0;
    THANDLE(ASYNC_OP) pull_op_1 = NULL;
    CHANNEL_RESULT pull_result_1 = channel_internal_pull(channel_internal, g.g_pull_correlation_id, test_on_data_available_cb_abandoned, &pull_context_1, &pull_op_1);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result_1);
    ASSERT_IS_NOT_NULL(pull_op_1);
    int32_t pull_context_2 = 0;
    THANDLE(ASYNC_OP) pull_op_2 = NULL;
    umock_c_reset_all_calls();

    setup_first_op_expectations();

    //act
    CHANNEL_RESULT pull_result_2 = channel_internal_pull(channel_internal, g.g_pull_correlation_id, test_on_data_available_cb_abandoned, &pull_context_2, &pull_op_2);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result_2);
    ASSERT_IS_NOT_NULL(pull_op_2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    channel_internal_close(channel_internal);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op_2, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op_1, NULL);

    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/*Tests_SRS_CHANNEL_INTERNAL_43_152: [channel_internal_pull shall call sm_exec_begin. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_010: [channel_internal_pull shall call srw_lock_acquire_exclusive.]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_170: [channel_internal_pull shall check if is_open is true. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_108: [If the first operation in the list of pending operations contains a non - NULL on_data_consumed_cb : ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_109: [channel_internal_pull shall call DList_RemoveHeadList on the list of pending operations to obtain the operation.]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_112: [channel_internal_pull shall store the correlation_id, on_data_available_cb and pull_context in the obtained operation.]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_113: [channel_internal_pull shall call threadpool_schedule_work with execute_callbacks as work_function and the obtained operation as work_function_context.]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_114: [channel_internal_pull shall set * out_op_pull to the THANDLE(ASYNC_OP) of the obtained operation.]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_115: [channel_internal_pull shall call srw_lock_release_exclusive.]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_011: [channel_internal_pull shall succeeds and return CHANNEL_RESULT_OK.]*/
TEST_FUNCTION(channel_internal_pull_after_push_succeeds)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = test_create_and_open_channel_internal();
    int32_t push_context = 0;
    THANDLE(ASYNC_OP) push_op = NULL;
    THANDLE(RC_PTR) rc_ptr = rc_ptr_create_with_move_pointer(test_data, test_data_dispose, NULL);
    CHANNEL_RESULT push_result_1 = channel_internal_push(channel_internal, g.g_push_correlation_id, rc_ptr, test_on_data_consumed_cb_ok, &push_context, &push_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result_1);
    ASSERT_IS_NOT_NULL(push_op);
    umock_c_reset_all_calls();

    int32_t pull_context = 0;
    THANDLE(ASYNC_OP) pull_op = NULL;
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_context;
    setup_second_op_expectations(true, false, &work_function, &work_context);
    setup_op_cleanup_expectations(true);

    //act
    CHANNEL_RESULT pull_result = channel_internal_pull(channel_internal, g.g_pull_correlation_id, test_on_data_available_cb_ok, &pull_context, &pull_op);
    work_function(work_context);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_IS_NOT_NULL(pull_op);
    ASSERT_ARE_EQUAL(int32_t, 1, push_context);
    ASSERT_ARE_EQUAL(int32_t, 1, pull_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&push_op, NULL);
    THANDLE_ASSIGN(RC_PTR)(&rc_ptr, NULL);
    channel_internal_close(channel_internal);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/*Tests_SRS_CHANNEL_INTERNAL_43_023: [If there are any failures, channel_internal_pull shall fail and return CHANNEL_RESULT_ERROR.]*/
TEST_FUNCTION(channel_internal_pull_as_first_op_fails_when_underlying_functions_fail)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = test_create_and_open_channel_internal();
    THANDLE(ASYNC_OP) pull_op = NULL;
    umock_c_reset_all_calls();

    int32_t pull_context = 0;
    setup_first_op_expectations();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            CHANNEL_RESULT result = channel_internal_pull(channel_internal, g.g_pull_correlation_id, test_on_data_available_cb_abandoned, &pull_context, &pull_op);

            // assert
            ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_ERROR, result, "On failed call %zu", i);
            ASSERT_IS_NULL(pull_op, "On failed call %zu", i);
        }
    }

    //cleanup
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/*Tests_SRS_CHANNEL_INTERNAL_43_023: [If there are any failures, channel_internal_pull shall fail and return CHANNEL_RESULT_ERROR.]*/
TEST_FUNCTION(channel_internal_pull_as_second_op_fails_if_underlying_functions_fail)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = test_create_and_open_channel_internal();
    THANDLE(RC_PTR) data = rc_ptr_create_with_move_pointer(test_data, test_data_dispose, NULL);
    int32_t push_context = 0;
    THANDLE(ASYNC_OP) push_op = NULL;
    CHANNEL_RESULT push_result = channel_internal_push(channel_internal, g.g_push_correlation_id, data, test_on_data_consumed_cb_ok, &push_context, &push_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result);
    ASSERT_IS_NOT_NULL(push_op);

    THANDLE(ASYNC_OP) pull_op = NULL;

    umock_c_reset_all_calls();

    THREADPOOL_WORK_FUNCTION work_function;
    void* work_context;
    setup_second_op_expectations(true, true, &work_function, &work_context);
    int pull_context = 0;

    //act
    CHANNEL_RESULT result = channel_internal_pull(channel_internal, g.g_pull_correlation_id, test_on_data_available_cb_abandoned, &pull_context, &pull_op);

    // assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_ERROR, result);
    ASSERT_IS_NULL(pull_op);
    ASSERT_ARE_EQUAL(int, 0, pull_context);
    ASSERT_ARE_EQUAL(int, 0, push_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //arrange
    setup_op_cleanup_expectations(false);

    //act
    work_function(work_context);


    //assert
    ASSERT_ARE_EQUAL(int, 0, pull_context);
    ASSERT_ARE_EQUAL(int, 1, push_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&push_op, NULL);
    THANDLE_ASSIGN(RC_PTR)(&data, NULL);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/* channel_internal_push */

/*Tests_SRS_CHANNEL_INTERNAL_43_153: [ channel_internal_push shall call sm_exec_begin. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_116: [ channel_internal_push shall call srw_lock_acquire_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_171: [ channel_internal_push shall check if is_open is true. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_117: [ If the list of pending operations is empty or the first operation in the list of pending operations contains a non-NULL on_data_consumed_cb: ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_119: [ channel_internal_push shall create a THANDLE(ASYNC_OP) by calling async_op_create with cancel_op as cancel. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_120: [ channel_internal_push shall store the correlation_id, on_data_consumed_cb, push_context and data in the THANDLE(ASYNC_OP). ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_127: [ channel_internal_push shall set the result of the created operation to CHANNEL_CALLBACK_RESULT_OK. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_121: [ channel_internal_push shall insert the created THANDLE(ASYNC_OP) in the list of pending operations by calling DList_InsertTailList. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_123: [ channel_internal_push shall set *out_op_push to the created THANDLE(ASYNC_OP). ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_131: [ channel_internal_push shall call srw_lock_release_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_132: [ channel_internal_push shall succeed and return CHANNEL_RESULT_OK. ]*/
TEST_FUNCTION(channel_internal_push_on_empty_succeeds)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = test_create_and_open_channel_internal();
    THANDLE(RC_PTR) data = rc_ptr_create_with_move_pointer(test_data, test_data_dispose, NULL);
    THANDLE(ASYNC_OP) push_op = NULL;
    umock_c_reset_all_calls();

    int32_t push_context = 0;
    setup_first_op_expectations();

    //act
    CHANNEL_RESULT pull_result = channel_internal_push(channel_internal, g.g_push_correlation_id, data, test_on_data_consumed_cb_abandoned, &push_context, &push_op);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_IS_NOT_NULL(push_op);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    channel_internal_close(channel_internal);
    ASSERT_ARE_EQUAL(int, 1, push_context);
    THANDLE_ASSIGN(ASYNC_OP)(&push_op, NULL);
    THANDLE_ASSIGN(RC_PTR)(&data, NULL);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/*Tests_SRS_CHANNEL_INTERNAL_43_153: [ channel_internal_push shall call sm_exec_begin. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_116: [ channel_internal_push shall call srw_lock_acquire_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_171: [ channel_internal_push shall check if is_open is true. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_117: [ If the list of pending operations is empty or the first operation in the list of pending operations contains a non-NULL on_data_consumed_cb: ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_119: [ channel_internal_push shall create a THANDLE(ASYNC_OP) by calling async_op_create with cancel_op as cancel. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_120: [ channel_internal_push shall store the correlation_id, on_data_consumed_cb, push_context and data in the THANDLE(ASYNC_OP). ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_127: [ channel_internal_push shall set the result of the created operation to CHANNEL_CALLBACK_RESULT_OK. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_121: [ channel_internal_push shall insert the created THANDLE(ASYNC_OP) in the list of pending operations by calling DList_InsertTailList. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_123: [ channel_internal_push shall set *out_op_push to the created THANDLE(ASYNC_OP). ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_131: [ channel_internal_push shall call srw_lock_release_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_132: [ channel_internal_push shall succeed and return CHANNEL_RESULT_OK. ]*/
TEST_FUNCTION(channel_internal_push_after_push_succeeds)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = test_create_and_open_channel_internal();
    int32_t push_context_1 = 0;
    THANDLE(RC_PTR) data_1 = rc_ptr_create_with_move_pointer(test_data, test_data_dispose, NULL);
    THANDLE(ASYNC_OP) push_op_1 = NULL;
    CHANNEL_RESULT push_result_1 = channel_internal_push(channel_internal, g.g_push_correlation_id, data_1, test_on_data_consumed_cb_abandoned, &push_context_1, &push_op_1);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result_1);
    ASSERT_IS_NOT_NULL(push_op_1);
    int32_t push_context_2 = 0;
    THANDLE(RC_PTR) data_2 = rc_ptr_create_with_move_pointer(test_data_2, test_data_dispose, NULL);
    THANDLE(ASYNC_OP) push_op_2 = NULL;
    umock_c_reset_all_calls();

    setup_first_op_expectations();

    //act
    CHANNEL_RESULT push_result_2 = channel_internal_push(channel_internal, g.g_push_correlation_id, data_2, test_on_data_consumed_cb_abandoned, &push_context_2, &push_op_2);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result_2);
    ASSERT_IS_NOT_NULL(push_op_2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    channel_internal_close(channel_internal);
    THANDLE_ASSIGN(ASYNC_OP)(&push_op_2, NULL);
    THANDLE_ASSIGN(RC_PTR)(&data_2, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&push_op_1, NULL);
    THANDLE_ASSIGN(RC_PTR)(&data_1, NULL);

    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/*Tests_SRS_CHANNEL_INTERNAL_43_153: [ channel_internal_push shall call sm_exec_begin. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_116: [ channel_internal_push shall call srw_lock_acquire_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_171: [ channel_internal_push shall check if is_open is true. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_124: [ Otherwise (the first operation in the list of pending operations contains a non-NULL on_data_available_cb): ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_125: [ channel_internal_push shall call DList_RemoveHeadList on the list of pending operations to obtain the operation. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_128: [ channel_internal_push shall store the correlation_id, on_data_consumed_cb, push_context and data in the obtained operation. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_129: [ channel_internal_push shall call threadpool_schedule_work with execute_callbacks as work_function and the obtained operation as work_function_context. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_130: [ channel_internal_push shall set *out_op_push to the THANDLE(ASYNC_OP) of the obtained operation. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_131: [ channel_internal_push shall call srw_lock_release_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_132: [ channel_internal_push shall succeed and return CHANNEL_RESULT_OK. ]*/
TEST_FUNCTION(channel_internal_push_after_pull_succeeds)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = test_create_and_open_channel_internal();
    int32_t pull_context = 0;
    THANDLE(ASYNC_OP) pull_op = NULL;
    CHANNEL_RESULT pull_result_1 = channel_internal_pull(channel_internal, g.g_pull_correlation_id, test_on_data_available_cb_ok, &pull_context, &pull_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result_1);
    ASSERT_IS_NOT_NULL(pull_op);

    int32_t push_context = 0;
    THANDLE(ASYNC_OP) push_op = NULL;
    THANDLE(RC_PTR) rc_ptr = rc_ptr_create_with_move_pointer(test_data, test_data_dispose, NULL);
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_context;
    umock_c_reset_all_calls();

    setup_second_op_expectations(false, false, &work_function, &work_context);
    setup_op_cleanup_expectations(true);

    //act
    CHANNEL_RESULT push_result = channel_internal_push(channel_internal, g.g_push_correlation_id, rc_ptr, test_on_data_consumed_cb_ok, &push_context, &push_op);
    work_function(work_context);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result);
    ASSERT_IS_NOT_NULL(push_op);
    ASSERT_ARE_EQUAL(int32_t, 1, pull_context);
    ASSERT_ARE_EQUAL(int32_t, 1, push_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(RC_PTR)(&rc_ptr, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&push_op, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    channel_internal_close(channel_internal);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}


/*Tests_SRS_CHANNEL_INTERNAL_43_041: [ If there are any failures, channel_internal_push shall fail and return CHANNEL_RESULT_ERROR. ]*/
TEST_FUNCTION(channel_internal_push_as_first_op_fails_when_underlying_functions_fail)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = test_create_and_open_channel_internal();
    THANDLE(ASYNC_OP) push_op = NULL;
    THANDLE(RC_PTR) data = rc_ptr_create_with_move_pointer(test_data, test_data_dispose, NULL);
    umock_c_reset_all_calls();

    int32_t push_context = 0;
    setup_first_op_expectations();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            CHANNEL_RESULT result = channel_internal_push(channel_internal, g.g_push_correlation_id, data, test_on_data_consumed_cb_abandoned, &push_context, &push_op);

            // assert
            ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_ERROR, result, "On failed call %zu", i);
            ASSERT_IS_NULL(push_op, "On failed call %zu", i);
        }
    }

    //cleanup
    THANDLE_ASSIGN(RC_PTR)(&data, NULL);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/*Tests_SRS_CHANNEL_INTERNAL_43_041: [ If there are any failures, channel_internal_push shall fail and return CHANNEL_RESULT_ERROR. ]*/
TEST_FUNCTION(channel_internal_push_as_second_op_fails_if_underlying_functions_fail)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = test_create_and_open_channel_internal();
    THANDLE(ASYNC_OP) pull_op = NULL;
    int32_t pull_context = 0;
    CHANNEL_RESULT pull_result = channel_internal_pull(channel_internal, g.g_pull_correlation_id, test_on_data_available_cb_ok, &pull_context, &pull_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_IS_NOT_NULL(pull_op);

    THANDLE(ASYNC_OP) push_op = NULL;
    THANDLE(RC_PTR) data = rc_ptr_create_with_move_pointer(test_data, test_data_dispose, NULL);

    umock_c_reset_all_calls();

    THREADPOOL_WORK_FUNCTION work_function;
    void* work_context;
    setup_second_op_expectations(false, true, &work_function, &work_context);
    int push_context = 0;

    //act
    CHANNEL_RESULT result = channel_internal_push(channel_internal, g.g_push_correlation_id, data, test_on_data_consumed_cb_ok, &push_context, &push_op);

    // assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_ERROR, result);
    ASSERT_IS_NULL(push_op);
    ASSERT_ARE_EQUAL(int, 0, push_context);
    ASSERT_ARE_EQUAL(int, 0, pull_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //arrange
    setup_op_cleanup_expectations(false);

    //act
    work_function(work_context);

    //assert
    ASSERT_ARE_EQUAL(int, 0, push_context);
    ASSERT_ARE_EQUAL(int, 1, pull_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(RC_PTR)(&data, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/* cancel_op */

/*Tests_SRS_CHANNEL_INTERNAL_43_154: [ cancel_op shall call sm_exec_begin. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_134: [ cancel_op shall call srw_lock_acquire_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_135: [ If the operation is in the list of pending operations, cancel_op shall call DList_RemoveEntryList to remove it. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_139: [ cancel_op shall call srw_lock_release_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_136: [ If the result of the operation is CHANNEL_CALLBACK_RESULT_OK, cancel_op shall set it to CHANNEL_CALLBACK_RESULT_CANCELLED. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_138: [ If the operation had been found in the list of pending operations, cancel_op shall call threadpool_schedule_work with execute_callbacks as work_function and the operation as work_function_context. ]*/
TEST_FUNCTION(channel_internal_cancel_op_cancels_pull)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = test_create_and_open_channel_internal();
    int32_t pull_context = 0;
    THANDLE(ASYNC_OP) pull_op = NULL;
    CHANNEL_RESULT pull_result = channel_internal_pull(channel_internal, g.g_pull_correlation_id, test_on_data_available_cb_cancelled, &pull_context, &pull_op);
    ASSERT_IS_NOT_NULL(pull_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    umock_c_reset_all_calls();

    THREADPOOL_WORK_FUNCTION work_function;
    void* work_context;

    setup_cancel_op_expectations(false, &work_function, &work_context);

    //act
    real_async_op_cancel(pull_op);

    //assert
    ASSERT_ARE_EQUAL(int, 1, pull_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/*Tests_SRS_CHANNEL_INTERNAL_43_154: [ cancel_op shall call sm_exec_begin. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_134: [ cancel_op shall call srw_lock_acquire_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_136: [ If the result of the operation is CHANNEL_CALLBACK_RESULT_OK, cancel_op shall set it to CHANNEL_CALLBACK_RESULT_CANCELLED. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_139: [ cancel_op shall call srw_lock_release_exclusive. ]*/
/*Tests_SRS_CHANNEL_INTERNAL_43_156: [ cancel_op shall call sm_exec_end. ]*/
TEST_FUNCTION(channel_internal_cancel_op_cancels_matched_op)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = test_create_and_open_channel_internal();
    THANDLE(RC_PTR) data = rc_ptr_create_with_move_pointer(test_data, test_data_dispose, NULL);
    umock_c_reset_all_calls();

    THREADPOOL_WORK_FUNCTION work_function;
    void* work_context;
    setup_first_op_expectations();
    setup_second_op_expectations(false, false, &work_function, &work_context);

    int32_t pull_context = 0;
    THANDLE(ASYNC_OP) pull_op = NULL;
    int32_t push_context = 0;
    THANDLE(ASYNC_OP) push_op = NULL;

    CHANNEL_RESULT pull_result = channel_internal_pull(channel_internal, g.g_pull_correlation_id, test_on_data_available_cb_matched_cancelled, &pull_context, &pull_op);
    CHANNEL_RESULT push_result = channel_internal_push(channel_internal, g.g_push_correlation_id, data, test_on_data_consumed_cb_cancelled, &push_context, &push_op);

    ASSERT_IS_NOT_NULL(pull_op);
    ASSERT_IS_NOT_NULL(push_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    setup_op_cleanup_expectations(true);

    //act
    real_async_op_cancel(pull_op);
    work_function(work_context);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_ARE_EQUAL(int, 1, pull_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&push_op, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    THANDLE_ASSIGN(RC_PTR)(&data, NULL);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/*
* This test fails because there is an inherent limitation that prevents cancelling an abandoned operation.
* The test is disabled until the underlying issue is fixed. TODO Task 30251613
*/
/*Tests_SRS_CHANNEL_INTERNAL_43_155: [ If there are any failures, cancel_op shall fail. ]*/
DISABLE_TEST_FUNCTION(channel_internal_cancel_op_fails_to_cancel_abandoned_op)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = test_create_and_open_channel_internal();
    int32_t pull_context = 0;
    THANDLE(ASYNC_OP) pull_op = NULL;
    setup_first_op_expectations();
    CHANNEL_RESULT pull_result = channel_internal_pull(channel_internal, g.g_pull_correlation_id, test_on_data_available_cb_abandoned, &pull_context, &pull_op);
    ASSERT_IS_NOT_NULL(pull_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);

    setup_channel_internal_close_expectations(1);
    channel_internal_close(channel_internal);

    ASSERT_ARE_EQUAL(int, 1, pull_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));

    //act
    real_async_op_cancel(pull_op);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}


/* execute_callbacks */

/*Tests_SRS_CHANNEL_INTERNAL_43_148: [ If channel_internal_op_context is NULL, execute_callbacks shall terminate the process. ]*/
TEST_FUNCTION(channel_internal_execute_callback_fails_with_NULL_context)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = test_create_and_open_channel_internal();
    THANDLE(RC_PTR) data = rc_ptr_create_with_move_pointer(test_data, test_data_dispose, NULL);
    umock_c_reset_all_calls();

    THREADPOOL_WORK_FUNCTION work_function;
    void* work_context;
    setup_first_op_expectations();
    setup_second_op_expectations(false, false, &work_function, &work_context);

    int32_t pull_context = 0;
    THANDLE(ASYNC_OP) pull_op = NULL;
    int32_t push_context = 0;
    THANDLE(ASYNC_OP) push_op = NULL;

    CHANNEL_RESULT pull_result = channel_internal_pull(channel_internal, g.g_pull_correlation_id, test_on_data_available_cb_ok, &pull_context, &pull_op);
    CHANNEL_RESULT push_result = channel_internal_push(channel_internal, g.g_push_correlation_id, data, test_on_data_consumed_cb_ok, &push_context, &push_op);

    ASSERT_IS_NOT_NULL(pull_op);
    ASSERT_IS_NOT_NULL(push_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(ps_util_terminate_process());

    //act
    work_function(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // calling execute_callbacks with NULL context should not have had any effect. Calling again to perform cleanup
    setup_op_cleanup_expectations(true);

    work_function(work_context);

    //assert
    ASSERT_ARE_EQUAL(int, 1, pull_context);
    ASSERT_ARE_EQUAL(int, 1, push_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    THANDLE_ASSIGN(ASYNC_OP)(&push_op, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    THANDLE_ASSIGN(RC_PTR)(&data, NULL);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
