// Copyright (c) Microsoft. All rights reserved.

#include <stddef.h>
#include <stdlib.h>

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/async_type_helper.h"
#include "c_util/async_retry_wrapper.h"
#include "test_async.h"
#include "test_ref_counted.h"
#include "test_async_retry_wrappers.h"



/*Tests_SRS_ASYNC_TYPE_HELPER_42_003: [ DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER shall expand to: ]*/
DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER(TEST_REFCOUNTED_HANDLE, dst, src)
{
    test_refcounted_inc_ref(src);
    *dst = src;
    return 0;
}

/*Tests_SRS_ASYNC_TYPE_HELPER_42_005: [ DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER shall expand to: ]*/
DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER(TEST_REFCOUNTED_HANDLE, arg)
{
    test_refcounted_dec_ref(arg);
}

int TEST_REFCOUNTED_HANDLE_ptr_t_copy(TEST_REFCOUNTED_HANDLE_ptr_t* dst, const_TEST_REFCOUNTED_HANDLE_ptr_t src, size_t item_count)
{
    int result;
    *dst = malloc(sizeof(TEST_REFCOUNTED_HANDLE) * item_count);
    if (*dst == NULL)
    {
        result = MU_FAILURE;
    }
    else
    {
        (void)memcpy(*dst, src, sizeof(TEST_REFCOUNTED_HANDLE) * item_count);
        for (size_t i = 0; i < item_count; i++)
        {
            test_refcounted_inc_ref((*dst)[i]);
        }

        result = 0;
    }

    return result;
}

