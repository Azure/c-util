// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "cancellation_token_ut_pch.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#undef ENABLE_MOCKS_DECL
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, void, on_cancel, void*, context);

MOCKABLE_FUNCTION(, void, on_cancel2, void*, context);
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

    REGISTER_INTERLOCKED_GLOBAL_MOCK_HOOK();
    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_TCALL_DISPATCHER_CANCELLATION_TOKEN_CANCEL_CALL_GLOBAL_MOCK_HOOK();

    REGISTER_UMOCK_ALIAS_TYPE(TCALL_DISPATCHER(CANCELLATION_TOKEN_CANCEL_CALL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(TCALL_DISPATCHER_TARGET_HANDLE(CANCELLATION_TOKEN_CANCEL_CALL), void*);
    REGISTER_UMOCK_ALIAS_TYPE(TCALL_DISPATCHER_TARGET_FUNC_TYPE_NAME(CANCELLATION_TOKEN_CANCEL_CALL), void*);

    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_negative_tests_deinit();
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

/*Tests_SRS_CANCELLATION_TOKEN_04_001: [ cancellation_token_create shall allocate memory for a THANDLE(CANCELLATION_TOKEN). ]*/
/*Tests_SRS_CANCELLATION_TOKEN_04_003: [ cancellation_token_create shall create a TCALL_DISPATCHER handle by calling TCALL_DISPATCHER_CREATE(CANCELLATION_TOKEN_CANCEL_CALL). ]*/
/*Tests_SRS_CANCELLATION_TOKEN_04_004: [ cancellation_token_create shall set the initial state to be equal to canceled parameter. ]*/
/*Tests_SRS_CANCELLATION_TOKEN_04_005: [ cancellation_token_create shall return a valid THANDLE(CANCELLATION_TOKEN) when successful. ]*/
TEST_FUNCTION(cancellation_token_create_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(TCALL_DISPATCHER_CREATE(CANCELLATION_TOKEN_CANCEL_CALL)());
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TCALL_DISPATCHER_INITIALIZE_MOVE(CANCELLATION_TOKEN_CANCEL_CALL)(IGNORED_ARG, IGNORED_ARG));

    // act
    THANDLE(CANCELLATION_TOKEN) token = cancellation_token_create(false);

    // assert
    ASSERT_IS_NOT_NULL(token);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean
    THANDLE_ASSIGN(CANCELLATION_TOKEN)(&token, NULL);
}

/*Tests_SRS_CANCELLATION_TOKEN_04_002: [ If any underlying error occurs cancellation_token_create shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_cancellation_token_create_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(TCALL_DISPATCHER_CREATE(CANCELLATION_TOKEN_CANCEL_CALL)());
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(TCALL_DISPATCHER_INITIALIZE_MOVE(CANCELLATION_TOKEN_CANCEL_CALL)(IGNORED_ARG, IGNORED_ARG));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE(CANCELLATION_TOKEN) token = cancellation_token_create(false);

            // assert
            ASSERT_IS_NULL(token);
        }
    }
}

/*Tests_SRS_CANCELLATION_TOKEN_04_025: [ cancellation_token_dispose shall free the TCALL_DISPATCHER by assigning NULL to the dispatcher handle by calling TCALL_DISPATCHER_ASSIGN(CANCELLATION_TOKEN_CANCEL_CALL). ]*/
TEST_FUNCTION(cancellation_token_dispose_frees_resources)
{
    // arrange
    THANDLE(CANCELLATION_TOKEN) token = cancellation_token_create(false);
    ASSERT_IS_NOT_NULL(token);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(TCALL_DISPATCHER_ASSIGN(CANCELLATION_TOKEN_CANCEL_CALL)(IGNORED_ARG, NULL));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(CANCELLATION_TOKEN)(&token, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CANCELLATION_TOKEN_04_004: [ cancellation_token_create shall set the initial state to be equal to canceled parameter. ]*/
/*Tests_SRS_CANCELLATION_TOKEN_04_008: [ cancellation_token_is_canceled shall return true if the token has been canceled and false otherwise. ]*/
TEST_FUNCTION(cancellation_token_create_not_cancelled)
{
    // arrange
    THANDLE(CANCELLATION_TOKEN) token = cancellation_token_create(false);
    ASSERT_IS_NOT_NULL(token);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));

    // act
    bool canceled = cancellation_token_is_canceled(token);

    // assert
    ASSERT_IS_FALSE(canceled);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean
    THANDLE_ASSIGN(CANCELLATION_TOKEN)(&token, NULL);
}

/*Tests_SRS_CANCELLATION_TOKEN_04_004: [ cancellation_token_create shall set the initial state to be equal to canceled parameter. ]*/
/*Tests_SRS_CANCELLATION_TOKEN_04_008: [ cancellation_token_is_canceled shall return true if the token has been canceled and false otherwise. ]*/
TEST_FUNCTION(cancellation_token_create_cancelled)
{
    // arrange
    THANDLE(CANCELLATION_TOKEN) token = cancellation_token_create(true);
    ASSERT_IS_NOT_NULL(token);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));

    // act
    bool canceled = cancellation_token_is_canceled(token);

    // assert
    ASSERT_IS_TRUE(canceled);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean
    THANDLE_ASSIGN(CANCELLATION_TOKEN)(&token, NULL);
}

