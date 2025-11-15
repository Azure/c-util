// Copyright (c) Microsoft. All rights reserved.


#include "tp_worker_thread_ut_pch.h"

MU_DEFINE_ENUM_STRINGS(SM_RESULT, SM_RESULT_VALUES)

TEST_DEFINE_ENUM_TYPE(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT_VALUES)

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

MOCK_FUNCTION_WITH_CODE(, void, test_worker_function, void*, worker_function_context)
MOCK_FUNCTION_END();

typedef struct WORKER_FUNC_CONTEXT_TAG
{
    TP_WORKER_THREAD_HANDLE worker_thread;
    uint32_t schedule_again_count;
    uint32_t schedule_again_count_per_call;
} WORKER_FUNC_CONTEXT;

static EXECUTION_ENGINE_HANDLE test_execution_engine = (EXECUTION_ENGINE_HANDLE)0x4242;
static WORKER_FUNC_CONTEXT test_worker_context = { 0 };

static void hook_test_worker_function(void* worker_function_context)
{
    WORKER_FUNC_CONTEXT* context = worker_function_context;
    if (context->schedule_again_count > 0)
    {
        context->schedule_again_count--;
        for (uint32_t i = 0; i < context->schedule_again_count_per_call; i++)
        {
            ASSERT_ARE_EQUAL(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_OK, tp_worker_thread_schedule_process(context->worker_thread));
        }
    }
}

static THANDLE(THREADPOOL) hook_threadpool_create(EXECUTION_ENGINE_HANDLE execution_engine)
{
    (void)execution_engine;

    THANDLE(THREADPOOL) result = real_threadpool_thandle_create();
    ASSERT_IS_NOT_NULL(result);

    return result;
}

static struct G_TAG
{
    THANDLE(THREADPOOL_WORK_ITEM) last_created_work_item;
} g = { NULL };

static THANDLE(THREADPOOL_WORK_ITEM) hook_threadpool_create_work_item(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context)
{
    (void)threadpool;
    (void)work_function;
    (void)work_function_context;
    THANDLE(THREADPOOL_WORK_ITEM) result = real_threadpool_work_item_thandle_create();
    ASSERT_IS_NOT_NULL(result);
    THANDLE_ASSIGN(REAL_THREADPOOL_WORK_ITEM)(&g.last_created_work_item, result);
    return result;
}

static void expect_create(void)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL_WORK_ITEM)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
}

static TP_WORKER_THREAD_HANDLE test_create(void)
{
    expect_create();
    TP_WORKER_THREAD_HANDLE result = tp_worker_thread_create(test_execution_engine, test_worker_function, &test_worker_context);
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    return result;
}

static void expect_open(THREADPOOL_WORK_FUNCTION* captured_work_func, void** captured_work_context)
{
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(threadpool_create(test_execution_engine));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_create_work_item(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_work_function(captured_work_func)
        .CaptureArgumentValue_work_function_context(captured_work_context);
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(THREADPOOL_WORK_ITEM)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));
}

static TP_WORKER_THREAD_HANDLE test_create_and_open(THREADPOOL_WORK_FUNCTION* captured_work_func, void** captured_work_context)
{
    TP_WORKER_THREAD_HANDLE result = test_create();

    expect_open(captured_work_func, captured_work_context);

    ASSERT_ARE_EQUAL(int, 0, tp_worker_thread_open(result));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    return result;
}

static void expect_schedule_first_time(void)
{
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(threadpool_schedule_work_item(IGNORED_ARG, g.last_created_work_item));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
}

static void expect_schedule_already_running(void)
{
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

}

static void test_schedule_work_first_time(TP_WORKER_THREAD_HANDLE worker_thread)
{
    expect_schedule_first_time();
    ASSERT_ARE_EQUAL(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_OK, tp_worker_thread_schedule_process(worker_thread));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
}

static void test_schedule_work_already_running(TP_WORKER_THREAD_HANDLE worker_thread)
{
    expect_schedule_already_running();
    ASSERT_ARE_EQUAL(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_OK, tp_worker_thread_schedule_process(worker_thread));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
}

static void expect_close(void)
{
    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(THREADPOOL_WORK_ITEM)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(THREADPOOL)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));
}

