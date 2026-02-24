// Copyright (c) Microsoft. All rights reserved.


#include "async_retry_wrapper_ut_pch.h"

static TEST_ASYNC_HANDLE test_handle = (TEST_ASYNC_HANDLE)0x42;

static TEST_REFCOUNTED_HANDLE test_ref_counted[2];

static void* test_callback_context = (void*)0x4242;

TEST_DEFINE_ENUM_TYPE(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(TEST_ASYNC_API_DEFAULT_RESULT, TEST_ASYNC_API_DEFAULT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(TEST_ASYNC_API_DEFAULT_RESULT, TEST_ASYNC_API_DEFAULT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(TEST_ASYNC_API_MULTIPLE_RETRY_RESULT, TEST_ASYNC_API_MULTIPLE_RETRY_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(TEST_ASYNC_API_MULTIPLE_RETRY_RESULT, TEST_ASYNC_API_MULTIPLE_RETRY_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(TEST_ASYNC_API_SYNC_RESULT, TEST_ASYNC_API_SYNC_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(TEST_ASYNC_API_SYNC_RESULT, TEST_ASYNC_API_SYNC_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_VALUES);

MOCK_FUNCTION_WITH_CODE(, void, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, void*, context, TEST_ASYNC_API_DEFAULT_RESULT, result, int, callback_arg1, int, callback_arg2)
    ASSERT_ARE_NOT_EQUAL(TEST_ASYNC_API_DEFAULT_RESULT, TEST_ASYNC_RESULT_FOR_RETRY, result);
MOCK_FUNCTION_END();

MOCK_FUNCTION_WITH_CODE(, void, mock_ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE, void*, context, TEST_ASYNC_API_MULTIPLE_RETRY_RESULT, result, int, callback_arg1, int, callback_arg2)
    ASSERT_ARE_NOT_EQUAL(TEST_ASYNC_API_MULTIPLE_RETRY_RESULT, TEST_ASYNC_MULTIPLE_RETRY_RETRY_1, result);
    ASSERT_ARE_NOT_EQUAL(TEST_ASYNC_API_MULTIPLE_RETRY_RESULT, TEST_ASYNC_MULTIPLE_RETRY_RETRY_2, result);
    ASSERT_ARE_NOT_EQUAL(TEST_ASYNC_API_MULTIPLE_RETRY_RESULT, TEST_ASYNC_MULTIPLE_RETRY_RETRY_3, result);
MOCK_FUNCTION_END();

MOCK_FUNCTION_WITH_CODE(, void, mock_ON_DO_SOMETHING_NO_IN_ARGS_ASYNC_COMPLETE, void*, context, TEST_ASYNC_API_DEFAULT_RESULT, result, int, callback_arg1, int, callback_arg2)
    ASSERT_ARE_NOT_EQUAL(TEST_ASYNC_API_DEFAULT_RESULT, TEST_ASYNC_RESULT_FOR_RETRY, result);
MOCK_FUNCTION_END();

MOCK_FUNCTION_WITH_CODE(, void, mock_ON_DO_SOMETHING_NO_OUT_ARGS_ASYNC_COMPLETE, void*, context, TEST_ASYNC_API_DEFAULT_RESULT, result)
    ASSERT_ARE_NOT_EQUAL(TEST_ASYNC_API_DEFAULT_RESULT, TEST_ASYNC_RESULT_FOR_RETRY, result);
MOCK_FUNCTION_END();

MOCK_FUNCTION_WITH_CODE(, void, mock_ON_DO_SOMETHING_NO_ARGS_ASYNC_COMPLETE, void*, context, TEST_ASYNC_API_DEFAULT_RESULT, result)
    ASSERT_ARE_NOT_EQUAL(TEST_ASYNC_API_DEFAULT_RESULT, TEST_ASYNC_RESULT_FOR_RETRY, result);
MOCK_FUNCTION_END();

MOCK_FUNCTION_WITH_CODE(, void, mock_ON_DO_SOMETHING_CHARPTR_IN_ARG_ASYNC_COMPLETE, void*, context, const char*, charptr_arg, TEST_ASYNC_API_DEFAULT_RESULT, result)
    ASSERT_ARE_NOT_EQUAL(TEST_ASYNC_API_DEFAULT_RESULT, TEST_ASYNC_RESULT_FOR_RETRY, result);
MOCK_FUNCTION_END();

MOCK_FUNCTION_WITH_CODE(, void, mock_ON_DO_SOMETHING_REF_COUNTED_IN_ARG_ASYNC_COMPLETE, void*, context, TEST_ASYNC_API_DEFAULT_RESULT, result)
    ASSERT_ARE_NOT_EQUAL(TEST_ASYNC_API_DEFAULT_RESULT, TEST_ASYNC_RESULT_FOR_RETRY, result);
MOCK_FUNCTION_END();

MOCK_FUNCTION_WITH_CODE(, void, mock_ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE, void*, context, uint32_t, value, TEST_ASYNC_API_DEFAULT_RESULT, result)
    ASSERT_ARE_NOT_EQUAL(TEST_ASYNC_API_DEFAULT_RESULT, TEST_ASYNC_RESULT_FOR_RETRY, result);
MOCK_FUNCTION_END();

MOCK_FUNCTION_WITH_CODE(, void, mock_ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE, void*, context, TEST_ASYNC_API_DEFAULT_RESULT, result, uint8_t, value);
    ASSERT_ARE_NOT_EQUAL(TEST_ASYNC_API_DEFAULT_RESULT, TEST_ASYNC_RESULT_FOR_RETRY, result);
MOCK_FUNCTION_END();

MOCK_FUNCTION_WITH_CODE(, void, mock_ON_DO_SOMETHING_NO_RETRY_ASYNC_COMPLETE, void*, context, int, callback_arg1, int, callback_arg2)
MOCK_FUNCTION_END();

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

static THREADPOOL_WORK_FUNCTION saved_threadpool_work;
static void* saved_threadpool_context;
static int hook_threadpool_schedule_work(THANDLE(THREADPOOL) threadpool, THREADPOOL_WORK_FUNCTION work_function, void* work_function_context)
{
    (void)threadpool;
    saved_threadpool_work = work_function;
    saved_threadpool_context = work_function_context;
    return 0;
}

static void setup_test_async_do_something_async(int arg1, int arg2, ON_DO_SOMETHING_ASYNC_COMPLETE* saved_callback, void** saved_context)
{
    int function_call_result;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_async(test_handle, arg1, arg2, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_async_complete(saved_callback)
        .CaptureArgumentValue_context(saved_context);

    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_async)(test_handle, g.test_threadpool, arg1, arg2, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);
}

static void setup_test_async_do_something_multiple_retry_conditions_async(int arg1, int arg2, ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE* saved_callback, void** saved_context)
{
    int function_call_result;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_multiple_retry_conditions_async(test_handle, arg1, arg2, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_async_multiple_retry_conditions_complete(saved_callback)
        .CaptureArgumentValue_context(saved_context);

    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_multiple_retry_conditions_async)(test_handle, g.test_threadpool, arg1, arg2, mock_ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE, test_callback_context, &function_call_result);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);
}

static void setup_test_async_do_something_ref_counted_array_in_arg_async(ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE* saved_callback, void** saved_context)
{
    int function_call_result;
    const TEST_REFCOUNTED_HANDLE* saved_array;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_refcounted_inc_ref(test_ref_counted[0]));
    STRICT_EXPECTED_CALL(test_refcounted_inc_ref(test_ref_counted[1]));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_ref_counted_array_in_arg_async(test_handle, IGNORED_ARG, MU_COUNT_ARRAY_ITEMS(test_ref_counted), IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_ref_counted_array_in_arg_async_complete(saved_callback)
        .CaptureArgumentValue_context(saved_context)
        .CaptureArgumentValue_test_ref_counted_array(&saved_array);

    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_ref_counted_array_in_arg_async)(test_handle, g.test_threadpool, test_ref_counted, MU_COUNT_ARRAY_ITEMS(test_ref_counted), mock_ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);

    ASSERT_ARE_EQUAL(void_ptr, saved_array[0], test_ref_counted[0]);
    ASSERT_ARE_EQUAL(void_ptr, saved_array[1], test_ref_counted[1]);
}

static void setup_test_async_do_something_2_charptr_in_args_async_async(const char* arg1, const char* arg2, ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE* saved_callback, void** saved_context)
{
    int function_call_result;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)(IGNORED_ARG, arg1));
    STRICT_EXPECTED_CALL(ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)(IGNORED_ARG, arg2));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_2_charptr_in_args_async(test_handle, arg1, arg2, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_2_charptr_in_args_async_complete(saved_callback)
        .CaptureArgumentValue_context(saved_context);

    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_2_charptr_in_args_async)(test_handle, g.test_threadpool, arg1, arg2, mock_ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);
}

static void setup_test_async_do_something_with_multiple_enum_return_async(int arg1, int arg2, ON_DO_SOMETHING_ASYNC_COMPLETE* saved_callback, void** saved_context)
{
    TEST_ASYNC_API_SYNC_MULTIPLE_RESULT function_call_result;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_async(test_handle, arg1, arg2, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK)
        .CaptureArgumentValue_on_do_something_async_complete(saved_callback)
        .CaptureArgumentValue_context(saved_context);

    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_with_multiple_enum_return_async)(test_handle, g.test_threadpool, arg1, arg2, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);
}

