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
#include "umock_c/umock_lock_factory_default.h"

#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/sync.h"
#include "c_pal/execution_engine.h"
#include "c_pal/threadapi.h"

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
#include "real_doublylinkedlist.h"
#include "real_async_op.h"
#include "real_rc_ptr.h"

#include "c_util/channel_internal.h"

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

MOCK_FUNCTION_WITH_CODE(, void, mocked_threadpool_schedule_work, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context)
MOCK_FUNCTION_END()

static struct TEST_THREADPOOL_TAG
{
    THANDLE(THREADPOOL) test_threadpool;
} test_threadpool = {(void*)0x1000};
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
    ASSERT_ARE_EQUAL(int, 0, (*(int32_t*)context)++);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_ABANDONED, result);
    ASSERT_IS_NULL(data);

}

static void test_push_callback_abandoned(void* context, CHANNEL_CALLBACK_RESULT result)
{
    ASSERT_ARE_EQUAL(int, 0, (*(int32_t*)context)++);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_ABANDONED, result);
}

static void test_pull_callback_ok(void* context, CHANNEL_CALLBACK_RESULT result, THANDLE(RC_PTR) data)
{
    ASSERT_ARE_EQUAL(int, 0, (*(int32_t*)context)++);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_OK, result);
    ASSERT_ARE_EQUAL(void_ptr, data->ptr, test_data);
}

static void test_push_callback_ok(void* context, CHANNEL_CALLBACK_RESULT result)
{
    ASSERT_ARE_EQUAL(int, 0, (*(int32_t*)context)++);
    ASSERT_ARE_EQUAL(CHANNEL_CALLBACK_RESULT, CHANNEL_CALLBACK_RESULT_OK, result);
}

static void setup_channel_internal_create_and_open_expectations()
{
    STRICT_EXPECTED_CALL(srw_lock_create(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_ARG));
}

static void setup_first_op_expectations()
{
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_ARG));
    STRICT_EXPECTED_CALL(async_op_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(ASYNC_OP)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_PTR)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_InsertTailList(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(ASYNC_OP)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));
}

static void setup_second_op_expectations(bool is_pull, THREADPOOL_WORK_FUNCTION* captured_work_function, void** captured_work_context)
{
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_IsListEmpty(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_ARG));
    is_pull? (void)0 : (void)STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_PTR)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_schedule_work(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_work_function(captured_work_function)
        .CaptureArgumentValue_work_function_context(captured_work_context);
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(ASYNC_OP)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));
}

static void setup_channel_internal_close_expectations(size_t num_ops, THREADPOOL_WORK_FUNCTION* work_functions, void** work_contexts)
{
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_ARG));
    for (size_t i = 0; i < num_ops; ++i)
    {
        STRICT_EXPECTED_CALL(threadpool_schedule_work(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
            .CaptureArgumentValue_work_function(&work_functions[i])
            .CaptureArgumentValue_work_function_context(&work_contexts[i]);
        STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));

    for (size_t i = 0; i < num_ops; ++i)
    {
        STRICT_EXPECTED_CALL (THANDLE_ASSIGN(RC_PTR)(IGNORED_ARG, NULL));
        STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(ASYNC_OP)(IGNORED_ARG, IGNORED_ARG));
        STRICT_EXPECTED_CALL(THANDLE_ASSIGN(ASYNC_OP)(IGNORED_ARG, NULL));
    }
}

static void cleanup_pending_ops(THANDLE(CHANNEL_INTERNAL) channel_internal, int32_t* op_contexts, size_t num_ops)
{
    THREADPOOL_WORK_FUNCTION* work_functions = real_gballoc_hl_malloc(sizeof(THREADPOOL_WORK_FUNCTION) * num_ops);
    ASSERT_IS_NOT_NULL(work_functions);
    void** work_contexts = real_gballoc_hl_malloc(sizeof(void*) * num_ops);
    ASSERT_IS_NOT_NULL(work_contexts);
    setup_channel_internal_close_expectations(num_ops, work_functions, work_contexts);
    channel_internal_close(channel_internal);
    for (size_t i = 0; i < num_ops; ++i)
    {
        work_functions[i](work_contexts[i]);
        ASSERT_ARE_EQUAL(int32_t, 1, op_contexts[i]);
    }
    real_gballoc_hl_free(work_contexts);
    real_gballoc_hl_free(work_functions);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_SRW_LOCK_GLOBAL_MOCK_HOOK();
    REGISTER_DOUBLYLINKEDLIST_GLOBAL_MOCK_HOOKS();
    REGISTER_ASYNC_OP_GLOBAL_MOCK_HOOKS();
    REGISTER_RC_PTR_GLOBAL_MOCK_HOOKS();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(srw_lock_create, NULL);

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
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
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
    THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create_and_open(test_threadpool.test_threadpool);

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
            THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create_and_open(test_threadpool.test_threadpool);

            // assert
            ASSERT_IS_NULL(channel_internal, "On failed call %zu", i);
        }
    }
}

