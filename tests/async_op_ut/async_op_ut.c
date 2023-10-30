// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdlib.h>
#include <stddef.h>


#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"

#define ENABLE_MOCKS

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
MOCKABLE_FUNCTION(, void, user_free, void*, ptr);
MOCKABLE_FUNCTION(, void, user_cancel, void*, ptr);
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/thandle.h"
#include "c_util/async_op.h"

TEST_DEFINE_ENUM_TYPE(ASYNC_OP_STATE, ASYNC_OP_STATE_VALUES);

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(setsBufferTempSize)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    umock_c_init(on_umock_c_error);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(realloc, NULL);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(function_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleans)
{
}

/*Tests_SRS_ASYNC_OP_02_001: [ If context_align is not a power of 2 then async_op_create shall fail and return NULL. ]*/
TEST_FUNCTION(async_op_create_with_context_align_0_fails) /*note:0 is not a power of 2*/
{
    ///arrange

    ///act
    THANDLE(ASYNC_OP) async_op = async_op_create(NULL, 1, 0, NULL);

    ///assert
    ASSERT_IS_NULL(async_op);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

/*Tests_SRS_ASYNC_OP_02_001: [ If context_align is not a power of 2 then async_op_create shall fail and return NULL. ]*/
TEST_FUNCTION(async_op_create_with_context_align_3_fails) /*note:3 is not a power of 2*/
{
    ///arrange

    ///act
    THANDLE(ASYNC_OP) async_op = async_op_create(NULL, 1, 3, NULL);

    ///assert
    ASSERT_IS_NULL(async_op);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_OP_02_004: [ If there are any failures then async_op_create shall fail and return NULL. ]*/
TEST_FUNCTION(async_op_create_overflow) /*note:would cause oveflow. note:next test doesn't cause overflow*/
{
    ///arrange

    ///act
    THANDLE(ASYNC_OP) async_op = async_op_create(NULL, 1U<<31, 1U<<31, NULL);

    ///assert
    ASSERT_IS_NULL(async_op);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_OP_02_004: [ If there are any failures then async_op_create shall fail and return NULL. ]*/
/*Tests_SRS_ASYNC_OP_02_003: [ async_op_create shall compute context (that the user is supposed to use), record cancel, dispose, set state to ASYNC_RUNNING and return a non-NULL value. ]*/
TEST_FUNCTION(async_op_create_no_overflow_max_alignment) /*note:asks for a 2GB aligned 2GB-1 memory size*/
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, (1U << 31) - 1 + (1U << 31) -1 , IGNORED_ARG));

    ///act
    THANDLE(ASYNC_OP) async_op = async_op_create(NULL, (1U << 31) - 1, 1U << 31, NULL);

    ///assert
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_IS_TRUE((((uintptr_t)async_op->context) % (1U << 31)) == 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    (void)memset(async_op->context, '3', (1U << 31) - 1); /*asserts that the memory is USABLE by setting all bytes to '3'. for valgrind/fsanitize*/

    ///clean
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

/*Tests_SRS_ASYNC_OP_02_002: [ async_op_create shall call THANDLE_MALLOC_FLEX with the extra size set as (context_size > 0) * (context_size + context_align - 1).]*/
/*Tests_SRS_ASYNC_OP_02_003: [ async_op_create shall compute context (that the user is supposed to use), record cancel, dispose, set state to ASYNC_RUNNING and return a non-NULL value. ]*/
TEST_FUNCTION(async_op_create_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 100+256-1, IGNORED_ARG));

    ///act
    THANDLE(ASYNC_OP) async_op = async_op_create(NULL, 100, 256, NULL);

    ///assert
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_IS_TRUE(((uintptr_t)async_op->context) % 256 == 0);
    (void)memset(async_op->context, '3', 100); /*asserts that the memory is USABLE by setting all bytes to '3'. for valgrind/fsanitize*/
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

/*Tests_SRS_ASYNC_OP_02_002: [ async_op_create shall call THANDLE_MALLOC_FLEX with the extra size set as (context_size > 0) * (context_size + context_align - 1).]*/
TEST_FUNCTION(async_op_create_with_context_size_0_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, IGNORED_ARG));

    ///act
    THANDLE(ASYNC_OP) async_op = async_op_create(NULL, 0, 256, NULL);

    ///assert
    ASSERT_IS_NOT_NULL(async_op);
    ASSERT_IS_TRUE(((uintptr_t)async_op->context) % 256 == 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

/*Tests_SRS_ASYNC_OP_02_003: [ async_op_create shall compute context (that the user is supposed to use), record cancel, dispose, set state to ASYNC_RUNNING and return a non-NULL value. ]*/
TEST_FUNCTION(async_op_create_calls_dispose)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 100 + 256 - 1, IGNORED_ARG));
    THANDLE(ASYNC_OP) async_op = async_op_create(NULL, 100, 256, user_free);
    ASSERT_IS_NOT_NULL(async_op);

    STRICT_EXPECTED_CALL(user_free(async_op->context));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_ASYNC_OP_02_005: [ If async_op is NULL then async_op_cancel shall return ASYNC_INVALID_ARG. ]*/
