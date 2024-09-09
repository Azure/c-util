// Copyright (c) Microsoft. All rights reserved.

#ifndef TEST_SYNC_WRAPPERS_H
#define TEST_SYNC_WRAPPERS_H

#include <stddef.h>


#include "sync_wrapper.h"
#include "test_async.h"
#include "test_ref_counted.h"
#include "umock_c/umock_c_prod.h"



typedef char* charptr_t;
typedef char* charptr_fail_t;
typedef TEST_REFCOUNTED_HANDLE* TEST_REFCOUNTED_HANDLE_ptr_t;
typedef TEST_MY_POINTER* TEST_MY_POINTER_PTR;
typedef const TEST_MY_POINTER* TEST_MY_POINTER_CONST_PTR;

#define ASYNC_TYPE_HELPER_NO_POINTER_DECLARATION_TEST_MY_POINTER_PTR 1

DECLARE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_async, int, IN_ARGS(int, arg1, int, arg2), OUT_ARG(TEST_ASYNC_API_RESULT, result), OUT_ARG(int, callback_arg1), OUT_ARG(int, callback_arg2))
DECLARE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_in_args_async, int, IN_ARGS(), OUT_ARG(TEST_ASYNC_API_RESULT, result), OUT_ARG(int, callback_arg1), OUT_ARG(int, callback_arg2))
DECLARE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_out_args_async, int, IN_ARGS(int, arg1, int, arg2))
DECLARE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_args_async, int, IN_ARGS())
DECLARE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_charptr_out_arg_async, int, IN_ARGS(), OUT_ARG(charptr_t, charptr_arg))
DECLARE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_charptr_out_arg_fail_async, int, IN_ARGS(), OUT_ARG(charptr_fail_t, charptr_arg))
DECLARE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_ref_counted_out_arg_async, int, IN_ARGS(), OUT_ARG(TEST_REFCOUNTED_HANDLE, test_ref_counted))
DECLARE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_2_out_args_2nd_fails_async, int, IN_ARGS(), OUT_ARG(charptr_t, charptr_arg), OUT_ARG(charptr_fail_t, charptr_fail_arg))
DECLARE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_ref_counted_array_out_arg_async, int, IN_ARGS(),
    OUT_ARG(TEST_REFCOUNTED_HANDLE_ptr_t, test_ref_counted_array),
    OUT_ARG(size_t, item_count))
DECLARE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_with_different_return_type_async, TEST_SYNC_API_RESULT, IN_ARGS(int, arg1, int, arg2), OUT_ARG(TEST_ASYNC_API_RESULT, result), OUT_ARG(int, callback_arg1), OUT_ARG(int, callback_arg2))

DECLARE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_have_pointer_type_out_arg_async, int, IN_ARGS(int, arg1), OUT_ARG(TEST_MY_POINTER_PTR, test_my_pointer))

DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(charptr_t, dst, src)
DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(charptr_t, arg)

DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(TEST_MY_POINTER_PTR, dst, src)
DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(TEST_MY_POINTER_PTR, arg)

int TEST_REFCOUNTED_HANDLE_ptr_t_copy(TEST_REFCOUNTED_HANDLE_ptr_t* dst, const TEST_REFCOUNTED_HANDLE_ptr_t src, size_t item_count);
void TEST_REFCOUNTED_HANDLE_ptr_t_free(const TEST_REFCOUNTED_HANDLE_ptr_t arg, size_t item_count);



#endif // TEST_SYNC_WRAPPERS_H
