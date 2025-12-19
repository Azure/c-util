// Copyright (c) Microsoft. All rights reserved.



#include "watchdog_ut_pch.h"

static void* test_callback_context = (void*)0x100A;

static THANDLE(THREADPOOL) test_threadpool = (void*)0x100B;
static THANDLE(THREADPOOL_TIMER) test_timer_instance = (void*)0x100C;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

MOCK_FUNCTION_WITH_CODE(, void, test_callback, void*, context, const char*, message)
MOCK_FUNCTION_END();

typedef void(*TIMER_STOP_HOOK)(void* context);
static bool timer_stop_calls_callback = false;
static TIMER_STOP_HOOK timer_stop_hook = NULL;
static void* timer_stop_hook_context = NULL;

static void my_THANDLE_THREADPOOL_TIMER_ASSIGN(THANDLE(THREADPOOL_TIMER)* left, THANDLE(THREADPOOL_TIMER) right)
{
    (void)left;
    (void)right;

    if (timer_stop_calls_callback)
    {
        timer_stop_hook(timer_stop_hook_context);
    }
}

static void expect_start(uint32_t timeout, THANDLE(real_RC_STRING) message, THREADPOOL_WORK_FUNCTION* callback, void** context)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create("watchdog")).SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, message));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));

    STRICT_EXPECTED_CALL(threadpool_timer_start(test_threadpool, timeout, 0, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_work_function(callback)
        .CaptureArgumentValue_work_function_context(context)
        .SetReturn(test_timer_instance);
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(THREADPOOL_TIMER)(IGNORED_ARG, IGNORED_ARG));
}

static WATCHDOG_HANDLE do_start(THREADPOOL_WORK_FUNCTION* callback, void** context)
{
    THANDLE(real_RC_STRING) temp_message = real_rc_string_create("message");
    ASSERT_IS_NOT_NULL(temp_message);

    expect_start(42, temp_message, callback, context);
    WATCHDOG_HANDLE result = watchdog_start(test_threadpool, 42, temp_message, test_callback, test_callback_context);
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    THANDLE_ASSIGN(real_RC_STRING)(&temp_message, NULL);

    return result;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_RC_STRING_GLOBAL_MOCK_HOOKS();
    REGISTER_SM_GLOBAL_MOCK_HOOK();


    REGISTER_GLOBAL_MOCK_RETURNS(threadpool_timer_start, test_timer_instance, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(threadpool_timer_restart, 0, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(THREADPOOL_TIMER), my_THANDLE_THREADPOOL_TIMER_ASSIGN);

    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(THREADPOOL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(THREADPOOL_TIMER), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREADPOOL_WORK_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(RC_STRING), void*);
    REGISTER_UMOCK_ALIAS_TYPE(SM_HANDLE, void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    timer_stop_calls_callback = false;

    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

//
// watchdog_start
//

/*Tests_SRS_WATCHDOG_42_029: [ If threadpool is NULL then watchdog_start shall fail and return NULL. ]*/
TEST_FUNCTION(watchdog_start_NULL_threadpool_fails)
{
    // arrange
    THANDLE(real_RC_STRING) temp_message = real_rc_string_create("message");
    ASSERT_IS_NOT_NULL(temp_message);

    // act
    WATCHDOG_HANDLE result = watchdog_start(NULL, 42, temp_message, test_callback, test_callback_context);

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);

    // cleanup
    THANDLE_ASSIGN(real_RC_STRING)(&temp_message, NULL);
}

/*Tests_SRS_WATCHDOG_42_030: [ If callback is NULL then watchdog_start shall fail and return NULL. ]*/
TEST_FUNCTION(watchdog_start_NULL_callback_fails)
{
    // arrange
    THANDLE(real_RC_STRING) temp_message = real_rc_string_create("message");
    ASSERT_IS_NOT_NULL(temp_message);

    // act
    WATCHDOG_HANDLE result = watchdog_start(test_threadpool, 42, temp_message, NULL, test_callback_context);

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);

    // cleanup
    THANDLE_ASSIGN(real_RC_STRING)(&temp_message, NULL);
}

/*Tests_SRS_WATCHDOG_42_016: [ watchdog_start shall allocate memory for the WATCHDOG_HANDLE. ]*/
/*Tests_SRS_WATCHDOG_45_006: [ watchdog_start shall call sm_create to create an SM_HANDLE handle state. ]*/
/*Tests_SRS_WATCHDOG_45_007: [ watchdog_start shall call sm_open_begin to move timer to the open state. ]*/
/*Tests_SRS_WATCHDOG_45_008: [ watchdog_start shall call sm_open_end to move timer to the open state. ]*/
/*Tests_SRS_WATCHDOG_42_028: [ watchdog_start shall store the message. ]*/
/*Tests_SRS_WATCHDOG_42_018: [ watchdog_start shall create a timer that expires after timeout_ms by calling threadpool_timer_start with watchdog_expired_callback as the callback. ]*/
/*Tests_SRS_WATCHDOG_42_020: [ watchdog_start shall succeed and return the allocated handle. ]*/
TEST_FUNCTION(watchdog_start_succeeds)
{
    // arrange
    THANDLE(real_RC_STRING) temp_message = real_rc_string_create("message");
    ASSERT_IS_NOT_NULL(temp_message);

    THREADPOOL_WORK_FUNCTION callback;
    void* context;
    expect_start(42, temp_message, &callback, &context);

    // act
    WATCHDOG_HANDLE result = watchdog_start(test_threadpool, 42, temp_message, test_callback, test_callback_context);

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    // cleanup
    watchdog_stop(result);
    THANDLE_ASSIGN(real_RC_STRING)(&temp_message, NULL);
}

/*Tests_SRS_WATCHDOG_42_019: [ If there are any errors then watchdog_start shall fail and return NULL. ]*/
TEST_FUNCTION(watchdog_start_fails_when_underlying_functions_fail)
{
    // arrange
    THANDLE(real_RC_STRING) temp_message = real_rc_string_create("message");
    ASSERT_IS_NOT_NULL(temp_message);

    umock_c_reset_all_calls();

    THREADPOOL_WORK_FUNCTION callback;
    void* context;
    expect_start(42, temp_message, &callback, &context);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            WATCHDOG_HANDLE result = watchdog_start(test_threadpool, 42, temp_message, test_callback, test_callback_context);

            // assert
            ASSERT_IS_NULL(result, "On failed call %zu", i);
        }
    }

    // cleanup
    THANDLE_ASSIGN(real_RC_STRING)(&temp_message, NULL);
}

