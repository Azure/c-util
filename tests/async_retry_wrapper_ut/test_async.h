// Copyright (c) Microsoft. All rights reserved.

#ifndef TEST_ASYNC_H
#define TEST_ASYNC_H

#include <stddef.h>
#include <stdint.h>


#include "macro_utils/macro_utils.h"
#include "test_ref_counted.h"
#include "umock_c/umock_c_prod.h"



#define TEST_ASYNC_API_DEFAULT_RESULT_VALUES \
    TEST_ASYNC_DUMMY_RESULT_1, \
    TEST_ASYNC_DUMMY_RESULT_2, \
    TEST_ASYNC_RESULT_FOR_RETRY, \
    TEST_ASYNC_RESULT_FOR_ERRORS, \
    TEST_ASYNC_RESULT_FOR_TIMEOUT

MU_DEFINE_ENUM(TEST_ASYNC_API_DEFAULT_RESULT, TEST_ASYNC_API_DEFAULT_RESULT_VALUES);

#define TEST_ASYNC_API_MULTIPLE_RETRY_RESULT_VALUES \
    TEST_ASYNC_MULTIPLE_RETRY_OK, \
    TEST_ASYNC_MULTIPLE_RETRY_RETRY_1, \
    TEST_ASYNC_MULTIPLE_RETRY_RETRY_2, \
    TEST_ASYNC_MULTIPLE_RETRY_RETRY_3, \
    TEST_ASYNC_MULTIPLE_RETRY_ERROR, \
    TEST_ASYNC_MULTIPLE_RETRY_TIMEOUT

MU_DEFINE_ENUM(TEST_ASYNC_API_MULTIPLE_RETRY_RESULT, TEST_ASYNC_API_MULTIPLE_RETRY_RESULT_VALUES);

#define TEST_ASYNC_API_SYNC_RESULT_VALUES \
    TEST_ASYNC_API_SYNC_RESULT_OK, \
    TEST_ASYNC_API_SYNC_FOR_RETRY, \
    TEST_ASYNC_API_SYNC_FOR_ERRORS

MU_DEFINE_ENUM(TEST_ASYNC_API_SYNC_RESULT, TEST_ASYNC_API_SYNC_RESULT_VALUES);

#define TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_VALUES \
    TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK, \
    TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_1, \
    TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_2, \
    TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_3, \
    TEST_ASYNC_API_SYNC_MULTIPLE_FOR_ERRORS

MU_DEFINE_ENUM(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_VALUES);

typedef void (*ON_DO_SOMETHING_ASYNC_COMPLETE)(void* context, TEST_ASYNC_API_DEFAULT_RESULT result, int callback_arg1, int callback_arg2);
typedef void (*ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE)(void* context, TEST_ASYNC_API_MULTIPLE_RETRY_RESULT result, int callback_arg1, int callback_arg2);
typedef void (*ON_DO_SOMETHING_NO_IN_ARGS_ASYNC_COMPLETE)(void* context, TEST_ASYNC_API_DEFAULT_RESULT result, int callback_arg1, int callback_arg2);
typedef void (*ON_DO_SOMETHING_NO_OUT_ARGS_ASYNC_COMPLETE)(void* context, TEST_ASYNC_API_DEFAULT_RESULT result);
typedef void (*ON_DO_SOMETHING_NO_ARGS_ASYNC_COMPLETE)(void* context, TEST_ASYNC_API_DEFAULT_RESULT result);
typedef void (*ON_DO_SOMETHING_CHARPTR_IN_ARG_ASYNC_COMPLETE)(void* context, const char* charptr_arg, TEST_ASYNC_API_DEFAULT_RESULT result);
typedef void (*ON_DO_SOMETHING_REF_COUNTED_IN_ARG_ASYNC_COMPLETE)(void* context, TEST_ASYNC_API_DEFAULT_RESULT result);
typedef void (*ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE)(void* context, uint32_t value, TEST_ASYNC_API_DEFAULT_RESULT result);
typedef void (*ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE)(void* context, TEST_ASYNC_API_DEFAULT_RESULT result, uint8_t value);
typedef void (*ON_DO_SOMETHING_NO_RETRY_ASYNC_COMPLETE)(void* context, int callback_arg1, int callback_arg2);

typedef struct TEST_ASYNC_TAG* TEST_ASYNC_HANDLE;

MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_async, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_multiple_retry_conditions_async, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE, on_do_something_async_multiple_retry_conditions_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_no_in_args_async, TEST_ASYNC_HANDLE, test_async_api, ON_DO_SOMETHING_NO_IN_ARGS_ASYNC_COMPLETE, on_do_something_no_in_args_async_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_no_out_args_async, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_NO_OUT_ARGS_ASYNC_COMPLETE, on_do_something_no_out_args_async_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_no_args_async, TEST_ASYNC_HANDLE, test_async_api, ON_DO_SOMETHING_NO_ARGS_ASYNC_COMPLETE, on_do_something_no_args_async_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_charptr_in_arg_async, TEST_ASYNC_HANDLE, test_async_api, const char*, charptr_arg, ON_DO_SOMETHING_CHARPTR_IN_ARG_ASYNC_COMPLETE, on_do_something_charptr_in_arg_async_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_ref_counted_in_arg_async, TEST_ASYNC_HANDLE, test_async_api, TEST_REFCOUNTED_HANDLE, test_ref_counted, ON_DO_SOMETHING_REF_COUNTED_IN_ARG_ASYNC_COMPLETE, on_do_something_ref_counted_in_arg_async_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_2_charptr_in_args_async, TEST_ASYNC_HANDLE, test_async_api, const char*, charptr_arg1, const char*, charptr_arg2, ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE, on_do_something_2_charptr_in_args_async_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_ref_counted_array_in_arg_async, TEST_ASYNC_HANDLE, test_async_api, const TEST_REFCOUNTED_HANDLE*, test_ref_counted_array, size_t, item_count, ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE, on_do_something_ref_counted_array_in_arg_async_complete, void*, context)(0, MU_FAILURE);

MOCKABLE_FUNCTION_WITH_RETURNS(, TEST_ASYNC_API_SYNC_RESULT, test_async_do_something_with_enum_return_async, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete, void*, context)(TEST_ASYNC_API_SYNC_RESULT_OK, TEST_ASYNC_API_SYNC_FOR_ERRORS);
MOCKABLE_FUNCTION_WITH_RETURNS(, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT, test_async_do_something_with_multiple_enum_return_async, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete, void*, context)(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK, TEST_ASYNC_API_SYNC_MULTIPLE_FOR_ERRORS);
MOCKABLE_FUNCTION_WITH_RETURNS(, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT, test_async_do_something_with_multiple_enum_return_no_async_retry_async, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_NO_RETRY_ASYNC_COMPLETE, on_do_something_no_retry_async_complete, void*, context)(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK, TEST_ASYNC_API_SYNC_MULTIPLE_FOR_ERRORS);

/*below function declarations are copy&paste from the above (test_async_do_something_async...test_async_do_something_with_multiple_enum_return_no_async_retry_async) and they add a "_with_timeout"*/
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_async_with_timeout, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_multiple_retry_conditions_async_with_timeout, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE, on_do_something_async_multiple_retry_conditions_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_no_in_args_async_with_timeout, TEST_ASYNC_HANDLE, test_async_api, ON_DO_SOMETHING_NO_IN_ARGS_ASYNC_COMPLETE, on_do_something_no_in_args_async_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_no_out_args_async_with_timeout, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_NO_OUT_ARGS_ASYNC_COMPLETE, on_do_something_no_out_args_async_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_no_args_async_with_timeout, TEST_ASYNC_HANDLE, test_async_api, ON_DO_SOMETHING_NO_ARGS_ASYNC_COMPLETE, on_do_something_no_args_async_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_charptr_in_arg_async_with_timeout, TEST_ASYNC_HANDLE, test_async_api, const char*, charptr_arg, ON_DO_SOMETHING_CHARPTR_IN_ARG_ASYNC_COMPLETE, on_do_something_charptr_in_arg_async_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_ref_counted_in_arg_async_with_timeout, TEST_ASYNC_HANDLE, test_async_api, TEST_REFCOUNTED_HANDLE, test_ref_counted, ON_DO_SOMETHING_REF_COUNTED_IN_ARG_ASYNC_COMPLETE, on_do_something_ref_counted_in_arg_async_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_2_charptr_in_args_async_with_timeout, TEST_ASYNC_HANDLE, test_async_api, const char*, charptr_arg1, const char*, charptr_arg2, ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE, on_do_something_2_charptr_in_args_async_complete, void*, context)(0, MU_FAILURE);
MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_ref_counted_array_in_arg_async_with_timeout, TEST_ASYNC_HANDLE, test_async_api, const TEST_REFCOUNTED_HANDLE*, test_ref_counted_array, size_t, item_count, ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE, on_do_something_ref_counted_array_in_arg_async_complete, void*, context)(0, MU_FAILURE);

MOCKABLE_FUNCTION_WITH_RETURNS(, TEST_ASYNC_API_SYNC_RESULT, test_async_do_something_with_enum_return_async_with_timeout, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete, void*, context)(TEST_ASYNC_API_SYNC_RESULT_OK, TEST_ASYNC_API_SYNC_FOR_ERRORS);
MOCKABLE_FUNCTION_WITH_RETURNS(, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT, test_async_do_something_with_multiple_enum_return_async_with_timeout, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete, void*, context)(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK, TEST_ASYNC_API_SYNC_MULTIPLE_FOR_ERRORS);
MOCKABLE_FUNCTION_WITH_RETURNS(, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT, test_async_do_something_with_multiple_enum_return_no_async_retry_async_with_timeout, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_NO_RETRY_ASYNC_COMPLETE, on_do_something_no_retry_async_complete, void*, context)(TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK, TEST_ASYNC_API_SYNC_MULTIPLE_FOR_ERRORS);

MOCKABLE_FUNCTION_WITH_RETURNS(, int, test_async_do_something_async_with_timeout_2, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete, void*, context)(0, MU_FAILURE);

#endif // TEST_ASYNC_H
