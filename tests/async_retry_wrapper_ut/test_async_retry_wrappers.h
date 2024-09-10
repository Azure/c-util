// Copyright (c) Microsoft. All rights reserved.

#ifndef TEST_ASYNC_RETRY_WRAPPERS_H
#define TEST_ASYNC_RETRY_WRAPPERS_H

#include <stddef.h>


#include "c_util/async_retry_wrapper.h"
#include "c_util/async_type_helper.h"
#include "test_async.h"
#include "test_ref_counted.h"
#include "umock_c/umock_c_prod.h"



#define ASYNC_TYPE_HELPER_HAS_CONST_const_TEST_REFCOUNTED_HANDLE_ptr_t 1
#define ASYNC_TYPE_HELPER_NON_CONST_TYPE_const_TEST_REFCOUNTED_HANDLE_ptr_t TEST_REFCOUNTED_HANDLE_ptr_t

typedef char* charptr_fail_t;
typedef const char* const_charptr_fail_t;

typedef TEST_REFCOUNTED_HANDLE* TEST_REFCOUNTED_HANDLE_ptr_t;
typedef const TEST_REFCOUNTED_HANDLE* const_TEST_REFCOUNTED_HANDLE_ptr_t;


/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_001: [ DECLARE_ASYNC_RETRY_WRAPPER shall expand to the function declaration: ]*/
DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_async, int,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_multiple_retry_conditions_async, int,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE, on_do_something_async_multiple_retry_conditions_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_in_args_async, int,
    IN_ARGS(ARG_CB(ON_DO_SOMETHING_NO_IN_ARGS_ASYNC_COMPLETE, on_do_something_no_in_args_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_out_args_async, int,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_NO_OUT_ARGS_ASYNC_COMPLETE, on_do_something_no_out_args_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_args_async, int,
    IN_ARGS(ARG_CB(ON_DO_SOMETHING_NO_ARGS_ASYNC_COMPLETE, on_do_something_no_args_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_charptr_in_arg_async, int,
    IN_ARGS(ARG(const_charptr_t, charptr_arg), ARG_CB(ON_DO_SOMETHING_CHARPTR_IN_ARG_ASYNC_COMPLETE, on_do_something_charptr_in_arg_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_ref_counted_in_arg_async, int,
    IN_ARGS(ARG(TEST_REFCOUNTED_HANDLE, test_ref_counted), ARG_CB(ON_DO_SOMETHING_REF_COUNTED_IN_ARG_ASYNC_COMPLETE, on_do_something_ref_counted_in_arg_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_2_charptr_in_args_async, int,
    IN_ARGS(ARG(const_charptr_t, charptr_arg1), ARG(const_charptr_t, charptr_arg2), ARG_CB(ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE, on_do_something_2_charptr_in_args_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_ref_counted_array_in_arg_async, int,
    IN_ARGS(ARG_EX(const_TEST_REFCOUNTED_HANDLE_ptr_t, test_ref_counted_array, TEST_REFCOUNTED_HANDLE_ptr_t_copy, TEST_REFCOUNTED_HANDLE_ptr_t_free, item_count), ARG(size_t, item_count), ARG_CB(ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE, on_do_something_ref_counted_array_in_arg_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_with_enum_return_async, TEST_ASYNC_API_SYNC_RESULT,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_with_multiple_enum_return_async, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void*, context)));

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_046: [ DECLARE_ASYNC_RETRY_WRAPPER shall also expand to the function declaration: ]*/
DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_with_multiple_enum_return_no_async_retry_async, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_NO_RETRY_ASYNC_COMPLETE, on_do_something_no_retry_async_complete), ARG_CONTEXT(void*, context)));

/*below declarations are copy&paste from (test_async_do_something_async...test_async_do_something_with_multiple_enum_return_no_async_retry_async) but add suffix "_with_timeout" */
DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_async_with_timeout, int,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_multiple_retry_conditions_async_with_timeout, int,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_MULTIPLE_RETRY_CONDITIONS_COMPLETE, on_do_something_async_multiple_retry_conditions_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_in_args_async_with_timeout, int,
    IN_ARGS(ARG_CB(ON_DO_SOMETHING_NO_IN_ARGS_ASYNC_COMPLETE, on_do_something_no_in_args_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_out_args_async_with_timeout, int,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_NO_OUT_ARGS_ASYNC_COMPLETE, on_do_something_no_out_args_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_args_async_with_timeout, int,
    IN_ARGS(ARG_CB(ON_DO_SOMETHING_NO_ARGS_ASYNC_COMPLETE, on_do_something_no_args_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_charptr_in_arg_async_with_timeout, int,
    IN_ARGS(ARG(const_charptr_t, charptr_arg), ARG_CB(ON_DO_SOMETHING_CHARPTR_IN_ARG_ASYNC_COMPLETE, on_do_something_charptr_in_arg_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_ref_counted_in_arg_async_with_timeout, int,
    IN_ARGS(ARG(TEST_REFCOUNTED_HANDLE, test_ref_counted), ARG_CB(ON_DO_SOMETHING_REF_COUNTED_IN_ARG_ASYNC_COMPLETE, on_do_something_ref_counted_in_arg_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_2_charptr_in_args_async_with_timeout, int,
    IN_ARGS(ARG(const_charptr_t, charptr_arg1), ARG(const_charptr_t, charptr_arg2), ARG_CB(ON_DO_SOMETHING_2_CHARPTR_IN_ARGS_ASYNC_COMPLETE, on_do_something_2_charptr_in_args_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_ref_counted_array_in_arg_async_with_timeout, int,
    IN_ARGS(ARG_EX(const_TEST_REFCOUNTED_HANDLE_ptr_t, test_ref_counted_array, TEST_REFCOUNTED_HANDLE_ptr_t_copy, TEST_REFCOUNTED_HANDLE_ptr_t_free, item_count), ARG(size_t, item_count), ARG_CB(ON_DO_SOMETHING_REF_COUNTED_ARRAY_IN_ARG_ASYNC_COMPLETE, on_do_something_ref_counted_array_in_arg_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_with_enum_return_async_with_timeout, TEST_ASYNC_API_SYNC_RESULT,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void*, context)));

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_with_multiple_enum_return_async_with_timeout, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void*, context)));

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_046: [ DECLARE_ASYNC_RETRY_WRAPPER shall also expand to the function declaration: ]*/
DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_with_multiple_enum_return_no_async_retry_async_with_timeout, TEST_ASYNC_API_SYNC_MULTIPLE_RESULT,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_NO_RETRY_ASYNC_COMPLETE, on_do_something_no_retry_async_complete), ARG_CONTEXT(void*, context)));

/*end of copy& paste from(test_async_do_something_async...test_async_do_something_with_multiple_enum_return_no_async_retry_async) but add suffix "_with_timeout" */

DECLARE_ASYNC_RETRY_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_async_with_timeout_2, int,
    IN_ARGS(ARG(int, arg1), ARG(int, arg2), ARG_CB(ON_DO_SOMETHING_ASYNC_COMPLETE, on_do_something_async_complete), ARG_CONTEXT(void*, context)));

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_035: [ copy_function shall be used as a function with the following declaration: ]*/
int TEST_REFCOUNTED_HANDLE_ptr_t_copy(TEST_REFCOUNTED_HANDLE_ptr_t* dst, const_TEST_REFCOUNTED_HANDLE_ptr_t src, size_t item_count);

/*Tests_SRS_ASYNC_RETRY_WRAPPER_42_036: [ free_function shall be used as a function with the following declaration: ]*/
void TEST_REFCOUNTED_HANDLE_ptr_t_free(TEST_REFCOUNTED_HANDLE_ptr_t arg, size_t item_count);

/*Tests_SRS_ASYNC_TYPE_HELPER_42_002: [ DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER shall expand to: ]*/
DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(TEST_REFCOUNTED_HANDLE, dst, src);

/*Tests_SRS_ASYNC_TYPE_HELPER_42_004: [ DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER shall expand to: ]*/
DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(TEST_REFCOUNTED_HANDLE, arg);



#endif // TEST_ASYNC_RETRY_WRAPPERS_H