//
// watchdog_expired_callback
//

/*Tests_SRS_WATCHDOG_42_027: [ If context is NULL then watchdog_expired_callback shall terminate the process. ]*/
TEST_FUNCTION(watchdog_expired_callback_with_NULL_context_terminates_process)
{
    // arrange
    THREADPOOL_WORK_FUNCTION callback;
    void* context;
    WATCHDOG_HANDLE result = do_start(&callback, &context);

    STRICT_EXPECTED_CALL(ps_util_terminate_process());

    // act
    callback(NULL);

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    watchdog_stop(result);
}

/*Tests_SRS_WATCHDOG_45_009: [ watchdog_expired_callback shall call sm_exec_begin. ]*/
/*Tests_SRS_WATCHDOG_45_010: [ if sm_exec_begin returns SM_EXEC_GRANTED, ]*/
/*Tests_SRS_WATCHDOG_42_021: [ watchdog_expired_callback shall call callback with the context and message from watchdog_start. ]*/
/*Tests_SRS_WATCHDOG_45_002: [ watchdog_expired_callback shall sm_exec_end ]*/
TEST_FUNCTION(watchdog_expired_callback_works)
{
    // arrange
    THREADPOOL_WORK_FUNCTION callback;
    void* context;
    WATCHDOG_HANDLE result = do_start(&callback, &context);

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_callback(test_callback_context, "message"));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    callback(context);

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    watchdog_stop(result);
}

//
// watchdog_reset
//

