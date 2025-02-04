// Copyright (c) Microsoft. All rights reserved.

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "real_gballoc_ll.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/log_critical_and_terminate.h"
#include "c_pal/threadpool.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/thandle.h"

#include "clds/mpsc_lock_free_queue.h"

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "../reals/real_mpsc_lock_free_queue.h"
#include "real_interlocked.h"
#include "real_interlocked_hl.h"

#include "c_util/queue_processor.h"

static THANDLE(THREADPOOL) g_test_threadpool = (void*)0x44433;
static void* g_test_queue_processor_context = (void*)0x5001;

MU_DEFINE_ENUM_STRINGS(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES)

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

MOCK_FUNCTION_WITH_CODE(, void, test_on_queue_processor_error, void*, context)
MOCK_FUNCTION_END();
MOCK_FUNCTION_WITH_CODE(, void, test_process_item_func_1, void*, context)
MOCK_FUNCTION_END();
MOCK_FUNCTION_WITH_CODE(, void, test_process_item_func_2, void*, context)
MOCK_FUNCTION_END();
MOCK_FUNCTION_WITH_CODE(, void, test_process_item_func_3, void*, context)
MOCK_FUNCTION_END();
MOCK_FUNCTION_WITH_CODE(, void, test_process_item_add_new_item, void*, context)
    queue_processor_schedule_item((QUEUE_PROCESSOR_HANDLE)context, test_process_item_func_1, (void*)0x6000);
MOCK_FUNCTION_END();

static QUEUE_PROCESSOR_HANDLE setup_open_queue_processor(void)
{
    QUEUE_PROCESSOR_HANDLE queue_processor = queue_processor_create(g_test_threadpool);
    ASSERT_IS_NOT_NULL(queue_processor);

    ASSERT_ARE_EQUAL(int, 0, queue_processor_open(queue_processor, test_on_queue_processor_error, (void*)0x4243));
    umock_c_reset_all_calls();

    return queue_processor;
}

static void mock_schedule_item(QUEUE_PROCESSOR_HANDLE queue_processor, THREADPOOL_WORK_FUNCTION* work_function, void** work_function_context, QUEUE_PROCESSOR_PROCESS_ITEM process_item_func, void* context)
{
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_schedule_work(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_work_function(work_function)
        .CaptureArgumentValue_work_function_context(work_function_context);
    ASSERT_ARE_EQUAL(int, 0, queue_processor_schedule_item(queue_processor, process_item_func, context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mpsc_lock_free_queue_create, NULL);

    REGISTER_MPSC_LOCK_FREE_QUEUE_GLOBAL_MOCK_HOOKS();

    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(THREADPOOL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREADPOOL_WORK_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(MPSC_LOCK_FREE_QUEUE_HANDLE, void*);

    REGISTER_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
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
    umock_c_negative_tests_deinit();
}

/* queue_processor_create */

// Tests_SRS_QUEUE_PROCESSOR_11_001: [ If threadpool is NULL, queue_processor_create shall fail and return NULL. ]
TEST_FUNCTION(queue_processor_create_with_NULL_threadpool_fails)
{
    // arrange
    QUEUE_PROCESSOR_HANDLE queue_processor;

    // act
    queue_processor = queue_processor_create(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_002: [ Otherwise, queue_processor_create shall create a new queue processor and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_003: [ queue_processor_create shall create a threadpool object by calling threadpool_create. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_045: [ queue_processor_create shall create a multi producer single consumer queue. ]*/
// Tests_SRS_QUEUE_PROCESSOR_01_003: [ queue_processor_create shall initialize its threadpool object by calling THANDLE_INITIALIZE(THREADPOOL). ]
TEST_FUNCTION(queue_processor_create_succeeds)
{
    // arrange
    QUEUE_PROCESSOR_HANDLE queue_processor;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_create());
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));

    // act
    queue_processor = queue_processor_create(g_test_threadpool);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(queue_processor);

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_004: [ If any error occurs, queue_processor_create shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_queue_processor_create_fails)
{
    // arrange
    QUEUE_PROCESSOR_HANDLE queue_processor;
    size_t i;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_create());
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));

    umock_c_negative_tests_snapshot();

    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            queue_processor = queue_processor_create(g_test_threadpool);

            // assert
            ASSERT_IS_NULL(queue_processor, "On failed call %zu", i);
        }
    }
}