static void test_close(TP_WORKER_THREAD_HANDLE worker_thread)
{
    expect_close();

    tp_worker_thread_close(worker_thread);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
}

static void expect_destroy(void)
{
    STRICT_EXPECTED_CALL(execution_engine_dec_ref(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_SM_GLOBAL_MOCK_HOOK();
    REGISTER_REAL_THANDLE_MOCK_HOOK(THREADPOOL);
    REGISTER_REAL_THANDLE_MOCK_HOOK(THREADPOOL_WORK_ITEM);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(threadpool_create, hook_threadpool_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(threadpool_create, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(threadpool_create_work_item, hook_threadpool_create_work_item);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(threadpool_create_work_item, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(test_worker_function, hook_test_worker_function);

    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(THREADPOOL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREADPOOL_WORK_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(THREADPOOL_WORK_ITEM), void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    THANDLE_ASSIGN(REAL_THREADPOOL_WORK_ITEM)(&g.last_created_work_item, NULL);

    test_worker_context.schedule_again_count = 0;
    test_worker_context.schedule_again_count_per_call = 1;

    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

//
// tp_worker_thread_create
//

/*Tests_SRS_TP_WORKER_THREAD_42_001: [ If execution_engine is NULL then tp_worker_thread_create shall fail and return NULL. ]*/
TEST_FUNCTION(tp_worker_thread_create_with_NULL_execution_engine_fails)
{
    // arrange

    // act
    TP_WORKER_THREAD_HANDLE result = tp_worker_thread_create(NULL, test_worker_function, &test_worker_context);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_TP_WORKER_THREAD_42_002: [ If worker_func is NULL then tp_worker_thread_create shall fail and return NULL. ]*/
TEST_FUNCTION(tp_worker_thread_create_with_NULL_worker_func_fails)
{
    // arrange

    // act
    TP_WORKER_THREAD_HANDLE result = tp_worker_thread_create(test_execution_engine, NULL, &test_worker_context);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_TP_WORKER_THREAD_42_003: [ tp_worker_thread_create shall allocate memory for the worker thread. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_004: [ tp_worker_thread_create shall call sm_create. ]*/
/*Tests_SRS_TP_WORKER_THREAD_45_001: [ tp_worker_thread_create shall save the execution_engine and call execution_engine_inc_ref. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_006: [ tp_worker_thread_create shall return the worker thread handle. ]*/
TEST_FUNCTION(tp_worker_thread_create_succeeds)
{
    // arrange
    expect_create();

    // act
    TP_WORKER_THREAD_HANDLE result = tp_worker_thread_create(test_execution_engine, test_worker_function, &test_worker_context);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(result);
}

/*Tests_SRS_TP_WORKER_THREAD_42_007: [ If there are any other errors then tp_worker_thread_create shall fail and return NULL. ]*/
TEST_FUNCTION(tp_worker_thread_create_fails_if_underlying_functions_fail)
{
    // arrange
    expect_create();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            TP_WORKER_THREAD_HANDLE result = tp_worker_thread_create(test_execution_engine, test_worker_function, &test_worker_context);

            // assert
            ASSERT_IS_NULL(result);
        }
    }
}

//
// tp_worker_thread_destroy
//

/*Tests_SRS_TP_WORKER_THREAD_42_008: [ If worker_thread is NULL then tp_worker_thread_destroy shall return. ]*/
TEST_FUNCTION(tp_worker_thread_destroy_with_NULL_worker_thread_returns)
{
    // arrange

    // act
    tp_worker_thread_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_TP_WORKER_THREAD_45_002: [ tp_worker_thread_destroy shall call execution_engine_dec_ref. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_011: [ tp_worker_thread_destroy shall call sm_destroy. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_012: [ tp_worker_thread_destroy shall free the memory allocated for the worker thread. ]*/
TEST_FUNCTION(tp_worker_thread_destroy_succeeds)
{
    // arrange
    TP_WORKER_THREAD_HANDLE worker_thread = test_create();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    expect_destroy();

    // act
    tp_worker_thread_destroy(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_TP_WORKER_THREAD_42_009: [ tp_worker_thread_destroy shall behave as if tp_worker_thread_close was called. ]*/
/*Tests_SRS_TP_WORKER_THREAD_45_002: [ tp_worker_thread_destroy shall call execution_engine_dec_ref. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_011: [ tp_worker_thread_destroy shall call sm_destroy. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_012: [ tp_worker_thread_destroy shall free the memory allocated for the worker thread. ]*/
TEST_FUNCTION(tp_worker_thread_destroy_implicitly_closes)
{
    // arrange
    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    TP_WORKER_THREAD_HANDLE worker_thread = test_create_and_open(&captured_work_func, &captured_work_context);

    expect_close();
    expect_destroy();

    // act
    tp_worker_thread_destroy(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//
// tp_worker_thread_open
//

/*Tests_SRS_TP_WORKER_THREAD_42_013: [ If worker_thread is NULL then tp_worker_thread_open shall fail and return a non-zero value. ]*/
TEST_FUNCTION(tp_worker_thread_open_with_NULL_worker_thread_fails)
{
    // arrange

    // act
    int result = tp_worker_thread_open(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_TP_WORKER_THREAD_42_014: [ tp_worker_thread_open shall call sm_open_begin. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_015: [ tp_worker_thread_open shall initialize the state to IDLE. ]*/
/*Tests_SRS_TP_WORKER_THREAD_45_003: [ tp_worker_thread_open shall call threadpool_create with the saved execution_engine. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_041: [ tp_worker_thread_open shall call threadpool_create_work_item with the threadpool, tp_worker_on_threadpool_work and worker_thread. ]*/
/*Tests_SRS_TP_WORKER_THREAD_01_001: [ tp_worker_thread_open shall save the THANDLE(THREADPOOL_WORK_ITEM) for later use by using THANDLE_INITIALIZE_MOVE(THREADPOOL_WORK_ITEM). ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_018: [ tp_worker_thread_open shall call sm_open_end with true. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_019: [ tp_worker_thread_open shall succeed and return 0. ]*/
TEST_FUNCTION(tp_worker_thread_open_succeeds)
{
    // arrange
    TP_WORKER_THREAD_HANDLE worker_thread = test_create();

    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    expect_open(&captured_work_func, &captured_work_context);

    // act
    int result = tp_worker_thread_open(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

/*Tests_SRS_TP_WORKER_THREAD_42_014: [ tp_worker_thread_open shall call sm_open_begin. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_020: [ If there are any errors then tp_worker_thread_open shall fail and return a non-zero value. ]*/
TEST_FUNCTION(tp_worker_thread_open_twice_fails)
{
    // arrange
    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    TP_WORKER_THREAD_HANDLE worker_thread = test_create_and_open(&captured_work_func, &captured_work_context);

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));

    // act
    int result = tp_worker_thread_open(worker_thread);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

/*Tests_SRS_TP_WORKER_THREAD_45_004: [ If threadpool_create fails then tp_worker_thread_open shall call sm_open_end with false. ]*/
TEST_FUNCTION(tp_worker_thread_open_fails_if_threadpool_create_fails)
{
    // arrange
    TP_WORKER_THREAD_HANDLE worker_thread = test_create();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_create(test_execution_engine))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, false));

    // act
    int result = tp_worker_thread_open(worker_thread);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

/*Tests_SRS_TP_WORKER_THREAD_42_043: [ If threadpool_create_work_item fails then tp_worker_thread_open shall call sm_open_end with false. ]*/
TEST_FUNCTION(tp_worker_thread_open_fails_if_threadpool_create_work_item_fails)
{
    // arrange
    TP_WORKER_THREAD_HANDLE worker_thread = test_create();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_create(test_execution_engine));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_create_work_item(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(THREADPOOL)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, false));

    // act
    int result = tp_worker_thread_open(worker_thread);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

/*Tests_SRS_TP_WORKER_THREAD_42_020: [ If there are any errors then tp_worker_thread_open shall fail and return a non-zero value. ]*/
TEST_FUNCTION(tp_worker_thread_open_fails_if_underlying_functions_fail)
{
    // arrange
    TP_WORKER_THREAD_HANDLE worker_thread = test_create();

    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    expect_open(&captured_work_func, &captured_work_context);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            int result = tp_worker_thread_open(worker_thread);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result);
        }
    }

    // assert
    umock_c_negative_tests_reset();
    int result = tp_worker_thread_open(worker_thread);
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

//
// tp_worker_thread_close
//

/*Tests_SRS_TP_WORKER_THREAD_42_021: [ If worker_thread is NULL then tp_worker_thread_close shall return. ]*/
TEST_FUNCTION(tp_worker_thread_close_with_NULL_worker_thread_returns)
{
    // arrange

    // act
    tp_worker_thread_close(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_TP_WORKER_THREAD_42_022: [ tp_worker_thread_close shall call sm_close_begin. ]*/
/*Tests_SRS_TP_WORKER_THREAD_01_002: [ tp_worker_thread_close shall call THANDLE_ASSIGN(THREADPOOL_WORK_ITEM) with NULL. ]*/
/*Tests_SRS_TP_WORKER_THREAD_45_005: [ tp_worker_thread_close shall call THANDLE_ASSIGN(THREADPOOL) with NULL. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_024: [ tp_worker_thread_close shall call sm_close_end. ]*/
TEST_FUNCTION(tp_worker_thread_close_succeeds)
{
    // arrange
    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    TP_WORKER_THREAD_HANDLE worker_thread = test_create_and_open(&captured_work_func, &captured_work_context);

    expect_close();

    // act
    tp_worker_thread_close(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

/*Tests_SRS_TP_WORKER_THREAD_42_022: [ tp_worker_thread_close shall call sm_close_begin. ]*/
TEST_FUNCTION(tp_worker_thread_close_without_open_returns)
{
    // arrange
    TP_WORKER_THREAD_HANDLE worker_thread = test_create();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));

    // act
    tp_worker_thread_close(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

/*Tests_SRS_TP_WORKER_THREAD_42_022: [ tp_worker_thread_close shall call sm_close_begin. ]*/
TEST_FUNCTION(tp_worker_thread_close_twice_returns)
{
    // arrange
    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    TP_WORKER_THREAD_HANDLE worker_thread = test_create_and_open(&captured_work_func, &captured_work_context);
    test_close(worker_thread);

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));

    // act
    tp_worker_thread_close(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

//
// tp_worker_thread_schedule_process
//

/*Tests_SRS_TP_WORKER_THREAD_42_025: [ If worker_thread is NULL then tp_worker_thread_schedule_process shall fail and return TP_WORKER_THREAD_SCHEDULE_PROCESS_INVALID_ARGS. ]*/
TEST_FUNCTION(tp_worker_thread_schedule_process_with_NULL_worker_thread_fails)
{
    // arrange

    // act
    TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT result = tp_worker_thread_schedule_process(NULL);

    // assert
    ASSERT_ARE_EQUAL(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_TP_WORKER_THREAD_42_026: [ tp_worker_thread_schedule_process shall call sm_exec_begin. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_027: [ If sm_exec_begin fails then tp_worker_thread_schedule_process shall return TP_WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE. ]*/
TEST_FUNCTION(tp_worker_thread_schedule_process_fails_if_not_open)
{
    // arrange
    TP_WORKER_THREAD_HANDLE worker_thread = test_create();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));

    // act
    TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT result = tp_worker_thread_schedule_process(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

/*Tests_SRS_TP_WORKER_THREAD_42_026: [ tp_worker_thread_schedule_process shall call sm_exec_begin. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_027: [ If sm_exec_begin fails then tp_worker_thread_schedule_process shall return TP_WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE. ]*/
TEST_FUNCTION(tp_worker_thread_schedule_process_fails_after_close)
{
    // arrange
    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    TP_WORKER_THREAD_HANDLE worker_thread = test_create_and_open(&captured_work_func, &captured_work_context);
    test_close(worker_thread);

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));

    // act
    TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT result = tp_worker_thread_schedule_process(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

/*Tests_SRS_TP_WORKER_THREAD_42_026: [ tp_worker_thread_schedule_process shall call sm_exec_begin. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_029: [ If the state is IDLE then: ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_030: [ tp_worker_thread_schedule_process shall set the state to EXECUTING. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_031: [ tp_worker_thread_schedule_process shall call threadpool_schedule_work_item on the work item created in the module open. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_035: [ tp_worker_thread_schedule_process shall return TP_WORKER_THREAD_SCHEDULE_PROCESS_OK. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_036: [ tp_worker_thread_schedule_process shall call sm_exec_end. ]*/
TEST_FUNCTION(tp_worker_thread_schedule_process_from_idle_succeeds)
{
    // arrange
    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    TP_WORKER_THREAD_HANDLE worker_thread = test_create_and_open(&captured_work_func, &captured_work_context);

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_schedule_work_item(IGNORED_ARG, g.last_created_work_item));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT result = tp_worker_thread_schedule_process(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

/*Tests_SRS_TP_WORKER_THREAD_42_026: [ tp_worker_thread_schedule_process shall call sm_exec_begin. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_029: [ If the state is IDLE then: ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_030: [ tp_worker_thread_schedule_process shall set the state to EXECUTING. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_031: [ tp_worker_thread_schedule_process shall call threadpool_schedule_work_item on the work item created in the module open. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_035: [ tp_worker_thread_schedule_process shall return TP_WORKER_THREAD_SCHEDULE_PROCESS_OK. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_036: [ tp_worker_thread_schedule_process shall call sm_exec_end. ]*/
TEST_FUNCTION(tp_worker_thread_schedule_process_from_idle_succeeds_after_thread_has_run)
{
    // arrange
    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    TP_WORKER_THREAD_HANDLE worker_thread = test_create_and_open(&captured_work_func, &captured_work_context);
    test_schedule_work_first_time(worker_thread);

    // Threadpool thread runs
    STRICT_EXPECTED_CALL(test_worker_function(&test_worker_context));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    captured_work_func(captured_work_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_schedule_work_item(IGNORED_ARG, g.last_created_work_item));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT result = tp_worker_thread_schedule_process(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

/*Tests_SRS_TP_WORKER_THREAD_42_026: [ tp_worker_thread_schedule_process shall call sm_exec_begin. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_028: [ If the state is EXECUTING then tp_worker_thread_schedule_process shall set the state to SCHEDULE_REQUESTED and return TP_WORKER_THREAD_SCHEDULE_PROCESS_OK. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_036: [ tp_worker_thread_schedule_process shall call sm_exec_end. ]*/
TEST_FUNCTION(tp_worker_thread_schedule_process_from_executing_succeeds)
{
    // arrange
    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    TP_WORKER_THREAD_HANDLE worker_thread = test_create_and_open(&captured_work_func, &captured_work_context);
    test_schedule_work_first_time(worker_thread);

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT result = tp_worker_thread_schedule_process(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

/*Tests_SRS_TP_WORKER_THREAD_42_026: [ tp_worker_thread_schedule_process shall call sm_exec_begin. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_028: [ If the state is EXECUTING then tp_worker_thread_schedule_process shall set the state to SCHEDULE_REQUESTED and return TP_WORKER_THREAD_SCHEDULE_PROCESS_OK. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_036: [ tp_worker_thread_schedule_process shall call sm_exec_end. ]*/
TEST_FUNCTION(tp_worker_thread_schedule_process_from_schedule_requested_succeeds)
{
    // arrange
    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    TP_WORKER_THREAD_HANDLE worker_thread = test_create_and_open(&captured_work_func, &captured_work_context);
    test_schedule_work_first_time(worker_thread);
    test_schedule_work_already_running(worker_thread);

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT result = tp_worker_thread_schedule_process(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

//
// tp_worker_on_threadpool_work
//

/*Tests_SRS_TP_WORKER_THREAD_42_037: [ If context is NULL then tp_worker_on_threadpool_work shall terminate the process. ]*/
TEST_FUNCTION(tp_worker_on_threadpool_work_with_NULL_context_terminates)
{
    // arrange
    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    TP_WORKER_THREAD_HANDLE worker_thread = test_create_and_open(&captured_work_func, &captured_work_context);
    test_schedule_work_first_time(worker_thread);

    STRICT_EXPECTED_CALL(ps_util_terminate_process());

    // act
    captured_work_func(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

/*Tests_SRS_TP_WORKER_THREAD_42_038: [ tp_worker_on_threadpool_work shall call worker_func with worker_func_context. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_039: [ If the state is EXECUTING then tp_worker_on_threadpool_work shall change the state to IDLE and return. ]*/
TEST_FUNCTION(tp_worker_on_threadpool_work_calls_worker_func)
{
    // arrange
    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    TP_WORKER_THREAD_HANDLE worker_thread = test_create_and_open(&captured_work_func, &captured_work_context);
    test_schedule_work_first_time(worker_thread);

    STRICT_EXPECTED_CALL(test_worker_function(&test_worker_context));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    captured_work_func(captured_work_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

/*Tests_SRS_TP_WORKER_THREAD_42_038: [ tp_worker_on_threadpool_work shall call worker_func with worker_func_context. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_040: [ If the state is SCHEDULE_REQUESTED then tp_worker_on_threadpool_work shall change the state to EXECUTING and repeat. ]*/
TEST_FUNCTION(tp_worker_on_threadpool_work_calls_worker_func_again_if_scheduled_while_running)
{
    // arrange
    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    TP_WORKER_THREAD_HANDLE worker_thread = test_create_and_open(&captured_work_func, &captured_work_context);
    test_schedule_work_first_time(worker_thread);

    test_worker_context.worker_thread = worker_thread;
    test_worker_context.schedule_again_count = 1;

    // First iteration
    STRICT_EXPECTED_CALL(test_worker_function(&test_worker_context));
    // Hook schedules again
    expect_schedule_already_running();
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // Second iteration
    STRICT_EXPECTED_CALL(test_worker_function(&test_worker_context));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    captured_work_func(captured_work_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

/*Tests_SRS_TP_WORKER_THREAD_42_038: [ tp_worker_on_threadpool_work shall call worker_func with worker_func_context. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_040: [ If the state is SCHEDULE_REQUESTED then tp_worker_on_threadpool_work shall change the state to EXECUTING and repeat. ]*/
TEST_FUNCTION(tp_worker_on_threadpool_work_calls_worker_func_again_each_time_scheduled_while_running)
{
    // arrange
    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    TP_WORKER_THREAD_HANDLE worker_thread = test_create_and_open(&captured_work_func, &captured_work_context);
    test_schedule_work_first_time(worker_thread);

    test_worker_context.worker_thread = worker_thread;
    test_worker_context.schedule_again_count = 3;

    // First iteration
    STRICT_EXPECTED_CALL(test_worker_function(&test_worker_context));
    // Hook schedules again
    expect_schedule_already_running();
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // Second iteration
    STRICT_EXPECTED_CALL(test_worker_function(&test_worker_context));
    // Hook schedules again
    expect_schedule_already_running();
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // Third iteration
    STRICT_EXPECTED_CALL(test_worker_function(&test_worker_context));
    // Hook schedules again
    expect_schedule_already_running();
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // Fourth iteration
    STRICT_EXPECTED_CALL(test_worker_function(&test_worker_context));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    captured_work_func(captured_work_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

/*Tests_SRS_TP_WORKER_THREAD_42_038: [ tp_worker_on_threadpool_work shall call worker_func with worker_func_context. ]*/
/*Tests_SRS_TP_WORKER_THREAD_42_040: [ If the state is SCHEDULE_REQUESTED then tp_worker_on_threadpool_work shall change the state to EXECUTING and repeat. ]*/
TEST_FUNCTION(tp_worker_on_threadpool_work_calls_worker_func_again_if_scheduled_while_running_but_only_once_if_more_scheduled)
{
    // arrange
    THREADPOOL_WORK_FUNCTION captured_work_func;
    void* captured_work_context;
    TP_WORKER_THREAD_HANDLE worker_thread = test_create_and_open(&captured_work_func, &captured_work_context);
    test_schedule_work_first_time(worker_thread);

    test_worker_context.worker_thread = worker_thread;
    test_worker_context.schedule_again_count = 1;
    test_worker_context.schedule_again_count_per_call = 3;

    // First iteration
    STRICT_EXPECTED_CALL(test_worker_function(&test_worker_context));
    // Hook schedules again
    expect_schedule_already_running();
    expect_schedule_already_running();
    expect_schedule_already_running();
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // Second iteration
    STRICT_EXPECTED_CALL(test_worker_function(&test_worker_context));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    captured_work_func(captured_work_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    tp_worker_thread_destroy(worker_thread);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
