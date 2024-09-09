// Copyright (c) Microsoft. All rights reserved.

#ifndef TEST_ASYNC_H
#define TEST_ASYNC_H

#include "macro_utils/macro_utils.h"
#include "test_ref_counted.h"
#include "umock_c/umock_c_prod.h"



#define TEST_ASYNC_API_RESULT_VALUES \
    DUMMY_RESULT_1, \
    DUMMY_RESULT_2

MU_DEFINE_ENUM(TEST_ASYNC_API_RESULT, TEST_ASYNC_API_RESULT_VALUES);

#define TEST_SYNC_API_RESULT_VALUES \
    DUMMY_RESULT_SYNC_1, \
    DUMMY_RESULT_SYNC_2

MU_DEFINE_ENUM(TEST_SYNC_API_RESULT, TEST_SYNC_API_RESULT_VALUES);

typedef struct TEST_MY_POINTER_TAG
{
    int x;
    int y;
} TEST_MY_POINTER;

typedef void (*ON_DO_SOMETHING_ASYNC_COMPLETE)(void* context, TEST_ASYNC_API_RESULT result, int callback_arg1, int callback_arg2);
typedef void (*ON_DO_SOMETHING_NO_IN_ARGS_ASYNC_COMPLETE)(void* context, TEST_ASYNC_API_RESULT result, int callback_arg1, int callback_arg2);
typedef void (*ON_DO_SOMETHING_NO_OUT_ARGS_ASYNC_COMPLETE)(void* context);
typedef void (*ON_DO_SOMETHING_NO_ARGS_ASYNC_COMPLETE)(void* context);
typedef void (*ON_DO_SOMETHING_CHARPTR_OUT_ARG_ASYNC_COMPLETE)(void* context, const char* charptr_arg);
typedef void (*ON_DO_SOMETHING_CHARPTR_OUT_ARG_FAIL_ASYNC_COMPLETE)(void* context, const char* charptr_arg);
typedef void (*ON_DO_SOMETHING_REF_COUNTED_OUT_ARG_ASYNC_COMPLETE)(void* context, TEST_REFCOUNTED_HANDLE test_ref_counted);
typedef void (*ON_DO_SOMETHING_2_OUT_ARGS_1ST_FAILS_ASYNC_COMPLETE)(void* context, const char* charptr_fail_arg, const char* charptr_arg);
typedef void (*ON_DO_SOMETHING_2_OUT_ARGS_2ND_FAILS_ASYNC_COMPLETE)(void* context, const char* charptr_arg, const char* charptr_fail_arg);
typedef void (*ON_DO_SOMETHING_REF_COUNTED_ARRAY_OUT_ARG_ASYNC_COMPLETE)(void* context, TEST_REFCOUNTED_HANDLE* test_ref_counted_array, size_t item_count);
typedef void (*ON_HAVE_POINTER_TYPE_OUT_ARG_ASYNC_COMPLETE)(void* context, TEST_MY_POINTER* test_my_pointer);

typedef struct TEST_ASYNC_TAG* TEST_ASYNC_HANDLE;

MOCKABLE_FUNCTION(, int, test_async_do_something_async, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete, void*, context);
MOCKABLE_FUNCTION(, int, test_async_do_something_no_in_args_async, TEST_ASYNC_HANDLE, test_async_api, ON_DO_SOMETHING_NO_IN_ARGS_ASYNC_COMPLETE, on_do_something_no_in_args_async_complete, void*, context);
MOCKABLE_FUNCTION(, int, test_async_do_something_no_out_args_async, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_NO_OUT_ARGS_ASYNC_COMPLETE, on_do_something_no_out_args_async_complete, void*, context);
MOCKABLE_FUNCTION(, int, test_async_do_something_no_args_async, TEST_ASYNC_HANDLE, test_async_api, ON_DO_SOMETHING_NO_ARGS_ASYNC_COMPLETE, on_do_something_no_args_async_complete, void*, context);
MOCKABLE_FUNCTION(, int, test_async_do_something_charptr_out_arg_async, TEST_ASYNC_HANDLE, test_async_api, ON_DO_SOMETHING_CHARPTR_OUT_ARG_ASYNC_COMPLETE, on_do_something_charptr_out_arg_async_complete, void*, context);
MOCKABLE_FUNCTION(, int, test_async_do_something_charptr_out_arg_fail_async, TEST_ASYNC_HANDLE, test_async_api, ON_DO_SOMETHING_CHARPTR_OUT_ARG_FAIL_ASYNC_COMPLETE, on_do_something_charptr_out_arg_fail_async_complete, void*, context);
MOCKABLE_FUNCTION(, int, test_async_do_something_ref_counted_out_arg_async, TEST_ASYNC_HANDLE, test_async_api, ON_DO_SOMETHING_REF_COUNTED_OUT_ARG_ASYNC_COMPLETE, on_do_something_ref_counted_out_arg_async_complete, void*, context);
MOCKABLE_FUNCTION(, int, test_async_do_something_2_out_args_1st_fails_async, TEST_ASYNC_HANDLE, test_async_api, ON_DO_SOMETHING_2_OUT_ARGS_1ST_FAILS_ASYNC_COMPLETE, on_do_something_2_out_args_2nd_fails_async_complete, void*, context);
MOCKABLE_FUNCTION(, int, test_async_do_something_2_out_args_2nd_fails_async, TEST_ASYNC_HANDLE, test_async_api, ON_DO_SOMETHING_2_OUT_ARGS_2ND_FAILS_ASYNC_COMPLETE, on_do_something_2_out_args_2nd_fails_async_complete, void*, context);
MOCKABLE_FUNCTION(, int, test_async_do_something_ref_counted_array_out_arg_async, TEST_ASYNC_HANDLE, test_async_api, ON_DO_SOMETHING_REF_COUNTED_ARRAY_OUT_ARG_ASYNC_COMPLETE, on_do_something_ref_counted_array_out_arg_async_complete, void*, context);

MOCKABLE_FUNCTION(, TEST_SYNC_API_RESULT, test_async_do_something_with_different_return_type_async, TEST_ASYNC_HANDLE, test_async_api, int, arg1, int, arg2, ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete, void*, context);

MOCKABLE_FUNCTION(, int, test_async_have_pointer_type_out_arg_async, TEST_ASYNC_HANDLE, test_async_api, int, arg1, ON_HAVE_POINTER_TYPE_OUT_ARG_ASYNC_COMPLETE, on_have_pointer_type_out_arg_async_complete, void*, context);



#endif // TEST_ASYNC_H
