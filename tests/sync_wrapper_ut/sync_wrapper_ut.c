// Copyright (c) Microsoft. All rights reserved.


#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>


#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_windows.h"
#include "umock_c/umock_c_negative_tests.h"

#include "c_pal/interlocked.h" /*included for mocking reasons - it will prohibit creation of mocks belonging to interlocked.h - at the moment verified through int tests - this is porting legacy code, temporary solution*/
#include "c_pal/sync.h" /*included for mocking reasons - it will prohibit creation of mocks belonging to sync.h - at the moment verified through int tests - this is porting legacy code, temporary solution*/

#define ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/interlocked_hl.h"
#include "c_pal/log_critical_and_terminate.h"

#include "test_async.h"
#include "test_ref_counted.h"
#include "../src/interlocked_hl.c"

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"


#include "test_sync_wrappers.h"

#include "c_util/sync_wrapper.h"

TEST_DEFINE_ENUM_TYPE(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_RESULT_VALUES)

TEST_DEFINE_ENUM_TYPE(TEST_SYNC_API_RESULT, TEST_SYNC_API_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(TEST_SYNC_API_RESULT, TEST_SYNC_API_RESULT_VALUES)

IMPLEMENT_UMOCK_C_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES)

static TEST_ASYNC_HANDLE test_handle = (TEST_ASYNC_HANDLE)0x42;
static ON_DO_SOMETHING_ASYNC_COMPLETE on_do_something_async_complete;
static void* on_do_something_async_complete_context;
static ON_DO_SOMETHING_NO_IN_ARGS_ASYNC_COMPLETE on_do_something_no_in_args_async_complete;
static void* on_do_something_no_in_args_async_complete_context;
static ON_DO_SOMETHING_NO_OUT_ARGS_ASYNC_COMPLETE on_do_something_no_out_args_async_complete;
static void* on_do_something_no_out_args_async_complete_context;
static ON_DO_SOMETHING_NO_ARGS_ASYNC_COMPLETE on_do_something_no_args_async_complete;
static void* on_do_something_no_args_async_complete_context;
static ON_DO_SOMETHING_CHARPTR_OUT_ARG_ASYNC_COMPLETE on_do_something_charptr_out_arg_async_complete;
static void* on_do_something_charptr_out_arg_async_complete_context;
static ON_DO_SOMETHING_CHARPTR_OUT_ARG_FAIL_ASYNC_COMPLETE on_do_something_charptr_out_arg_fail_async_complete;
static void* on_do_something_charptr_out_arg_fail_async_complete_context;
static ON_DO_SOMETHING_REF_COUNTED_OUT_ARG_ASYNC_COMPLETE on_do_something_ref_counted_out_arg_async_complete;
static void* on_do_something_ref_counted_out_arg_async_complete_context;
static ON_DO_SOMETHING_2_OUT_ARGS_2ND_FAILS_ASYNC_COMPLETE on_do_something_2_out_args_2nd_fails_async_complete;
static void* on_do_something_2_out_args_2nd_fails_async_complete_context;
static ON_DO_SOMETHING_REF_COUNTED_ARRAY_OUT_ARG_ASYNC_COMPLETE on_do_something_ref_counted_array_out_arg_async_complete;
static void* on_do_something_ref_counted_array_out_arg_async_complete_context;
static ON_HAVE_POINTER_TYPE_OUT_ARG_ASYNC_COMPLETE on_have_pointer_type_out_arg_async_complete;
static void* on_have_pointer_type_out_arg_async_complete_context;

static TEST_ASYNC_API_RESULT injected_test_async_result;
static int injected_callback_arg1;
static int injected_callback_arg2;
static bool call_test_async_result_with_NULL;
static size_t how_many_ref_counted_items_to_copy;

static TEST_REFCOUNTED_HANDLE test_ref_counted[2];
static TEST_MY_POINTER test_my_pointer;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

