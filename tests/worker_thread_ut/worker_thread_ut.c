// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>


#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "real_gballoc_ll.h"
static void* my_gballoc_malloc(size_t size)
{
    return real_gballoc_ll_malloc(size);
}

static void my_gballoc_free(void* ptr)
{
     real_gballoc_ll_free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/sm.h"
#include "c_util/singlylinkedlist.h"
#include "c_pal/threadapi.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_sm.h"

#include "c_util/worker_thread.h"

static THREAD_HANDLE test_thread_handle = (THREAD_HANDLE)0x4200;
static THREAD_START_FUNC saved_worker_thread_func;
static void* saved_worker_thread_func_context;

THREADAPI_RESULT my_ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
    saved_worker_thread_func = func;
    saved_worker_thread_func_context = arg;
    *threadHandle = test_thread_handle;
    return THREADAPI_OK;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_RESULT_VALUES);

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

    MOCK_FUNCTION_WITH_CODE(, void, test_worker_function, void*, worker_function_context)
        MOCK_FUNCTION_END();

    MOCK_FUNCTION_WITH_CODE(, HANDLE, mocked_CreateEventA,
        LPSECURITY_ATTRIBUTES, lpEventAttributes,
        BOOL, bManualReset,
        BOOL, bInitialState,
        LPCSTR, lpName)
        HANDLE my_result = CreateEventA(lpEventAttributes, bManualReset, bInitialState, lpName);
    MOCK_FUNCTION_END(my_result);

    MOCK_FUNCTION_WITH_CODE(, BOOL, mocked_CloseHandle,
        HANDLE, hObject)
        BOOL my_result = CloseHandle(hObject);
    MOCK_FUNCTION_END(my_result);

    MOCK_FUNCTION_WITH_CODE(, BOOL, mocked_SetEvent,
        HANDLE, hObject)
        BOOL my_result = SetEvent(hObject);
    MOCK_FUNCTION_END(my_result);

    MOCK_FUNCTION_WITH_CODE(, DWORD, mocked_WaitForMultipleObjects,
        DWORD, nCount,
        CONST HANDLE*, lpHandles,
        BOOL, bWaitAll,
        DWORD, dwMilliseconds)
    MOCK_FUNCTION_END(WAIT_OBJECT_0 + 1);

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");

    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types");

    REGISTER_GLOBAL_MOCK_HOOK(malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(free, my_gballoc_free);

    REGISTER_GLOBAL_MOCK_HOOK(ThreadAPI_Create, my_ThreadAPI_Create);
    REGISTER_GLOBAL_MOCK_RETURN(ThreadAPI_Join, THREADAPI_OK);

    REGISTER_SM_GLOBAL_MOCK_HOOK();

    REGISTER_UMOCK_ALIAS_TYPE(LPSECURITY_ATTRIBUTES, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BOOL, long);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_START_FUNC, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPCSTR, const char*);
    REGISTER_UMOCK_ALIAS_TYPE(THREAD_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DWORD, unsigned long);
    REGISTER_UMOCK_ALIAS_TYPE(SM_HANDLE, void*);
    
    REGISTER_TYPE(THREADAPI_RESULT, THREADAPI_RESULT);
    REGISTER_TYPE(SM_RESULT, SM_RESULT);
    REGISTER_TYPE(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_RESULT);
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

/* worker_thread_create */

/* Tests_SRS_WORKER_THREAD_01_001: [ worker_thread_create shall allocate memory for a new worker thread object and on success return a non-NULL handle to it. ]*/
/* Tests_SRS_WORKER_THREAD_01_022: [ worker_thread_create shall perform the following actions in order: ]*/
    /* Tests_SRS_WORKER_THREAD_01_037: [ worker_thread_create shall create a state manager object by calling sm_create with the name worker_thread. ]*/
    /* Tests_SRS_WORKER_THREAD_01_006: [ worker_thread_create shall create an auto reset unnamed event used for signaling the thread to shutdown by calling CreateEvent. ]*/
    /* Tests_SRS_WORKER_THREAD_01_007: [ worker_thread_create shall create an auto reset unnamed event used for signaling that a new execution of the worker function should be done by calling CreateEvent. ]*/
