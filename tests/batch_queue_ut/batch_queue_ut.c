// Copyright (c) Microsoft. All rights reserved.

#include <inttypes.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"

#define ENABLE_MOCKS
#include "c_logging/logger.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/execution_engine.h"
#include "c_pal/interlocked.h"
#include "c_pal/log_critical_and_terminate.h"
#include "c_pal/malloc_multi_flex.h"
#include "c_pal/sm.h"
#include "c_pal/thandle.h"
#include "c_pal/threadpool.h"
#include "c_pal/timer.h"
#include "c_pal/thandle.h"

#include "clds/mpsc_lock_free_queue.h"

#include "tp_worker_thread.h"

#include "c_util/tarray.h"
#include "c_util/tarray_ll.h"

#include "batch_queue_tarray_types.h"

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_interlocked.h"
#include "real_sm.h"
#include "real_mpsc_lock_free_queue.h"
#include "real_threadpool_thandle.h"

#include "batch_queue.h"


#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, BATCH_QUEUE_PROCESS_SYNC_RESULT, mocked_queue_process_batch_func, void*, process_batch_context, void**, items, uint32_t, item_count, BATCH_QUEUE_ON_LL_BATCH_COMPLETE, on_ll_batch_complete, void*, batch_context);
MOCKABLE_FUNCTION(, void, mocked_on_item_complete_callback, void*, context, BATCH_QUEUE_PROCESS_COMPLETE_RESULT, result, void*, ll_result);
MOCKABLE_FUNCTION(, void, mocked_on_faulted_callback, void*, context);
#undef ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"

MU_DEFINE_ENUM_STRINGS(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(BATCH_QUEUE_PROCESS_SYNC_RESULT, BATCH_QUEUE_PROCESS_SYNC_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(BATCH_QUEUE_PROCESS_SYNC_RESULT, BATCH_QUEUE_PROCESS_SYNC_RESULT_VALUES);

MU_DEFINE_ENUM_STRINGS(SM_RESULT, SM_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);

// Declare a real mock for TARRAY,
typedef BATCH_ITEM_CONTEXT_HANDLE REAL_BATCH_ITEM_CONTEXT_HANDLE;
TARRAY_DEFINE_STRUCT_TYPE(REAL_BATCH_ITEM_CONTEXT_HANDLE);

THANDLE_LL_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(REAL_BATCH_ITEM_CONTEXT_HANDLE), TARRAY_TYPEDEF_NAME(BATCH_ITEM_CONTEXT_HANDLE));
TARRAY_LL_TYPE_DECLARE(REAL_BATCH_ITEM_CONTEXT_HANDLE, BATCH_ITEM_CONTEXT_HANDLE);

static EXECUTION_ENGINE_HANDLE test_execution_engine = (EXECUTION_ENGINE_HANDLE)0x4242;
static TIMER_HANDLE test_timer = (TIMER_HANDLE)123402;

static BATCH_QUEUE_SETTINGS default_settings = {
    .max_pending_requests = 3,
    .max_batch_size = 3,
    .min_batch_size = 1,
    .min_wait_time = 10
};

#define TEST_TP_WORKER_THREAD_HANDLE (TP_WORKER_THREAD_HANDLE)0x4242

static THANDLE(THREADPOOL) hook_threadpool_create(EXECUTION_ENGINE_HANDLE execution_engine)
{
    // batch_queue_create calls THANDLE_INITIALIZE_MOVE, so we need to create a threadpool for every test that creates a batch queue.
    (void)execution_engine;
    THANDLE(THREADPOOL) temp = real_threadpool_thandle_create();
    ASSERT_IS_NOT_NULL(temp);
    return temp;
}

static void setup_batch_queue_create(THREADPOOL_WORK_FUNCTION* worker_thread, void ** worker_context)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_create());
    STRICT_EXPECTED_CALL(tp_worker_thread_create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)).CaptureArgumentValue_worker_func(worker_thread).CaptureArgumentValue_worker_func_context(worker_context);
    STRICT_EXPECTED_CALL(TARRAY_INITIALIZE(BATCH_ITEM_CONTEXT_HANDLE)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(interlocked_exchange_64(IGNORED_ARG, 0)).CallCannotFail();
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 0)).CallCannotFail();
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(execution_engine_inc_ref(test_execution_engine));
}

static void setup_batch_queue_close(uint32_t queued_item_count, uint32_t batch_staging_count)
{
    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_timer_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(tp_worker_thread_close(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(THREADPOOL)(IGNORED_ARG, NULL));
    for (uint32_t i = 0; i < batch_staging_count; i++)
    {
        STRICT_EXPECTED_CALL(mocked_on_item_complete_callback(IGNORED_ARG, BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED, NULL));
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    }
    for (uint32_t i = 0; i < queued_item_count; i++)
    {
        STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
        STRICT_EXPECTED_CALL(mocked_on_item_complete_callback(IGNORED_ARG, BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED, NULL));
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_ASSIGN(BATCH_ITEM_CONTEXT_HANDLE)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));
}