INTERLOCKED_HL_RESULT my_InterlockedHL_WaitForValue(int32_t volatile_atomic* address, int32_t value, uint32_t milliseconds)
{
    if (call_test_async_result_with_NULL)
    {
        on_do_something_async_complete(NULL, injected_test_async_result, injected_callback_arg1, injected_callback_arg2);
    }

    if (on_do_something_async_complete != NULL)
    {
        on_do_something_async_complete(on_do_something_async_complete_context, injected_test_async_result, injected_callback_arg1, injected_callback_arg2);
    }

    if (on_do_something_no_in_args_async_complete != NULL)
    {
        on_do_something_no_in_args_async_complete(on_do_something_no_in_args_async_complete_context, injected_test_async_result, injected_callback_arg1, injected_callback_arg2);
    }

    if (on_do_something_no_out_args_async_complete != NULL)
    {
        on_do_something_no_out_args_async_complete(on_do_something_no_out_args_async_complete_context);
    }

    if (on_do_something_no_args_async_complete != NULL)
    {
        on_do_something_no_args_async_complete(on_do_something_no_args_async_complete_context);
    }

    if (on_do_something_charptr_out_arg_async_complete != NULL)
    {
        on_do_something_charptr_out_arg_async_complete(on_do_something_charptr_out_arg_async_complete_context, "gogu");
    }

    if (on_do_something_charptr_out_arg_fail_async_complete != NULL)
    {
        on_do_something_charptr_out_arg_fail_async_complete(on_do_something_charptr_out_arg_fail_async_complete_context, "gogu");
    }

    if (on_do_something_ref_counted_out_arg_async_complete != NULL)
    {
        on_do_something_ref_counted_out_arg_async_complete(on_do_something_ref_counted_out_arg_async_complete_context, test_ref_counted[0]);
    }

    if (on_do_something_2_out_args_2nd_fails_async_complete != NULL)
    {
        on_do_something_2_out_args_2nd_fails_async_complete(on_do_something_2_out_args_2nd_fails_async_complete_context, "gogu", "the_other_gogu");
    }

    if (on_do_something_ref_counted_array_out_arg_async_complete != NULL)
    {
        on_do_something_ref_counted_array_out_arg_async_complete(on_do_something_ref_counted_array_out_arg_async_complete_context, &test_ref_counted[0], how_many_ref_counted_items_to_copy);
    }

    if (on_have_pointer_type_out_arg_async_complete != NULL)
    {
        on_have_pointer_type_out_arg_async_complete(on_have_pointer_type_out_arg_async_complete_context, &test_my_pointer);
    }

    return UMOCK_REAL(InterlockedHL_WaitForValue)(address, value, milliseconds);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_flex, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(InterlockedHL_WaitForValue, my_InterlockedHL_WaitForValue);
    REGISTER_GLOBAL_MOCK_HOOK(InterlockedHL_SetAndWake, UMOCK_REAL(InterlockedHL_SetAndWake));

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(test_async_do_something_async, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(InterlockedHL_WaitForValue, INTERLOCKED_HL_ERROR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);

    REGISTER_UMOCK_ALIAS_TYPE(TEST_ASYNC_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_NO_IN_ARGS_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_NO_OUT_ARGS_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_NO_ARGS_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_CHARPTR_OUT_ARG_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_CHARPTR_OUT_ARG_FAIL_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_REF_COUNTED_OUT_ARG_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_2_OUT_ARGS_2ND_FAILS_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_REF_COUNTED_ARRAY_OUT_ARG_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_HAVE_POINTER_TYPE_OUT_ARG_ASYNC_COMPLETE, void*);

    REGISTER_TYPE(TEST_SYNC_API_RESULT, TEST_SYNC_API_RESULT);
    REGISTER_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    on_do_something_async_complete = NULL;
    on_do_something_async_complete_context = NULL;
    on_do_something_no_in_args_async_complete = NULL;
    on_do_something_no_in_args_async_complete_context = NULL;
    on_do_something_no_out_args_async_complete = NULL;
    on_do_something_no_out_args_async_complete_context = NULL;
    on_do_something_no_args_async_complete = NULL;
    on_do_something_no_args_async_complete_context = NULL;
    on_do_something_charptr_out_arg_async_complete = NULL;
    on_do_something_charptr_out_arg_async_complete_context = NULL;
    on_do_something_charptr_out_arg_fail_async_complete = NULL;
    on_do_something_charptr_out_arg_fail_async_complete_context = NULL;
    on_do_something_ref_counted_out_arg_async_complete = NULL;
    on_do_something_ref_counted_out_arg_async_complete_context = NULL;
    on_do_something_2_out_args_2nd_fails_async_complete = NULL;
    on_do_something_2_out_args_2nd_fails_async_complete_context = NULL;
    on_do_something_ref_counted_array_out_arg_async_complete = NULL;
    on_do_something_ref_counted_array_out_arg_async_complete_context = NULL;
    on_have_pointer_type_out_arg_async_complete = NULL;
    on_have_pointer_type_out_arg_async_complete_context = NULL;
    call_test_async_result_with_NULL = false;
    how_many_ref_counted_items_to_copy = 1;

    for (size_t i = 0; i < MU_COUNT_ARRAY_ITEMS(test_ref_counted); i++)
    {
        test_ref_counted[i] = test_refcounted_create();
        ASSERT_IS_NOT_NULL(test_ref_counted[i]);
    }

    test_my_pointer.x = 999;
    test_my_pointer.y = 777;

    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    for (size_t i = 0; i < MU_COUNT_ARRAY_ITEMS(test_ref_counted); i++)
    {
        test_refcounted_dec_ref(test_ref_counted[i]);
    }

    umock_c_negative_tests_deinit();
}

/* Tests_SRS_SYNC_WRAPPER_01_001: [ DECLARE_SYNC_WRAPPER shall expand to the function declaration: ]*/
/* Tests_SRS_SYNC_WRAPPER_01_002: [ The generated synchronous wrapper shall have the declaration: ]*/
/* Tests_SRS_SYNC_WRAPPER_01_003: [ If async_handle is NULL, the synchronous wrapper shall fail and return SYNC_WRAPPER_INVALID_ARGS. ]*/
TEST_FUNCTION(sync_wrapper_call_with_NULL_handle_fails)
{
    // arrange
    int internal_result;
    TEST_ASYNC_API_RESULT async_result;
    int callback_arg_1;
    int callback_arg_2;

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_async)(NULL, 42, 43, &internal_result, &async_result, &callback_arg_1, &callback_arg_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_INVALID_ARGS, result);
}