/* channel_internal_close */

/*Codes_SRS_CHANNEL_INTERNAL_43_094: [ channel_dispose shall call srw_lock_acquire_exclusive. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_095: [ channel_dispose shall iterate over the list of pending operations and do the following: ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_096: [ set the result of the operation to CHANNEL_CALLBACK_RESULT_ABANDONED. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_097: [ call threadpool_schedule_work with execute_callbacks as work_function. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_100: [ channel_dispose shall call srw_lock_release_exclusive. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_145: [ execute_callbacks shall call the stored callback(s) with the result of the operation. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_147: [ execute_callbacks shall perform cleanup of the operation. ]*/
TEST_FUNCTION(channel_internal_close_calls_underlying_functions)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create_and_open(test_threadpool.test_threadpool);
    THANDLE(ASYNC_OP) pull_op = NULL;
    int32_t pull_context = 0;
    CHANNEL_RESULT pull_result = channel_internal_pull(channel_internal, test_pull_callback_abandoned, &pull_context, &pull_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    umock_c_reset_all_calls();

    THREADPOOL_WORK_FUNCTION* work_function = real_gballoc_hl_malloc(sizeof(THREADPOOL_WORK_FUNCTION) * 1);
    void** work_context = real_gballoc_hl_malloc(sizeof(void*) * 1);

    ASSERT_IS_NOT_NULL(work_function);
    ASSERT_IS_NOT_NULL(work_context);

    setup_channel_internal_close_expectations(1, work_function, work_context);

    //act
    channel_internal_close(channel_internal);
    work_function[0](work_context[0]);

    //assert
    ASSERT_ARE_EQUAL(int, 1, pull_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    real_gballoc_hl_free(work_context);
    real_gballoc_hl_free(work_function);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}


/* channel_internal_dispose */

/*Codes_SRS_CHANNEL_INTERNAL_43_099: [ channel_internal_dispose shall call srw_lock_destroy. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_091: [ channel_internal_dispose shall release the reference to THANDLE(THREADPOOL). ]*/
TEST_FUNCTION(channel_internal_dispose_calls_underlying_functions)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create_and_open(test_threadpool.test_threadpool);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(srw_lock_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(THREADPOOL)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* channel_internal_pull */

/*Codes_SRS_CHANNEL_INTERNAL_43_116: [ channel_internal_push shall call srw_lock_acquire_exclusive. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_117: [ If the list of pending operations is empty or the first operation in the list of pending operations contains a non-NULL push_callback: ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_103: [ channel_internal_pull shall create a THANDLE(ASYNC_OP) by calling async_op_create with cancel_op as cancel. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_104: [ channel_internal_pull shall store the pull_callback and pull_context in the THANDLE(ASYNC_OP). ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_111: [ channel_internal_pull shall set the result of the created operation to CHANNEL_CALLBACK_RESULT_OK. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_105: [ channel_internal_pull shall insert the created THANDLE(ASYNC_OP) in the list of pending operations by calling DList_InsertTailList. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_107: [ channel_internal_pull shall set *out_op_pull to the created THANDLE(ASYNC_OP). ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_115: [ channel_internal_pull shall call srw_lock_release_exclusive. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_011: [ channel_internal_pull shall succeeds and return CHANNEL_RESULT_OK. ]*/
TEST_FUNCTION(channel_internal_pull_on_empty_succeeds)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create_and_open(test_threadpool.test_threadpool);
    THANDLE(ASYNC_OP) pull_op = NULL;
    umock_c_reset_all_calls();

    int32_t pull_context = 0;
    setup_first_op_expectations();

    //act
    CHANNEL_RESULT pull_result = channel_internal_pull(channel_internal, test_pull_callback_abandoned, &pull_context, &pull_op);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result);
    ASSERT_IS_NOT_NULL(pull_op);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    cleanup_pending_ops(channel_internal, (int32_t[]){pull_context}, 1);
    ASSERT_ARE_EQUAL(int, 1, pull_context);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op, NULL);
    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}

