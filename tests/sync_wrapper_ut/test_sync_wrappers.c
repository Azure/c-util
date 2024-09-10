// Copyright (c) Microsoft. All rights reserved.

#include <stddef.h>
#include <stdlib.h>

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/async_type_helper.h"
#include "c_util/sync_wrapper.h"
#include "test_async.h"
#include "test_ref_counted.h"

#include "test_sync_wrappers.h"

#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_TEST_ASYNC_API_RESULT 1

#define ASYNC_TYPE_HELPER_USE_CONST_TYPE_charptr_fail_t 1
#define ASYNC_TYPE_HELPER_CONST_TYPE_charptr_fail_t const_charptr_t

DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER(charptr_t, dst, src)
{
    int result;

    size_t str_length = strlen(src) + 1;
    *dst = malloc(str_length);
    if (*dst == NULL)
    {
        result = MU_FAILURE;
    }
    else
    {
        (void)memcpy(*dst, src, str_length);
        result = 0;
    }

    return result;
}

DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER(charptr_t, arg)
{
    free(arg);
}

DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER(charptr_fail_t, dst, src)
{
    (void)dst;
    (void)src;
    return MU_FAILURE;
}

DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER(charptr_fail_t, arg)
{
    (void)arg;
}

DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER(TEST_REFCOUNTED_HANDLE, dst, src)
{
    test_refcounted_inc_ref(src);
    *dst = src;
    return 0;
}

DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER(TEST_REFCOUNTED_HANDLE, arg)
{
    test_refcounted_dec_ref(arg);
}

int TEST_REFCOUNTED_HANDLE_ptr_t_copy(TEST_REFCOUNTED_HANDLE_ptr_t* dst, const TEST_REFCOUNTED_HANDLE_ptr_t src, size_t item_count)
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

void TEST_REFCOUNTED_HANDLE_ptr_t_free(const TEST_REFCOUNTED_HANDLE_ptr_t arg, size_t item_count)
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

DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER(TEST_MY_POINTER_PTR, dst, src)
{
    if (dst == NULL || src == NULL)
    {
        return MU_FAILURE;
    }
    else
    {
        dst->x = src->x;
        dst->y = src->y;
        return 0;
    }
}
DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER(TEST_MY_POINTER_PTR, arg)
{
    (void)arg;
}

DEFINE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_async, int, 0, IN_ARGS(int, arg1, int, arg2), OUT_ARG(TEST_ASYNC_API_RESULT, result), OUT_ARG(int, callback_arg1), OUT_ARG(int, callback_arg2))
DEFINE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_in_args_async, int, 0, IN_ARGS(), OUT_ARG(TEST_ASYNC_API_RESULT, result), OUT_ARG(int, callback_arg1), OUT_ARG(int, callback_arg2))
DEFINE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_out_args_async, int, 0, IN_ARGS(int, arg1, int, arg2))
DEFINE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_no_args_async, int, 0, IN_ARGS())
DEFINE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_charptr_out_arg_async, int, 0, IN_ARGS(), OUT_ARG(charptr_t, charptr_arg))
DEFINE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_charptr_out_arg_fail_async, int, 0, IN_ARGS(), OUT_ARG(charptr_fail_t, charptr_arg))
DEFINE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_ref_counted_out_arg_async, int, 0, IN_ARGS(), OUT_ARG(TEST_REFCOUNTED_HANDLE, test_ref_counted))
DEFINE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_2_out_args_2nd_fails_async, int, 0, IN_ARGS(), OUT_ARG(charptr_t, charptr_arg), OUT_ARG(charptr_fail_t, charptr_fail_arg))
DEFINE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_ref_counted_array_out_arg_async, int, 0, IN_ARGS(),
    OUT_ARG_EX(TEST_REFCOUNTED_HANDLE_ptr_t, test_ref_counted_array, TEST_REFCOUNTED_HANDLE_ptr_t_copy, TEST_REFCOUNTED_HANDLE_ptr_t_free, item_count),
    OUT_ARG(size_t, item_count))
DEFINE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_do_something_with_different_return_type_async, TEST_SYNC_API_RESULT, DUMMY_RESULT_SYNC_1, IN_ARGS(int, arg1, int, arg2), OUT_ARG(TEST_ASYNC_API_RESULT, result), OUT_ARG(int, callback_arg1), OUT_ARG(int, callback_arg2))

DEFINE_SYNC_WRAPPER(TEST_ASYNC_HANDLE, test_async_have_pointer_type_out_arg_async, int, 0, IN_ARGS(int, arg1), OUT_ARG(TEST_MY_POINTER_PTR, test_my_pointer))