/* Tests_SRS_SYNC_WRAPPER_42_002: [ If async_function_call_result is NULL then the synchronous wrapper shall fail and return SYNC_WRAPPER_INVALID_ARGS. ]*/
TEST_FUNCTION(sync_wrapper_call_with_NULL_async_function_call_result_fails)
{
    // arrange
    TEST_ASYNC_API_RESULT async_result;
    int callback_arg_1;
    int callback_arg_2;

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_async)(test_handle, 42, 43, NULL, &async_result, &callback_arg_1, &callback_arg_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_INVALID_ARGS, result);
}

/* Tests_SRS_SYNC_WRAPPER_01_004: [ If any of the out arguments pointers is NULL, the synchronous wrapper shall fail and return SYNC_WRAPPER_INVALID_ARGS. ]*/
TEST_FUNCTION(sync_wrapper_call_with_NULL_async_result_argument_fails)
{
    // arrange
    int internal_result;
    int callback_arg_1;
    int callback_arg_2;

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_async)(test_handle, 42, 43, &internal_result, NULL, &callback_arg_1, &callback_arg_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_INVALID_ARGS, result);
}

/* Tests_SRS_SYNC_WRAPPER_01_004: [ If any of the out arguments pointers is NULL, the synchronous wrapper shall fail and return SYNC_WRAPPER_INVALID_ARGS. ]*/
TEST_FUNCTION(sync_wrapper_call_with_NULL_callback_arg1_fails)
{
    // arrange
    int internal_result;
    TEST_ASYNC_API_RESULT async_result;
    int callback_arg_2;

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_async)(test_handle, 42, 43, &internal_result, &async_result, NULL, &callback_arg_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_INVALID_ARGS, result);
}

/* Tests_SRS_SYNC_WRAPPER_01_004: [ If any of the out arguments pointers is NULL, the synchronous wrapper shall fail and return SYNC_WRAPPER_INVALID_ARGS. ]*/
TEST_FUNCTION(sync_wrapper_call_with_NULL_callback_arg_2_fails)
{
    // arrange
    int internal_result;
    TEST_ASYNC_API_RESULT async_result;
    int callback_arg_1;

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_async)(test_handle, 42, 43, &internal_result, &async_result, &callback_arg_1, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_INVALID_ARGS, result);
}