/*Codes_SRS_CHANNEL_INTERNAL_43_116: [ channel_internal_push shall call srw_lock_acquire_exclusive. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_117: [ If the list of pending operations is empty or the first operation in the list of pending operations contains a non-NULL push_callback: ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_103: [ channel_internal_pull shall create a THANDLE(ASYNC_OP) by calling async_op_create with cancel_op as cancel. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_104: [ channel_internal_pull shall store the pull_callback and pull_context in the THANDLE(ASYNC_OP). ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_111: [ channel_internal_pull shall set the result of the created operation to CHANNEL_CALLBACK_RESULT_OK. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_105: [ channel_internal_pull shall insert the created THANDLE(ASYNC_OP) in the list of pending operations by calling DList_InsertTailList. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_107: [ channel_internal_pull shall set *out_op_pull to the created THANDLE(ASYNC_OP). ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_115: [ channel_internal_pull shall call srw_lock_release_exclusive. ]*/
/*Codes_SRS_CHANNEL_INTERNAL_43_011: [ channel_internal_pull shall succeeds and return CHANNEL_RESULT_OK. ]*/
TEST_FUNCTION(channel_internal_pull_after_pull_succeeds)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create_and_open(test_threadpool.test_threadpool);
    int32_t pull_context_1 = 0;
    THANDLE(ASYNC_OP) pull_op_1 = NULL;
    CHANNEL_RESULT pull_result_1 = channel_internal_pull(channel_internal, test_pull_callback_abandoned, &pull_context_1, &pull_op_1);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result_1);
    ASSERT_IS_NOT_NULL(pull_op_1);
    umock_c_reset_all_calls();
    
    int32_t pull_context_2 = 0;
    THANDLE(ASYNC_OP) pull_op_2 = NULL;
    setup_first_op_expectations();

    //act
    CHANNEL_RESULT pull_result_2 = channel_internal_pull(channel_internal, test_pull_callback_abandoned, &pull_context_2, &pull_op_2);

    //assert
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, pull_result_2);
    ASSERT_IS_NOT_NULL(pull_op_2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    cleanup_pending_ops(channel_internal, (int32_t[]){pull_context_1, pull_context_2}, 2);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op_2, NULL);
    THANDLE_ASSIGN(ASYNC_OP)(&pull_op_1, NULL);

    THANDLE_ASSIGN(CHANNEL_INTERNAL)(&channel_internal, NULL);
}


TEST_FUNCTION(channel_internal_pull_after_push_succeeds)
{
    //arrange
    THANDLE(CHANNEL_INTERNAL) channel_internal = channel_internal_create_and_open(test_threadpool.test_threadpool);
    int32_t push_context = 0;
    THANDLE(ASYNC_OP) push_op = NULL;
    THANDLE(RC_PTR) rc_ptr = rc_ptr_create_with_move_pointer(test_data, do_nothing);
    CHANNEL_RESULT push_result_1 = channel_internal_push(channel_internal, rc_ptr, test_push_callback_ok, &push_context, &push_op);
    ASSERT_ARE_EQUAL(CHANNEL_RESULT, CHANNEL_RESULT_OK, push_result_1);
    ASSERT_IS_NOT_NULL(push_op);
    umock_c_reset_all_calls();

    int32_t pull_context = 0;
    THANDLE(ASYNC_OP) pull_op = NULL;
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_context;
    setup_second_op_expectations(true, &work_function, &work_context);
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_PTR)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(ASYNC_OP)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(ASYNC_OP)(IGNORED_ARG, NULL));

    //act
    CHANNEL_RESULT pull_result = channel_internal_pull(channel_internal, test_pull_callback_ok, &pull_context, &pull_op);
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

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