/* queue_processor_destroy */

/* Tests_SRS_QUEUE_PROCESSOR_01_005: [ If queue_processor is NULL, queue_processor_destroy shall return. ]*/
TEST_FUNCTION(queue_processor_destroy_with_NULL_queue_processor_returns)
{
    // arrange

    // act
    queue_processor_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_QUEUE_PROCESSOR_01_008: [ queue_processor_destroy shall assign NULL the threadpool initialized in queue_processor_create. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_046: [ queue_processor_destroy shall destroy the multi producer single consumer queue created in queue_processor_create. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_009: [ queue_processor_destroy shall free the memory associated with the worker. ]*/
TEST_FUNCTION(queue_processor_destroy_frees_the_resources)
{
    // arrange
    MPSC_LOCK_FREE_QUEUE_HANDLE test_queue;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_create())
        .CaptureReturn(&test_queue);
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    QUEUE_PROCESSOR_HANDLE queue_processor = queue_processor_create(g_test_threadpool);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_destroy(test_queue));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(THREADPOOL)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    queue_processor_destroy(queue_processor);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_QUEUE_PROCESSOR_01_006: [ Otherwise queue_processor_destroy shall wait until the state is either CLOSED or OPEN. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_007: [ queue_processor_destroy shall perform a close in case the queue_processor is not CLOSED. ]*/
TEST_FUNCTION(queue_processor_destroy_frees_the_resources_and_frees_not_processed_items)
{
    // arrange
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_schedule_work(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(MU_FAILURE);
    ASSERT_ARE_EQUAL(int, 0, queue_processor_schedule_item(queue_processor, test_process_item_func_1, (void*)0x5001));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_process_item_func_1((void*)0x5001));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(THREADPOOL)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    queue_processor_destroy(queue_processor);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* queue_processor_open */

/* Tests_SRS_QUEUE_PROCESSOR_01_010: [ If queue_processor is NULL, queue_processor_open shall fail and return a non-zero value. ]*/
TEST_FUNCTION(queue_processor_open_with_NULL_queue_processor_fails)
{
    // arrange
    int result;

    // act
    result = queue_processor_open(NULL, test_on_queue_processor_error, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_048: [ If on_error is NULL, queue_processor_open shall fail and return a non-zero value. ]*/
TEST_FUNCTION(queue_processor_open_with_NULL_on_error_fails)
{
    // arrange
    int result;
    QUEUE_PROCESSOR_HANDLE queue_processor = queue_processor_create(g_test_threadpool);
    umock_c_reset_all_calls();

    // act
    result = queue_processor_open(queue_processor, NULL, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_013: [ Otherwise, queue_processor_open shall switch the state to QUEUE_PROCESSOR_STATE_OPENING. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_016: [ On success, queue_processor_open shall return 0. ]*/
TEST_FUNCTION(queue_processor_open_succeeds)
{
    // arrange
    int result;
    QUEUE_PROCESSOR_HANDLE queue_processor = queue_processor_create(g_test_threadpool);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));

    // act
    result = queue_processor_open(queue_processor, test_on_queue_processor_error, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_049: [ on_error_context shall be allowed to be NULL. ]*/
TEST_FUNCTION(queue_processor_open_succeeds_with_NULL_on_error_context)
{
    // arrange
    int result;
    QUEUE_PROCESSOR_HANDLE queue_processor = queue_processor_create(g_test_threadpool);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));

    // act
    result = queue_processor_open(queue_processor, test_on_queue_processor_error, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_014: [ If the queue_processor state was not QUEUE_PROCESSOR_STATE_CLOSED, queue_processor_open shall fail and return non-zero value. ]*/
TEST_FUNCTION(queue_processor_open_when_opening_fails)
{
    // arrange
    int result;
    QUEUE_PROCESSOR_HANDLE queue_processor = queue_processor_create(g_test_threadpool);
    ASSERT_ARE_EQUAL(int, 0, queue_processor_open(queue_processor, test_on_queue_processor_error, (void*)0x4243));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    result = queue_processor_open(queue_processor, test_on_queue_processor_error, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* queue_processor_close */

/* Tests_SRS_QUEUE_PROCESSOR_01_024: [ If queue_processor is NULL, queue_processor_close shall return. ]*/
TEST_FUNCTION(queue_processor_close_with_NULL_queue_processor_returns)
{
    // arrange

    // act
    queue_processor_close(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_QUEUE_PROCESSOR_01_025: [ If the state of queue_processor is not OPEN, queue_processor_close shall return. ]*/
TEST_FUNCTION(queue_processor_close_when_not_yet_open_returns)
{
    // arrange
    QUEUE_PROCESSOR_HANDLE queue_processor = queue_processor_create(g_test_threadpool);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    queue_processor_close(queue_processor);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_025: [ If the state of queue_processor is not OPEN, queue_processor_close shall return. ]*/
TEST_FUNCTION(queue_processor_close_when_already_closed_returns)
{
    // arrange
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    queue_processor_close(queue_processor);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    queue_processor_close(queue_processor);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_026: [ Otherwise, queue_processor_close shall switch the state to QUEUE_PROCESSOR_STATE_CLOSING. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_027: [ queue_processor_close shall wait for any ongoing queue_processor_schedule_item API calls to complete. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_029: [ queue_processor_close shall set the state to CLOSED. ]*/
TEST_FUNCTION(queue_processor_close_closes_the_underlying_objects)
{
    // arrange
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));

    // act
    queue_processor_close(queue_processor);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_027: [ queue_processor_close shall wait for any ongoing queue_processor_schedule_item API calls to complete. ]*/
TEST_FUNCTION(queue_processor_close_pending_api_call_fails_aborts)
{
    // arrange
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(INTERLOCKED_HL_ERROR);
    STRICT_EXPECTED_CALL(ps_util_terminate_process());
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));

    // act
    queue_processor_close(queue_processor);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_030: [ After a close, a succesfull call to queue_processor_open shall be possible. ]*/
TEST_FUNCTION(queue_processor_open_after_close_succeeds)
{
    // arrange
    int result;
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    queue_processor_close(queue_processor);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));

    // act
    result = queue_processor_open(queue_processor, test_on_queue_processor_error, (void*)0x4243);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_051: [ Any queued items that were not processed shall be processed by queue_processor_close. ]*/
// Tests_SRS_QUEUE_PROCESSOR_11_003: [ If the process state is IDLE then queue_processor_close shall change it to PROCESSING_STATE_EXECUTING and call the on_threadpool_work function ]
TEST_FUNCTION(queue_processor_close_processes_1_unprocessed_item)
{
    // arrange
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_schedule_work(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_work_function(&work_function)
        .CaptureArgumentValue_work_function_context(&work_function_context);
    ASSERT_ARE_EQUAL(int, 0, queue_processor_schedule_item(queue_processor, test_process_item_func_1, (void*)0x5001));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG))
        .SetReturn(NULL);
    work_function(work_function_context);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_process_item_func_1((void*)0x5001));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));

    // act
    queue_processor_close(queue_processor);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_051: [ Any queued items that were not processed shall be processed by queue_processor_close. ]*/
TEST_FUNCTION(queue_processor_close_processes_2_unprocessed_items)
{
    // arrange
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_schedule_work(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_work_function(&work_function)
        .CaptureArgumentValue_work_function_context(&work_function_context);
    ASSERT_ARE_EQUAL(int, 0, queue_processor_schedule_item(queue_processor, test_process_item_func_1, (void*)0x5001));
    ASSERT_ARE_EQUAL(int, 0, queue_processor_schedule_item(queue_processor, test_process_item_func_2, (void*)0x5002));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG))
        .SetReturn(NULL);
    work_function(work_function_context);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_process_item_func_1((void*)0x5001));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));

    STRICT_EXPECTED_CALL(test_process_item_func_2((void*)0x5002));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));

    // act
    queue_processor_close(queue_processor);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* queue_processor_schedule_item */

/* Tests_SRS_QUEUE_PROCESSOR_01_031: [ If queue_processor is NULL, queue_processor_schedule_item shall fail and return a non-zero value. ]*/
TEST_FUNCTION(queue_processor_schedule_item_with_NULL_queue_processor_fails)
{
    // arrange
    int result;

    // act
    result = queue_processor_schedule_item(NULL, test_process_item_func_1, (void*)0x5001);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_047: [ If process_item_func is NULL, queue_processor_schedule_item shall fail and return non-zero value. ]*/
TEST_FUNCTION(queue_processor_schedule_item_with_NULL_process_item_func_fails)
{
    // arrange
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    int result;

    // act
    result = queue_processor_schedule_item(queue_processor, NULL, (void*)0x5001);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_032: [ If the queue_processor state is not OPEN, queue_processor_schedule_item shall fail and return non-zero value. ]*/
TEST_FUNCTION(queue_processor_schedule_item_when_not_yet_open_fails)
{
    // arrange
    int result;
    QUEUE_PROCESSOR_HANDLE queue_processor = queue_processor_create(g_test_threadpool);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));

    // act
    result = queue_processor_schedule_item(queue_processor, test_process_item_func_1, (void*)0x5001);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_032: [ If the queue_processor state is not OPEN, queue_processor_schedule_item shall fail and return non-zero value. ]*/
TEST_FUNCTION(queue_processor_schedule_item_when_already_closed_fails)
{
    // arrange
    int result;
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    queue_processor_close(queue_processor);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));

    // act
    result = queue_processor_schedule_item(queue_processor, test_process_item_func_1, (void*)0x5001);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_033: [ Otherwise queue_processor_schedule_item shall allocate a context where process_item_func and process_item_context shall be stored. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_034: [ queue_processor_schedule_item shall enqueue the context in the multi producer single consumer queue created in queue_processor_create. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_036: [ If the processing state is IDLE, queue_processor_schedule_item shall switch the processing state to EXECUTING queue_processor_schedule_item shall call threadpool_schedule_work to schedule a work item and pass as callback on_threadpool_work and queue_processor as context. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_037: [ On success, queue_processor_schedule_item returns 0. ]*/
TEST_FUNCTION(queue_processor_schedule_item_succeeds)
{
    // arrange
    int result;
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_schedule_work(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_work_function(&work_function)
        .CaptureArgumentValue_work_function_context(&work_function_context);

    // act
    result = queue_processor_schedule_item(queue_processor, test_process_item_func_1, (void*)0x5001);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    work_function(work_function_context);
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_052: [ process_item_context shall be allowed to be NULL. ]*/
TEST_FUNCTION(queue_processor_schedule_item_with_NULL_context_succeeds)
{
    // arrange
    int result;
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_schedule_work(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_work_function(&work_function)
        .CaptureArgumentValue_work_function_context(&work_function_context);

    // act
    result = queue_processor_schedule_item(queue_processor, test_process_item_func_1, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    work_function(work_function_context);
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_036: [ If the processing state is IDLE, queue_processor_schedule_item shall switch the processing state to EXECUTING queue_processor_schedule_item shall call threadpool_schedule_work to schedule a work item and pass as callback on_threadpool_work and queue_processor as context. ]*/
TEST_FUNCTION(queue_processor_schedule_item_twice_only_schedules_the_threadpool_work_item_once)
{
    // arrange
    int result_1;
    int result_2;
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_schedule_work(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_work_function(&work_function)
        .CaptureArgumentValue_work_function_context(&work_function_context);

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    result_1 = queue_processor_schedule_item(queue_processor, test_process_item_func_1, (void*)0x5001);
    result_2 = queue_processor_schedule_item(queue_processor, test_process_item_func_2, (void*)0x5002);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result_1);
    ASSERT_ARE_EQUAL(int, 0, result_2);

    // cleanup
    work_function(work_function_context);
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_038: [ If any error occurs, queue_processor_schedule_item shall fail and return non-zero value. ]*/
TEST_FUNCTION(when_malloc_fails_queue_processor_schedule_item_fails)
{
    // arrange
    int result;
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));

    // act
    result = queue_processor_schedule_item(queue_processor, test_process_item_func_1, (void*)0x5001);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_038: [ If any error occurs, queue_processor_schedule_item shall fail and return non-zero value. ]*/
TEST_FUNCTION(when_mpsc_lock_free_queue_enqueue_fails_queue_processor_schedule_item_fails)
{
    // arrange
    int result;
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));

    // act
    result = queue_processor_schedule_item(queue_processor, test_process_item_func_1, (void*)0x5001);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_050: [ If scheduling the threadpool work item fails, an error shall be indicated by calling the on_error callback passed to queue_processor_open. ]*/
// Tests_SRS_QUEUE_PROCESSOR_11_002: [ ... and shall set the state to QUEUE_PROCESSOR_STATE_FAULTED
TEST_FUNCTION(when_threadpool_schedule_work_fails_queue_processor_schedule_item_indicates_an_error_to_the_user)
{
    // arrange
    int result;
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_schedule_work(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_work_function(&work_function)
        .CaptureArgumentValue_work_function_context(&work_function_context)
        .SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_on_queue_processor_error((void*)0x4243));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    result = queue_processor_schedule_item(queue_processor, test_process_item_func_1, (void*)0x5001);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* on_threadpool_work */

/* Tests_SRS_QUEUE_PROCESSOR_01_039: [ If context is NULL, on_threadpool_work shall return. ]*/
TEST_FUNCTION(on_threadpool_work_with_NULL_context_returns)
{
    // arrange
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    mock_schedule_item(queue_processor, &work_function, &work_function_context, test_process_item_func_1, g_test_queue_processor_context);

    // act
    work_function(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    work_function(work_function_context);
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_040: [ Otherwise on_threadpool_work shall use context as the QUEUE_PROCESSOR_HANDLE passed in queue_processor_schedule_item. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_041: [ While there are items in the queue: ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_042: [ on_threadpool_work shall dequeue an item from the queue. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_043: [ on_threadpool_work shall call the process_item_func with the context set to process_item_context. ]*/
TEST_FUNCTION(on_threadpool_work_calls_the_processing_function)
{
    // arrange
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    mock_schedule_item(queue_processor, &work_function, &work_function_context, test_process_item_func_1, g_test_queue_processor_context);

    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_process_item_func_1((void*)0x5001));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));

    // act
    work_function(work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_040: [ Otherwise on_threadpool_work shall use context as the QUEUE_PROCESSOR_HANDLE passed in queue_processor_schedule_item. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_041: [ While there are items in the queue: ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_042: [ on_threadpool_work shall dequeue an item from the queue. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_043: [ on_threadpool_work shall call the process_item_func with the context set to process_item_context. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_053: [ If the processing state is EXECUTING, queue_processor_schedule_item shall switch the processing state to NEW_DATA to signal on_threadpool_work that new items need to be processed. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_054: [ If the processing state is NEW_DATA, it shall be set to EXECUTING and the queue shall be re-examined and all new items in it processed. ]*/
TEST_FUNCTION(on_threadpool_work_calls_the_processing_function_for_2_items)
{
    // arrange
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    mock_schedule_item(queue_processor, &work_function, &work_function_context, test_process_item_func_1, g_test_queue_processor_context);
    ASSERT_ARE_EQUAL(int, 0, queue_processor_schedule_item(queue_processor, test_process_item_func_2, (void*)0x5002));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_process_item_func_1((void*)0x5001));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));

    STRICT_EXPECTED_CALL(test_process_item_func_2((void*)0x5002));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));

    // act
    work_function(work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_040: [ Otherwise on_threadpool_work shall use context as the QUEUE_PROCESSOR_HANDLE passed in queue_processor_schedule_item. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_041: [ While there are items in the queue: ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_042: [ on_threadpool_work shall dequeue an item from the queue. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_043: [ on_threadpool_work shall call the process_item_func with the context set to process_item_context. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_053: [ If the processing state is EXECUTING, queue_processor_schedule_item shall switch the processing state to NEW_DATA to signal on_threadpool_work that new items need to be processed. ]*/
TEST_FUNCTION(on_threadpool_work_processes_an_item_that_is_newly_added)
{
    // arrange
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    mock_schedule_item(queue_processor, &work_function, &work_function_context, test_process_item_add_new_item, queue_processor);

    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_process_item_add_new_item((void*)queue_processor));
    // new item enqueue
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_process_item_func_1((void*)0x6000));

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));

    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // one more pass because NEW_DATA
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));

    // act
    work_function(work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_055: [ If the processing state is EXECUTING, it shall be set to IDLE. ]*/
TEST_FUNCTION(on_threadpool_work_processes_2_items_in_2_threadpool_work_items)
{
    // arrange
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    umock_c_reset_all_calls();


    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_schedule_work(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_work_function(&work_function)
        .CaptureArgumentValue_work_function_context(&work_function_context);

    // Work Function
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_process_item_func_1((void*)0x6001));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));

    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_schedule_work(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // Work Function
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_process_item_func_2((void*)0x6002));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));

    // act
    ASSERT_ARE_EQUAL(int, 0, queue_processor_schedule_item(queue_processor, test_process_item_func_1, (void*)0x6001));
    work_function(work_function_context);
    ASSERT_ARE_EQUAL(int, 0, queue_processor_schedule_item(queue_processor, test_process_item_func_2, (void*)0x6002));
    work_function(work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    queue_processor_destroy(queue_processor);
}

/* Tests_SRS_QUEUE_PROCESSOR_01_040: [ Otherwise on_threadpool_work shall use context as the QUEUE_PROCESSOR_HANDLE passed in queue_processor_schedule_item. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_041: [ While there are items in the queue: ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_042: [ on_threadpool_work shall dequeue an item from the queue. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_043: [ on_threadpool_work shall call the process_item_func with the context set to process_item_context. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_053: [ If the processing state is EXECUTING, queue_processor_schedule_item shall switch the processing state to NEW_DATA to signal on_threadpool_work that new items need to be processed. ]*/
/* Tests_SRS_QUEUE_PROCESSOR_01_054: [ If the processing state is NEW_DATA, it shall be set to EXECUTING and the queue shall be re-examined and all new items in it processed. ]*/
TEST_FUNCTION(on_threadpool_work_calls_the_processing_function_for_3_items)
{
    // arrange
    QUEUE_PROCESSOR_HANDLE queue_processor = setup_open_queue_processor();
    THREADPOOL_WORK_FUNCTION work_function;
    void* work_function_context;
    mock_schedule_item(queue_processor, &work_function, &work_function_context, test_process_item_func_1, g_test_queue_processor_context);
    ASSERT_ARE_EQUAL(int, 0, queue_processor_schedule_item(queue_processor, test_process_item_func_2, (void*)0x5002));
    ASSERT_ARE_EQUAL(int, 0, queue_processor_schedule_item(queue_processor, test_process_item_func_3, (void*)0x5003));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_process_item_func_1(g_test_queue_processor_context));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));

    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_process_item_func_2((void*)0x5002));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));

    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_process_item_func_3((void*)0x5003));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));

    // act
    work_function(work_function_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    queue_processor_destroy(queue_processor);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