/* Tests_SRS_SYNC_WRAPPER_01_006: [ The synchronous wrapper shall call the asynchronous function async_function_name, pass the in_args as arguments together with the generated completion function and a context used to store out argument pointers. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_007: [ The synchronous wrapper shall wait for the callback to be finished by using InterlockedHL_WaitForValue. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_008: [ On success, the synchronous wrapper shall return SYNC_WRAPPER_OK. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_011: [ DEFINE_SYNC_WRAPPER shall generate a callback to be passed to the asynchronous function with the following declaration: ]*/
/* Tests_SRS_SYNC_WRAPPER_01_013: [ Otherwise, on_{async_function_name}_complete shall store the values of the out args into the context created in synchronous wrapper function. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_015: [ on_{async_function_name}_complete shall unblock the synchronous wrapper function call. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_016: [ SYNC_WRAPPER shall expand async_function_name to the name of the synchronous wrapper around async_function_name. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_009: [ On success, the synchronous wrapper shall return in the out args the values of the arguments received in the callback. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_017: [ For each argument: ]*/
/* Tests_SRS_SYNC_WRAPPER_01_018: [ If SYNC_WRAPPER_USE_ASSIGN_COPY_{type} is defined as 1, on_{async_function_name}_complete shall copy the argument value by assigning it. ]*/
TEST_FUNCTION(sync_wrapper_call_succeeds)
{
    // arrange
    int internal_result;
    TEST_ASYNC_API_RESULT async_result = TEST_ASYNC_API_RESULT_INVALID;
    int callback_arg_1 = 0;
    int callback_arg_2 = 0;

    injected_test_async_result = DUMMY_RESULT_1;
    injected_callback_arg1 = 44;
    injected_callback_arg2 = 45;

    STRICT_EXPECTED_CALL(test_async_do_something_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_async_complete(&on_do_something_async_complete)
        .CaptureArgumentValue_context(&on_do_something_async_complete_context);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, UINT32_MAX));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, 1));

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_async)(test_handle, 42, 43, &internal_result, &async_result, &callback_arg_1, &callback_arg_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_OK, result);
    ASSERT_ARE_EQUAL(int, 44, callback_arg_1);
    ASSERT_ARE_EQUAL(int, 45, callback_arg_2);
    ASSERT_ARE_EQUAL(int, DUMMY_RESULT_1, async_result);
    ASSERT_ARE_EQUAL(int, 0, internal_result);
}

/* Tests_SRS_SYNC_WRAPPER_01_009: [ On success, the synchronous wrapper shall return in the out args the values of the arguments received in the callback. ]*/
TEST_FUNCTION(sync_wrapper_call_succeeds_2)
{
    // arrange
    int internal_result;
    TEST_ASYNC_API_RESULT async_result = TEST_ASYNC_API_RESULT_INVALID;
    int callback_arg_1 = 0;
    int callback_arg_2 = 0;

    injected_test_async_result = DUMMY_RESULT_2;
    injected_callback_arg1 = 1;
    injected_callback_arg2 = 2;

    STRICT_EXPECTED_CALL(test_async_do_something_async(test_handle, 100, 1, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_async_complete(&on_do_something_async_complete)
        .CaptureArgumentValue_context(&on_do_something_async_complete_context);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, UINT32_MAX));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, 1));

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_async)(test_handle, 100, 1, &internal_result, &async_result, &callback_arg_1, &callback_arg_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_OK, result);
    ASSERT_ARE_EQUAL(int, 1, callback_arg_1);
    ASSERT_ARE_EQUAL(int, 2, callback_arg_2);
    ASSERT_ARE_EQUAL(int, DUMMY_RESULT_2, async_result);
}

/* Tests_SRS_SYNC_WRAPPER_42_003: [ If the asynchronous function returns something other than expected_return then the synchronous wrapper shall return SYNC_WRAPPER_CALL_ERROR. ]*/
TEST_FUNCTION(when_async_function_fails_sync_wrapper_call_fails_with_)
{
    // arrange
    int internal_result;
    TEST_ASYNC_API_RESULT async_result = TEST_ASYNC_API_RESULT_INVALID;
    int callback_arg_1 = 0;
    int callback_arg_2 = 0;

    injected_test_async_result = DUMMY_RESULT_1;
    injected_callback_arg1 = 44;
    injected_callback_arg2 = 45;

    STRICT_EXPECTED_CALL(test_async_do_something_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_async_complete(&on_do_something_async_complete)
        .CaptureArgumentValue_context(&on_do_something_async_complete_context)
        .SetReturn(MU_FAILURE);

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_async)(test_handle, 42, 43, &internal_result, &async_result, &callback_arg_1, &callback_arg_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_CALL_ERROR, result);
}