/*Tests_SRS_CANCELLATION_TOKEN_04_006: [ cancellation_token_is_canceled shall return false if cancellation_token is NULL. ]*/
TEST_FUNCTION(cancellation_token_is_canceled_returns_false_with_null_token)
{
    // arrange
    // act
    bool canceled = cancellation_token_is_canceled(NULL);

    // assert
    ASSERT_IS_FALSE(canceled);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean
}

/*Tests_SRS_CANCELLATION_TOKEN_04_013: [ cancellation_token_register_notify shall call TCALL_DISPATCHER_REGISTER_TARGET(CANCELLATION_TOKEN_CANCEL_CALL) to register on_cancel with the dispatcher. ]*/
/*Tests_SRS_CANCELLATION_TOKEN_04_014: [ cancellation_token_register_notify shall call THANDLE_MALLOC(CANCELLATION_TOKEN_REGISTRATION) to allocate a CANCELLATION_TOKEN_REGISTRATION handle. ]*/
/*Tests_SRS_CANCELLATION_TOKEN_04_015: [ cancellation_token_register_notify shall initialize and return a valid THANDLE(CANCELLATION_TOKEN_REGISTRATION) when successful. ]*/
TEST_FUNCTION(cancellation_token_register_notify_succeeds)
{
    // arrange
    THANDLE(CANCELLATION_TOKEN) token = cancellation_token_create(false);
    ASSERT_IS_NOT_NULL(token);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TCALL_DISPATCHER_REGISTER_TARGET(CANCELLATION_TOKEN_CANCEL_CALL)(IGNORED_ARG, on_cancel, (void*)0x42));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG));

    // act
    THANDLE(CANCELLATION_TOKEN_REGISTRATION) registration = cancellation_token_register_notify(token, on_cancel, (void*)0x42);

    // assert
    ASSERT_IS_NOT_NULL(registration);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean
    THANDLE_ASSIGN(CANCELLATION_TOKEN_REGISTRATION)(&registration, NULL);
    THANDLE_ASSIGN(CANCELLATION_TOKEN)(&token, NULL);
}