void TEST_REFCOUNTED_HANDLE_ptr_t_free(TEST_REFCOUNTED_HANDLE_ptr_t arg, size_t item_count)
{
    if (arg != NULL)
    {
        for (size_t i = 0; i < item_count; i++)
        {
            test_refcounted_dec_ref(arg[i]);
        }
        free(arg);
    }
}

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_002: [ DEFINE_ASYNC_RETRY_WRAPPER shall generate an asynchronous retry wrapper with the declaration: ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_014: [ DEFINE_ASYNC_RETRY_WRAPPER shall generate a callback to be passed to the asynchronous function with the following declaration: ]*/
/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_022: [ DEFINE_ASYNC_RETRY_WRAPPER shall generate a function to retry the asynchronous function with the following declaration: ]*/

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_async, int, 0,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS), ARG(int, callback_arg1, 0), ARG(int, callback_arg2, -1)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_multiple_retry_conditions_async, int, 0,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE, on_do_something_async_multiple_retry_conditions_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_MULTIPLE_RETRY_RESULT, result, TEST_ASYNC_MULTIPLE_RETRY_ERROR), ARG(int, callback_arg1, 0), ARG(int, callback_arg2, 0)),
    RETRY_ON_ASYNC(TEST_ASYNC_MULTIPLE_RETRY_RETRY_1, TEST_ASYNC_MULTIPLE_RETRY_RETRY_2, TEST_ASYNC_MULTIPLE_RETRY_RETRY_3), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_in_args_async, int, 0,
    IN_ARGS(ARG_CB(ON_DO_SOMETHING_NO_IN_ARGS_ASYNC_COMPLETE, on_do_something_no_in_args_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS), ARG(int, callback_arg1, 0), ARG(int, callback_arg2, 0)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_out_args_async, int, 0,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_NO_OUT_ARGS_ASYNC_COMPLETE, on_do_something_no_out_args_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_args_async, int, 0,
    IN_ARGS(ARG_CB(ON_DO_SOMETHING_NO_ARGS_ASYNC_COMPLETE, on_do_something_no_args_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_charptr_in_arg_async, int, 0,
    IN_ARGS(ARG(const_charptr_t, charptr_arg), ARG_CB(ON_DO_SOMETHING_CHARPTR_IN_ARG_ASYNC_COMPLETE, on_do_something_charptr_in_arg_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ARG(const_charptr_t, charptr_arg, NULL), ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_ref_counted_in_arg_async, int, 0,
    IN_ARGS(ARG(TEST_REFCOUNTED_HANDLE, test_ref_counted), ARG_CB(ON_DO_SOMETHING_REF_COUNTED_IN_ARG_ASYNC_COMPLETE, on_do_something_ref_counted_in_arg_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_2_charptr_in_args_async, int, 0,
    IN_ARGS(ARG(const_charptr_t, charptr_arg1), ARG(const_charptr_t, charptr_arg2), ARG_CB(ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE, on_do_something_2_charptr_in_args_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ARG(uint32_t, value, UINT32_MAX), ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_ref_counted_array_in_arg_async, int, 0,
    IN_ARGS(ARG_EX(const_TEST_REFCOUNTED_HANDLE_ptr_t, test_ref_counted_array, TEST_REFCOUNTED_HANDLE_ptr_t_copy, TEST_REFCOUNTED_HANDLE_ptr_t_free, item_count), ARG(size_t, item_count), ARG_CB(ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE, on_do_something_ref_counted_array_in_arg_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS), ARG(uint8_t, value, 0)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_with_enum_return_async, TEST_ASYNC_API_SYNC_RESULT, TEST_ASYNC_API_SYNC_RESULT_OK,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS), ARG(int, callback_arg1, 0), ARG(int, callback_arg2, -1)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC(TEST_ASYNC_API_SYNC_FOR_RETRY));

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_with_multiple_enum_return_async, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS), ARG(int, callback_arg1, 0), ARG(int, callback_arg2, -1)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_1, TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_2, TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_3));

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_047: [ DEFINE_ASYNC_RETRY_WRAPPER shall generate an asynchronous retry wrapper that supports timeouts with the declaration: ]*/
DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_with_multiple_enum_return_no_async_retry_async, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_NO_RETRY_ASYNC_COMPLETE, on_do_something_no_retry_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ARG(int, callback_arg1, 0), ARG(int, callback_arg2, -1)),
    RETRY_ON_ASYNC(), RETRY_ON_SYNC(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_1, TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_2, TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_3));

/*below wrapper declarations are copy&paste from above (test_async_do_something_async... test_async_do_something_with_multiple_enum_return_no_async_retry_async) but appends to the name "_with_timeout"*/

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_async_with_timeout, int, 0,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS, TEST_ASYNC_RESULT_FOR_TIMEOUT), ARG(int, callback_arg1, 0), ARG(int, callback_arg2, -1)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_multiple_retry_conditions_async_with_timeout, int, 0,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE, on_do_something_async_multiple_retry_conditions_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_MULTIPLE_RETRY_RESULT, result, TEST_ASYNC_MULTIPLE_RETRY_ERROR, TEST_ASYNC_MULTIPLE_RETRY_TIMEOUT), ARG(int, callback_arg1, 0), ARG(int, callback_arg2, 0)),
    RETRY_ON_ASYNC(TEST_ASYNC_MULTIPLE_RETRY_RETRY_1, TEST_ASYNC_MULTIPLE_RETRY_RETRY_2, TEST_ASYNC_MULTIPLE_RETRY_RETRY_3), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_in_args_async_with_timeout, int, 0,
    IN_ARGS(ARG_CB(ON_DO_SOMETHING_NO_IN_ARGS_ASYNC_COMPLETE, on_do_something_no_in_args_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS, TEST_ASYNC_RESULT_FOR_TIMEOUT), ARG(int, callback_arg1, 0), ARG(int, callback_arg2, 0)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_out_args_async_with_timeout, int, 0,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_NO_OUT_ARGS_ASYNC_COMPLETE, on_do_something_no_out_args_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS, TEST_ASYNC_RESULT_FOR_TIMEOUT)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_args_async_with_timeout, int, 0,
    IN_ARGS(ARG_CB(ON_DO_SOMETHING_NO_ARGS_ASYNC_COMPLETE, on_do_something_no_args_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS, TEST_ASYNC_RESULT_FOR_TIMEOUT)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_charptr_in_arg_async_with_timeout, int, 0,
    IN_ARGS(ARG(const_charptr_t, charptr_arg), ARG_CB(ON_DO_SOMETHING_CHARPTR_IN_ARG_ASYNC_COMPLETE, on_do_something_charptr_in_arg_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ARG(const_charptr_t, charptr_arg, NULL), ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS, TEST_ASYNC_RESULT_FOR_TIMEOUT)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_ref_counted_in_arg_async_with_timeout, int, 0,
    IN_ARGS(ARG(TEST_REFCOUNTED_HANDLE, test_ref_counted), ARG_CB(ON_DO_SOMETHING_REF_COUNTED_IN_ARG_ASYNC_COMPLETE, on_do_something_ref_counted_in_arg_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS, TEST_ASYNC_RESULT_FOR_TIMEOUT)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_2_charptr_in_args_async_with_timeout, int, 0,
    IN_ARGS(ARG(const_charptr_t, charptr_arg1), ARG(const_charptr_t, charptr_arg2), ARG_CB(ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE, on_do_something_2_charptr_in_args_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ARG(uint32_t, value, UINT32_MAX), ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS, TEST_ASYNC_RESULT_FOR_TIMEOUT)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_ref_counted_array_in_arg_async_with_timeout, int, 0,
    IN_ARGS(ARG_EX(const_TEST_REFCOUNTED_HANDLE_ptr_t, test_ref_counted_array, TEST_REFCOUNTED_HANDLE_ptr_t_copy, TEST_REFCOUNTED_HANDLE_ptr_t_free, item_count), ARG(size_t, item_count), ARG_CB(ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE, on_do_something_ref_counted_array_in_arg_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS, TEST_ASYNC_RESULT_FOR_TIMEOUT), ARG(uint8_t, value, 0)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_with_enum_return_async_with_timeout, TEST_ASYNC_API_SYNC_RESULT, TEST_ASYNC_API_SYNC_RESULT_OK,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS, TEST_ASYNC_RESULT_FOR_TIMEOUT), ARG(int, callback_arg1, 0), ARG(int, callback_arg2, -1)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC(TEST_ASYNC_API_SYNC_FOR_RETRY));

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_with_multiple_enum_return_async_with_timeout, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS, TEST_ASYNC_RESULT_FOR_TIMEOUT), ARG(int, callback_arg1, 0), ARG(int, callback_arg2, -1)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_1, TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_2, TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_3));

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_047: [ DEFINE_ASYNC_RETRY_WRAPPER shall generate an asynchronous retry wrapper that supports timeouts with the declaration: ]*/
DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_with_multiple_enum_return_no_async_retry_async_with_timeout, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT_OK,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_NO_RETRY_ASYNC_COMPLETE, on_do_something_no_retry_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ARG(int, callback_arg1, 0), ARG(int, callback_arg2, -1)),
    RETRY_ON_ASYNC(), RETRY_ON_SYNC(TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_1, TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_2, TEST_ASYNC_API_SYNC_MULTIPLE_FOR_RETRY_3));

DEFINE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_async_with_timeout_2, int, 0, /*note: test_async_do_something_async_with_timeout_2 is the same as test_async_do_something_async_with_timeout, but doesn't have TEST_ASYNC_RESULT_FOR_TIMEOUT*/
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void_ptr, context)),
    OUT_ARGS(ENUM(TEST_ASYNC_API_DEFAULT_RESULT, result, TEST_ASYNC_RESULT_FOR_ERRORS), ARG(int, callback_arg1, 0), ARG(int, callback_arg2, -1)),
    RETRY_ON_ASYNC(TEST_ASYNC_RESULT_FOR_RETRY), RETRY_ON_SYNC());