/* Tests_SRS_SYNC_WRAPPER_01_010: [ If any other error occurs, the synchronous wrapper shall fail and return SYNC_WRAPPER_OTHER_ERROR. ]*/
TEST_FUNCTION(when_underlying_calls_fail_sync_wrapper_call_fails)
{
    // arrange
    int internal_result;
    TEST_ASYNC_API_RESULT async_result = TEST_ASYNC_API_RESULT_INVALID;
    int callback_arg_1 = 0;
    int callback_arg_2 = 0;

    injected_test_async_result = DUMMY_RESULT_1;
    injected_callback_arg1 = 44;
    injected_callback_arg2 = 45;

    STRICT_EXPECTED_CALL(test_async_do_something_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_async_complete(&on_do_something_async_complete)
        .CaptureArgumentValue_context(&on_do_something_async_complete_context);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, UINT32_MAX))
        .SetReturn(INTERLOCKED_HL_ERROR);

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_async)(test_handle, 42, 43, &internal_result, &async_result, &callback_arg_1, &callback_arg_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_OTHER_ERROR, result);
}

/* Tests_SRS_SYNC_WRAPPER_01_012: [ If context is NULL, on_{async_function_name}_complete shall terminate the process. ]*/
TEST_FUNCTION(callback_with_NULL_context_terminates_process)
{
    // arrange
    int internal_result;
    TEST_ASYNC_API_RESULT async_result = TEST_ASYNC_API_RESULT_INVALID;
    int callback_arg_1 = 0;
    int callback_arg_2 = 0;

    injected_test_async_result = DUMMY_RESULT_2;
    injected_callback_arg1 = 1;
    injected_callback_arg2 = 2;
    call_test_async_result_with_NULL = true;

    STRICT_EXPECTED_CALL(test_async_do_something_async(test_handle, 100, 1, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_async_complete(&on_do_something_async_complete)
        .CaptureArgumentValue_context(&on_do_something_async_complete_context);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, UINT32_MAX));
    STRICT_EXPECTED_CALL(ps_util_terminate_process());
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, 1));

    // act
    (void)SYNC_WRAPPER(test_async_do_something_async)(test_handle, 100, 1, &internal_result, &async_result, &callback_arg_1, &callback_arg_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SYNC_WRAPPER_01_006: [ The synchronous wrapper shall call the asynchronous function async_function_name, pass the in_args as arguments together with the generated completion function and a context used to store out argument pointers. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_007: [ The synchronous wrapper shall wait for the callback to be finished by using InterlockedHL_WaitForValue. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_008: [ On success, the synchronous wrapper shall return SYNC_WRAPPER_OK. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_011: [ DEFINE_SYNC_WRAPPER shall generate a callback to be passed to the asynchronous function with the following declaration: ]*/
/* Tests_SRS_SYNC_WRAPPER_01_013: [ Otherwise, on_{async_function_name}_complete shall store the values of the out args into the context created in synchronous wrapper function. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_015: [ on_{async_function_name}_complete shall unblock the synchronous wrapper function call. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_016: [ SYNC_WRAPPER shall expand async_function_name to the name of the synchronous wrapper around async_function_name. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_009: [ On success, the synchronous wrapper shall return in the out args the values of the arguments received in the callback. ]*/
TEST_FUNCTION(sync_wrapper_call_with_no_in_args_succeeds)
{
    // arrange
    int internal_result;
    TEST_ASYNC_API_RESULT async_result = TEST_ASYNC_API_RESULT_INVALID;
    int callback_arg_1 = 0;
    int callback_arg_2 = 0;

    injected_test_async_result = DUMMY_RESULT_1;
    injected_callback_arg1 = 44;
    injected_callback_arg2 = 45;

    STRICT_EXPECTED_CALL(test_async_do_something_no_in_args_async(test_handle, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_no_in_args_async_complete(&on_do_something_no_in_args_async_complete)
        .CaptureArgumentValue_context(&on_do_something_no_in_args_async_complete_context);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, UINT32_MAX));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, 1));

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_no_in_args_async)(test_handle, &internal_result, &async_result, &callback_arg_1, &callback_arg_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_OK, result);
    ASSERT_ARE_EQUAL(int, 44, callback_arg_1);
    ASSERT_ARE_EQUAL(int, 45, callback_arg_2);
    ASSERT_ARE_EQUAL(int, DUMMY_RESULT_1, async_result);
}

/* Tests_SRS_SYNC_WRAPPER_01_006: [ The synchronous wrapper shall call the asynchronous function async_function_name, pass the in_args as arguments together with the generated completion function and a context used to store out argument pointers. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_007: [ The synchronous wrapper shall wait for the callback to be finished by using InterlockedHL_WaitForValue. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_008: [ On success, the synchronous wrapper shall return SYNC_WRAPPER_OK. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_011: [ DEFINE_SYNC_WRAPPER shall generate a callback to be passed to the asynchronous function with the following declaration: ]*/
/* Tests_SRS_SYNC_WRAPPER_01_013: [ Otherwise, on_{async_function_name}_complete shall store the values of the out args into the context created in synchronous wrapper function. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_015: [ on_{async_function_name}_complete shall unblock the synchronous wrapper function call. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_016: [ SYNC_WRAPPER shall expand async_function_name to the name of the synchronous wrapper around async_function_name. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_009: [ On success, the synchronous wrapper shall return in the out args the values of the arguments received in the callback. ]*/
TEST_FUNCTION(sync_wrapper_call_with_no_out_args_succeeds)
{
    // arrange
    int internal_result;
    STRICT_EXPECTED_CALL(test_async_do_something_no_out_args_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_no_out_args_async_complete(&on_do_something_no_out_args_async_complete)
        .CaptureArgumentValue_context(&on_do_something_no_out_args_async_complete_context);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, UINT32_MAX));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, 1));

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_no_out_args_async)(test_handle, 42, 43, &internal_result);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_OK, result);
}