static int hook_copy_const_charptr_t(charptr_t* destination_arg, const const_charptr_t source_arg)
{
    size_t size_needed = strlen(source_arg) + 1;
    *destination_arg = real_gballoc_hl_malloc(size_needed);
    ASSERT_IS_NOT_NULL(*destination_arg);

    (void)strcpy(*destination_arg, source_arg);

    return 0;
}

static void hook_free_const_charptr_t(charptr_t arg)
{
    real_gballoc_hl_free(arg);
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
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");

    REGISTER_TEST_REF_COUNTED_GLOBAL_MOCK_HOOKS();

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);

    THANDLE(REAL_THREADPOOL) temp = THANDLE_MALLOC(REAL_THREADPOOL)(dispose_REAL_THREADPOOL_do_nothing);
    ASSERT_IS_NOT_NULL(temp);
    THANDLE_MOVE(REAL_THREADPOOL)(&g.test_threadpool, &temp);
    THANDLE_GET_T(REAL_THREADPOOL)(g.test_threadpool)->dummy = 11;

    REGISTER_GLOBAL_MOCK_RETURNS(threadpool_create, g.test_threadpool, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(threadpool_schedule_work, hook_threadpool_schedule_work);

    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE_MOVE(THREADPOOL), THANDLE_INITIALIZE_MOVE(REAL_THREADPOOL));
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(THREADPOOL), THANDLE_INITIALIZE(REAL_THREADPOOL));
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(THREADPOOL), THANDLE_ASSIGN(REAL_THREADPOOL));

    REGISTER_GLOBAL_MOCK_HOOK(ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t), hook_copy_const_charptr_t);
    REGISTER_GLOBAL_MOCK_HOOK(ASYNC_TYPE_HELPER_FREE_HANDLER(const_charptr_t), hook_free_const_charptr_t);

    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(THREADPOOL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(THREADPOOL_WORK_FUNCTION, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TEST_ASYNC_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TEST_REFCOUNTED_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_NO_IN_ARGS_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_NO_OUT_ARGS_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_NO_ARGS_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_CHARPTR_IN_ARG_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_REF_COUNTED_IN_ARG_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_DO_SOMETHING_NO_RETRY_ASYNC_COMPLETE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(charptr_t, char*);
    REGISTER_UMOCK_ALIAS_TYPE(const_charptr_t, const char*);
    REGISTER_UMOCK_ALIAS_TYPE(const const_charptr_t, const char*);

    REGISTER_TYPE(TEST_ASYNC_API_DEFAULT_RESULT, TEST_ASYNC_API_DEFAULT_RESULT);
    REGISTER_TYPE(TEST_ASYNC_API_MULTIPLE_RETRY_RESULT, TEST_ASYNC_API_MULTIPLE_RETRY_RESULT);
    REGISTER_TYPE(TEST_ASYNC_API_SYNC_RESULT, TEST_ASYNC_API_SYNC_RESULT);
    REGISTER_TYPE(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    THANDLE_ASSIGN(REAL_THREADPOOL)(&g.test_threadpool, NULL);
    umock_c_deinit();
    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    for (size_t i = 0; i < MU_COUNT_ARRAY_ITEMS(test_ref_counted); i++)
    {
        test_ref_counted[i] = real_test_refcounted_create();
        ASSERT_IS_NOT_NULL(test_ref_counted[i]);
    }

    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    for (size_t i = 0; i < MU_COUNT_ARRAY_ITEMS(test_ref_counted); i++)
    {
        real_test_refcounted_dec_ref(test_ref_counted[i]);
    }

    umock_c_negative_tests_deinit();
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_015: [ ASYNC_RETRY_WRAPPER shall expand async_function_name to the name of the asynchronous retry wrapper around async_function_name. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_003: [ If async_handle is NULL, the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_INVALID_ARGS. ]*/
TEST_FUNCTION(async_retry_wrapper_called_with_NULL_handle_fails)
{
    /// arrange
    int function_call_result;

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_async)(NULL, g.test_threadpool, 42, 43, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_INVALID_ARGS, result);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_037: [ If threadpool is NULL, the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_INVALID_ARGS. ]*/
TEST_FUNCTION(async_retry_wrapper_called_with_NULL_threadpool_fails)
{
    /// arrange
    int function_call_result;

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_async)(test_handle, NULL, 42, 43, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_INVALID_ARGS, result);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_004: [ If the parameter specified in ARG_CB(type, name) is NULL, the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_INVALID_ARGS. ]*/
TEST_FUNCTION(async_retry_wrapper_called_with_NULL_callback_fails)
{
    /// arrange
    int function_call_result;

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_async)(test_handle, g.test_threadpool, 42, 43, NULL, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_INVALID_ARGS, result);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_042: [ If async_function_result is NULL, the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_INVALID_ARGS. ]*/
TEST_FUNCTION(async_retry_wrapper_called_with_NULL_async_function_result_fails)
{
    /// arrange

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_async)(test_handle, g.test_threadpool, 42, 43, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, NULL);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_INVALID_ARGS, result);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_005: [ The asynchronous retry wrapper shall allocate a context for the asynchronous call. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_011: [ The asynchronous retry wrapper shall call the function async_function_name, passing the in_args that have been copied to the retry context with the exception of ARG_CB(...) and ARG_CONTEXT(...) which are instead passed as the generated callback handler and the allocated context. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_039: [ If async_function_name returns a value other than expected_return then the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_CALL_ERROR. ]*/
TEST_FUNCTION(async_retry_wrapper_fails_when_async_call_fails)
{
    /// arrange
    int function_call_result;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_async)(test_handle, g.test_threadpool, 42, 43, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_CALL_ERROR, result);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_005: [ The asynchronous retry wrapper shall allocate a context for the asynchronous call. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_011: [ The asynchronous retry wrapper shall call the function async_function_name, passing the in_args that have been copied to the retry context with the exception of ARG_CB(...) and ARG_CONTEXT(...) which are instead passed as the generated callback handler and the allocated context. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_013: [ On success, the asynchronous retry wrapper shall return ASYNC_RETRY_WRAPPER_OK. ]*/
TEST_FUNCTION(async_retry_wrapper_succeeds)
{
    /// arrange
    int function_call_result;
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_async_complete(&saved_callback)
        .CaptureArgumentValue_context(&saved_context);

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_async)(test_handle, g.test_threadpool, 42, 43, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);

    /// cleanup
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_ERRORS, 1, 1);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_005: [ The asynchronous retry wrapper shall allocate a context for the asynchronous call. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_011: [ The asynchronous retry wrapper shall call the function async_function_name, passing the in_args that have been copied to the retry context with the exception of ARG_CB(...) and ARG_CONTEXT(...) which are instead passed as the generated callback handler and the allocated context. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_013: [ On success, the asynchronous retry wrapper shall return ASYNC_RETRY_WRAPPER_OK. ]*/
TEST_FUNCTION(async_retry_wrapper_without_in_args_succeeds)
{
    /// arrange
    int function_call_result;
    ON_DO_SOMETHING_NO_IN_ARGS_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_no_in_args_async(test_handle, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_no_in_args_async_complete(&saved_callback)
        .CaptureArgumentValue_context(&saved_context);

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_no_in_args_async)(test_handle, g.test_threadpool, mock_ON_DO_SOMETHING_NO_IN_ARGS_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);

    /// cleanup
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_ERRORS, 1, 1);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_005: [ The asynchronous retry wrapper shall allocate a context for the asynchronous call. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_006: [ The asynchronous retry wrapper shall copy each argument from IN_ARGS(...) to the allocated context. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_008: [ If an argument in IN_ARGS(...) has a type that does not define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type} as 1, then the asynchronous retry wrapper shall copy the argument by calling async_retry_wrapper_{type}_copy. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_011: [ The asynchronous retry wrapper shall call the function async_function_name, passing the in_args that have been copied to the retry context with the exception of ARG_CB(...) and ARG_CONTEXT(...) which are instead passed as the generated callback handler and the allocated context. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_013: [ On success, the asynchronous retry wrapper shall return ASYNC_RETRY_WRAPPER_OK. ]*/
TEST_FUNCTION(async_retry_wrapper_with_charptr_in_args_succeeds)
{
    /// arrange
    int function_call_result;
    ON_DO_SOMETHING_CHARPTR_IN_ARG_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)(IGNORED_ARG, "mystring"));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_charptr_in_arg_async(test_handle, "mystring", IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_charptr_in_arg_async_complete(&saved_callback)
        .CaptureArgumentValue_context(&saved_context);

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_charptr_in_arg_async)(test_handle, g.test_threadpool, "mystring", mock_ON_DO_SOMETHING_CHARPTR_IN_ARG_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);

    /// cleanup
    saved_callback(saved_context, NULL, TEST_ASYNC_RESULT_FOR_ERRORS);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_006: [ The asynchronous retry wrapper shall copy each argument from IN_ARGS(...) to the allocated context. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_008: [ If an argument in IN_ARGS(...) has a type that does not define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type} as 1, then the asynchronous retry wrapper shall copy the argument by calling async_retry_wrapper_{type}_copy. ]*/
TEST_FUNCTION(async_retry_wrapper_with_ref_counted_in_args_succeeds)
{
    /// arrange
    int function_call_result;
    ON_DO_SOMETHING_REF_COUNTED_IN_ARG_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_refcounted_inc_ref(test_ref_counted[0]));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_ref_counted_in_arg_async(test_handle, test_ref_counted[0], IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_ref_counted_in_arg_async_complete(&saved_callback)
        .CaptureArgumentValue_context(&saved_context);

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_ref_counted_in_arg_async)(test_handle, g.test_threadpool, test_ref_counted[0], mock_ON_DO_SOMETHING_REF_COUNTED_IN_ARG_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);

    /// cleanup
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_ERRORS);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_010: [ If there are any failures when copying the input arguments then the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_COPY_ARG_ERROR. ]*/
TEST_FUNCTION(async_retry_wrapper_with_charptr_in_args_fails_when_copying_fails)
{
    /// arrange
    int function_call_result;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)(IGNORED_ARG, "mystring"))
        .SetReturn(42);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_charptr_in_arg_async)(test_handle, g.test_threadpool, "mystring", mock_ON_DO_SOMETHING_CHARPTR_IN_ARG_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_COPY_ARG_ERROR, result);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_006: [ The asynchronous retry wrapper shall copy each argument from IN_ARGS(...) to the allocated context. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_008: [ If an argument in IN_ARGS(...) has a type that does not define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type} as 1, then the asynchronous retry wrapper shall copy the argument by calling async_retry_wrapper_{type}_copy. ]*/
TEST_FUNCTION(async_retry_wrapper_with_2_charptr_in_args_succeeds)
{
    /// arrange
    int function_call_result;
    ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)(IGNORED_ARG, "mystring"));
    STRICT_EXPECTED_CALL(ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)(IGNORED_ARG, "other"));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_2_charptr_in_args_async(test_handle, "mystring", "other", IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_2_charptr_in_args_async_complete(&saved_callback)
        .CaptureArgumentValue_context(&saved_context);

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_2_charptr_in_args_async)(test_handle, g.test_threadpool, "mystring", "other", mock_ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);

    /// cleanup
    saved_callback(saved_context, 0, TEST_ASYNC_RESULT_FOR_ERRORS);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_010: [ If there are any failures when copying the input arguments then the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_COPY_ARG_ERROR. ]*/
TEST_FUNCTION(async_retry_wrapper_with_2_charptr_in_args_fails_when_copying_first_fails)
{
    /// arrange
    int function_call_result;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)(IGNORED_ARG, "mystring"))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_2_charptr_in_args_async)(test_handle, g.test_threadpool, "mystring", "other", mock_ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_COPY_ARG_ERROR, result);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_010: [ If there are any failures when copying the input arguments then the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_COPY_ARG_ERROR. ]*/
TEST_FUNCTION(async_retry_wrapper_with_2_charptr_in_args_fails_when_copying_second_fails)
{
    /// arrange
    int function_call_result;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)(IGNORED_ARG, "mystring"));
    STRICT_EXPECTED_CALL(ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)(IGNORED_ARG, "other"))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(ASYNC_TYPE_HELPER_FREE_HANDLER(const_charptr_t)(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_2_charptr_in_args_async)(test_handle, g.test_threadpool, "mystring", "other", mock_ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_COPY_ARG_ERROR, result);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_006: [ The asynchronous retry wrapper shall copy each argument from IN_ARGS(...) to the allocated context. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_007: [ If an argument in IN_ARGS(...) is specified with ARG_EX(type, name, copy_function, free_function, ...) then the asynchronous retry wrapper shall call copy_function and pass the ... arguments in order to copy to the context. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_035: [ copy_function shall be used as a function with the following declaration: ]*/
/*Tests_SRS_ASYNC_TYPE_HELPER_42_001: [ ASYNC_TYPE_HELPER_COPY_HANDLER shall expand arg_type to the name of the copy handler for the arg_type. ]*/
TEST_FUNCTION(async_retry_wrapper_with_ref_counted_array_in_arg_succeeds)
{
    /// arrange
    int function_call_result;
    ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE saved_callback;
    void* saved_context;
    const TEST_REFCOUNTED_HANDLE* saved_array;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    // TEST_REFCOUNTED_HANDLE_ptr_t_copy
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_refcounted_inc_ref(test_ref_counted[0]));
    STRICT_EXPECTED_CALL(test_refcounted_inc_ref(test_ref_counted[1]));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_ref_counted_array_in_arg_async(test_handle, IGNORED_ARG, MU_COUNT_ARRAY_ITEMS(test_ref_counted), IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_ref_counted_array_in_arg_async_complete(&saved_callback)
        .CaptureArgumentValue_context(&saved_context)
        .CaptureArgumentValue_test_ref_counted_array(&saved_array);

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_ref_counted_array_in_arg_async)(test_handle, g.test_threadpool, test_ref_counted, MU_COUNT_ARRAY_ITEMS(test_ref_counted), mock_ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);

    ASSERT_ARE_EQUAL(void_ptr, saved_array[0], test_ref_counted[0]);
    ASSERT_ARE_EQUAL(void_ptr, saved_array[1], test_ref_counted[1]);

    /// cleanup
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_ERRORS, 0);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_010: [ If there are any failures when copying the input arguments then the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_COPY_ARG_ERROR. ]*/
TEST_FUNCTION(async_retry_wrapper_with_ref_counted_array_in_arg_fails_when_copy_fails)
{
    /// arrange
    int function_call_result;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    // TEST_REFCOUNTED_HANDLE_ptr_t_copy
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_ref_counted_array_in_arg_async)(test_handle, g.test_threadpool, test_ref_counted, MU_COUNT_ARRAY_ITEMS(test_ref_counted), mock_ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_COPY_ARG_ERROR, result);
}

// on_{async_function_name}_complete

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_016: [ If context is NULL, on_{async_function_name}_complete shall terminate the process. ]*/
TEST_FUNCTION(on_test_async_do_something_async_complete_with_NULL_context_terminates_program)
{
    /// arrange
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    setup_test_async_do_something_async(100, 200, &saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(ps_util_terminate_process());

    /// act
    saved_callback(NULL, TEST_ASYNC_DUMMY_RESULT_1, 1, 1);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    /// cleanup
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_ERRORS, 1, 1);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_018: [ on_{async_function_name}_complete shall call the user callback specified in ARG_CB(...), passing the context from ARG_CONTEXT(...), and the out_args as they were received by this callback handler. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_11_001: [ on_{async_function_name}_complete shall assign the threadpool to NULL. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_021: [ on_{async_function_name}_complete shall free the allocated context. ]*/
TEST_FUNCTION(on_test_async_do_something_async_complete_succeeds)
{
    /// arrange
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    setup_test_async_do_something_async(100, 200, &saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(mock_ON_DO_SOMETHING_ASYNC_COMPLETE(test_callback_context, TEST_ASYNC_DUMMY_RESULT_1, 500, 42));
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    saved_callback(saved_context, TEST_ASYNC_DUMMY_RESULT_1, 500, 42);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_018: [ on_{async_function_name}_complete shall call the user callback specified in ARG_CB(...), passing the context from ARG_CONTEXT(...), and the out_args as they were received by this callback handler. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_021: [ on_{async_function_name}_complete shall free the allocated context. ]*/
TEST_FUNCTION(on_test_async_do_something_async_complete_succeeds_other_result)
{
    /// arrange
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    setup_test_async_do_something_async(100, 200, &saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(mock_ON_DO_SOMETHING_ASYNC_COMPLETE(test_callback_context, TEST_ASYNC_DUMMY_RESULT_2, 2, 3));
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    saved_callback(saved_context, TEST_ASYNC_DUMMY_RESULT_2, 2, 3);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_018: [ on_{async_function_name}_complete shall call the user callback specified in ARG_CB(...), passing the context from ARG_CONTEXT(...), and the out_args as they were received by this callback handler. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_021: [ on_{async_function_name}_complete shall free the allocated context. ]*/
TEST_FUNCTION(on_test_async_do_something_async_complete_succeeds_other_result_2)
{
    /// arrange
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    setup_test_async_do_something_async(100, 200, &saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(mock_ON_DO_SOMETHING_ASYNC_COMPLETE(test_callback_context, (TEST_ASYNC_API_DEFAULT_RESULT)42, 2, 3));
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    saved_callback(saved_context, (TEST_ASYNC_API_DEFAULT_RESULT)42, 2, 3);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_019: [ on_{async_function_name}_complete shall free each argument in IN_ARGS(...) that is specified with ARG_EX(type, name, copy_function, free_function, ...) by calling free_function and passing the ... arguments. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_036: [ free_function shall be used as a function with the following declaration: ]*/
/*Tests_SRS_ASYNC_TYPE_HELPER_42_014: [ ASYNC_TYPE_HELPER_FREE_HANDLER shall expand arg_type to the name of the free handler for the arg_type. ]*/
TEST_FUNCTION(on_test_async_do_something_ref_counted_array_in_arg_async_complete_succeeds_frees_arguments)
{
    /// arrange
    ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    setup_test_async_do_something_ref_counted_array_in_arg_async(&saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(mock_ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE(test_callback_context, TEST_ASYNC_DUMMY_RESULT_1, 42));
    STRICT_EXPECTED_CALL(test_refcounted_dec_ref(test_ref_counted[0]));
    STRICT_EXPECTED_CALL(test_refcounted_dec_ref(test_ref_counted[1]));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    saved_callback(saved_context, TEST_ASYNC_DUMMY_RESULT_1, 42);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_020: [ on_{async_function_name}_complete shall free each argument in IN_ARGS(...) that has a type that does not define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type} as 1 by calling async_retry_wrapper_{type}_free. ]*/
TEST_FUNCTION(on_test_async_do_something_2_charptr_in_args_async_complete_succeeds_frees_arguments)
{
    /// arrange
    ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    setup_test_async_do_something_2_charptr_in_args_async_async("foo", "bar", &saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(mock_ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE(test_callback_context, 42, TEST_ASYNC_DUMMY_RESULT_1));
    STRICT_EXPECTED_CALL(ASYNC_TYPE_HELPER_FREE_HANDLER(const_charptr_t)("foo"));
    STRICT_EXPECTED_CALL(ASYNC_TYPE_HELPER_FREE_HANDLER(const_charptr_t)("bar"));
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    saved_callback(saved_context, 42, TEST_ASYNC_DUMMY_RESULT_1);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_01_001: [ If any error occurs, on_{async_function_name}_complete shall call the user callback specified in ARG_CB(...), passing the context from ARG_CONTEXT(...), and the error_value from all of the ARG(type, name, error_value)'s in out_args. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_021: [ on_{async_function_name}_complete shall free the allocated context. ]*/
TEST_FUNCTION(when_threadpool_schedule_work_fails_error_is_indicated)
{
    /// arrange
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    setup_test_async_do_something_async(100, 200, &saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(MU_FAILURE);

    STRICT_EXPECTED_CALL(mock_ON_DO_SOMETHING_ASYNC_COMPLETE(test_callback_context, TEST_ASYNC_RESULT_FOR_ERRORS, 0, -1));
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_RETRY, 500, 42);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_006: [ The asynchronous retry wrapper shall copy each argument from IN_ARGS(...) to the allocated context. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_009: [ Otherwise, the asynchronous retry wrapper shall copy the argument to the context via assignment. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_017: [ If the out_arg specified as ENUM(type, name, error_value) has one of the values from RETRY_ON_ASYNC(...) then on_{async_function_name}_complete shall call threadpool_schedule_work with {async_function_name}_do_retry as the work_function to retry the asynchronous call and return. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_023: [ {async_function_name}_do_retry shall call the function async_function_name, passing the in_args that have been copied to the retry context with the exception of ARG_CB(...) and ARG_CONTEXT(...) which are instead passed as the generated callback handler and the allocated context. ]*/
TEST_FUNCTION(on_test_async_do_something_async_complete_retries_once)
{
    /// arrange
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback2;
    void* saved_context;
    void* saved_context2;

    setup_test_async_do_something_async(100, 200, &saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_async(test_handle, 100, 200, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_async_complete(&saved_callback2)
        .CaptureArgumentValue_context(&saved_context2);

    /// act
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_RETRY, 500, 42);
    saved_threadpool_work(saved_threadpool_context);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    /// cleanup
    saved_callback2(saved_context2, TEST_ASYNC_DUMMY_RESULT_1, 500, 42);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_017: [ If the out_arg specified as ENUM(type, name, error_value) has one of the values from RETRY_ON_ASYNC(...) then on_{async_function_name}_complete shall call threadpool_schedule_work with {async_function_name}_do_retry as the work_function to retry the asynchronous call and return. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_023: [ {async_function_name}_do_retry shall call the function async_function_name, passing the in_args that have been copied to the retry context with the exception of ARG_CB(...) and ARG_CONTEXT(...) which are instead passed as the generated callback handler and the allocated context. ]*/
TEST_FUNCTION(on_test_async_do_something_async_complete_retries_until_enum_is_different)
{
    /// arrange
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback2;
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback3;
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback4;
    void* saved_context;
    void* saved_context2;
    void* saved_context3;
    void* saved_context4;

    setup_test_async_do_something_async(100, 200, &saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_async(test_handle, 100, 200, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_async_complete(&saved_callback2)
        .CaptureArgumentValue_context(&saved_context2);

    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_async(test_handle, 100, 200, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_async_complete(&saved_callback3)
        .CaptureArgumentValue_context(&saved_context3);

    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_async(test_handle, 100, 200, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_async_complete(&saved_callback4)
        .CaptureArgumentValue_context(&saved_context4);

    STRICT_EXPECTED_CALL(mock_ON_DO_SOMETHING_ASYNC_COMPLETE(test_callback_context, TEST_ASYNC_RESULT_FOR_ERRORS, 500, 45));
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_RETRY, 500, 42);
    saved_threadpool_work(saved_threadpool_context);
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_RETRY, 500, 43);
    saved_threadpool_work(saved_threadpool_context);
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_RETRY, 500, 44);
    saved_threadpool_work(saved_threadpool_context);
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_ERRORS, 500, 45);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_017: [ If the out_arg specified as ENUM(type, name, error_value) has one of the values from RETRY_ON_ASYNC(...) then on_{async_function_name}_complete shall call threadpool_schedule_work with {async_function_name}_do_retry as the work_function to retry the asynchronous call and return. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_023: [ {async_function_name}_do_retry shall call the function async_function_name, passing the in_args that have been copied to the retry context with the exception of ARG_CB(...) and ARG_CONTEXT(...) which are instead passed as the generated callback handler and the allocated context. ]*/
TEST_FUNCTION(on_test_async_do_something_multiple_retry_conditions_async_complete_retries_until_enum_is_not_a_retry_value)
{
    /// arrange
    ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE saved_callback;
    ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE saved_callback2;
    ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE saved_callback3;
    ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE saved_callback4;
    void* saved_context;
    void* saved_context2;
    void* saved_context3;
    void* saved_context4;

    setup_test_async_do_something_multiple_retry_conditions_async(100, 200, &saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_multiple_retry_conditions_async(test_handle, 100, 200, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_async_multiple_retry_conditions_complete(&saved_callback2)
        .CaptureArgumentValue_context(&saved_context2);

    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_multiple_retry_conditions_async(test_handle, 100, 200, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_async_multiple_retry_conditions_complete(&saved_callback3)
        .CaptureArgumentValue_context(&saved_context3);

    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_multiple_retry_conditions_async(test_handle, 100, 200, IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_async_multiple_retry_conditions_complete(&saved_callback4)
        .CaptureArgumentValue_context(&saved_context4);

    STRICT_EXPECTED_CALL(mock_ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE(test_callback_context, TEST_ASYNC_MULTIPLE_RETRY_OK, 7, 8));
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    saved_callback(saved_context, TEST_ASYNC_MULTIPLE_RETRY_RETRY_1, 1, 2);
    saved_threadpool_work(saved_threadpool_context);
    saved_callback(saved_context, TEST_ASYNC_MULTIPLE_RETRY_RETRY_2, 3, 4);
    saved_threadpool_work(saved_threadpool_context);
    saved_callback(saved_context, TEST_ASYNC_MULTIPLE_RETRY_RETRY_3, 5, 6);
    saved_threadpool_work(saved_threadpool_context);
    saved_callback(saved_context, TEST_ASYNC_MULTIPLE_RETRY_OK, 7, 8);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_023: [ {async_function_name}_do_retry shall call the function async_function_name, passing the in_args that have been copied to the retry context with the exception of ARG_CB(...) and ARG_CONTEXT(...) which are instead passed as the generated callback handler and the allocated context. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_041: [ If async_function_name returns a value other than expected_return then: ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_025: [ {async_function_name}_do_retry shall call the user callback specified in ARG_CB(...), passing the context from ARG_CONTEXT(...), and the error_value from all of the ARG(type, name, error_value)'s in out_args. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_028: [ {async_function_name}_do_retry shall free the allocated context. ]*/
TEST_FUNCTION(on_test_async_do_something_async_complete_calls_callback_when_retry_fails)
{
    /// arrange
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    setup_test_async_do_something_async(100, 200, &saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_async(test_handle, 100, 200, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mock_ON_DO_SOMETHING_ASYNC_COMPLETE(test_callback_context, TEST_ASYNC_RESULT_FOR_ERRORS, 0, -1));
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_RETRY, 500, 42);
    saved_threadpool_work(saved_threadpool_context);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_023: [ {async_function_name}_do_retry shall call the function async_function_name, passing the in_args that have been copied to the retry context with the exception of ARG_CB(...) and ARG_CONTEXT(...) which are instead passed as the generated callback handler and the allocated context. ]*/
TEST_FUNCTION(on_test_async_do_something_ref_counted_array_in_arg_async_complete_does_retry)
{
    /// arrange
    ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE saved_callback;
    ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE saved_callback2;
    void* saved_context;
    void* saved_context2;
    const TEST_REFCOUNTED_HANDLE* saved_array;

    setup_test_async_do_something_ref_counted_array_in_arg_async(&saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_ref_counted_array_in_arg_async(test_handle, IGNORED_ARG, MU_COUNT_ARRAY_ITEMS(test_ref_counted), IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_ref_counted_array_in_arg_async_complete(&saved_callback2)
        .CaptureArgumentValue_context(&saved_context2)
        .CaptureArgumentValue_test_ref_counted_array(&saved_array);

    /// act
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_RETRY, 42);
    saved_threadpool_work(saved_threadpool_context);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ASSERT_ARE_EQUAL(void_ptr, saved_array[0], test_ref_counted[0]);
    ASSERT_ARE_EQUAL(void_ptr, saved_array[1], test_ref_counted[1]);

    /// cleanup
    saved_callback2(saved_context2, TEST_ASYNC_RESULT_FOR_ERRORS, 42);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_041: [ If async_function_name returns a value other than expected_return then: ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_025: [ {async_function_name}_do_retry shall call the user callback specified in ARG_CB(...), passing the context from ARG_CONTEXT(...), and the error_value from all of the ARG(type, name, error_value)'s in out_args. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_026: [ {async_function_name}_do_retry shall free each argument in IN_ARGS(...) that is specified with ARG_EX(type, name, copy_function, free_function, ...) by calling free_function and passing the ... arguments. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_028: [ {async_function_name}_do_retry shall free the allocated context. ]*/
TEST_FUNCTION(on_test_async_do_something_ref_counted_array_in_arg_async_complete_frees_resources_when_retry_fails)
{
    /// arrange
    ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    setup_test_async_do_something_ref_counted_array_in_arg_async(&saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_ref_counted_array_in_arg_async(test_handle, IGNORED_ARG, MU_COUNT_ARRAY_ITEMS(test_ref_counted), IGNORED_ARG, IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mock_ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE(test_callback_context, TEST_ASYNC_RESULT_FOR_ERRORS, 0));
    STRICT_EXPECTED_CALL(test_refcounted_dec_ref(test_ref_counted[0]));
    STRICT_EXPECTED_CALL(test_refcounted_dec_ref(test_ref_counted[1]));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_RETRY, 42);
    saved_threadpool_work(saved_threadpool_context);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_023: [ {async_function_name}_do_retry shall call the function async_function_name, passing the in_args that have been copied to the retry context with the exception of ARG_CB(...) and ARG_CONTEXT(...) which are instead passed as the generated callback handler and the allocated context. ]*/
TEST_FUNCTION(on_test_async_do_something_2_charptr_in_args_async_complete_does_retry)
{
    /// arrange
    ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE saved_callback;
    ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE saved_callback2;
    void* saved_context;
    void* saved_context2;

    setup_test_async_do_something_2_charptr_in_args_async_async("foo", "bar", &saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_2_charptr_in_args_async(test_handle, "foo", "bar", IGNORED_ARG, IGNORED_ARG))
        .CaptureArgumentValue_on_do_something_2_charptr_in_args_async_complete(&saved_callback2)
        .CaptureArgumentValue_context(&saved_context2);

    /// act
    saved_callback(saved_context, 42, TEST_ASYNC_RESULT_FOR_RETRY);
    saved_threadpool_work(saved_threadpool_context);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    /// cleanup
    saved_callback(saved_context, 44, TEST_ASYNC_DUMMY_RESULT_2);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_041: [ If async_function_name returns a value other than expected_return then: ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_025: [ {async_function_name}_do_retry shall call the user callback specified in ARG_CB(...), passing the context from ARG_CONTEXT(...), and the error_value from all of the ARG(type, name, error_value)'s in out_args. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_027: [ {async_function_name}_do_retry shall free each argument in IN_ARGS(...) that has a type that does not define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type} as 1 by calling async_retry_wrapper_{type}_free. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_028: [ {async_function_name}_do_retry shall free the allocated context. ]*/
TEST_FUNCTION(on_test_async_do_something_2_charptr_in_args_async_complete_frees_resources_when_retry_fails)
{
    /// arrange
    ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    setup_test_async_do_something_2_charptr_in_args_async_async("foo", "bar", &saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_2_charptr_in_args_async(test_handle, "foo", "bar", IGNORED_ARG, IGNORED_ARG))
        .SetReturn(1);
    STRICT_EXPECTED_CALL(mock_ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE(test_callback_context, UINT32_MAX, TEST_ASYNC_RESULT_FOR_ERRORS));
    STRICT_EXPECTED_CALL(ASYNC_TYPE_HELPER_FREE_HANDLER(const_charptr_t)("foo"));
    STRICT_EXPECTED_CALL(ASYNC_TYPE_HELPER_FREE_HANDLER(const_charptr_t)("bar"));
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    saved_callback(saved_context, 42, TEST_ASYNC_RESULT_FOR_RETRY);
    saved_threadpool_work(saved_threadpool_context);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_039: [ If async_function_name returns a value other than expected_return then the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_CALL_ERROR. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_043: [ The asynchronous retry wrapper shall store the result of async_function_name in async_function_result. ]*/
TEST_FUNCTION(async_retry_wrapper_fails_when_async_call_fails_using_other_enum_return)
{
    /// arrange
    TEST_ASYNC_API_SYNC_RESULT function_call_result;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_FOR_ERRORS);
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_with_enum_return_async)(test_handle, g.test_threadpool, 42, 43, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_CALL_ERROR, result);
    ASSERT_ARE_EQUAL(TEST_ASYNC_API_SYNC_RESULT, TEST_ASYNC_API_SYNC_FOR_ERRORS, function_call_result);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_039: [ If async_function_name returns a value other than expected_return then the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_CALL_ERROR. ]*/
TEST_FUNCTION(async_retry_wrapper_succeeds_when_async_call_succeeds_using_other_enum_return)
{
    /// arrange
    TEST_ASYNC_API_SYNC_RESULT function_call_result;
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_RESULT_OK)
        .CaptureArgumentValue_on_do_something_async_complete(&saved_callback)
        .CaptureArgumentValue_context(&saved_context);

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_with_enum_return_async)(test_handle, g.test_threadpool, 42, 43, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);
    ASSERT_ARE_EQUAL(TEST_ASYNC_API_SYNC_RESULT, TEST_ASYNC_API_SYNC_RESULT_OK, function_call_result);

    /// cleanup
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_ERRORS, 1, 1);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_038: [ While async_function_name returns one of the values from RETRY_ON_SYNC(...), it shall be called again in a loop. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_044: [ Before each retry of the function, the asynchronous retry wrapper shall yield execution by a call to ThreadAPI_Sleep. ]*/
TEST_FUNCTION(async_retry_wrapper_retries_synchronously)
{
    /// arrange
    TEST_ASYNC_API_SYNC_RESULT function_call_result;
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    for (uint32_t i = 0; i < 5; i++)
    {
        STRICT_EXPECTED_CALL(test_async_do_something_with_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
            .SetReturn(TEST_ASYNC_API_SYNC_FOR_RETRY);
        STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(test_async_do_something_with_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_RESULT_OK)
        .CaptureArgumentValue_on_do_something_async_complete(&saved_callback)
        .CaptureArgumentValue_context(&saved_context);

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_with_enum_return_async)(test_handle, g.test_threadpool, 42, 43, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);

    /// cleanup
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_ERRORS, 1, 1);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_038: [ While async_function_name returns one of the values from RETRY_ON_SYNC(...), it shall be called again in a loop. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_044: [ Before each retry of the function, the asynchronous retry wrapper shall yield execution by a call to ThreadAPI_Sleep. ]*/
TEST_FUNCTION(async_retry_wrapper_retries_synchronously_then_fails)
{
    /// arrange
    TEST_ASYNC_API_SYNC_RESULT function_call_result;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    for (uint32_t i = 0; i < 5; i++)
    {
        STRICT_EXPECTED_CALL(test_async_do_something_with_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
            .SetReturn(TEST_ASYNC_API_SYNC_FOR_RETRY);
        STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(test_async_do_something_with_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_FOR_ERRORS);
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_with_enum_return_async)(test_handle, g.test_threadpool, 42, 43, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_CALL_ERROR, result);
    ASSERT_ARE_EQUAL(TEST_ASYNC_API_SYNC_RESULT, TEST_ASYNC_API_SYNC_FOR_ERRORS, function_call_result);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_038: [ While async_function_name returns one of the values from RETRY_ON_SYNC(...), it shall be called again in a loop. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_044: [ Before each retry of the function, the asynchronous retry wrapper shall yield execution by a call to ThreadAPI_Sleep. ]*/
TEST_FUNCTION(async_retry_wrapper_retries_synchronously_with_multiple_possible_return_values)
{
    /// arrange
    TEST_ASYNC_API_SYNC_MULTIPLE_RESULT function_call_result;
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_1);
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_2);
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_3);
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK)
        .CaptureArgumentValue_on_do_something_async_complete(&saved_callback)
        .CaptureArgumentValue_context(&saved_context);

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_with_multiple_enum_return_async)(test_handle, g.test_threadpool, 42, 43, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);

    /// cleanup
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_ERRORS, 1, 1);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_038: [ While async_function_name returns one of the values from RETRY_ON_SYNC(...), it shall be called again in a loop. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_044: [ Before each retry of the function, the asynchronous retry wrapper shall yield execution by a call to ThreadAPI_Sleep. ]*/
TEST_FUNCTION(async_retry_wrapper_retries_synchronously_with_multiple_possible_return_values_no_async_retries)
{
    /// arrange
    TEST_ASYNC_API_SYNC_MULTIPLE_RESULT function_call_result;
    ON_DO_SOMETHING_NO_RETRY_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_no_async_retry_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_1);
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_no_async_retry_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_2);
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_no_async_retry_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_3);
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_no_async_retry_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK)
        .CaptureArgumentValue_on_do_something_no_retry_async_complete(&saved_callback)
        .CaptureArgumentValue_context(&saved_context);

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER(test_async_do_something_with_multiple_enum_return_no_async_retry_async)(test_handle, g.test_threadpool, 42, 43, mock_ON_DO_SOMETHING_NO_RETRY_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);

    /// cleanup
    saved_callback(saved_context, 1, 1);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_040: [ While async_function_name returns one of the values from RETRY_ON_SYNC(...), it shall be called again in a loop. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_045: [ Before each retry of the function, {async_function_name}_do_retry shall yield execution by a call to ThreadAPI_Sleep. ]*/
TEST_FUNCTION(test_async_do_something_with_multiple_enum_return_async_retries_synchronously_in_async_retry)
{
    /// arrange
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback2;
    void* saved_context;
    void* saved_context2;

    setup_test_async_do_something_with_multiple_enum_return_async(42, 43, &saved_callback, &saved_context);

    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_1);
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_2);
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_3);
    STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK)
        .CaptureArgumentValue_on_do_something_async_complete(&saved_callback2)
        .CaptureArgumentValue_context(&saved_context2);

    /// act
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_RETRY, 500, 42);
    saved_threadpool_work(saved_threadpool_context);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    /// cleanup
    saved_callback2(saved_context2, TEST_ASYNC_DUMMY_RESULT_1, 500, 42);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_048: [ {async_function_name}_async_retry_wrapper_with_timeout shall get the current time by calling timer_global_get_elapsed_ms. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_050: [ {async_function_name}_async_retry_wrapper_with_timeout shall otherwise behave the same as {async_function_name}_async_retry_wrapper. ]*/
TEST_FUNCTION(async_retry_wrapper_retries_synchronously_with_timeout)
{
    /// arrange
    TEST_ASYNC_API_SYNC_MULTIPLE_RESULT function_call_result;
    ON_DO_SOMETHING_NO_RETRY_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(1000);
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    for (uint32_t i = 0; i < 5; i++)
    {
        STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_no_async_retry_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
            .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_1);
        STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
            .SetReturn(1000 + i);
        STRICT_EXPECTED_CALL(ThreadAPI_Sleep(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_no_async_retry_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK)
        .CaptureArgumentValue_on_do_something_no_retry_async_complete(&saved_callback)
        .CaptureArgumentValue_context(&saved_context);

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER_WITH_TIMEOUT(test_async_do_something_with_multiple_enum_return_no_async_retry_async)(test_handle, g.test_threadpool, 1000, 42, 43, mock_ON_DO_SOMETHING_NO_RETRY_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);

    /// cleanup
    saved_callback(saved_context, 1, 1);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_049: [ Before each retry of the function, If timeout_ms milliseconds have elapsed then {async_function_name}_async_retry_wrapper_with_timeout shall fail and return ASYNC_RETRY_WRAPPER_TIMEOUT. ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_050: [ {async_function_name}_async_retry_wrapper_with_timeout shall otherwise behave the same as {async_function_name}_async_retry_wrapper. ]*/
TEST_FUNCTION(async_retry_wrapper_retries_synchronously_with_timeout_fails_when_function_times_out)
{
    /// arrange
    TEST_ASYNC_API_SYNC_MULTIPLE_RESULT function_call_result;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(1000);
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_no_async_retry_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_1);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(1000 + 1000);
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER_WITH_TIMEOUT(test_async_do_something_with_multiple_enum_return_no_async_retry_async)(test_handle, g.test_threadpool, 1000, 42, 43, mock_ON_DO_SOMETHING_NO_RETRY_ASYNC_COMPLETE, test_callback_context, &function_call_result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_TIMEOUT, result);
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_02_001: [ Before each retry of the function, if timeout_ms milliseconds have elapsed since the initial call to ASYNC_RETRY_WRAPPER(async_function_name) then {async_function_name}_async_retry_wrapper_with_timeout shall call shall call the user callback specified in ARG_CB(...), passing the context from ARG_CONTEXT(...), the error_value from all of the ARG(type, name, error_value)'s in out_args. and the timeout_error_value for the ENUM(...) argument. ]*/
TEST_FUNCTION(async_retry_wrapper_without_timeout_2_calls_user_callback_with_error_value) /*note: same test as before, but uses test_async_do_something_async_with_timeout_2 instead of test_async_do_something_async_with_timeout. test_async_do_something_async_with_timeout_2 is defined without a TIMEOUT_ERROR_VALUE*/
{
    ///arrange
    int function_call_result = -1;/*want to see it changed*/
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(1000);
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_async_with_timeout(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(0) /*0 means "callback will come"*/
        .CaptureArgumentValue_on_do_something_async_complete(&saved_callback)
        .CaptureArgumentValue_context(&saved_context);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(1000 + 2500); /*timeout, because from start (1000), 2500 ms passed so now we timeout*/

    /*note: TEST_ASYNC_RESULT_FOR_TIMEOUT below*/
    STRICT_EXPECTED_CALL(mock_ON_DO_SOMETHING_ASYNC_COMPLETE(test_callback_context, TEST_ASYNC_RESULT_FOR_TIMEOUT, 0, -1));
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act(1) - sync
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER_WITH_TIMEOUT(test_async_do_something_async_with_timeout)(test_handle, g.test_threadpool, 2500 /*ms*/, 42, 43, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, &function_call_result);
    ///act(2) - async
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_RETRY, 500, 501);


    /// assert
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);
    ASSERT_ARE_EQUAL(int, 0, function_call_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_02_002: [ If there's no timeout_error_value for ENUM(...) argument then the error_value shall be instead passed. ]*/
TEST_FUNCTION(async_retry_wrapper_with_timeout_calls_user_callback_with_timeout_value)
{
    ///arrange
    int function_call_result = -1;/*want to see it changed*/
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(1000);
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_async_with_timeout_2(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(0) /*0 means "callback will come"*/
        .CaptureArgumentValue_on_do_something_async_complete(&saved_callback)
        .CaptureArgumentValue_context(&saved_context);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(1000 +  2500); /*timeout, because from start (1000), 2500 ms passed so now we timeout*/ /*and since TIMEOUT_ERROR_VALUE is not with ENUM of test_async_do_something_async_with_timeout_2, we expect the error value to come to the user callback*/

    /*note: TEST_ASYNC_RESULT_FOR_ERRORS below, since there's no TIMEOUT specified at define_retry_wrapper time*/
    STRICT_EXPECTED_CALL(mock_ON_DO_SOMETHING_ASYNC_COMPLETE(test_callback_context, TEST_ASYNC_RESULT_FOR_ERRORS, 0, -1));
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act(1) - sync
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER_WITH_TIMEOUT(test_async_do_something_async_with_timeout_2)(test_handle, g.test_threadpool, 2500 /*ms*/, 42, 43, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, &function_call_result);
    ///act(2) - async
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_RETRY, 500, 501);

    /// assert
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);
    ASSERT_ARE_EQUAL(int, 0, function_call_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_02_003: [ If the out_arg specified as ENUM(type, name, error_value) has one of the values from RETRY_ON_ASYNC(...) and the the time measured from the initial call to ASYNC_RETRY_WRAPPER() exceeded timeout_ms then the user callback  specified in ARG_CB(...) shall be called, passing the context from ARG_CONTEXT(...), and the out_args as they were received by this callback handler with the exception of the ENUM(...) which will have the value specified for timeout_error_value. ]*/
TEST_FUNCTION(async_retry_wrapper_with_timeout_calls_user_callback_with_timeout_value_for_ENUM)
{
    ///arrange
    TEST_ASYNC_API_SYNC_MULTIPLE_RESULT function_call_result = TEST_ASYNC_API_SYNC_MULTIPLE_FOR_ERRORS;/*want to see it changed*/
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(1000);
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_async_with_timeout(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK) /*TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK means "callback will come"*/
        .CaptureArgumentValue_on_do_something_async_complete(&saved_callback)
        .CaptureArgumentValue_context(&saved_context);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(1000 + 500); /*no timeout yet, timeout would be at 1000+2500*/
    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_async_with_timeout(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_1); /*TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_1 means the synchronous call needs to be retried*/
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(1000 + 2500); /*timed out*/

    /*note: TEST_ASYNC_RESULT_FOR_TIMEOUT below*/
    STRICT_EXPECTED_CALL(mock_ON_DO_SOMETHING_ASYNC_COMPLETE(test_callback_context, TEST_ASYNC_RESULT_FOR_TIMEOUT, 0, -1));
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act(1) - sync
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER_WITH_TIMEOUT(test_async_do_something_with_multiple_enum_return_async_with_timeout)(test_handle, g.test_threadpool, 2500 /*ms*/, 42, 43, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, &function_call_result);
    ///act(2) - async callback comes with with a retry value
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_RETRY, 500, 501);
    ///act(3) - threadpool decides to call the user callback
    saved_threadpool_work(saved_threadpool_context);

    /// assert
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);
    ASSERT_ARE_EQUAL(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK, function_call_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_02_004: [ If there's no timeout_error_value specified by ENUM(...) macro then the error_value shall be used instead. ]*/
TEST_FUNCTION(async_retry_wrapper_with_timeout_calls_user_callback_with_error_value_for_ENUM)
{
    ///arrange
    TEST_ASYNC_API_SYNC_MULTIPLE_RESULT function_call_result = TEST_ASYNC_API_SYNC_MULTIPLE_FOR_ERRORS;/*want to see it changed*/
    ON_DO_SOMETHING_ASYNC_COMPLETE saved_callback;
    void* saved_context;

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(1000);
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(THREADPOOL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK) /*TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK means "callback will come"*/
        .CaptureArgumentValue_on_do_something_async_complete(&saved_callback)
        .CaptureArgumentValue_context(&saved_context);
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(1000 + 500); /*no timeout yet, timeout would be at 1000+2500*/
    STRICT_EXPECTED_CALL(threadpool_schedule_work(g.test_threadpool, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_async_do_something_with_multiple_enum_return_async(test_handle, 42, 43, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_1); /*TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_1 means the synchronous call needs to be retried*/
    STRICT_EXPECTED_CALL(timer_global_get_elapsed_ms())
        .SetReturn(1000 + 2500); /*timed out*/

    /*note: TEST_ASYNC_RESULT_FOR_ERRORS below, since there's no TIMEOUT_ERROR_VALUE specified for ENUM of test_async_do_something_with_multiple_enum_return_async*/
    STRICT_EXPECTED_CALL(mock_ON_DO_SOMETHING_ASYNC_COMPLETE(test_callback_context, TEST_ASYNC_RESULT_FOR_ERRORS, 0, -1));
    STRICT_EXPECTED_CALL(THREADPOOL_ASSIGN(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act(1) - sync
    ASYNC_RETRY_WRAPPER_RESULT result = ASYNC_RETRY_WRAPPER_WITH_TIMEOUT(test_async_do_something_with_multiple_enum_return_async)(test_handle, g.test_threadpool, 2500 /*ms*/, 42, 43, mock_ON_DO_SOMETHING_ASYNC_COMPLETE, test_callback_context, &function_call_result);
    ///act(2) - async callback comes with with a retry value
    saved_callback(saved_context, TEST_ASYNC_RESULT_FOR_RETRY, 500, 501);
    ///act(3) - threadpool decides to call the user callback
    saved_threadpool_work(saved_threadpool_context);

    /// assert
    ASSERT_ARE_EQUAL(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_OK, result);
    ASSERT_ARE_EQUAL(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK, function_call_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