/*Tests_SRS_WATCHDOG_42_031: [ If watchdog is NULL then watchdog_reset shall return. ]*/
TEST_FUNCTION(watchdog_reset_with_NULL_watchdog_returns)
{
    // arrange

    // act
    watchdog_reset(NULL);

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_WATCHDOG_45_011: [ watchdog_reset shall call sm_close_begin. ]*/
/*Tests_SRS_WATCHDOG_45_018: [ If sm_close_begin returns SM_EXEC_GRANTED, ]*/
/*- Tests_SRS_WATCHDOG_42_033: [ watchdog_reset shall cancel the current timer by calling threadpool_timer_cancel. ]*/
/*- Tests_SRS_WATCHDOG_45_012: [ watchdog_reset shall call sm_close_end. ]*/
/*Tests_SRS_WATCHDOG_45_013: [ watchdog_reset shall call sm_open_begin. ]*/
/*Tests_SRS_WATCHDOG_45_014: [ watchdog_reset shall call sm_open_end if sm_open_begin succeeds. ]*/
/*Tests_SRS_WATCHDOG_42_035: [ watchdog_reset shall restart the timer by calling threadpool_timer_restart with the original timeout_ms from the call to start. ]*/
TEST_FUNCTION(watchdog_reset_cancels_and_restarts_the_timer)
{
    // arrange
    THREADPOOL_WORK_FUNCTION callback;
    void* context;
    WATCHDOG_HANDLE result = do_start(&callback, &context);

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_timer_cancel(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));
    STRICT_EXPECTED_CALL(threadpool_timer_restart(IGNORED_ARG, 42, 0));

    // act
    watchdog_reset(result);

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    watchdog_stop(result);
}

//
// watchdog_stop
//

/*Tests_SRS_WATCHDOG_42_022: [ If watchdog is NULL then watchdog_stop shall return. ]*/
TEST_FUNCTION(watchdog_stop_with_NULL_watchdog_returns)
{
    // arrange

    // act
    watchdog_stop(NULL);

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_WATCHDOG_45_015: [ watchdog_stop shall call sm_close_begin. ]*/
/*Tests_SRS_WATCHDOG_45_016: [ watchdog_stop shall call sm_close_end if sm_close_begin succeeds. ]*/
/*Tests_SRS_WATCHDOG_42_024: [ watchdog_stop shall stop and cleanup the timer by decrementing timer reference count. ]*/
/*Tests_SRS_WATCHDOG_45_017: [ watchdog_stop shall call sm_destroy. ]*/
/*Tests_SRS_WATCHDOG_42_025: [ watchdog_stop shall free the watchdog. ]*/
TEST_FUNCTION(watchdog_stop_stops_the_timer)
{
    // arrange
    THREADPOOL_WORK_FUNCTION callback;
    void* context;
    WATCHDOG_HANDLE result = do_start(&callback, &context);

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(THREADPOOL_TIMER)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(sm_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_STRING)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    watchdog_stop(result);

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_WATCHDOG_45_015: [ watchdog_stop shall call sm_close_begin. ]*/
/*Tests_SRS_WATCHDOG_45_016: [ watchdog_stop shall call sm_close_end if sm_close_begin succeeds. ]*/
/*Tests_SRS_WATCHDOG_42_024: [ watchdog_stop shall stop and cleanup the timer by decrementing timer reference count. ]*/
/*Tests_SRS_WATCHDOG_45_017: [ watchdog_stop shall call sm_destroy. ]*/
/*Tests_SRS_WATCHDOG_42_025: [ watchdog_stop shall free the watchdog. ]*/
TEST_FUNCTION(watchdog_stop_stops_the_timer_after_it_fired)
{
    // arrange
    THREADPOOL_WORK_FUNCTION callback;
    void* context;
    WATCHDOG_HANDLE result = do_start(&callback, &context);

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_callback(test_callback_context, "message"));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
    callback(context);

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(THREADPOOL_TIMER)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(sm_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_STRING)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    watchdog_stop(result);

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

typedef struct DO_FIRE_TIMER_CONTEXT_TAG
{
    THREADPOOL_WORK_FUNCTION callback;
    void* context;
} DO_FIRE_TIMER_CONTEXT;

static void do_fire_timer(void* context)
{
    DO_FIRE_TIMER_CONTEXT* fire_timer_context = context;
    fire_timer_context->callback(fire_timer_context->context);
}

/*Tests_SRS_WATCHDOG_42_021: [ watchdog_expired_callback shall call callback with the context and message from watchdog_start. ]*/
TEST_FUNCTION(watchdog_stop_prevents_callback_from_calling_if_timer_fires_on_stop)
{
    // arrange
    DO_FIRE_TIMER_CONTEXT fire_timer_context;
    WATCHDOG_HANDLE result = do_start(&fire_timer_context.callback, &fire_timer_context.context);

    timer_stop_hook = do_fire_timer;
    timer_stop_hook_context = &fire_timer_context;
    timer_stop_calls_callback = true;

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(THREADPOOL_TIMER)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG)); //callback checking state.
    STRICT_EXPECTED_CALL(sm_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_STRING)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    watchdog_stop(result);

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