/* Tests_SRS_SYNC_WRAPPER_01_006: [ The synchronous wrapper shall call the asynchronous function async_function_name, pass the in_args as arguments together with the generated completion function and a context used to store out argument pointers. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_007: [ The synchronous wrapper shall wait for the callback to be finished by using InterlockedHL_WaitForValue. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_008: [ On success, the synchronous wrapper shall return SYNC_WRAPPER_OK. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_011: [ DEFINE_SYNC_WRAPPER shall generate a callback to be passed to the asynchronous function with the following declaration: ]*/
/* Tests_SRS_SYNC_WRAPPER_01_013: [ Otherwise, on_{async_function_name}_complete shall store the values of the out args into the context created in synchronous wrapper function. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_015: [ on_{async_function_name}_complete shall unblock the synchronous wrapper function call. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_016: [ SYNC_WRAPPER shall expand async_function_name to the name of the synchronous wrapper around async_function_name. ]*/
/* Tests_SRS_SYNC_WRAPPER_01_009: [ On success, the synchronous wrapper shall return in the out args the values of the arguments received in the callback. ]*/
TEST_FUNCTION(sync_wrapper_call_with_no_args_succeeds)
{
    // arrange
    int internal_result;
    STRICT_EXPECTED_CALL(test_async_do_something_no_args_async(test_handle, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_no_args_async_complete(&on_do_something_no_args_async_complete)
        .CaptureArgumentValue_context(&on_do_something_no_args_async_complete_context);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, UINT32_MAX));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, 1));

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_no_args_async)(test_handle, &internal_result);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_OK, result);
}

/* Tests_SRS_SYNC_WRAPPER_01_017: [ For each argument: ]*/
/* Tests_SRS_SYNC_WRAPPER_01_019: [ If SYNC_WRAPPER_USE_ASSIGN_COPY_{type} is not defined, on_{async_function_name}_complete shall call a copy function with the following declaration: ]*/
/* Tests_SRS_SYNC_WRAPPER_01_026: [ OUT_ARG shall be expanded to the appropriate argument wherever needed by using arg_type and arg_name. ]*/
TEST_FUNCTION(sync_wrapper_call_with_custom_copy_succeeds)
{
    // arrange
    int internal_result;
    char* charptr_arg;

    STRICT_EXPECTED_CALL(test_async_do_something_charptr_out_arg_async(test_handle, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_charptr_out_arg_async_complete(&on_do_something_charptr_out_arg_async_complete)
        .CaptureArgumentValue_context(&on_do_something_charptr_out_arg_async_complete_context);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, UINT32_MAX));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, 1));

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_charptr_out_arg_async)(test_handle, &internal_result, &charptr_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_OK, result);

    // cleanup
    free(charptr_arg);
}

