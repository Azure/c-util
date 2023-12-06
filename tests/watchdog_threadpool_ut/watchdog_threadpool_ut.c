// Copyright (c) Microsoft. All rights reserved.

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle.h"
#include "c_pal/execution_engine.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_interlocked.h"

#include "c_util/bs_watchdog_threadpool.h"

static EXECUTION_ENGINE_HANDLE test_execution_engine = (EXECUTION_ENGINE_HANDLE)0x1001;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static void expect_init_verbose(int threapool_open_result)
{
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(execution_engine_create(NULL));
    STRICT_EXPECTED_CALL(threadpool_create(test_execution_engine));
    STRICT_EXPECTED_CALL(threadpool_open(IGNORED_ARG))
        .SetReturn(threapool_open_result);

    if (threapool_open_result != 0)
    {
        STRICT_EXPECTED_CALL(THANDLE_ASSIGN(THREADPOOL)(IGNORED_ARG, NULL));
        STRICT_EXPECTED_CALL(execution_engine_dec_ref(test_execution_engine));
    }
    else
    {
        STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
}

static void expect_init(void)
{
    expect_init_verbose(0);
}

static void expect_init_fail(void)
{
    expect_init_verbose(MU_FAILURE);
}

static void expect_deinit(void)
{
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_close(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(THREADPOOL)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(execution_engine_dec_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
}

static void do_init(void)
{
    expect_init();
    ASSERT_ARE_EQUAL(int, 0, bs_watchdog_threadpool_init());
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

typedef struct THREADPOOL_TAG
{
    uint8_t dummy;
} THREADPOOL;

typedef THREADPOOL REAL_THREADPOOL;
THANDLE_TYPE_DECLARE(REAL_THREADPOOL);
THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(REAL_THREADPOOL, real_gballoc_hl_malloc, real_gballoc_hl_malloc_flex, real_gballoc_hl_free);

static struct G_TAG /*g comes from "global*/
{
    THANDLE(REAL_THREADPOOL) test_threadpool;
} g;

static void dispose_REAL_THREADPOOL_do_nothing(REAL_THREADPOOL* nothing)
{
    (void)nothing;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();

    THANDLE(REAL_THREADPOOL) temp = THANDLE_MALLOC(REAL_THREADPOOL)(dispose_REAL_THREADPOOL_do_nothing);
    ASSERT_IS_NOT_NULL(temp);
    THANDLE_MOVE(REAL_THREADPOOL)(&g.test_threadpool, &temp);
    THANDLE_GET_T(REAL_THREADPOOL)(g.test_threadpool)->dummy = 11;


    REGISTER_GLOBAL_MOCK_RETURNS(execution_engine_create, test_execution_engine, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(threadpool_create, g.test_threadpool, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(threadpool_open, 0, MU_FAILURE);

    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(REAL_THREADPOOL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(THREADPOOL), void*);
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE_MOVE(THREADPOOL), THANDLE_INITIALIZE_MOVE(REAL_THREADPOOL));
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(THREADPOOL), THANDLE_INITIALIZE(REAL_THREADPOOL));
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    THANDLE_ASSIGN(REAL_THREADPOOL)(&g.test_threadpool, NULL);

    umock_c_deinit();
    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    bs_watchdog_threadpool_deinit();

    umock_c_negative_tests_deinit();
}

//
// bs_watchdog_threadpool_init
//

/*Tests_SRS_BS_WATCHDOG_THREADPOOL_42_002: [ bs_watchdog_threadpool_init shall create an execution engine by calling execution_engine_create. ]*/
/*Tests_SRS_BS_WATCHDOG_THREADPOOL_42_003: [ bs_watchdog_threadpool_init shall create a threadpool by calling threadpool_create. ]*/
/*Tests_SRS_BS_WATCHDOG_THREADPOOL_42_004: [ bs_watchdog_threadpool_init shall open the threadpool by calling threadpool_open. ]*/
/*Tests_SRS_BS_WATCHDOG_THREADPOOL_42_006: [ bs_watchdog_threadpool_init shall store the threadpool. ]*/
/*Tests_SRS_BS_WATCHDOG_THREADPOOL_42_008: [ bs_watchdog_threadpool_init shall return 0. ]*/
TEST_FUNCTION(bs_watchdog_threadpool_init_succeeds)
{
    // arrange
    expect_init();

    // act
    int result = bs_watchdog_threadpool_init();

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
}

/*Tests_SRS_BS_WATCHDOG_THREADPOOL_42_001: [ If the watchdog threadpool has already been initialized then bs_watchdog_threadpool_init shall fail and return a non-zero value. ]*/
TEST_FUNCTION(bs_watchdog_threadpool_init_twice_fails)
{
    // arrange
    do_init();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    int result = bs_watchdog_threadpool_init();

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/*Tests_SRS_BS_WATCHDOG_THREADPOOL_42_007: [ If there are any other errors then bs_watchdog_threadpool_init shall fail and return a non-zero value. ]*/
TEST_FUNCTION(bs_watchdog_threadpool_init_fails_when_open_thread_pool_fails_async)
{
    // arrange
    expect_init_fail();

    // act
    int result = bs_watchdog_threadpool_init();

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/*Tests_SRS_BS_WATCHDOG_THREADPOOL_42_007: [ If there are any other errors then bs_watchdog_threadpool_init shall fail and return a non-zero value. ]*/
TEST_FUNCTION(bs_watchdog_threadpool_init_fails_when_underlying_functions_fail)
{
    // arrange
    expect_init();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            int result = bs_watchdog_threadpool_init();

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
        }
    }
}

//
// bs_watchdog_threadpool_deinit
//

/*Tests_SRS_BS_WATCHDOG_THREADPOOL_42_009: [ If the watchdog threadpool has not been initialized then bs_watchdog_deinit shall return. ]*/
TEST_FUNCTION(bs_watchdog_threadpool_deinit_without_init_returns)
{
    // arrange
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    bs_watchdog_threadpool_deinit();

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_BS_WATCHDOG_THREADPOOL_42_010: [ bs_watchdog_threadpool_deinit shall close the threadpool by calling threadpool_close. ]*/
/*Tests_SRS_BS_WATCHDOG_THREADPOOL_42_011: [ bs_watchdog_threadpool_deinit shall destroy the threadpool by assign threadpool to NULL. ]*/
/*Tests_SRS_BS_WATCHDOG_THREADPOOL_42_016: [ bs_watchdog_threadpool_deinit shall destroy the execution_engine by calling execution_engine_dec_ref. ]*/
TEST_FUNCTION(bs_watchdog_threadpool_deinit_cleans_up)
{
    // arrange
    do_init();

    expect_deinit();

    // act
    bs_watchdog_threadpool_deinit();

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_BS_WATCHDOG_THREADPOOL_42_013: [ After bs_watchdog_threadpool_deinit returns then bs_watchdog_threadpool_init may be called again. ]*/
TEST_FUNCTION(bs_watchdog_threadpool_init_works_after_deinit)
{
    // arrange
    do_init();
    expect_deinit();
    bs_watchdog_threadpool_deinit();

    expect_init();

    // act
    int result = bs_watchdog_threadpool_init();

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
}

//
// bs_watchdog_threadpool_get
//

/*Tests_SRS_BS_WATCHDOG_THREADPOOL_42_014: [ bs_watchdog_threadpool_get shall return the threadpool created by bs_watchdog_threadpool_init. ]*/
TEST_FUNCTION(bs_watchdog_threadpool_get_returns_threadpool)
{
    // arrange
    do_init();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));

    // act
    THANDLE(THREADPOOL) result = bs_watchdog_threadpool_get();

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint8_t, g.test_threadpool->dummy, result->dummy);

    // Cleanup
    THANDLE_ASSIGN(REAL_THREADPOOL)(&result, NULL);
}

/*Tests_SRS_BS_WATCHDOG_THREADPOOL_42_015: [ If the watchdog threadpool has not been initialized then bs_watchdog_threadpool_get shall return NULL. ]*/
TEST_FUNCTION(bs_watchdog_threadpool_get_without_init_returns_NULL)
{
    // arrange
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));

    // act
    THANDLE(THREADPOOL) result = bs_watchdog_threadpool_get();

    // arrange
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