TEST_FUNCTION(worker_thread_create_succeeds)
{
    // arrange
    WORKER_THREAD_HANDLE worker_thread;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create("worker_thread"));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));

    // act
    worker_thread = worker_thread_create(test_worker_function, (void*)0x4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(worker_thread);

    // cleanup
    worker_thread_destroy(worker_thread);
}

/* Tests_SRS_WORKER_THREAD_01_003: [ If worker_func is NULL, worker_thread_create shall fail and return NULL. ]*/
TEST_FUNCTION(worker_thread_create_with_NULL_worker_function_fails)
{
    // arrange
    WORKER_THREAD_HANDLE worker_thread;

    // act
    worker_thread = worker_thread_create(NULL, (void*)0x4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(worker_thread);

    // cleanup
    worker_thread_destroy(worker_thread);
}

/* Tests_SRS_WORKER_THREAD_01_004: [ worker_func_context shall be allowed to be NULL. ]*/
TEST_FUNCTION(worker_thread_create_with_NULL_worker_func_context_succeeds)
{
    // arrange
    WORKER_THREAD_HANDLE worker_thread;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create("worker_thread"));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL));

    // act
    worker_thread = worker_thread_create(test_worker_function, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(worker_thread);

    // cleanup
    worker_thread_destroy(worker_thread);
}

/* Tests_SRS_WORKER_THREAD_01_008: [ If any error occurs, worker_thread_create shall fail and return NULL. ]*/
TEST_FUNCTION(when_the_underlying_calls_fails_worker_thread_create_fails)
{
    // arrange
    size_t i;
    WORKER_THREAD_HANDLE worker_thread;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(sm_create("worker_thread"))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL))
        .SetFailReturn(NULL);

    umock_c_negative_tests_snapshot();

    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            worker_thread = worker_thread_create(test_worker_function, (void*)0x4242);

            // assert
            ASSERT_IS_NULL(worker_thread, "On failed call %zu", i);
        }
    }
}

/* worker_thread_destroy */

/* Tests_SRS_WORKER_THREAD_01_009: [ worker_thread_destroy shall free the resources associated with the worker thread handle. ]*/
/* Tests_SRS_WORKER_THREAD_01_013: [ worker_thread_destroy shall free the events created in worker_thread_create by calling CloseHandle on them. ]*/
/* Tests_SRS_WORKER_THREAD_01_038: [ worker_thread_destroy shall destroy the state manager object created in worker_thread_create. ]*/
TEST_FUNCTION(worker_thread_destroy_frees_the_resources_allocated_by_create)
{
    // arrange
    HANDLE shutdown_event;
    HANDLE execute_work_event;
    SM_HANDLE sm;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create("worker_thread"))
        .CaptureReturn(&sm);
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL))
        .CaptureReturn(&shutdown_event);
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL))
        .CaptureReturn(&execute_work_event);
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_threadHandle(&test_thread_handle, sizeof(test_thread_handle));

    WORKER_THREAD_HANDLE worker_thread = worker_thread_create(test_worker_function, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_close_begin(sm));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(execute_work_event));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(shutdown_event));
    STRICT_EXPECTED_CALL(sm_destroy(sm));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    worker_thread_destroy(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_WORKER_THREAD_01_010: [ If worker_thread is NULL, worker_thread_destroy shall return. ]*/
TEST_FUNCTION(worker_thread_destroy_with_NULL_handle_returns_without_freeing_resources)
{
    // arrange

    // act
    worker_thread_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_WORKER_THREAD_01_023: [ Any errors in worker_thread_destroy shall be ignored. ]*/
TEST_FUNCTION(when_closing_the_shutdown_event_handle_fails_worker_thread_destroy_ignores_the_error)
{
    // arrange
    HANDLE shutdown_event;
    HANDLE execute_work_event;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create("worker_thread"));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL))
        .CaptureReturn(&shutdown_event);
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL))
        .CaptureReturn(&execute_work_event);

    WORKER_THREAD_HANDLE worker_thread = worker_thread_create(test_worker_function, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(execute_work_event))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_CloseHandle(shutdown_event));
    STRICT_EXPECTED_CALL(sm_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    worker_thread_destroy(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_WORKER_THREAD_01_023: [ Any errors in worker_thread_destroy shall be ignored. ]*/
TEST_FUNCTION(when_closing_the_execute_work_event_handle_fails_worker_thread_destroy_ignores_the_error)
{
    // arrange
    HANDLE shutdown_event;
    HANDLE execute_work_event;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create("worker_thread"));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL))
        .CaptureReturn(&shutdown_event);
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL))
        .CaptureReturn(&execute_work_event);

    WORKER_THREAD_HANDLE worker_thread = worker_thread_create(test_worker_function, (void*)0x4242);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(execute_work_event));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(shutdown_event))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(sm_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    worker_thread_destroy(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

static WORKER_THREAD_HANDLE setup_worker_thread_create(HANDLE* shutdown_event, HANDLE* execute_work_event)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_create("worker_thread"));
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL))
        .CaptureReturn(shutdown_event);
    STRICT_EXPECTED_CALL(mocked_CreateEventA(NULL, FALSE, FALSE, NULL))
        .CaptureReturn(execute_work_event);

    WORKER_THREAD_HANDLE worker_thread = worker_thread_create(test_worker_function, (void*)0x4242);
    ASSERT_IS_NOT_NULL(worker_thread);
    umock_c_reset_all_calls();

    return worker_thread;
}