/* Tests_SRS_SYNC_WRAPPER_01_020: [ If the callback fails to copy any argument value, the synchronous wrapper shall return SYNC_WRAPPER_CALLBACK_COPY_ARG_ERROR. ]*/
TEST_FUNCTION(when_copy_arg_fails_sync_wrapper_call_with_custom_copy_fails)
{
    // arrange
    int internal_result;
    char* charptr_arg;

    STRICT_EXPECTED_CALL(test_async_do_something_charptr_out_arg_fail_async(test_handle, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_charptr_out_arg_fail_async_complete(&on_do_something_charptr_out_arg_fail_async_complete)
        .CaptureArgumentValue_context(&on_do_something_charptr_out_arg_fail_async_complete_context);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, UINT32_MAX));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, 1));

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_charptr_out_arg_fail_async)(test_handle, &internal_result, &charptr_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_CALLBACK_COPY_ARG_ERROR, result);
}

/* Tests_SRS_SYNC_WRAPPER_01_017: [ For each argument: ]*/
/* Tests_SRS_SYNC_WRAPPER_01_019: [ If SYNC_WRAPPER_USE_ASSIGN_COPY_{type} is not defined, on_{async_function_name}_complete shall call a copy function with the following declaration: ]*/
TEST_FUNCTION(sync_wrapper_call_with_custom_copy_for_ref_counted_handle_succeeds)
{
    // arrange
    int internal_result;
    TEST_REFCOUNTED_HANDLE ref_counted_arg;

    STRICT_EXPECTED_CALL(test_async_do_something_ref_counted_out_arg_async(test_handle, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_ref_counted_out_arg_async_complete(&on_do_something_ref_counted_out_arg_async_complete)
        .CaptureArgumentValue_context(&on_do_something_ref_counted_out_arg_async_complete_context);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, UINT32_MAX));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, 1));

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_ref_counted_out_arg_async)(test_handle, &internal_result, &ref_counted_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_OK, result);

    // cleanup
    test_refcounted_dec_ref(ref_counted_arg);
}

/* Tests_SRS_SYNC_WRAPPER_01_020: [ If the callback fails to copy any argument value, the synchronous wrapper shall return SYNC_WRAPPER_CALLBACK_COPY_ARG_ERROR. ]*/
TEST_FUNCTION(when_copy_arg_fails_for_the_2nd_arg_sync_wrapper_call_with_custom_copy_frees_the_first_out_arg)
{
    // arrange
    int internal_result;
    char* charptr_arg;
    char* charptr_fail_arg;

    STRICT_EXPECTED_CALL(test_async_do_something_2_out_args_2nd_fails_async(test_handle, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_2_out_args_2nd_fails_async_complete(&on_do_something_2_out_args_2nd_fails_async_complete)
        .CaptureArgumentValue_context(&on_do_something_2_out_args_2nd_fails_async_complete_context);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, UINT32_MAX));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)); // 1st out arg copy
    STRICT_EXPECTED_CALL(free(IGNORED_ARG)); // 1st out arg free
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, 1));

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_2_out_args_2nd_fails_async)(test_handle, &internal_result, &charptr_arg, &charptr_fail_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_CALLBACK_COPY_ARG_ERROR, result);
}

/* Tests_SRS_SYNC_WRAPPER_01_020: [ If the callback fails to copy any argument value, the synchronous wrapper shall return SYNC_WRAPPER_CALLBACK_COPY_ARG_ERROR. ]*/
TEST_FUNCTION(when_copy_arg_fails_for_the_1st_arg_sync_wrapper_call_with_custom_copy_does_not_copy_the_second_one)
{
    // arrange
    int internal_result;
    char* charptr_arg;
    char* charptr_fail_arg;

    STRICT_EXPECTED_CALL(test_async_do_something_2_out_args_2nd_fails_async(test_handle, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_2_out_args_2nd_fails_async_complete(&on_do_something_2_out_args_2nd_fails_async_complete)
        .CaptureArgumentValue_context(&on_do_something_2_out_args_2nd_fails_async_complete_context);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, UINT32_MAX));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL); // 1st out arg copy
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, 1));

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_2_out_args_2nd_fails_async)(test_handle, &internal_result, &charptr_arg, &charptr_fail_arg);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_CALLBACK_COPY_ARG_ERROR, result);
}