/*Tests_SRS_CANCELLATION_TOKEN_04_023: [ cancellation_token_registration_dispose shall un-register the callback from TCALL_DISPATCHER by calling TCALL_DISPATCHER_UNREGISTER_TARGET(CANCELLATION_TOKEN_CANCEL_CALL). ]*/
/*Tests_SRS_CANCELLATION_TOKEN_04_024: [ cancellation_token_registration_dispose shall free resources. ]*/
TEST_FUNCTION(cancellation_token_registration_dispose_frees_resources)
{
    // arrange
    THANDLE(CANCELLATION_TOKEN) token = cancellation_token_create(false);
    ASSERT_IS_NOT_NULL(token);
    THANDLE(CANCELLATION_TOKEN_REGISTRATION) registration = cancellation_token_register_notify(token, on_cancel, (void*)0x42);
    ASSERT_IS_NOT_NULL(registration);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(TCALL_DISPATCHER_UNREGISTER_TARGET(CANCELLATION_TOKEN_CANCEL_CALL)(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(CANCELLATION_TOKEN_REGISTRATION)(&registration, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean
    THANDLE_ASSIGN(CANCELLATION_TOKEN)(&token, NULL);
}

/*Tests_SRS_CANCELLATION_TOKEN_04_022: [ If the cancellation token is already in canceled state, then cancellation_token_register_notify shall immediately call on_cancel and take no further action and return NULL. ]*/
TEST_FUNCTION(cancellation_token_register_notify_calls_callback_on_canceled_token)
{
    // arrange
    THANDLE(CANCELLATION_TOKEN) token = cancellation_token_create(true);
    ASSERT_IS_NOT_NULL(token);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(on_cancel((void*)0x42));

     // act
    THANDLE(CANCELLATION_TOKEN_REGISTRATION) registration = cancellation_token_register_notify(token, on_cancel, (void*)0x42);

    // assert
    ASSERT_IS_NULL(registration);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean
    THANDLE_ASSIGN(CANCELLATION_TOKEN)(&token, NULL);
}

/*Tests_SRS_CANCELLATION_TOKEN_04_012: [ cancellation_token_register_notify shall fail and return NULL when any underlying call fails. ]*/
TEST_FUNCTION(when_underlying_calls_fail_cancellation_token_register_notify_fails)
{
    // arrange
    THANDLE(CANCELLATION_TOKEN) token = cancellation_token_create(false);
    ASSERT_IS_NOT_NULL(token);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_add(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(TCALL_DISPATCHER_REGISTER_TARGET(CANCELLATION_TOKEN_CANCEL_CALL)(IGNORED_ARG, on_cancel, (void*)0x42));
    STRICT_EXPECTED_CALL(interlocked_increment(IGNORED_ARG))
        .CallCannotFail();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE(CANCELLATION_TOKEN_REGISTRATION) registration = cancellation_token_register_notify(token, on_cancel, (void*)0x42);

            // assert
            ASSERT_IS_NULL(registration);
        }
    }

    // clean
    THANDLE_ASSIGN(CANCELLATION_TOKEN)(&token, NULL);
}

/*Tests_SRS_CANCELLATION_TOKEN_04_010: [ cancellation_token_register_notify shall fail and return NULL when cancellation_token is NULL. ]*/
TEST_FUNCTION(cancellation_token_register_notify_fails_with_null_token)
{
    // arrange
    // act
    THANDLE(CANCELLATION_TOKEN_REGISTRATION) registration = cancellation_token_register_notify(NULL, on_cancel, (void*)0x42);

    // assert
    ASSERT_IS_NULL(registration);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean
}

/*Tests_SRS_CANCELLATION_TOKEN_04_011: [ cancellation_token_register_notify shall fail and return NULL when on_cancel is NULL. ]*/
TEST_FUNCTION(cancellation_token_register_notify_fails_with_null_callback)
{
    // arrange
    THANDLE(CANCELLATION_TOKEN) token = cancellation_token_create(false);
    ASSERT_IS_NOT_NULL(token);
    umock_c_reset_all_calls();

    // act
    THANDLE(CANCELLATION_TOKEN_REGISTRATION) registration = cancellation_token_register_notify(token, NULL, (void*)0x42);

    // assert
    ASSERT_IS_NULL(registration);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean
    THANDLE_ASSIGN(CANCELLATION_TOKEN)(&token, NULL);
}

/*Tests_SRS_CANCELLATION_TOKEN_04_019: [ cancellation_token_cancel shall set the state of the token to be "canceled". ]*/
/*Tests_SRS_CANCELLATION_TOKEN_04_020: [ cancellation_token_cancel shall return 0 when successful. ]*/
/*Tests_SRS_CANCELLATION_TOKEN_04_021: [ cancellation_token_cancel shall invoke all callbacks that have been registered on the token via cancellation_token_register_notify by calling TCALL_DISPATCHER_DISPATCH_CALL(CANCELLATION_TOKEN_CANCEL_CALL). ]*/
TEST_FUNCTION(cancellation_token_cancel_calls_callback)
{
    // arrange
    THANDLE(CANCELLATION_TOKEN) token = cancellation_token_create(false);
    ASSERT_IS_NOT_NULL(token);

    THANDLE(CANCELLATION_TOKEN_REGISTRATION) registration = cancellation_token_register_notify(token, on_cancel, (void*)0x42);
    ASSERT_IS_NOT_NULL(registration);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TCALL_DISPATCHER_DISPATCH_CALL(CANCELLATION_TOKEN_CANCEL_CALL)(IGNORED_ARG));
    STRICT_EXPECTED_CALL(on_cancel((void*)0x42));

    // act
    int result = cancellation_token_cancel(token);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(cancellation_token_is_canceled(token));

    // clean
    THANDLE_ASSIGN(CANCELLATION_TOKEN_REGISTRATION)(&registration, NULL);
    THANDLE_ASSIGN(CANCELLATION_TOKEN)(&token, NULL);
}

/*Tests_SRS_CANCELLATION_TOKEN_04_019: [ cancellation_token_cancel shall set the state of the token to be "canceled". ]*/
/*Tests_SRS_CANCELLATION_TOKEN_04_020: [ cancellation_token_cancel shall return 0 when successful. ]*/
/*Tests_SRS_CANCELLATION_TOKEN_04_021: [ cancellation_token_cancel shall invoke all callbacks that have been registered on the token via cancellation_token_register_notify by calling TCALL_DISPATCHER_DISPATCH_CALL(CANCELLATION_TOKEN_CANCEL_CALL). ]*/
TEST_FUNCTION(cancellation_token_cancel_calls_multiple_callbacks)
{
    // arrange
    THANDLE(CANCELLATION_TOKEN) token = cancellation_token_create(false);
    ASSERT_IS_NOT_NULL(token);

    THANDLE(CANCELLATION_TOKEN_REGISTRATION) registration1 = cancellation_token_register_notify(token, on_cancel, (void*)0x42);
    ASSERT_IS_NOT_NULL(registration1);

    THANDLE(CANCELLATION_TOKEN_REGISTRATION) registration2 = cancellation_token_register_notify(token, on_cancel2, (void*)0x84);
    ASSERT_IS_NOT_NULL(registration2);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TCALL_DISPATCHER_DISPATCH_CALL(CANCELLATION_TOKEN_CANCEL_CALL)(IGNORED_ARG));
    STRICT_EXPECTED_CALL(on_cancel((void*)0x42));
    STRICT_EXPECTED_CALL(on_cancel2((void*)0x84));

    // act
    int result = cancellation_token_cancel(token);

    // assert
    ASSERT_ARE_EQUAL(int, result, 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ASSERT_IS_TRUE(cancellation_token_is_canceled(token));

    // clean
    THANDLE_ASSIGN(CANCELLATION_TOKEN_REGISTRATION)(&registration1, NULL);
    THANDLE_ASSIGN(CANCELLATION_TOKEN_REGISTRATION)(&registration2, NULL);
    THANDLE_ASSIGN(CANCELLATION_TOKEN)(&token, NULL);
}

/*Tests_SRS_CANCELLATION_TOKEN_04_017: [ cancellation_token_cancel shall fail and return a non-zero value if cancellation_token is NULL. ]*/
TEST_FUNCTION(cancellation_token_cancel_with_null_token_fails)
{
    // arrange

    // act
    int result = cancellation_token_cancel(NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean
}

/*Tests_SRS_CANCELLATION_TOKEN_04_018: [ cancellation_token_cancel shall fail and return a non-zero value if the state of the token is already "canceled". ]*/
TEST_FUNCTION(cancellation_token_cancel_fails_when_cancelled_twice)
{
    // arrange
    THANDLE(CANCELLATION_TOKEN) token = cancellation_token_create(false);
    ASSERT_IS_NOT_NULL(token);

    THANDLE(CANCELLATION_TOKEN_REGISTRATION) registration = cancellation_token_register_notify(token, on_cancel, (void*)0x42);
    ASSERT_IS_NOT_NULL(registration);

    int result = cancellation_token_cancel(token);
    ASSERT_ARE_EQUAL(int, result, 0);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    result = cancellation_token_cancel(token);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean
    THANDLE_ASSIGN(CANCELLATION_TOKEN_REGISTRATION)(&registration, NULL);
    THANDLE_ASSIGN(CANCELLATION_TOKEN)(&token, NULL);
}

/*Tests_SRS_CANCELLATION_TOKEN_04_018: [ cancellation_token_cancel shall fail and return a non-zero value if the state of the token is already "canceled". ]*/
TEST_FUNCTION(cancellation_token_cancel_fails_on_cancelled_token)
{
    // arrange
    THANDLE(CANCELLATION_TOKEN) token = cancellation_token_create(true);
    ASSERT_IS_NOT_NULL(token);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_compare_exchange(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    int result = cancellation_token_cancel(token);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // clean
    THANDLE_ASSIGN(CANCELLATION_TOKEN)(&token, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