static WORKER_THREAD_HANDLE setup_worker_thread_open(HANDLE* shutdown_event, HANDLE* execute_work_event)
{
    WORKER_THREAD_HANDLE worker_thread = setup_worker_thread_create(shutdown_event, execute_work_event);

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_threadHandle(&test_thread_handle, sizeof(test_thread_handle));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));

    ASSERT_ARE_EQUAL(int, 0, worker_thread_open(worker_thread));
    umock_c_reset_all_calls();

    return worker_thread;
}

/* Tests_SRS_WORKER_THREAD_01_039: [ If the worker thread is open, worker_thread_destroy shall perform a close. ]*/
TEST_FUNCTION(destroy_also_closes_the_worker_thread)
{
    // arrange
    HANDLE shutdown_event;
    HANDLE execute_work_event;

    WORKER_THREAD_HANDLE worker_thread = setup_worker_thread_open(&shutdown_event, &execute_work_event);

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetEvent(shutdown_event));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(test_thread_handle, IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(execute_work_event));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(shutdown_event))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(sm_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    worker_thread_destroy(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* worker_thread_open */

/* Tests_SRS_WORKER_THREAD_01_026: [ If worker_thread is NULL, worker_thread_open shall fail and return a non-zero value. ]*/
TEST_FUNCTION(worker_thread_open_with_NULL_worker_thread_fails)
{
    // arrange

    // act
    int result = worker_thread_open(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_WORKER_THREAD_01_027: [ Otherwise, worker_thread_open shall call sm_open_begin. ]*/
/* Tests_SRS_WORKER_THREAD_01_028: [ worker_thread_open shall start a worker_thread thread that will call worker_func by calling ThreadAPI_Create. ]*/
/* Tests_SRS_WORKER_THREAD_01_029: [ worker_thread_open shall call sm_open_end with success reflecting whether any error has occured during the open. ]*/
/* Tests_SRS_WORKER_THREAD_01_030: [ On success, worker_thread_open shall return 0. ]*/
TEST_FUNCTION(worker_thread_open_succeeds)
{
    HANDLE shutdown_event;
    HANDLE execute_work_event;

    // arrange
    WORKER_THREAD_HANDLE worker_thread = setup_worker_thread_create(&shutdown_event, &execute_work_event);

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_threadHandle(&test_thread_handle, sizeof(test_thread_handle));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));

    // act
    int result = worker_thread_open(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    worker_thread_destroy(worker_thread);
}

/* Tests_SRS_WORKER_THREAD_01_031: [ If any error occurs, worker_thread_open shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_underlying_calls_fail_open_fails)
{
    HANDLE shutdown_event;
    HANDLE execute_work_event;
    size_t i;

    // arrange
    WORKER_THREAD_HANDLE worker_thread = setup_worker_thread_create(&shutdown_event, &execute_work_event);

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG))
        .SetFailReturn(SM_EXEC_REFUSED);
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_threadHandle(&test_thread_handle, sizeof(test_thread_handle))
        .SetFailReturn(THREADAPI_ERROR);
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));

    umock_c_negative_tests_snapshot();

    for (i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            int result = worker_thread_open(worker_thread);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
        }
    }

    // cleanup
    worker_thread_destroy(worker_thread);
}

/* Tests_SRS_WORKER_THREAD_01_029: [ worker_thread_open shall call sm_open_end with success reflecting whether any error has occured during the open. ]*/
TEST_FUNCTION(when_ThreadAPI_Create_fails_sm_open_end_is_called_with_false)
{
    HANDLE shutdown_event;
    HANDLE execute_work_event;

    // arrange
    WORKER_THREAD_HANDLE worker_thread = setup_worker_thread_create(&shutdown_event, &execute_work_event);

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_threadHandle(&test_thread_handle, sizeof(test_thread_handle))
        .SetReturn(THREADAPI_ERROR);
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, false));

    // act
    int result = worker_thread_open(worker_thread);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    worker_thread_destroy(worker_thread);
}

/* Tests_SRS_WORKER_THREAD_01_031: [ If any error occurs, worker_thread_open shall fail and return a non-zero value. ]*/
TEST_FUNCTION(open_after_open_fails)
{
    HANDLE shutdown_event;
    HANDLE execute_work_event;

    // arrange
    WORKER_THREAD_HANDLE worker_thread = setup_worker_thread_open(&shutdown_event, &execute_work_event);

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));

    // act
    int result = worker_thread_open(worker_thread);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    worker_thread_destroy(worker_thread);
}