/* Tests_SRS_SYNC_WRAPPER_01_025: [ copy_function shall be used as a function with the following declaration ]*/
/* Tests_SRS_SYNC_WRAPPER_01_027: [ free_function shall be used as a function with the following declaration ]*/
TEST_FUNCTION(sync_wrapper_call_with_custom_copy_of_an_array_succeeds)
{
    // arrange
    int internal_result;
    TEST_REFCOUNTED_HANDLE_ptr_t test_ref_counted_array_ptr;
    size_t test_item_count;

    STRICT_EXPECTED_CALL(test_async_do_something_ref_counted_array_out_arg_async(test_handle, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_ref_counted_array_out_arg_async_complete(&on_do_something_ref_counted_array_out_arg_async_complete)
        .CaptureArgumentValue_context(&on_do_something_ref_counted_array_out_arg_async_complete_context);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, UINT32_MAX));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, 1));

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_ref_counted_array_out_arg_async)(test_handle, &internal_result, &test_ref_counted_array_ptr, &test_item_count);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_OK, result);

    // cleanup
    TEST_REFCOUNTED_HANDLE_ptr_t_free(test_ref_counted_array_ptr, 1);
}

/* Tests_SRS_SYNC_WRAPPER_01_025: [ copy_function shall be used as a function with the following declaration ]*/
/* Tests_SRS_SYNC_WRAPPER_01_027: [ free_function shall be used as a function with the following declaration ]*/
TEST_FUNCTION(sync_wrapper_call_with_custom_copy_of_an_array_with_2_elements_succeeds)
{
    // arrange
    int internal_result;
    TEST_REFCOUNTED_HANDLE_ptr_t test_ref_counted_array_ptr;
    size_t test_item_count;

    how_many_ref_counted_items_to_copy = 2;

    STRICT_EXPECTED_CALL(test_async_do_something_ref_counted_array_out_arg_async(test_handle, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_ref_counted_array_out_arg_async_complete(&on_do_something_ref_counted_array_out_arg_async_complete)
        .CaptureArgumentValue_context(&on_do_something_ref_counted_array_out_arg_async_complete_context);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, UINT32_MAX));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, 1));

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_ref_counted_array_out_arg_async)(test_handle, &internal_result, &test_ref_counted_array_ptr, &test_item_count);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_OK, result);

    // cleanup
    TEST_REFCOUNTED_HANDLE_ptr_t_free(test_ref_counted_array_ptr, 2);
}

/* Tests_SRS_SYNC_WRAPPER_42_001: [ The synchronous wrapper shall store the result of the asynchronous function call in async_function_call_result. ]*/
TEST_FUNCTION(sync_wrapper_call_stores_internal_result_on_failure)
{
    // arrange
    TEST_SYNC_API_RESULT internal_result;
    TEST_ASYNC_API_RESULT async_result = TEST_ASYNC_API_RESULT_INVALID;
    int callback_arg_1 = 0;
    int callback_arg_2 = 0;

    injected_test_async_result = DUMMY_RESULT_1;
    injected_callback_arg1 = 44;
    injected_callback_arg2 = 45;

    STRICT_EXPECTED_CALL(test_async_do_something_with_different_return_type_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(DUMMY_RESULT_SYNC_2);

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_do_something_with_different_return_type_async)(test_handle, 42, 43, &internal_result, &async_result, &callback_arg_1, &callback_arg_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_CALL_ERROR, result);
    ASSERT_ARE_EQUAL(TEST_SYNC_API_RESULT, DUMMY_RESULT_SYNC_2, internal_result);
}

TEST_FUNCTION(sync_wrapper_call_with_non_pointer_out_succeeds)
{
    // arrange
    int internal_result;
    TEST_MY_POINTER output_ptr;

    how_many_ref_counted_items_to_copy = 2;

    STRICT_EXPECTED_CALL(test_async_have_pointer_type_out_arg_async(test_handle, 42, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_have_pointer_type_out_arg_async_complete(&on_have_pointer_type_out_arg_async_complete)
        .CaptureArgumentValue_context(&on_have_pointer_type_out_arg_async_complete_context);
    STRICT_EXPECTED_CALL(InterlockedHL_WaitForValue(IGNORED_ARG, 1, UINT32_MAX));
    STRICT_EXPECTED_CALL(InterlockedHL_SetAndWake(IGNORED_ARG, 1));

    // act
    SYNC_WRAPPER_RESULT result = SYNC_WRAPPER(test_async_have_pointer_type_out_arg_async)(test_handle, 42, &internal_result, &output_ptr);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_OK, result);

    ASSERT_ARE_EQUAL(int, test_my_pointer.x, output_ptr.x);
    ASSERT_ARE_EQUAL(int, test_my_pointer.y, output_ptr.y);

    // cleanup
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