static void setup_batch_queue_destroy_empty_queue(uint32_t batch_staging_count, bool should_close)
{
    if (should_close)
    {
        setup_batch_queue_close(0, batch_staging_count);
    }
    else
    {
        STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    }

    STRICT_EXPECTED_CALL(tp_worker_thread_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(execution_engine_dec_ref(test_execution_engine));
    STRICT_EXPECTED_CALL(sm_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
}

static void setup_batch_queue_open()
{
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_CREATE(BATCH_ITEM_CONTEXT_HANDLE)());
    STRICT_EXPECTED_CALL(TARRAY_MOVE(BATCH_ITEM_CONTEXT_HANDLE)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_create(test_execution_engine));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(tp_worker_thread_open(IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_timer_start(IGNORED_ARG, UINT32_MAX, 0, IGNORED_ARG,IGNORED_ARG,IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_timer_cancel(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));
}

static BATCH_QUEUE_HANDLE test_batch_queue_create(BATCH_QUEUE_SETTINGS settings, BATCH_QUEUE_PROCESS_BATCH process_batch_func, void* process_batch_context, BATCH_QUEUE_ON_BATCH_FAULTED on_batch_faulted_func, void* on_batch_faulted_context)
{
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    setup_batch_queue_create(&worker_thread, &worker_context);
    BATCH_QUEUE_HANDLE result = batch_queue_create(settings, test_execution_engine, process_batch_func, process_batch_context, on_batch_faulted_func, on_batch_faulted_context);
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    return result;
}

static BATCH_QUEUE_HANDLE test_batch_queue_create_and_get_worker_thread(BATCH_QUEUE_SETTINGS settings, BATCH_QUEUE_PROCESS_BATCH process_batch_func, void* process_batch_context, BATCH_QUEUE_ON_BATCH_FAULTED on_batch_faulted_func, void* on_batch_faulted_context, THREADPOOL_WORK_FUNCTION* worker_thread, void** worker_context)
{
    setup_batch_queue_create(worker_thread, worker_context);
    BATCH_QUEUE_HANDLE result = batch_queue_create(settings, test_execution_engine, process_batch_func, process_batch_context, on_batch_faulted_func, on_batch_faulted_context);
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    return result;
}

static void test_batch_queue_open(BATCH_QUEUE_HANDLE test_handle)
{
    setup_batch_queue_open();
    int result = batch_queue_open(test_handle);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

static void test_batch_enqueue_item(BATCH_QUEUE_HANDLE test_handle, uint64_t* item, BATCH_QUEUE_ON_ITEM_COMPLETE per_item_callback, void* per_item_context)
{
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(tp_worker_thread_schedule_process(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    BATCH_QUEUE_ENQUEUE_RESULT result = batch_queue_enqueue(test_handle, item, sizeof(uint64_t), per_item_callback, per_item_context);
    ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

static void setup_work_thread_iteration_1_add_to_staging()
{
    // thread entry - check timer
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_ENSURE_CAPACITY(BATCH_ITEM_CONTEXT_HANDLE)(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
}


static void setup_work_thread_iteration_2_add_to_staging_send_batch(BATCH_QUEUE_PROCESS_SYNC_RESULT process_func_return, BATCH_QUEUE_ON_LL_BATCH_COMPLETE* process_batch_ll_callback_func, void** process_batch_ll_callback_context)
{
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_increment_64(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_queue_process_batch_func(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_ll_batch_complete(process_batch_ll_callback_func)
        .CaptureArgumentValue_batch_context(process_batch_ll_callback_context)
        .SetReturn(process_func_return);
    if (process_func_return == BATCH_QUEUE_PROCESS_SYNC_NOT_OPEN)
    {
        STRICT_EXPECTED_CALL(mocked_on_item_complete_callback(IGNORED_ARG, BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED, IGNORED_ARG));
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
        STRICT_EXPECTED_CALL(sm_fault(IGNORED_ARG));
        STRICT_EXPECTED_CALL(mocked_on_faulted_callback(IGNORED_ARG));
        STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
        STRICT_EXPECTED_CALL(gballoc_hl_free(IGNORED_ARG));
        STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    }
    else if (process_func_return != BATCH_QUEUE_PROCESS_SYNC_OK)
    {
        STRICT_EXPECTED_CALL(mocked_on_item_complete_callback(IGNORED_ARG, BATCH_QUEUE_PROCESS_COMPLETE_ERROR, IGNORED_ARG));
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
        STRICT_EXPECTED_CALL(sm_fault(IGNORED_ARG));
        STRICT_EXPECTED_CALL(mocked_on_faulted_callback(IGNORED_ARG));
        STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
        STRICT_EXPECTED_CALL(gballoc_hl_free(IGNORED_ARG));
        STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    }

    STRICT_EXPECTED_CALL(TARRAY_ENSURE_CAPACITY(BATCH_ITEM_CONTEXT_HANDLE)(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
}

static void setup_work_thread_iteration_2_add_to_staging_send_batch_with_items
(
    BATCH_QUEUE_PROCESS_SYNC_RESULT process_func_return,
    BATCH_QUEUE_ON_LL_BATCH_COMPLETE* process_batch_ll_callback_func,
    void** process_batch_ll_callback_context,
    void*** items,
    uint32_t* item_count
)
{
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_increment_64(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_queue_process_batch_func(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_items(items)
        .CaptureArgumentValue_item_count(item_count)
        .CaptureArgumentValue_on_ll_batch_complete(process_batch_ll_callback_func)
        .CaptureArgumentValue_batch_context(process_batch_ll_callback_context)
        .SetReturn(process_func_return);
    if (process_func_return == BATCH_QUEUE_PROCESS_SYNC_NOT_OPEN)
    {
        STRICT_EXPECTED_CALL(mocked_on_item_complete_callback(IGNORED_ARG, BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED, IGNORED_ARG));
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
        STRICT_EXPECTED_CALL(sm_fault(IGNORED_ARG));
        STRICT_EXPECTED_CALL(mocked_on_faulted_callback(IGNORED_ARG));
        STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
        STRICT_EXPECTED_CALL(gballoc_hl_free(IGNORED_ARG));
        STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    }
    else if (process_func_return != BATCH_QUEUE_PROCESS_SYNC_OK)
    {
        STRICT_EXPECTED_CALL(mocked_on_item_complete_callback(IGNORED_ARG, BATCH_QUEUE_PROCESS_COMPLETE_ERROR, IGNORED_ARG));
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
        STRICT_EXPECTED_CALL(sm_fault(IGNORED_ARG));
        STRICT_EXPECTED_CALL(mocked_on_faulted_callback(IGNORED_ARG));
        STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
        STRICT_EXPECTED_CALL(gballoc_hl_free(IGNORED_ARG));
        STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    }

    STRICT_EXPECTED_CALL(TARRAY_ENSURE_CAPACITY(BATCH_ITEM_CONTEXT_HANDLE)(IGNORED_ARG, 1));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_SM_GLOBAL_MOCK_HOOK();
    REGISTER_MPSC_LOCK_FREE_QUEUE_GLOBAL_MOCK_HOOKS();

    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE_MOVE(THREADPOOL), THANDLE_INITIALIZE_MOVE(REAL_THREADPOOL));
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(THREADPOOL), THANDLE_INITIALIZE(REAL_THREADPOOL));
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(THREADPOOL), THANDLE_ASSIGN(REAL_THREADPOOL));

    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(TARRAY_TYPEDEF_NAME(BATCH_ITEM_CONTEXT_HANDLE)), THANDLE_ASSIGN(TARRAY_TYPEDEF_NAME(REAL_BATCH_ITEM_CONTEXT_HANDLE)));
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(TARRAY_TYPEDEF_NAME(BATCH_ITEM_CONTEXT_HANDLE)), THANDLE_INITIALIZE(TARRAY_TYPEDEF_NAME(REAL_BATCH_ITEM_CONTEXT_HANDLE)));
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE_MOVE(TARRAY_TYPEDEF_NAME(BATCH_ITEM_CONTEXT_HANDLE)), THANDLE_INITIALIZE_MOVE(TARRAY_TYPEDEF_NAME(REAL_BATCH_ITEM_CONTEXT_HANDLE)));
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_MOVE(TARRAY_TYPEDEF_NAME(BATCH_ITEM_CONTEXT_HANDLE)), THANDLE_MOVE(TARRAY_TYPEDEF_NAME(REAL_BATCH_ITEM_CONTEXT_HANDLE)));
    REGISTER_GLOBAL_MOCK_HOOK(TARRAY_CREATE_WITH_CAPACITY(BATCH_ITEM_CONTEXT_HANDLE), TARRAY_CREATE_WITH_CAPACITY(REAL_BATCH_ITEM_CONTEXT_HANDLE));
    REGISTER_GLOBAL_MOCK_HOOK(TARRAY_CREATE(BATCH_ITEM_CONTEXT_HANDLE), TARRAY_CREATE(REAL_BATCH_ITEM_CONTEXT_HANDLE));
    REGISTER_GLOBAL_MOCK_HOOK(TARRAY_MOVE(BATCH_ITEM_CONTEXT_HANDLE), TARRAY_MOVE(REAL_BATCH_ITEM_CONTEXT_HANDLE));
    REGISTER_GLOBAL_MOCK_HOOK(TARRAY_INITIALIZE_MOVE(BATCH_ITEM_CONTEXT_HANDLE), TARRAY_INITIALIZE_MOVE(REAL_BATCH_ITEM_CONTEXT_HANDLE));
    REGISTER_GLOBAL_MOCK_HOOK(TARRAY_INITIALIZE(BATCH_ITEM_CONTEXT_HANDLE), TARRAY_INITIALIZE(REAL_BATCH_ITEM_CONTEXT_HANDLE));
    REGISTER_GLOBAL_MOCK_HOOK(TARRAY_ASSIGN(BATCH_ITEM_CONTEXT_HANDLE), TARRAY_ASSIGN(REAL_BATCH_ITEM_CONTEXT_HANDLE));
    REGISTER_GLOBAL_MOCK_HOOK(TARRAY_ENSURE_CAPACITY(BATCH_ITEM_CONTEXT_HANDLE), TARRAY_ENSURE_CAPACITY(REAL_BATCH_ITEM_CONTEXT_HANDLE));

    REGISTER_TYPE(SM_RESULT, SM_RESULT);
    REGISTER_TYPE(BATCH_QUEUE_PROCESS_SYNC_RESULT, BATCH_QUEUE_PROCESS_SYNC_RESULT);
    REGISTER_TYPE(BATCH_QUEUE_PROCESS_COMPLETE_RESULT, BATCH_QUEUE_PROCESS_COMPLETE_RESULT);

    REGISTER_UMOCK_ALIAS_TYPE(BATCH_QUEUE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(EXECUTION_ENGINE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TP_WORKER_THREAD_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TP_WORKER_THREAD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TP_WORKER_THREAD_SCHEDULE_PROCESS_RESULT, int);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(THREADPOOL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREADPOOL_WORK_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TIMER_INSTANCE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TARRAY(BATCH_ITEM_CONTEXT_HANDLE), void*);
    REGISTER_UMOCK_ALIAS_TYPE(MPSC_LOCK_FREE_QUEUE_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SM_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BATCH_QUEUE_ON_LL_BATCH_COMPLETE, void*);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(tp_worker_thread_create, TEST_TP_WORKER_THREAD_HANDLE, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(tp_worker_thread_open, 0, 100);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(tp_worker_thread_schedule_process, TP_WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_queue_process_batch_func, BATCH_QUEUE_PROCESS_SYNC_OK, BATCH_QUEUE_PROCESS_SYNC_ERROR);

    REGISTER_GLOBAL_MOCK_HOOK(threadpool_create, hook_threadpool_create);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(threadpool_create, NULL);
    REGISTER_GLOBAL_MOCK_RETURNS(threadpool_timer_start, 0, MU_FAILURE);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mpsc_lock_free_queue_enqueue, MU_FAILURE);
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

/* Tests_SRS_BATCH_QUEUE_42_001: [ If execution_engine is NULL then batch_queue_create shall fail and return NULL. ]*/
TEST_FUNCTION(batch_queue_create_null_execution_engine_returns_null)
{
    // arrange
    uint64_t process_batch_context;
    uint64_t on_batch_faulted_context;

    // act
    BATCH_QUEUE_HANDLE result = batch_queue_create(default_settings, NULL, mocked_queue_process_batch_func, &process_batch_context, mocked_on_faulted_callback, &on_batch_faulted_context);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_BATCH_QUEUE_42_002: [ If process_batch_func is NULL then batch_queue_create shall fail and return NULL. ]*/
TEST_FUNCTION(batch_queue_create_null_queue_process_func_returns_null)
{
    // arrange
    uint64_t process_batch_context;
    uint64_t on_batch_faulted_context;

    // act
    BATCH_QUEUE_HANDLE result = batch_queue_create(default_settings, test_execution_engine, NULL, &process_batch_context, mocked_on_faulted_callback, &on_batch_faulted_context);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_BATCH_QUEUE_42_003: [ If on_batch_faulted_func is NULL then batch_queue_create shall fail and return NULL. ]*/
TEST_FUNCTION(batch_queue_create_null_batch_fault_func_returns_null)
{
    // arrange
    uint64_t process_batch_context;
    uint64_t on_batch_faulted_context;

    // act
    BATCH_QUEUE_HANDLE result = batch_queue_create(default_settings, test_execution_engine, mocked_queue_process_batch_func, &process_batch_context, NULL, &on_batch_faulted_context);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_BATCH_QUEUE_42_004: [ If settings.max_pending_requests is 0 then batch_queue_create shall fail and return NULL. ]*/
TEST_FUNCTION(batch_queue_create_0_max_pending_requests_returns_null)
{
    // arrange
    uint64_t process_batch_context;
    uint64_t on_batch_faulted_context;
    static BATCH_QUEUE_SETTINGS queue_settings = {
        .max_pending_requests = 0,
        .max_batch_size = 3,
        .min_batch_size = 1,
        .min_wait_time = 10
    };
    // act
    BATCH_QUEUE_HANDLE result = batch_queue_create(queue_settings, test_execution_engine, mocked_queue_process_batch_func, &process_batch_context, mocked_on_faulted_callback, &on_batch_faulted_context);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_BATCH_QUEUE_42_005: [ batch_queue_create shall allocate memory for the batch queue. ]*/
/* Tests_SRS_BATCH_QUEUE_42_006: [ batch_queue_create shall call sm_create. ]*/
/* Tests_SRS_BATCH_QUEUE_45_013: [ batch_queue_create shall keep the execution engine, and call execution_engine_inc_ref. ]*/
/* Tests_SRS_BATCH_QUEUE_42_007: [ batch_queue_create shall create a multi producer single consumer queue by calling mpsc_lock_free_queue_create. ]*/
/* Tests_SRS_BATCH_QUEUE_42_008: [ batch_queue_create shall create a worker thread by calling tp_worker_thread_create with batch_queue_worker_thread. ]*/
/* Tests_SRS_BATCH_QUEUE_42_020: [ batch_queue_create shall initialize the batch staging array item count and batch size to 0. ]*/
/* Tests_SRS_BATCH_QUEUE_42_009: [ batch_queue_create shall return the batch queue handle. ]*/
TEST_FUNCTION(batch_queue_create_success)
{
    // arrange
    uint64_t process_batch_context;
    uint64_t on_batch_faulted_context;

    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    setup_batch_queue_create(&worker_thread, &worker_context);

    // act
    BATCH_QUEUE_HANDLE result = batch_queue_create(default_settings, test_execution_engine, mocked_queue_process_batch_func, &process_batch_context, mocked_on_faulted_callback, &on_batch_faulted_context);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_destroy(result);

}

/* Tests_SRS_BATCH_QUEUE_42_010: [ If there are any other errors then batch_queue_create shall fail and return NULL. ]*/
TEST_FUNCTION(batch_queue_create_negative_tests)
{
    // arrange
    uint64_t process_batch_context;
    uint64_t on_batch_faulted_context;

    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    setup_batch_queue_create(&worker_thread, &worker_context);
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            BATCH_QUEUE_HANDLE result = batch_queue_create(default_settings, test_execution_engine, mocked_queue_process_batch_func, &process_batch_context, mocked_on_faulted_callback, &on_batch_faulted_context);

            // assert
            ASSERT_IS_NULL(result);
        }
    }
    // cleanup
}

/* Tests_SRS_BATCH_QUEUE_42_011: [ If batch_queue is NULL then batch_queue_destroy shall return. ]*/
TEST_FUNCTION(batch_queue_destroy_null_handle_returns)
{
    // arrange

    // act
    batch_queue_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_BATCH_QUEUE_42_013: [ batch_queue_destroy shall call tp_worker_thread_destroy. ]*/
/* Tests_SRS_BATCH_QUEUE_42_014: [ batch_queue_destroy shall call mpsc_lock_free_queue_destroy. ]*/
/* Tests_SRS_BATCH_QUEUE_45_014: [ batch_queue_destroy shall call execution_engine_dec_ref on the execution engine. ]*/
/* Tests_SRS_BATCH_QUEUE_42_015: [ batch_queue_destroy shall call sm_destroy. ]*/
/* Tests_SRS_BATCH_QUEUE_42_016: [ batch_queue_destroy shall free the memory allocated for the batch queue. ]*/
TEST_FUNCTION(batch_queue_destroy_success)
{
    // arrange
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL);
    umock_c_reset_all_calls();

    setup_batch_queue_destroy_empty_queue(0, false);

    // act
    batch_queue_destroy(test_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_BATCH_QUEUE_42_012: [ batch_queue_destroy shall behave as if batch_queue_close was called. ]*/
/* Tests_SRS_BATCH_QUEUE_42_013: [ batch_queue_destroy shall call tp_worker_thread_destroy. ]*/
/* Tests_SRS_BATCH_QUEUE_42_014: [ batch_queue_destroy shall call mpsc_lock_free_queue_destroy. ]*/
/* Tests_SRS_BATCH_QUEUE_45_014: [ batch_queue_destroy shall call execution_engine_dec_ref on the execution engine. ]*/
/* Tests_SRS_BATCH_QUEUE_42_015: [ batch_queue_destroy shall call sm_destroy. ]*/
/* Tests_SRS_BATCH_QUEUE_42_016: [ batch_queue_destroy shall free the memory allocated for the batch queue. ]*/
TEST_FUNCTION(batch_queue_destroy_closes_after_open)
{
    // arrange

    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL);
    test_batch_queue_open(test_handle);
    umock_c_reset_all_calls();

    setup_batch_queue_destroy_empty_queue(0, true);

    // act
    batch_queue_destroy(test_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_BATCH_QUEUE_42_017: [ If batch_queue is NULL then batch_queue_open shall fail and return a non-zero value. ]*/
TEST_FUNCTION(batch_queue_open_fails_if_null_handle)
{
    // arrange

    // act
    int result = batch_queue_open(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_BATCH_QUEUE_42_018: [ batch_queue_open shall call sm_open_begin. ]*/
/* Tests_SRS_BATCH_QUEUE_42_019: [ batch_queue_open shall call TARRAY_CREATE to create a batch staging array. ]*/
/* Tests_SRS_BATCH_QUEUE_45_015: [ batch_queue_open shall call threadpool_create. ]*/
/* Tests_SRS_BATCH_QUEUE_42_022: [ batch_queue_open shall call tp_worker_thread_open. ]*/
/* Tests_SRS_BATCH_QUEUE_42_107: [ batch_queue_open shall call threadpool_timer_start with UINT32_MAX as the start_delay_ms, 0 as the timer_period_ms, and batch_queue_timer_work as the work_function. ]*/
/* Tests_SRS_BATCH_QUEUE_42_108: [ batch_queue_open shall call threadpool_timer_cancel. ]*/
/* Tests_SRS_BATCH_QUEUE_42_024: [ batch_queue_open shall call sm_open_end with true. ]*/
/* Tests_SRS_BATCH_QUEUE_42_025: [ batch_queue_open shall succeed and return 0. ]*/
TEST_FUNCTION(batch_queue_open_success)
{
    // arrange
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL);
    umock_c_reset_all_calls();

    setup_batch_queue_open();

    // act
    int result = batch_queue_open(test_handle);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_021: [ If TARRAY_CREATE fails then batch_queue_open shall call sm_open_end with false. ]*/
/* Tests_SRS_BATCH_QUEUE_42_023: [ If anything fails after sm_open_begin then batch_queue_open shall call sm_open_end with false. ]*/
/* Tests_SRS_BATCH_QUEUE_42_026: [ If there are any errors then batch_queue_open shall fail and return a non-zero value. ]*/
TEST_FUNCTION(batch_queue_open_negative_tests)
{
    // arrange
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL);
    umock_c_reset_all_calls();

    setup_batch_queue_open();
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            int result = batch_queue_open(test_handle);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result);
        }
    }
    // cleanup
    batch_queue_destroy(test_handle);
}

static void setup_for_timer_work_func(BATCH_QUEUE_HANDLE test_handle, THREADPOOL_WORK_FUNCTION *timer_work, void** timer_work_context)
{
    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_CREATE(BATCH_ITEM_CONTEXT_HANDLE)());
    STRICT_EXPECTED_CALL(TARRAY_MOVE(BATCH_ITEM_CONTEXT_HANDLE)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_create(test_execution_engine));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(tp_worker_thread_open(IGNORED_ARG));
    STRICT_EXPECTED_CALL(threadpool_timer_start(IGNORED_ARG, UINT32_MAX, 0, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)).CaptureArgumentValue_work_function(timer_work).CaptureArgumentValue_work_function_context(timer_work_context);
    STRICT_EXPECTED_CALL(threadpool_timer_cancel(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));
    int result = batch_queue_open(test_handle);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
}

/* Tests_SRS_BATCH_QUEUE_42_067: [ If context is NULL then batch_queue_timer_work shall terminate the process. ]*/
TEST_FUNCTION(batch_queue_timer_work_null_context_terminates)
{
    // arrange
    THREADPOOL_WORK_FUNCTION timer_work = NULL;
    void* timer_work_context = NULL;
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL);

    setup_for_timer_work_func(test_handle, &timer_work, &timer_work_context);

    STRICT_EXPECTED_CALL(ps_util_terminate_process());

    // act
    timer_work(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_068: [ batch_queue_timer_work shall call tp_worker_thread_schedule_process. ]*/
TEST_FUNCTION(batch_queue_timer_work_success)
{
    // arrange
    THREADPOOL_WORK_FUNCTION timer_work = NULL;
    void* timer_work_context = NULL;
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL);

    setup_for_timer_work_func(test_handle, &timer_work, &timer_work_context);

    STRICT_EXPECTED_CALL(tp_worker_thread_schedule_process(IGNORED_ARG));

    // act
    timer_work(timer_work_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);
}

TEST_FUNCTION(batch_queue_timer_work_thread_schedule_error_INVALID_STATE_is_ignored)
{
    // arrange
    THREADPOOL_WORK_FUNCTION timer_work = NULL;
    void* timer_work_context = NULL;
    void* faulted_context;
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, (void*)&faulted_context);

    setup_for_timer_work_func(test_handle, &timer_work, &timer_work_context);

    STRICT_EXPECTED_CALL(tp_worker_thread_schedule_process(IGNORED_ARG))
        .SetReturn(TP_WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE);

    // act
    timer_work(timer_work_context);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_027: [ If batch_queue is NULL then batch_queue_close shall return. ]*/
TEST_FUNCTION(batch_queue_close_null_handle_returns)
{
    // arrange

    // act
    batch_queue_close(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_BATCH_QUEUE_42_028: [ batch_queue_close shall call sm_close_begin. ]*/
/* Tests_SRS_BATCH_QUEUE_42_029: [ batch_queue_close shall call threadpool_timer_destroy. ]*/
/* Tests_SRS_BATCH_QUEUE_42_030: [ batch_queue_close shall call tp_worker_thread_close. ]*/
/* Tests_SRS_BATCH_QUEUE_45_016: [ batch_queue_close shall call THANDLE_ASSIGN(THREADPOOL) with NULL. ]*/
/* Tests_SRS_BATCH_QUEUE_42_034: [ batch_queue_close shall call TARRAY_ASSIGN with NULL to free the batch staging array. ]*/
/* Tests_SRS_BATCH_QUEUE_42_035: [ batch_queue_close shall call sm_close_end. ]*/
TEST_FUNCTION(batch_queue_close_simple_success)
{
    // arrange
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL);
    test_batch_queue_open(test_handle);
    umock_c_reset_all_calls();

    setup_batch_queue_close(0, 0);

    // act
    batch_queue_close(test_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_028: [ batch_queue_close shall call sm_close_begin. ]*/
TEST_FUNCTION(batch_queue_close_sm_close_fails)
{
    // arrange
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));

    // act
    batch_queue_close(test_handle);

    // assert

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    // cleanup
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_028: [ batch_queue_close shall call sm_close_begin. ]*/
TEST_FUNCTION(batch_queue_close_sm_close_after_close_fails)
{
    // arrange
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL);
    setup_batch_queue_close(0, 0);
    batch_queue_close(test_handle);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));

    // act
    batch_queue_close(test_handle);

    // assert

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    // cleanup
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_029: [ batch_queue_close shall call threadpool_timer_destroy. ]*/
/* Tests_SRS_BATCH_QUEUE_42_030: [ batch_queue_close shall call tp_worker_thread_close. ]*/
/* Tests_SRS_BATCH_QUEUE_45_016: [ batch_queue_close shall call THANDLE_ASSIGN(THREADPOOL) with NULL. ]*/
/* Tests_SRS_BATCH_QUEUE_42_031: [ For each request in the queue: ]*/
/* Tests_SRS_BATCH_QUEUE_42_032: [ batch_queue_close shall call on_item_complete with BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED and NULL for the ll_result. ]*/
/* Tests_SRS_BATCH_QUEUE_42_033: [ batch_queue_close shall free the memory associated with the request. ]*/
/* Tests_SRS_BATCH_QUEUE_42_034: [ batch_queue_close shall call TARRAY_ASSIGN with NULL to free the batch staging array. ]*/
/* Tests_SRS_BATCH_QUEUE_42_035: [ batch_queue_close shall call sm_close_end. ]*/
TEST_FUNCTION(batch_queue_close_with_enqueued_items)
{
    // arrange
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    void* on_faulted_context;
    void* on_item_complete_context;

    // default settings should trigger batch_queue_send_batch to be called on second iteration
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, (void*)&on_faulted_context, &worker_thread, &worker_context);
    test_batch_queue_open(test_handle);
    uint64_t value1 = 0x10001;
    uint64_t value2 = 0x10002;
    test_batch_enqueue_item(test_handle, &value1, mocked_on_item_complete_callback, &on_item_complete_context);
    test_batch_enqueue_item(test_handle, &value2, mocked_on_item_complete_callback, &on_item_complete_context);
    umock_c_reset_all_calls();

    setup_batch_queue_close(2, 0);

    // act
    batch_queue_close(test_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_029: [ batch_queue_close shall call threadpool_timer_destroy. ]*/
/* Tests_SRS_BATCH_QUEUE_42_030: [ batch_queue_close shall call tp_worker_thread_close. ]*/
/* Tests_SRS_BATCH_QUEUE_45_016: [ batch_queue_close shall call THANDLE_ASSIGN(THREADPOOL) with NULL. ]*/
/* Tests_SRS_BATCH_QUEUE_42_102: [ For each request in the batch staging array: ]*/
/* Tests_SRS_BATCH_QUEUE_42_103: [ batch_queue_close shall call on_item_complete with BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED and NULL for the ll_result. ]*/
/* Tests_SRS_BATCH_QUEUE_42_104: [ batch_queue_close shall free the memory associated with the request. ]*/
/* Tests_SRS_BATCH_QUEUE_42_034: [ batch_queue_close shall call TARRAY_ASSIGN with NULL to free the batch staging array. ]*/
/* Tests_SRS_BATCH_QUEUE_42_035: [ batch_queue_close shall call sm_close_end. ]*/
TEST_FUNCTION(batch_queue_close_with_staged_items)
{
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    BATCH_QUEUE_SETTINGS batch_settings = {
        .max_pending_requests = 5,
        .max_batch_size = 100,
        .min_batch_size = 30,
        .min_wait_time = 10
    };
    void* process_batch_context;
    void* on_item_complete_context;
    // batch_settings should not trigger batch_queue_send_batch
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(batch_settings, mocked_queue_process_batch_func, (void*)&process_batch_context, mocked_on_faulted_callback, NULL, &worker_thread, &worker_context);
    test_batch_queue_open(test_handle);
    uint64_t value1 = 0x10001;
    test_batch_enqueue_item(test_handle, &value1, mocked_on_item_complete_callback, &on_item_complete_context);
    umock_c_reset_all_calls();

    // iteration 1 - dequeue item and put in staging
    setup_work_thread_iteration_1_add_to_staging();

    // iteration 2 - no new items, is not the min batch size, wait time is not reached, set timer
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms()).SetReturn(5);
    STRICT_EXPECTED_CALL(threadpool_timer_restart(IGNORED_ARG, 4, 0));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //iteration 3 - force stop the worker thread.
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG)).SetReturn(SM_ERROR);
    worker_thread(worker_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    setup_batch_queue_close(0, 1);

    // act
    batch_queue_close(test_handle);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);

}

/* Tests_SRS_BATCH_QUEUE_42_036: [ If batch_queue is NULL then batch_queue_enqueue shall fail and return BATCH_QUEUE_ENQUEUE_INVALID_ARGS. ]*/
TEST_FUNCTION(batch_queue_enqueue_fails_with_null_queue)
{
    // arrange

    // act
    BATCH_QUEUE_ENQUEUE_RESULT result = batch_queue_enqueue(NULL, NULL, 0, mocked_on_item_complete_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

/* Tests_SRS_BATCH_QUEUE_42_037: [ If item is NULL then batch_queue_enqueue shall fail and return BATCH_QUEUE_ENQUEUE_INVALID_ARGS. ]*/
TEST_FUNCTION(batch_queue_enqueue_fails_if_item_is_null)
{
    // arrange
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL);
    umock_c_reset_all_calls();

    // act
    BATCH_QUEUE_ENQUEUE_RESULT result = batch_queue_enqueue(test_handle, NULL, 1, mocked_on_item_complete_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_destroy(test_handle);
}

/*Tests_SRS_BATCH_QUEUE_42_038: [ If item_size is 0 then batch_queue_enqueue shall fail and return BATCH_QUEUE_ENQUEUE_INVALID_ARGS. ]*/
TEST_FUNCTION(batch_queue_enqueue_fails_if_item_size_is_0)
{
    // arrange
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL);
    umock_c_reset_all_calls();

    uint64_t item;

    // act
    BATCH_QUEUE_ENQUEUE_RESULT result = batch_queue_enqueue(test_handle, &item, 0, mocked_on_item_complete_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_039: [ If on_item_complete is NULL then batch_queue_enqueue shall fail and return BATCH_QUEUE_ENQUEUE_INVALID_ARGS. ]*/
TEST_FUNCTION(batch_queue_enqueue_fails_if_item_callback_is_null)
{
    // arrange
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL);
    umock_c_reset_all_calls();

    uint64_t item;

    // act
    BATCH_QUEUE_ENQUEUE_RESULT result = batch_queue_enqueue(test_handle, &item, sizeof(uint64_t), NULL, NULL);

    // assert
    ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_040: [ batch_queue_enqueue shall call sm_exec_begin. ] */
/* Tests_SRS_BATCH_QUEUE_42_041: [ If sm_exec_begin fails then batch_queue_enqueue shall return BATCH_QUEUE_ENQUEUE_INVALID_STATE. ] */
TEST_FUNCTION(batch_queue_enqueue_fails_if_not_open)
{
    // arrange
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL);
    umock_c_reset_all_calls();

    uint64_t item;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));

    // act
    BATCH_QUEUE_ENQUEUE_RESULT result = batch_queue_enqueue(test_handle, &item, sizeof(uint64_t), mocked_on_item_complete_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_INVALID_STATE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_040: [ batch_queue_enqueue shall call sm_exec_begin. ] */
/* Tests_SRS_BATCH_QUEUE_42_042: [ batch_queue_enqueue shall get the current time to store with the request by calling timer_global_get_elapsed_ms. ]*/
/* Tests_SRS_BATCH_QUEUE_42_043: [ batch_queue_enqueue shall allocate memory for the request context. ]*/
/* Tests_SRS_BATCH_QUEUE_42_044: [ batch_queue_enqueue shall call mpsc_lock_free_queue_enqueue. ]*/
/* Tests_SRS_BATCH_QUEUE_42_045: [ batch_queue_enqueue shall call tp_worker_thread_schedule_process. ]*/
/* Tests_SRS_BATCH_QUEUE_42_049: [ batch_queue_enqueue shall call sm_exec_end. ]*/
/* Tests_SRS_BATCH_QUEUE_42_050: [ batch_queue_enqueue shall succeed and return BATCH_QUEUE_ENQUEUE_OK. ]*/
TEST_FUNCTION(batch_queue_enqueue_success)
{
    // arrange
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL);
    test_batch_queue_open(test_handle);
    umock_c_reset_all_calls();

    uint64_t item;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms());
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(tp_worker_thread_schedule_process(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    BATCH_QUEUE_ENQUEUE_RESULT result = batch_queue_enqueue(test_handle, &item, sizeof(uint64_t), mocked_on_item_complete_callback, NULL);

    // assert
    ASSERT_ARE_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_051: [ If any other error occurs then batch_queue_enqueue shall fail and return BATCH_QUEUE_ENQUEUE_ERROR. ]*/
TEST_FUNCTION(batch_queue_enqueue_negative_tests)
{
    // arrange
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL);
    test_batch_queue_open(test_handle);
    umock_c_reset_all_calls();

    uint64_t item;

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .CallCannotFail();
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_enqueue(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(tp_worker_thread_schedule_process(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            BATCH_QUEUE_ENQUEUE_RESULT result = batch_queue_enqueue(test_handle, &item, sizeof(uint64_t), mocked_on_item_complete_callback, NULL);

            // assert
            ASSERT_ARE_NOT_EQUAL(BATCH_QUEUE_ENQUEUE_RESULT, BATCH_QUEUE_ENQUEUE_OK, result);
        }
    }
    // cleanup
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_052: [ If context is NULL then batch_queue_worker_thread shall terminate the process. ]*/
TEST_FUNCTION(batch_queue_worker_thread_null_context_terminates)
{
    // arrange
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL, &worker_thread, &worker_context);

    STRICT_EXPECTED_CALL(ps_util_terminate_process());

    // act
    worker_thread(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_052: [ If context is NULL then batch_queue_worker_thread shall terminate the process. ]*/
/* Tests_SRS_BATCH_QUEUE_42_053: [ If a timer has been started then batch_queue_worker_thread shall call threadpool_timer_cancel. ]*/
/* Tests_SRS_BATCH_QUEUE_42_054: [ batch_queue_worker_thread shall do the following until there are no more items in the queue: ]*/
/* Tests_SRS_BATCH_QUEUE_42_105: [ batch_queue_worker_thread shall call sm_exec_begin. ]*/
/* Tests_SRS_BATCH_QUEUE_42_109: [ If sm_exec_begin fails then batch_queue_worker_thread shall exit the loop. ]*/
TEST_FUNCTION(batch_queue_worker_thread_sm_exec_fails_not_open)
{
    // arrange
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL, &worker_thread, &worker_context);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));

    // act
    worker_thread(worker_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_45_010: [ batch_queue_worker_thread shall call sm_exec_begin to check the timer. ]*/
/* Tests_SRS_BATCH_QUEUE_45_011: [ batch_queue_worker_thread shall call sm_exec_end when it is done checking the timer. ]*/
/* Tests_SRS_BATCH_QUEUE_42_053: [ If a timer has been started then batch_queue_worker_thread shall call threadpool_timer_cancel. ]*/
/* Tests_SRS_BATCH_QUEUE_42_054: [ batch_queue_worker_thread shall do the following until there are no more items in the queue: ]*/
/* Tests_SRS_BATCH_QUEUE_42_105: [ batch_queue_worker_thread shall call sm_exec_begin. ]*/
/* Tests_SRS_BATCH_QUEUE_42_055: [ batch_queue_worker_thread shall call mpsc_lock_free_queue_dequeue. ]*/
/* Tests_SRS_BATCH_QUEUE_42_059: [ batch_queue_worker_thread shall call TARRAY_ENSURE_CAPACITY on the batch staging array to ensure it can hold the new item. ]*/
/* Tests_SRS_BATCH_QUEUE_45_001: [ If TARRAY_ENSURE_CAPACITY fails then, ]*/
/* Tests_SRS_BATCH_QUEUE_45_002: [ batch_queue_worker_thread shall call sm_fault. ]*/
/* Tests_SRS_BATCH_QUEUE_45_003: [ batch_queue_worker_thread shall call on_batch_faulted_func with on_batch_faulted_context. ]*/
/* Tests_SRS_BATCH_QUEUE_45_005: [ batch_queue_worker_thread shall free the batch_item_context. ]*/
/* Tests_SRS_BATCH_QUEUE_45_004: [ batch_queue_worker_thread shall exit the loop. ]*/
TEST_FUNCTION(batch_queue_worker_thread_item_queued_TARRAY_ENSURE_CAPACITY_fails_ends_loop)
{
    // arrange
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL, &worker_thread, &worker_context);
    test_batch_queue_open(test_handle);
    uint64_t value1 = 0x10001;
    test_batch_enqueue_item(test_handle, &value1, mocked_on_item_complete_callback, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_ENSURE_CAPACITY(BATCH_ITEM_CONTEXT_HANDLE)(IGNORED_ARG, 1))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(sm_fault(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_on_faulted_callback(NULL));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    worker_thread(worker_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);
}


/* Tests_SRS_BATCH_QUEUE_45_010: [ batch_queue_worker_thread shall call sm_exec_begin to check the timer. ]*/
/* Tests_SRS_BATCH_QUEUE_45_011: [ batch_queue_worker_thread shall call sm_exec_end when it is done checking the timer. ]*/
/* Tests_SRS_BATCH_QUEUE_42_056: [ If there are items in the batch staging array and the size of the dequeued item would cause the staging batch to exceed max_batch_size then: ]*/
/* Tests_SRS_BATCH_QUEUE_42_057: [ batch_queue_worker_thread shall call batch_queue_send_batch. ]*/
/* Tests_SRS_BATCH_QUEUE_42_058: [ If batch_queue_send_batch fails then batch_queue_worker_thread shall exit the loop. ]*/
/* Tests_SRS_BATCH_QUEUE_42_072: [ batch_queue_send_batch shall increment the number of pending batches. ]*/
/* Tests_SRS_BATCH_QUEUE_42_073: [ batch_queue_send_batch shall allocate a context for the batch and move the items from the batch staging array to the batch context. ]*/
/* Tests_SRS_BATCH_QUEUE_42_074: [ batch_queue_send_batch shall call sm_exec_begin. ]*/
/* Tests_SRS_BATCH_QUEUE_42_075: [ batch_queue_send_batch shall call process_batch_func with the batch context. ]*/
/* Tests_SRS_BATCH_QUEUE_42_076: [ If process_batch_func returns anything other than BATCH_QUEUE_PROCESS_SYNC_OK then: ]*/
/* Tests_SRS_BATCH_QUEUE_42_077: [ If process_batch_func returns BATCH_QUEUE_PROCESS_SYNC_NOT_OPEN then batch_queue_send_batch shall call on_item_complete for each item in the batch with BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED and NULL for the ll_result. ]*/
/* Tests_SRS_BATCH_QUEUE_45_007: [ batch_queue_send_batch shall free each item in the batch. ]*/
/* Tests_SRS_BATCH_QUEUE_45_008: [ batch_queue_send_batch shall reset the batch staging array size to 0. ]*/
/* Tests_SRS_BATCH_QUEUE_45_009: [ batch_queue_send_batch shall call sm_fault. ]*/
/* Tests_SRS_BATCH_QUEUE_42_081: [ batch_queue_send_batch shall decrement the number of pending batches. ]*/
/* Tests_SRS_BATCH_QUEUE_42_082: [ batch_queue_send_batch shall call sm_exec_end. ]*/
/* Tests_SRS_BATCH_QUEUE_42_083: [ batch_queue_send_batch shall call on_batch_faulted_func with on_batch_faulted_context. ]*/
/* Tests_SRS_BATCH_QUEUE_42_084: [ batch_queue_send_batch shall return a non-zero value. ]*/
TEST_FUNCTION(batch_queue_worker_thread_item_queued_send_batch_batch_process_call_returns_not_open)
{
    // arrange
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    void* on_faulted_context;

    void* error_producing_context;
    void* on_item_complete_context;
    BATCH_QUEUE_ON_LL_BATCH_COMPLETE process_batch_ll_callback_func = NULL;
    void* process_batch_ll_callback_context = NULL;

    // default settings should trigger batch_queue_send_batch to be called on second iteration
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(default_settings, mocked_queue_process_batch_func, (void*)&error_producing_context, mocked_on_faulted_callback, (void*)&on_faulted_context, &worker_thread, &worker_context);
    test_batch_queue_open(test_handle);
    uint64_t value1 = 0x10001;
    uint64_t value2 = 0x10002;
    test_batch_enqueue_item(test_handle, &value1, mocked_on_item_complete_callback, &on_item_complete_context);
    test_batch_enqueue_item(test_handle, &value2, mocked_on_item_complete_callback, &on_item_complete_context);
    umock_c_reset_all_calls();

    // iteration 1
    setup_work_thread_iteration_1_add_to_staging();

    // iteration 2
    setup_work_thread_iteration_2_add_to_staging_send_batch(BATCH_QUEUE_PROCESS_SYNC_NOT_OPEN, &process_batch_ll_callback_func, &process_batch_ll_callback_context);

    // act
    worker_thread(worker_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_45_012: [ If the pending requests are greater than or equal to max_pending_requests then batch_queue_worker_thread shall exit the loop. ]*/
/* Tests_SRS_BATCH_QUEUE_42_061: [ If the batch staging array is not empty and the number of pending batches is less than max_pending_requests then: ]*/
TEST_FUNCTION(batch_queue_worker_thread_item_queued_two_batched_processes_hit_max_pending)
{
    // arrange
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    void* on_faulted_context;

    void* error_producing_context;
    void* on_item_complete_context;
    BATCH_QUEUE_ON_LL_BATCH_COMPLETE process_batch_ll_callback_func = NULL;
    void* process_batch_ll_callback_context = NULL;
    void** items;
    uint32_t item_count;
    BATCH_QUEUE_SETTINGS batch_settings = default_settings;
    batch_settings.max_pending_requests = 1;
    // default settings should trigger batch_queue_send_batch to be called on second iteration
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(batch_settings, mocked_queue_process_batch_func, (void*)&error_producing_context, mocked_on_faulted_callback, (void*)&on_faulted_context, &worker_thread, &worker_context);
    test_batch_queue_open(test_handle);
    uint64_t value1 = 0x10001;
    uint64_t value2 = 0x10002;
    uint64_t value3 = 0x10003;
    test_batch_enqueue_item(test_handle, &value1, mocked_on_item_complete_callback, &on_item_complete_context);
    test_batch_enqueue_item(test_handle, &value2, mocked_on_item_complete_callback, &on_item_complete_context);
    test_batch_enqueue_item(test_handle, &value3, mocked_on_item_complete_callback, &on_item_complete_context);
    umock_c_reset_all_calls();

    // iteration 1
    setup_work_thread_iteration_1_add_to_staging();

    // iteration 2
    setup_work_thread_iteration_2_add_to_staging_send_batch_with_items(BATCH_QUEUE_PROCESS_SYNC_OK, &process_batch_ll_callback_func, &process_batch_ll_callback_context, &items, &item_count);

    // iteration 3 - stop because we hit max pending
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    worker_thread(worker_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(uint32_t, 1, item_count);
    ASSERT_ARE_EQUAL(void_ptr, &value1, items[0]);

    // cleanup
    process_batch_ll_callback_func(process_batch_ll_callback_context, BATCH_QUEUE_PROCESS_COMPLETE_OK, NULL);
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_45_010: [ batch_queue_worker_thread shall call sm_exec_begin to check the timer. ]*/
/* Tests_SRS_BATCH_QUEUE_45_011: [ batch_queue_worker_thread shall call sm_exec_end when it is done checking the timer. ]*/
/* Tests_SRS_BATCH_QUEUE_42_056: [ If there are items in the batch staging array and the size of the dequeued item would cause the staging batch to exceed max_batch_size then: ]*/
/* Tests_SRS_BATCH_QUEUE_42_057: [ batch_queue_worker_thread shall call batch_queue_send_batch. ]*/
/* Tests_SRS_BATCH_QUEUE_42_058: [ If batch_queue_send_batch fails then batch_queue_worker_thread shall exit the loop. ]*/
/* Tests_SRS_BATCH_QUEUE_42_072: [ batch_queue_send_batch shall increment the number of pending batches. ]*/
/* Tests_SRS_BATCH_QUEUE_42_073: [ batch_queue_send_batch shall allocate a context for the batch and move the items from the batch staging array to the batch context. ]*/
/* Tests_SRS_BATCH_QUEUE_42_074: [ batch_queue_send_batch shall call sm_exec_begin. ]*/
/* Tests_SRS_BATCH_QUEUE_42_075: [ batch_queue_send_batch shall call process_batch_func with the batch context. ]*/
/* Tests_SRS_BATCH_QUEUE_42_076: [ If process_batch_func returns anything other than BATCH_QUEUE_PROCESS_SYNC_OK then: ]*/
/* Tests_SRS_BATCH_QUEUE_42_079: [ If process_batch_func returns anything else then batch_queue_send_batch shall call on_item_complete for each item in the batch with BATCH_QUEUE_PROCESS_COMPLETE_ERROR and NULL for the ll_result. ]*/
/* Tests_SRS_BATCH_QUEUE_45_007: [ batch_queue_send_batch shall free each item in the batch. ]*/
/* Tests_SRS_BATCH_QUEUE_45_008: [ batch_queue_send_batch shall reset the batch staging array size to 0. ]*/
/* Tests_SRS_BATCH_QUEUE_45_009: [ batch_queue_send_batch shall call sm_fault. ]*/
/* Tests_SRS_BATCH_QUEUE_42_081: [ batch_queue_send_batch shall decrement the number of pending batches. ]*/
/* Tests_SRS_BATCH_QUEUE_42_082: [ batch_queue_send_batch shall call sm_exec_end. ]*/
/* Tests_SRS_BATCH_QUEUE_42_083: [ batch_queue_send_batch shall call on_batch_faulted_func with on_batch_faulted_context. ]*/
/* Tests_SRS_BATCH_QUEUE_42_084: [ batch_queue_send_batch shall return a non-zero value. ]*/
TEST_FUNCTION(batch_queue_worker_thread_item_queued_send_batch_batch_process_call_returns_error)
{
    // arrange
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    volatile_atomic int32_t on_faulted_context;
    (void)real_interlocked_exchange(&on_faulted_context, 0);

    void* error_producing_context;
    void* on_item_complete_context;
    BATCH_QUEUE_ON_LL_BATCH_COMPLETE process_batch_ll_callback_func = NULL;
    void* process_batch_ll_callback_context = NULL;

    // default settings should trigger batch_queue_send_batch to be called on second iteration
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(default_settings, mocked_queue_process_batch_func, (void*)&error_producing_context, mocked_on_faulted_callback, (void*)&on_faulted_context, &worker_thread, &worker_context);
    test_batch_queue_open(test_handle);
    uint64_t value1 = 0x10001;
    uint64_t value2 = 0x10002;
    test_batch_enqueue_item(test_handle, &value1, mocked_on_item_complete_callback, &on_item_complete_context);
    test_batch_enqueue_item(test_handle, &value2, mocked_on_item_complete_callback, &on_item_complete_context);
    umock_c_reset_all_calls();

    // iteration 1
    setup_work_thread_iteration_1_add_to_staging();

    // iteration 2
    setup_work_thread_iteration_2_add_to_staging_send_batch(BATCH_QUEUE_PROCESS_SYNC_ERROR, &process_batch_ll_callback_func, &process_batch_ll_callback_context);

    // act
    worker_thread(worker_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    /*on_item_complete_context.expected_result = BATCH_QUEUE_PROCESS_COMPLETE_ABANDONED;*/
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_45_010: [ batch_queue_worker_thread shall call sm_exec_begin to check the timer. ]*/
/* Tests_SRS_BATCH_QUEUE_45_011: [ batch_queue_worker_thread shall call sm_exec_end when it is done checking the timer. ]*/
/* Tests_SRS_BATCH_QUEUE_42_056: [ If there are items in the batch staging array and the size of the dequeued item would cause the staging batch to exceed max_batch_size then: ]*/
/* Tests_SRS_BATCH_QUEUE_42_057: [ batch_queue_worker_thread shall call batch_queue_send_batch. ]*/
/* Tests_SRS_BATCH_QUEUE_42_058: [ If batch_queue_send_batch fails then batch_queue_worker_thread shall exit the loop. ]*/
/* Tests_SRS_BATCH_QUEUE_42_058: [ If batch_queue_send_batch fails then batch_queue_worker_thread shall exit the loop. ]*/
/* Tests_SRS_BATCH_QUEUE_42_072: [ batch_queue_send_batch shall increment the number of pending batches. ]*/
/* Tests_SRS_BATCH_QUEUE_42_073: [ batch_queue_send_batch shall allocate a context for the batch and move the items from the batch staging array to the batch context. ]*/
/* Tests_SRS_BATCH_QUEUE_42_110: [ If there are any other errors then batch_queue_send_batch shall fail and return a non-zero value. ]*/
TEST_FUNCTION(batch_queue_worker_thread_item_queued_send_batch_malloc_fails)
{
    // arrange
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    // default settings should trigger batch_queue_send_batch to be called on second iteration
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL, &worker_thread, &worker_context);
    test_batch_queue_open(test_handle);
    uint64_t value1 = 0x10001;
    uint64_t value2 = 0x10002;
    test_batch_enqueue_item(test_handle, &value1, mocked_on_item_complete_callback, NULL);
    test_batch_enqueue_item(test_handle, &value2, mocked_on_item_complete_callback, NULL);
    umock_c_reset_all_calls();

    // iteration 1
    setup_work_thread_iteration_1_add_to_staging();

    // iteration 2
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)).SetReturn(NULL);     //MALLOC_MULTI_FLEX_STRUCT(BATCH_CONTEXT)
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_ENSURE_CAPACITY(BATCH_ITEM_CONTEXT_HANDLE)(IGNORED_ARG, 2));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    worker_thread(worker_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);

}

/* Tests_SRS_BATCH_QUEUE_45_010: [ batch_queue_worker_thread shall call sm_exec_begin to check the timer. ]*/
/* Tests_SRS_BATCH_QUEUE_45_011: [ batch_queue_worker_thread shall call sm_exec_end when it is done checking the timer. ]*/
/* Tests_SRS_BATCH_QUEUE_42_056: [ If there are items in the batch staging array and the size of the dequeued item would cause the staging batch to exceed max_batch_size then: ]*/
/* Tests_SRS_BATCH_QUEUE_42_057: [ batch_queue_worker_thread shall call batch_queue_send_batch. ]*/
/* Tests_SRS_BATCH_QUEUE_42_058: [ If batch_queue_send_batch fails then batch_queue_worker_thread shall exit the loop. ]*/
/* Tests_SRS_BATCH_QUEUE_42_072: [ batch_queue_send_batch shall increment the number of pending batches. ]*/
/* Tests_SRS_BATCH_QUEUE_42_073: [ batch_queue_send_batch shall allocate a context for the batch and move the items from the batch staging array to the batch context. ]*/
/* Tests_SRS_BATCH_QUEUE_42_074: [ batch_queue_send_batch shall call sm_exec_begin. ]*/
TEST_FUNCTION(batch_queue_worker_thread_item_queued_send_batch_sm_exec_fails)
{
    // arrange
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    // default settings should trigger batch_queue_send_batch to be called on second iteration
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(default_settings, mocked_queue_process_batch_func, NULL, mocked_on_faulted_callback, NULL, &worker_thread, &worker_context);
    test_batch_queue_open(test_handle);
    uint64_t value1 = 0x10001;
    uint64_t value2 = 0x10002;
    test_batch_enqueue_item(test_handle, &value1, mocked_on_item_complete_callback, NULL);
    test_batch_enqueue_item(test_handle, &value2, mocked_on_item_complete_callback, NULL);
    umock_c_reset_all_calls();

    // iteration 1
    setup_work_thread_iteration_1_add_to_staging();

    // iteration 2
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG)).SetReturn(SM_EXEC_REFUSED);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_ENSURE_CAPACITY(BATCH_ITEM_CONTEXT_HANDLE)(IGNORED_ARG, 2));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    worker_thread(worker_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);

}

static void setup_batch_queue_worker_thread_item_queued_hits_min_time_wait(uint32_t min_wait_time, uint32_t staged_items, BATCH_QUEUE_ON_LL_BATCH_COMPLETE *process_batch_ll_callback_func, void** process_batch_ll_callback_context)
{
    // iteration 1 - dequeue item and put in staging
    setup_work_thread_iteration_1_add_to_staging();

    // iteration 2 - no new items, is not the min batch size, wait time is reached
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms()).SetReturn(min_wait_time + 1);

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_increment_64(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_queue_process_batch_func(IGNORED_ARG, IGNORED_ARG, staged_items, IGNORED_ARG, IGNORED_ARG)).CaptureArgumentValue_on_ll_batch_complete(process_batch_ll_callback_func).CaptureArgumentValue_batch_context(process_batch_ll_callback_context);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    //iteration 3 - force stop the worker thread.
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG)).SetReturn(SM_ERROR);
}

/* Tests_SRS_BATCH_QUEUE_42_054: [ batch_queue_worker_thread shall do the following until there are no more items in the queue: ]*/
/* Tests_SRS_BATCH_QUEUE_42_105: [ batch_queue_worker_thread shall call sm_exec_begin. ]*/
/* Tests_SRS_BATCH_QUEUE_42_055: [ batch_queue_worker_thread shall call mpsc_lock_free_queue_dequeue. ]*/
/* Tests_SRS_BATCH_QUEUE_42_061: [ If the batch staging array is not empty and the number of pending batches is less than max_pending_requests then: ]*/
/* Tests_SRS_BATCH_QUEUE_42_060: [ batch_queue_worker_thread shall add the dequeued item to the batch staging array, increment its count, and add the size of the item to the staging batch size. ]*/
/* Tests_SRS_BATCH_QUEUE_42_106: [ batch_queue_worker_thread shall call sm_exec_end. ]*/
/* Tests_SRS_BATCH_QUEUE_42_085: [ Otherwise (process_batch_func returns BATCH_QUEUE_PROCESS_SYNC_OK): ]*/
/* Tests_SRS_BATCH_QUEUE_42_086: [ batch_queue_send_batch shall reset the batch staging array size to 0. ]*/
/* Tests_SRS_BATCH_QUEUE_42_087: [ batch_queue_send_batch shall succeed and return 0. ]*/
TEST_FUNCTION(batch_queue_worker_thread_item_queued_hits_min_time_wait)
{
    // arrange
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    BATCH_QUEUE_SETTINGS batch_settings = {
        .max_pending_requests = 3,
        .max_batch_size = 100,
        .min_batch_size = 1,
        .min_wait_time = 10
    };
    void* process_batch_context;
    BATCH_QUEUE_ON_LL_BATCH_COMPLETE process_batch_ll_callback_func = NULL;
    void* process_batch_ll_callback_context = NULL;

    // batch_settings should not trigger batch_queue_send_batch
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(batch_settings, mocked_queue_process_batch_func, (void*) &process_batch_context, mocked_on_faulted_callback, NULL, &worker_thread, &worker_context);
    test_batch_queue_open(test_handle);
    uint64_t value1 = 0x10001;
    test_batch_enqueue_item(test_handle, &value1, mocked_on_item_complete_callback, NULL);
    umock_c_reset_all_calls();

    setup_batch_queue_worker_thread_item_queued_hits_min_time_wait(batch_settings.min_wait_time, 1, &process_batch_ll_callback_func, &process_batch_ll_callback_context);

    // act
    worker_thread(worker_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    process_batch_ll_callback_func(process_batch_ll_callback_context, BATCH_QUEUE_PROCESS_COMPLETE_OK, NULL);
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_054: [ batch_queue_worker_thread shall do the following until there are no more items in the queue: ]*/
/* Tests_SRS_BATCH_QUEUE_42_105: [ batch_queue_worker_thread shall call sm_exec_begin. ]*/
/* Tests_SRS_BATCH_QUEUE_42_055: [ batch_queue_worker_thread shall call mpsc_lock_free_queue_dequeue. ]*/
/* Tests_SRS_BATCH_QUEUE_42_060: [ batch_queue_worker_thread shall add the dequeued item to the batch staging array, increment its count, and add the size of the item to the staging batch size. ]*/
/* Tests_SRS_BATCH_QUEUE_42_061: [ If the batch staging array is not empty and the number of pending batches is less than max_pending_requests then: ]*/
/* Tests_SRS_BATCH_QUEUE_42_062: [ If the time since the first item in the batch staging array was enqueued is greater than or equal to min_wait_time then batch_queue_worker_thread shall call batch_queue_send_batch. ]*/
/* Tests_SRS_BATCH_QUEUE_42_063: [ Otherwise if the size of the batch staging array is greater than or equal to min_batch_size then batch_queue_worker_thread shall call batch_queue_send_batch. ]*/
/* Tests_SRS_BATCH_QUEUE_42_064: [ Otherwise batch_queue_worker_thread shall call threadpool_timer_restart with the difference between min_wait_time and the elapsed time of the first operation as the start_delay_ms and 0 as the timer_period_ms. ]*/
/* Tests_SRS_BATCH_QUEUE_42_106: [ batch_queue_worker_thread shall call sm_exec_end. ]*/
TEST_FUNCTION(batch_queue_worker_thread_item_queued_hits_min_time_wait_send_batch_fails)
{
    // arrange
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    BATCH_QUEUE_SETTINGS batch_settings = {
        .max_pending_requests = 3,
        .max_batch_size = 100,
        .min_batch_size = 1,
        .min_wait_time = 10
    };
    volatile_atomic int32_t process_batch_context = 0;
    // batch_settings should not trigger batch_queue_send_batch
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(batch_settings, mocked_queue_process_batch_func, (void*) &process_batch_context, mocked_on_faulted_callback, NULL, &worker_thread, &worker_context);
    test_batch_queue_open(test_handle);
    uint64_t value1 = 0x10001;
    test_batch_enqueue_item(test_handle, &value1, mocked_on_item_complete_callback, NULL);
    umock_c_reset_all_calls();

    // iteration 1 - dequeue item and put in staging
    setup_work_thread_iteration_1_add_to_staging();

    // iteration 2 - no new items, is not the min batch size, wait time is reached
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms()).SetReturn(11);

    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    worker_thread(worker_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int32_t, 0, real_interlocked_add(&process_batch_context, 0));

    // cleanup
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_061: [ If the batch staging array is not empty and the number of pending batches is less than max_pending_requests then: ]*/
TEST_FUNCTION(batch_queue_worker_thread_item_queued_timeout_not_reached)
{
    // arrange
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    BATCH_QUEUE_SETTINGS batch_settings = {
        .max_pending_requests = 5,
        .max_batch_size = 100,
        .min_batch_size = 30,
        .min_wait_time = 10
    };
    void* process_batch_context;
    //BATCH_QUEUE_ON_LL_BATCH_COMPLETE process_batch_ll_callback_func = NULL;
    //void* process_batch_ll_callback_context = NULL;

    // batch_settings should not trigger batch_queue_send_batch
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(batch_settings, mocked_queue_process_batch_func, (void*) &process_batch_context, mocked_on_faulted_callback, NULL, &worker_thread, &worker_context);
    test_batch_queue_open(test_handle);
    uint64_t value1 = 0x10001;
    test_batch_enqueue_item(test_handle, &value1, mocked_on_item_complete_callback, NULL);
    umock_c_reset_all_calls();

    // iteration 1 - dequeue item and put in staging
    setup_work_thread_iteration_1_add_to_staging();

    // iteration 2 - no new items, is not the min batch size, wait time is not reached, set timer
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, 0));
    STRICT_EXPECTED_CALL(mpsc_lock_free_queue_dequeue(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms()).SetReturn(5);
    STRICT_EXPECTED_CALL(threadpool_timer_restart(IGNORED_ARG, 4, 0));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
    //STRICT_EXPECTED_CALL(mocked_queue_process_batch_func(IGNORED_ARG, IGNORED_ARG, 1, IGNORED_ARG, IGNORED_ARG)).CaptureArgumentValue_on_ll_batch_complete(&process_batch_ll_callback_func).CaptureArgumentValue_batch_context(&process_batch_ll_callback_context);

    //iteration 3 - force stop the worker thread.
    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG)).SetReturn(SM_ERROR);

    // act
    worker_thread(worker_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    //process_batch_ll_callback_func(process_batch_ll_callback_context, BATCH_QUEUE_PROCESS_COMPLETE_OK, NULL);
    //free(process_batch_ll_callback_context);
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_088: [ If context is NULL then batch_queue_on_ll_batch_complete shall terminate the process. ]*/
TEST_FUNCTION(batch_queue_worker_thread_item_queued_and_batched_ll_queue_is_given_null)
{
    // arrange
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    BATCH_QUEUE_SETTINGS batch_settings = {
        .max_pending_requests = 3,
        .max_batch_size = 100,
        .min_batch_size = 1,
        .min_wait_time = 10
    };
    void* process_batch_context;
    // batch_settings should not trigger batch_queue_send_batch
    BATCH_QUEUE_ON_LL_BATCH_COMPLETE process_batch_ll_callback_func = NULL;
    void* process_batch_ll_callback_context = NULL;

    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(batch_settings, mocked_queue_process_batch_func, (void*)&process_batch_context, mocked_on_faulted_callback, NULL, &worker_thread, &worker_context);
    test_batch_queue_open(test_handle);
    uint64_t value1 = 0x10001;
    test_batch_enqueue_item(test_handle, &value1, mocked_on_item_complete_callback, NULL);
    umock_c_reset_all_calls();

    setup_batch_queue_worker_thread_item_queued_hits_min_time_wait(batch_settings.min_wait_time, 1, &process_batch_ll_callback_func, &process_batch_ll_callback_context);

    worker_thread(worker_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(ps_util_terminate_process());

    // act
    process_batch_ll_callback_func(NULL, BATCH_QUEUE_PROCESS_COMPLETE_OK, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //// cleanup
    process_batch_ll_callback_func(process_batch_ll_callback_context, BATCH_QUEUE_PROCESS_COMPLETE_OK, NULL);
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_089: [ For each item in the batch: ]*/
/* Tests_SRS_BATCH_QUEUE_42_090: [ batch_queue_on_ll_batch_complete shall call on_item_complete with the result and ll_result for the item. ]*/
/* Tests_SRS_BATCH_QUEUE_42_091: [ batch_queue_on_ll_batch_complete shall free the memory allocated during enqueue of the item. ]*/
/* Tests_SRS_BATCH_QUEUE_42_092: [ batch_queue_on_ll_batch_complete shall decrement the number of pending batches. ]*/
/* Tests_SRS_BATCH_QUEUE_42_093: [ batch_queue_on_ll_batch_complete shall call sm_exec_end. ]*/
/* Tests_SRS_BATCH_QUEUE_42_094: [ batch_queue_on_ll_batch_complete shall call tp_worker_thread_schedule_process. ]*/
/* Tests_SRS_BATCH_QUEUE_45_006: [ batch_queue_on_ll_batch_complete shall free the batch context. ]*/
TEST_FUNCTION(batch_queue_worker_thread_item_queued_and_batched_ll_queue_is_given_OK)
{
    // arrange
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    BATCH_QUEUE_SETTINGS batch_settings = {
        .max_pending_requests = 3,
        .max_batch_size = 100,
        .min_batch_size = 1,
        .min_wait_time = 10
    };
    void* process_batch_context;
    void* on_item_complete_context;
    BATCH_QUEUE_ON_LL_BATCH_COMPLETE process_batch_ll_callback_func = NULL;
    void* process_batch_ll_callback_context = NULL;

    // batch_settings should not trigger batch_queue_send_batch
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(batch_settings, mocked_queue_process_batch_func, (void*)&process_batch_context, mocked_on_faulted_callback, NULL, &worker_thread, &worker_context);
    test_batch_queue_open(test_handle);
    uint64_t value1 = 0x10001;
    test_batch_enqueue_item(test_handle, &value1, mocked_on_item_complete_callback, &on_item_complete_context);
    umock_c_reset_all_calls();

    setup_batch_queue_worker_thread_item_queued_hits_min_time_wait(batch_settings.min_wait_time, 1, &process_batch_ll_callback_func, &process_batch_ll_callback_context);

    worker_thread(worker_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_on_item_complete_callback(&on_item_complete_context, BATCH_QUEUE_PROCESS_COMPLETE_OK, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
    STRICT_EXPECTED_CALL(tp_worker_thread_schedule_process(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    process_batch_ll_callback_func(process_batch_ll_callback_context, BATCH_QUEUE_PROCESS_COMPLETE_OK, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);
}

/* Tests_SRS_BATCH_QUEUE_42_089: [ For each item in the batch: ]*/
/* Tests_SRS_BATCH_QUEUE_42_090: [ batch_queue_on_ll_batch_complete shall call on_item_complete with the result and ll_result for the item. ]*/
/* Tests_SRS_BATCH_QUEUE_42_091: [ batch_queue_on_ll_batch_complete shall free the memory allocated during enqueue of the item. ]*/
/* Tests_SRS_BATCH_QUEUE_42_092: [ batch_queue_on_ll_batch_complete shall decrement the number of pending batches. ]*/
/* Tests_SRS_BATCH_QUEUE_42_093: [ batch_queue_on_ll_batch_complete shall call sm_exec_end. ]*/
/* Tests_SRS_BATCH_QUEUE_42_094: [ batch_queue_on_ll_batch_complete shall call tp_worker_thread_schedule_process. ]*/
/* Tests_SRS_BATCH_QUEUE_45_006: [ batch_queue_on_ll_batch_complete shall free the batch context. ]*/
TEST_FUNCTION(batch_queue_worker_thread_item_queued_and_batched_ll_queue_tp_worker_error)
{
    // arrange
    THREADPOOL_WORK_FUNCTION worker_thread;
    void* worker_context;
    BATCH_QUEUE_SETTINGS batch_settings = {
        .max_pending_requests = 3,
        .max_batch_size = 100,
        .min_batch_size = 1,
        .min_wait_time = 10
    };
    void* process_batch_context;
    void* on_faulted_context;
    void* on_item_complete_context;
    BATCH_QUEUE_ON_LL_BATCH_COMPLETE process_batch_ll_callback_func = NULL;
    void* process_batch_ll_callback_context = NULL;

    // batch_settings should not trigger batch_queue_send_batch
    BATCH_QUEUE_HANDLE test_handle = test_batch_queue_create_and_get_worker_thread(batch_settings, mocked_queue_process_batch_func, (void*)&process_batch_context, mocked_on_faulted_callback, (void*)&on_faulted_context, &worker_thread, &worker_context);
    test_batch_queue_open(test_handle);
    uint64_t value1 = 0x10001;
    test_batch_enqueue_item(test_handle, &value1, mocked_on_item_complete_callback, &on_item_complete_context);
    umock_c_reset_all_calls();

    setup_batch_queue_worker_thread_item_queued_hits_min_time_wait(batch_settings.min_wait_time, 1, &process_batch_ll_callback_func, &process_batch_ll_callback_context);

    worker_thread(worker_context);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_on_item_complete_callback(&on_item_complete_context, BATCH_QUEUE_PROCESS_COMPLETE_OK, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));
    STRICT_EXPECTED_CALL(tp_worker_thread_schedule_process(IGNORED_ARG)).SetReturn(TP_WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    process_batch_ll_callback_func(process_batch_ll_callback_context, BATCH_QUEUE_PROCESS_COMPLETE_OK, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    batch_queue_close(test_handle);
    batch_queue_destroy(test_handle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