/* Tests_SRS_WORKER_THREAD_01_027: [ Otherwise, worker_thread_open shall call sm_open_begin. ]*/
/* Tests_SRS_WORKER_THREAD_01_028: [ worker_thread_open shall start a worker_thread thread that will call worker_func by calling ThreadAPI_Create. ]*/
/* Tests_SRS_WORKER_THREAD_01_029: [ worker_thread_open shall call sm_open_end with success reflecting whether any error has occured during the open. ]*/
/* Tests_SRS_WORKER_THREAD_01_030: [ On success, worker_thread_open shall return 0. ]*/
TEST_FUNCTION(a_seconds_open_after_a_failed_open_succeeds)
{
    HANDLE shutdown_event;
    HANDLE execute_work_event;

    // arrange
    WORKER_THREAD_HANDLE worker_thread = setup_worker_thread_create(&shutdown_event, &execute_work_event);

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_threadHandle(&test_thread_handle, sizeof(test_thread_handle))
        .SetReturn(THREADAPI_ERROR);
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, false));
    ASSERT_ARE_NOT_EQUAL(int, 0, worker_thread_open(worker_thread));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(sm_open_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ThreadAPI_Create(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .CopyOutArgumentBuffer_threadHandle(&test_thread_handle, sizeof(test_thread_handle));
    STRICT_EXPECTED_CALL(sm_open_end(IGNORED_ARG, true));

    // act
    int result = worker_thread_open(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    worker_thread_destroy(worker_thread);
}

/* worker_thread_close */

/* Tests_SRS_WORKER_THREAD_01_032: [ If worker_thread is NULL, worker_thread_close shall return. ]*/
TEST_FUNCTION(worker_thread_close_with_NULL_worker_thread_returns)
{
    // arrange
    // act
    worker_thread_close(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_WORKER_THREAD_01_033: [ Otherwise, worker_thread_close shall call sm_close_begin. ]*/
/* Tests_SRS_WORKER_THREAD_01_034: [ worker_thread_close shall signal the thread shutdown event in order to indicate that the thread shall shutdown. ]*/
/* Tests_SRS_WORKER_THREAD_01_035: [ worker_thread_close shall wait for the thread to join by using ThreadAPI_Join. ]*/
/* Tests_SRS_WORKER_THREAD_01_036: [ worker_thread_close shall call sm_close_end. ]*/
TEST_FUNCTION(worker_thread_close_closes_the_worker_thread)
{
    HANDLE shutdown_event;
    HANDLE execute_work_event;

    // arrange
    WORKER_THREAD_HANDLE worker_thread = setup_worker_thread_open(&shutdown_event, &execute_work_event);

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetEvent(shutdown_event));
    STRICT_EXPECTED_CALL(ThreadAPI_Join(test_thread_handle, IGNORED_ARG));
    STRICT_EXPECTED_CALL(sm_close_end(IGNORED_ARG));

    // act
    worker_thread_close(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    worker_thread_destroy(worker_thread);
}

/* Tests_SRS_WORKER_THREAD_01_040: [ If sm_close_begin does not return SM_EXEC_GRANTED, worker_thread_close shall return. ]*/
TEST_FUNCTION(when_sm_close_begin_does_not_return_granted_worker_thread_close_returns)
{
    HANDLE shutdown_event;
    HANDLE execute_work_event;

    // arrange
    WORKER_THREAD_HANDLE worker_thread = setup_worker_thread_open(&shutdown_event, &execute_work_event);

    STRICT_EXPECTED_CALL(sm_close_begin(IGNORED_ARG))
        .SetReturn(SM_EXEC_REFUSED);

    // act
    worker_thread_close(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    worker_thread_destroy(worker_thread);
}

/* worker_thread_schedule_process */

/* Tests_SRS_WORKER_THREAD_01_016: [ If worker_thread is NULL, worker_thread_schedule_process shall fail and return WORKER_THREAD_SCHEDULE_PROCESS_ERROR. ]*/
TEST_FUNCTION(worker_thread_schedule_process_with_NULL_handle_fails)
{
    // arrange
    int result;

    // act
    result = worker_thread_schedule_process(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_WORKER_THREAD_01_041: [ Otherwise worker_thread_schedule_process shall call sm_exec_begin. ]*/
/* Tests_SRS_WORKER_THREAD_01_017: [ worker_thread_schedule_process shall set the process event created in worker_thread_create. ]*/
/* Tests_SRS_WORKER_THREAD_01_043: [ worker_thread_schedule_process shall call sm_exec_end. ]*/
/* Tests_SRS_WORKER_THREAD_01_015: [ On success worker_thread_schedule_process shall return WORKER_THREAD_SCHEDULE_PROCESS_OK. ]*/
TEST_FUNCTION(worker_thread_schedule_process_succeeds)
{
    // arrange
    HANDLE shutdown_event;
    HANDLE execute_work_event;
    int result;

    WORKER_THREAD_HANDLE worker_thread = setup_worker_thread_open(&shutdown_event, &execute_work_event);

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetEvent(execute_work_event));
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    result = worker_thread_schedule_process(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_OK, result);

    // cleanup
    worker_thread_destroy(worker_thread);
}

/* Tests_SRS_WORKER_THREAD_01_018: [ If any other error occurs, worker_thread_schedule_process shall fail and return WORKER_THREAD_SCHEDULE_PROCESS_ERROR. ]*/
TEST_FUNCTION(when_setting_the_event_fails_worker_thread_schedule_process_also_fails)
{
    // arrange
    HANDLE shutdown_event;
    HANDLE execute_work_event;
    int result;

    WORKER_THREAD_HANDLE worker_thread = setup_worker_thread_open(&shutdown_event, &execute_work_event);

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_SetEvent(execute_work_event))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(sm_exec_end(IGNORED_ARG));

    // act
    result = worker_thread_schedule_process(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

    // cleanup
    worker_thread_destroy(worker_thread);
}

/* Tests_SRS_WORKER_THREAD_01_042: [ If sm_exec_begin does not grant the execution, worker_thread_schedule_process shall fail and return WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE. ]*/
TEST_FUNCTION(when_sm_foes_not_grant_the_execution_worker_thread_schedule_process_fails)
{
    // arrange
    HANDLE shutdown_event;
    HANDLE execute_work_event;
    int result;

    WORKER_THREAD_HANDLE worker_thread = setup_worker_thread_open(&shutdown_event, &execute_work_event);

    STRICT_EXPECTED_CALL(sm_exec_begin(IGNORED_ARG))
        .SetReturn(SM_EXEC_REFUSED);

    // act
    result = worker_thread_schedule_process(worker_thread);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(WORKER_THREAD_SCHEDULE_PROCESS_RESULT, WORKER_THREAD_SCHEDULE_PROCESS_INVALID_STATE, result);

    // cleanup
    worker_thread_destroy(worker_thread);
}

/* worker_thread */

/* Tests_SRS_WORKER_THREAD_01_019: [ The worker thread started by worker_thread_create shall wait for the 2 events: thread shutdown event and execute worker function event. ]*/
/* Tests_SRS_WORKER_THREAD_01_021: [ When the execute worker function event is signaled, the worker thread shall call the worker_func function passed to worker_thread_create and it shall pass worker_func_context as argument. ]*/
TEST_FUNCTION(when_the_execute_work_event_is_signaled_the_worker_thread_executes_the_work_function)
{
    // arrange
    HANDLE shutdown_event;
    HANDLE execute_work_event;
    int result;

    WORKER_THREAD_HANDLE worker_thread = setup_worker_thread_open(&shutdown_event, &execute_work_event);
    result = worker_thread_schedule_process(worker_thread);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_WaitForMultipleObjects(2, IGNORED_ARG, FALSE, INFINITE));
    STRICT_EXPECTED_CALL(test_worker_function((void*)0x4242));
    STRICT_EXPECTED_CALL(mocked_WaitForMultipleObjects(2, IGNORED_ARG, FALSE, INFINITE))
        .SetReturn(WAIT_OBJECT_0);

    // act
    saved_worker_thread_func(saved_worker_thread_func_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    worker_thread_destroy(worker_thread);
}

/* Tests_SRS_WORKER_THREAD_01_020: [ When the shutdown event is signaled, the worker thread shall exit. ]*/
TEST_FUNCTION(when_the_shutdown_event_is_signaled_the_worker_thread_exits)
{
    // arrange
    HANDLE shutdown_event;
    HANDLE execute_work_event;

    WORKER_THREAD_HANDLE worker_thread = setup_worker_thread_open(&shutdown_event, &execute_work_event);

    STRICT_EXPECTED_CALL(mocked_WaitForMultipleObjects(2, IGNORED_ARG, FALSE, INFINITE))
        .SetReturn(WAIT_OBJECT_0);

    // act
    saved_worker_thread_func(saved_worker_thread_func_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    worker_thread_destroy(worker_thread);
}

/* Tests_SRS_WORKER_THREAD_01_025: [ In case of any error, the worker thread shall exit. ]*/
TEST_FUNCTION(when_WaitForMultipleObjects_fails_the_worker_thread_exits)
{
    // arrange
    HANDLE shutdown_event;
    HANDLE execute_work_event;

    WORKER_THREAD_HANDLE worker_thread = setup_worker_thread_open(&shutdown_event, &execute_work_event);

    STRICT_EXPECTED_CALL(mocked_WaitForMultipleObjects(2, IGNORED_ARG, FALSE, INFINITE))
        .SetReturn(WAIT_FAILED);

    // act
    saved_worker_thread_func(saved_worker_thread_func_context);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    worker_thread_destroy(worker_thread);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
