// Copyright (c) Microsoft. All rights reserved.


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/threadpool.h"
#include "c_pal/ps_util.h"
#include "c_util/rc_string.h"
#include "c_pal/thandle.h"
#undef ENABLE_MOCKS

// Must include umock_c_prod so mocks are not expanded in real_rc_string
#include "umock_c/umock_c_prod.h"

#include "real_gballoc_hl.h"
#include "real_interlocked.h"
#include "real_interlocked_hl.h"
#include "real_rc_string.h"

#include "c_util/watchdog.h"

static void* test_callback_context = (void*)0x100A;

static THANDLE(THREADPOOL) test_threadpool = (void*)0x100B;
static TIMER_INSTANCE_HANDLE test_timer_instance = (TIMER_INSTANCE_HANDLE)0x100C;

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
static void hook_threadpool_timer_destroy(TIMER_INSTANCE_HANDLE timer)
{
    (void)timer;

    if (timer_stop_calls_callback)
    {
        timer_stop_hook(timer_stop_hook_context);
    }
}

static void expect_start(uint32_t timeout, THANDLE(real_RC_STRING) message, THREADPOOL_WORK_FUNCTION* callback, void** context)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, message));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(threadpool_timer_start(test_threadpool, timeout, 0, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_work_function(callback)
        .CaptureArgumentValue_work_function_context(context)
        .CopyOutArgumentBuffer_timer_handle(&test_timer_instance, sizeof(test_timer_instance));
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

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_RC_STRING_GLOBAL_MOCK_HOOKS();


    REGISTER_GLOBAL_MOCK_RETURNS(threadpool_timer_start, 0, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_RETURNS(threadpool_timer_restart, 0, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_HOOK(threadpool_timer_destroy, hook_threadpool_timer_destroy);

    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(THREADPOOL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(TIMER_INSTANCE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREADPOOL_WORK_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(RC_STRING), void*);
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
/*Tests_SRS_WATCHDOG_42_028: [ watchdog_start shall store the message. ]*/
/*Tests_SRS_WATCHDOG_42_017: [ watchdog_start shall set the state of the watchdog to RUNNING. ]*/
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

/*Tests_SRS_WATCHDOG_45_005: [ If the state of the watchdog is RUNNING then ]*/
    /*Tests_SRS_WATCHDOG_45_001: [ watchdog_expired_callback shall set the state to EXPIRING ]*/
    /*Tests_SRS_WATCHDOG_42_021: [ watchdog_expired_callback shall call callback with the context and message from watchdog_start. ]*/
    /*Tests_SRS_WATCHDOG_45_002: [ watchdog_expired_callback shall return the state to RUNNING. ]*/
TEST_FUNCTION(watchdog_expired_callback_works)
{
    // arrange
    THREADPOOL_WORK_FUNCTION callback;
    void* context;
    WATCHDOG_HANDLE result = do_start(&callback, &context);

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_callback(test_callback_context, "message"));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, IGNORED_ARG));

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

/*Tests_SRS_WATCHDOG_45_003: [ watchdog_reset shall wait until state is not EXPIRING. ]*/
/*Tests_SRS_WATCHDOG_42_032: [ watchdog_reset shall set the state of the watchdog to STOP. ]*/
/*Tests_SRS_WATCHDOG_42_033: [ watchdog_reset shall cancel the current timer by calling threadpool_timer_cancel. ]*/
/*Tests_SRS_WATCHDOG_42_034: [ watchdog_reset shall set the state of the watchdog to RUNNING. ]*/
/*Tests_SRS_WATCHDOG_42_035: [ watchdog_reset shall restart the timer by calling threadpool_timer_restart with the original timeout_ms from the call to start. ]*/
TEST_FUNCTION(watchdog_reset_cancels_and_restarts_the_timer)
{
    // arrange
    THREADPOOL_WORK_FUNCTION callback;
    void* context;
    WATCHDOG_HANDLE result = do_start(&callback, &context);

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForNotValue(IGNORED_ARG, IGNORED_ARG, UINT32_MAX));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_timer_cancel(test_timer_instance));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_timer_restart(test_timer_instance, 42, 0));

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

/*Tests_SRS_WATCHDOG_45_004: [ watchdog_stop shall wait until state is not EXPIRING. ]*/
/*Tests_SRS_WATCHDOG_42_023: [ watchdog_stop shall set the state of the watchdog to STOP. ]*/
/*Tests_SRS_WATCHDOG_42_024: [ watchdog_stop shall stop and cleanup the timer by calling threadpool_timer_destroy. ]*/
/*Tests_SRS_WATCHDOG_42_025: [ watchdog_stop shall free the watchdog. ]*/
TEST_FUNCTION(watchdog_stop_stops_the_timer)
{
    // arrange
    THREADPOOL_WORK_FUNCTION callback;
    void* context;
    WATCHDOG_HANDLE result = do_start(&callback, &context);

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForNotValue(IGNORED_ARG, IGNORED_ARG, UINT32_MAX));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_timer_destroy(test_timer_instance));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_STRING)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    watchdog_stop(result);

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_WATCHDOG_45_004: [ watchdog_stop shall wait until state is not EXPIRING. ]*/
/*Tests_SRS_WATCHDOG_42_023: [ watchdog_stop shall set the state of the watchdog to STOP. ]*/
/*Tests_SRS_WATCHDOG_42_024: [ watchdog_stop shall stop and cleanup the timer by calling threadpool_timer_destroy. ]*/
/*Tests_SRS_WATCHDOG_42_025: [ watchdog_stop shall free the watchdog. ]*/
TEST_FUNCTION(watchdog_stop_stops_the_timer_after_it_fired)
{
    // arrange
    THREADPOOL_WORK_FUNCTION callback;
    void* context;
    WATCHDOG_HANDLE result = do_start(&callback, &context);

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_callback(test_callback_context, "message"));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, IGNORED_ARG));
    callback(context);

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForNotValue(IGNORED_ARG, IGNORED_ARG, UINT32_MAX));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_timer_destroy(test_timer_instance));
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

/*Tests_SRS_WATCHDOG_45_004: [ watchdog_stop shall wait until state is not EXPIRING. ]*/
/*Tests_SRS_WATCHDOG_42_023: [ watchdog_stop shall set the state of the watchdog to STOP. ]*/
/*Tests_SRS_WATCHDOG_45_005: [ If the state of the watchdog is RUNNING then ]*/
    /*Tests_SRS_WATCHDOG_45_001: [ watchdog_expired_callback shall set the state to EXPIRING ]*/
    /*Tests_SRS_WATCHDOG_42_021: [ watchdog_expired_callback shall call callback with the context and message from watchdog_start. ]*/
    /*Tests_SRS_WATCHDOG_45_002: [ watchdog_expired_callback shall return the state to RUNNING. ]*/
TEST_FUNCTION(watchdog_stop_prevents_callback_from_calling_if_timer_fires_on_stop)
{
    // arrange
    DO_FIRE_TIMER_CONTEXT fire_timer_context;
    WATCHDOG_HANDLE result = do_start(&fire_timer_context.callback, &fire_timer_context.context);

    timer_stop_hook = do_fire_timer;
    timer_stop_hook_context = &fire_timer_context;
    timer_stop_calls_callback = true;

    STRICT_EXPECTED_CALL(InterlockedHL_WaitForNotValue(IGNORED_ARG, IGNORED_ARG, UINT32_MAX));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_timer_destroy(test_timer_instance));

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));  // callback checking state
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_STRING)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    watchdog_stop(result);

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