TEST_FUNCTION(async_op_cancel_with_async_op_NULL_returns_ASYNC_INVALID_ARG)
{
    ///arrange
    ASYNC_OP_STATE result;

    ///act
    result = async_op_cancel(NULL);

    ///assert
    ASSERT_ARE_EQUAL(ASYNC_OP_STATE, ASYNC_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_ASYNC_OP_02_006: [ async_op_cancel shall atomically switch the state to ASYNC_CANCELLING if the current state is ASYNC_RUNNING by using interlocked_compare_exchange. ]*/
/*Tests_SRS_ASYNC_OP_02_007: [ If async_op's cancel is non-NULL then async_op_cancel shall call it with async_op->context as parameter. ]*/
/*Tests_SRS_ASYNC_OP_02_008: [ async_op_cancel shall return the state of the operation. ]*/
TEST_FUNCTION(async_op_cancel_calls_user_cancel)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 100 + 256 - 1, IGNORED_ARG));
    THANDLE(ASYNC_OP) async_op = async_op_create(user_cancel, 100, 256, user_free);
    ASSERT_IS_NOT_NULL(async_op);

    STRICT_EXPECTED_CALL(user_cancel(async_op->context));

    ASYNC_OP_STATE result;

    ///act
    result = async_op_cancel(async_op);

    ///assert
    ASSERT_ARE_EQUAL(ASYNC_OP_STATE, ASYNC_CANCELLING, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

/*Tests_SRS_ASYNC_OP_02_006: [ async_op_cancel shall atomically switch the state to ASYNC_CANCELLING if the current state is ASYNC_RUNNING by using interlocked_compare_exchange. ]*/
/*Tests_SRS_ASYNC_OP_02_008: [ async_op_cancel shall return the state of the operation. ]*/
TEST_FUNCTION(async_op_cancel_calls_user_cancel_just_once)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 100 + 256 - 1, IGNORED_ARG));
    THANDLE(ASYNC_OP) async_op = async_op_create(user_cancel, 100, 256, user_free);
    ASSERT_IS_NOT_NULL(async_op);
    ASYNC_OP_STATE result;

    /*first cancel*/
    STRICT_EXPECTED_CALL(user_cancel(async_op->context));
    result = async_op_cancel(async_op);
    ASSERT_ARE_EQUAL(ASYNC_OP_STATE, ASYNC_CANCELLING, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///act - second cancel, does not call into user land a second time
    result = async_op_cancel(async_op);
    ASSERT_ARE_EQUAL(ASYNC_OP_STATE, ASYNC_CANCELLING, result);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

/*Tests_SRS_ASYNC_OP_02_007: [ If async_op's cancel is non-NULL then async_op_cancel shall call it with async_op->context as parameter. ]*/
/*Tests_SRS_ASYNC_OP_02_008: [ async_op_cancel shall return the state of the operation. ]*/
TEST_FUNCTION(async_op_cancel_with_cancel_NULL)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 100 + 256 - 1, IGNORED_ARG));
    THANDLE(ASYNC_OP) async_op = async_op_create(NULL, 100, 256, user_free);
    ASSERT_IS_NOT_NULL(async_op);

    ASYNC_OP_STATE result;

    ///act
    result = async_op_cancel(async_op);

    ///assert
    ASSERT_ARE_EQUAL(ASYNC_OP_STATE, ASYNC_CANCELLING, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
