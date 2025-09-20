// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "rc_string_utils_ut_pch.h"

CTEST_DEFINE_EQUALITY_ASSERTION_FUNCTIONS_FOR_TYPE(TEST_THANDLE_RC_STRING, );

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static void expect_rc_string_utils_split_by_char_does_split(uint32_t count, const char** expected_strings)
{
    STRICT_EXPECTED_CALL(rc_string_array_create(count));
    for (uint32_t i = 0; i < count; i++)
    {
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(rc_string_create_with_move_memory(expected_strings[i]));
        STRICT_EXPECTED_CALL(THANDLE_MOVE(RC_STRING)(IGNORED_ARG, IGNORED_ARG));
    }
}

static void validate_split(RC_STRING_ARRAY* result, uint32_t count, const char** expected_strings)
{
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(uint32_t, count, result->count);
    for (uint32_t i = 0; i < count; i++)
    {
        ASSERT_ARE_EQUAL(char_ptr, expected_strings[i], result->string_array[i]->string);
    }
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types");

    REGISTER_RC_STRING_GLOBAL_MOCK_HOOKS();
    REGISTER_RC_STRING_ARRAY_GLOBAL_MOCK_HOOK();

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(rc_string_create_with_move_memory, NULL);

    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(RC_STRING), void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

//
// rc_string_utils_split_by_char
//

/*Tests_SRS_RC_STRING_UTILS_42_001: [ If str is NULL then rc_string_utils_split_by_char shall fail and return NULL. ]*/
TEST_FUNCTION(rc_string_utils_split_by_char_with_NULL_str_fails)
{
    // arrange

    // act
    RC_STRING_ARRAY* result = rc_string_utils_split_by_char(NULL, ',');

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_RC_STRING_UTILS_42_002: [ rc_string_utils_split_by_char shall count the delimiter characters in str and add 1 to compute the number of resulting sub-strings. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_003: [ rc_string_utils_split_by_char shall call rc_string_array_create with the count of split strings. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_011: [ If only 1 sub-string is found (delimiter is not found in str) then rc_string_utils_split_by_char shall call THANDLE_ASSIGN(RC_STRING) to copy str into the allocated array. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_009: [ rc_string_utils_split_by_char shall return the allocated array. ]*/
TEST_FUNCTION(rc_string_utils_split_by_char_with_empty_string_just_does_assign)
{
    // arrange
    THANDLE(RC_STRING) str = real_rc_string_create("");
    ASSERT_IS_NOT_NULL(str);

    STRICT_EXPECTED_CALL(rc_string_array_create(1));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_STRING)(IGNORED_ARG, str));

    // act
    RC_STRING_ARRAY* result = rc_string_utils_split_by_char(str, ',');

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ASSERT_ARE_EQUAL(uint32_t, 1, result->count);
    ASSERT_ARE_EQUAL(TEST_THANDLE_RC_STRING, str, result->string_array[0]);

    // cleanup
    real_rc_string_array_destroy(result);
    THANDLE_ASSIGN(real_RC_STRING)(&str, NULL);
}

/*Tests_SRS_RC_STRING_UTILS_42_002: [ rc_string_utils_split_by_char shall count the delimiter characters in str and add 1 to compute the number of resulting sub-strings. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_003: [ rc_string_utils_split_by_char shall call rc_string_array_create with the count of split strings. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_011: [ If only 1 sub-string is found (delimiter is not found in str) then rc_string_utils_split_by_char shall call THANDLE_ASSIGN(RC_STRING) to copy str into the allocated array. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_009: [ rc_string_utils_split_by_char shall return the allocated array. ]*/
TEST_FUNCTION(rc_string_utils_split_by_char_with_no_delimiter_found_just_does_assign)
{
    // arrange
    THANDLE(RC_STRING) str = real_rc_string_create("hello world");
    ASSERT_IS_NOT_NULL(str);

    STRICT_EXPECTED_CALL(rc_string_array_create(1));
    STRICT_EXPECTED_CALL(THANDLE_ASSIGN(RC_STRING)(IGNORED_ARG, str));

    // act
    RC_STRING_ARRAY* result = rc_string_utils_split_by_char(str, ',');

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ASSERT_ARE_EQUAL(uint32_t, 1, result->count);
    ASSERT_ARE_EQUAL(TEST_THANDLE_RC_STRING, str, result->string_array[0]);

    // cleanup
    real_rc_string_array_destroy(result);
    THANDLE_ASSIGN(real_RC_STRING)(&str, NULL);
}

/*Tests_SRS_RC_STRING_UTILS_42_002: [ rc_string_utils_split_by_char shall count the delimiter characters in str and add 1 to compute the number of resulting sub-strings. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_003: [ rc_string_utils_split_by_char shall call rc_string_array_create with the count of split strings. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_004: [ For each delimiter found in the string, plus the null-terminator: ]*/
/*Tests_SRS_RC_STRING_UTILS_42_005: [ rc_string_utils_split_by_char shall allocate memory for the sub-string and a null-terminator. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_006: [ rc_string_utils_split_by_char shall copy the sub-string into the allocated memory. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_007: [ rc_string_utils_split_by_char shall call rc_string_create_with_move_memory. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_008: [ rc_string_utils_split_by_char shall call THANDLE_MOVE(RC_STRING) to move the allocated string into the allocated array. ]*/
TEST_FUNCTION(rc_string_utils_split_by_char_splits_into_two_strings_by_comma)
{
    // arrange
    THANDLE(RC_STRING) str = real_rc_string_create("value1,42");
    ASSERT_IS_NOT_NULL(str);

    const char* expected_strings[] = { "value1", "42" };

    expect_rc_string_utils_split_by_char_does_split(MU_COUNT_ARRAY_ITEMS(expected_strings), expected_strings);

    // act
    RC_STRING_ARRAY* result = rc_string_utils_split_by_char(str, ',');

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    validate_split(result, MU_COUNT_ARRAY_ITEMS(expected_strings), expected_strings);

    // cleanup
    real_rc_string_array_destroy(result);
    THANDLE_ASSIGN(real_RC_STRING)(&str, NULL);
}

/*Tests_SRS_RC_STRING_UTILS_42_002: [ rc_string_utils_split_by_char shall count the delimiter characters in str and add 1 to compute the number of resulting sub-strings. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_003: [ rc_string_utils_split_by_char shall call rc_string_array_create with the count of split strings. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_004: [ For each delimiter found in the string, plus the null-terminator: ]*/
/*Tests_SRS_RC_STRING_UTILS_42_005: [ rc_string_utils_split_by_char shall allocate memory for the sub-string and a null-terminator. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_006: [ rc_string_utils_split_by_char shall copy the sub-string into the allocated memory. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_007: [ rc_string_utils_split_by_char shall call rc_string_create_with_move_memory. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_008: [ rc_string_utils_split_by_char shall call THANDLE_MOVE(RC_STRING) to move the allocated string into the allocated array. ]*/
TEST_FUNCTION(rc_string_utils_split_by_char_splits_into_three_strings_by_trailing_comma)
{
    // arrange
    THANDLE(RC_STRING) str = real_rc_string_create("value1,42,");
    ASSERT_IS_NOT_NULL(str);

    const char* expected_strings[] = { "value1", "42", ""};

    expect_rc_string_utils_split_by_char_does_split(MU_COUNT_ARRAY_ITEMS(expected_strings), expected_strings);

    // act
    RC_STRING_ARRAY* result = rc_string_utils_split_by_char(str, ',');

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    validate_split(result, MU_COUNT_ARRAY_ITEMS(expected_strings), expected_strings);

    // cleanup
    real_rc_string_array_destroy(result);
    THANDLE_ASSIGN(real_RC_STRING)(&str, NULL);
}

/*Tests_SRS_RC_STRING_UTILS_42_002: [ rc_string_utils_split_by_char shall count the delimiter characters in str and add 1 to compute the number of resulting sub-strings. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_003: [ rc_string_utils_split_by_char shall call rc_string_array_create with the count of split strings. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_004: [ For each delimiter found in the string, plus the null-terminator: ]*/
/*Tests_SRS_RC_STRING_UTILS_42_005: [ rc_string_utils_split_by_char shall allocate memory for the sub-string and a null-terminator. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_006: [ rc_string_utils_split_by_char shall copy the sub-string into the allocated memory. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_007: [ rc_string_utils_split_by_char shall call rc_string_create_with_move_memory. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_008: [ rc_string_utils_split_by_char shall call THANDLE_MOVE(RC_STRING) to move the allocated string into the allocated array. ]*/
TEST_FUNCTION(rc_string_utils_split_by_char_splits_into_five_strings_by_character_dollar_sign)
{
    // arrange
    THANDLE(RC_STRING) str = real_rc_string_create("some$weird$example$goes,$ here");
    ASSERT_IS_NOT_NULL(str);

    const char* expected_strings[] = { "some", "weird", "example", "goes,", " here" };

    expect_rc_string_utils_split_by_char_does_split(MU_COUNT_ARRAY_ITEMS(expected_strings), expected_strings);

    // act
    RC_STRING_ARRAY* result = rc_string_utils_split_by_char(str, '$');

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    validate_split(result, MU_COUNT_ARRAY_ITEMS(expected_strings), expected_strings);

    // cleanup
    real_rc_string_array_destroy(result);
    THANDLE_ASSIGN(real_RC_STRING)(&str, NULL);
}

/*Tests_SRS_RC_STRING_UTILS_42_002: [ rc_string_utils_split_by_char shall count the delimiter characters in str and add 1 to compute the number of resulting sub-strings. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_003: [ rc_string_utils_split_by_char shall call rc_string_array_create with the count of split strings. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_004: [ For each delimiter found in the string, plus the null-terminator: ]*/
/*Tests_SRS_RC_STRING_UTILS_42_005: [ rc_string_utils_split_by_char shall allocate memory for the sub-string and a null-terminator. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_006: [ rc_string_utils_split_by_char shall copy the sub-string into the allocated memory. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_007: [ rc_string_utils_split_by_char shall call rc_string_create_with_move_memory. ]*/
/*Tests_SRS_RC_STRING_UTILS_42_008: [ rc_string_utils_split_by_char shall call THANDLE_MOVE(RC_STRING) to move the allocated string into the allocated array. ]*/
TEST_FUNCTION(rc_string_utils_split_by_char_splits_into_three_strings_by_comma_with_blanks_in_middle)
{
    // arrange
    THANDLE(RC_STRING) str = real_rc_string_create("value1,,42");
    ASSERT_IS_NOT_NULL(str);

    const char* expected_strings[] = { "value1", "", "42" };

    expect_rc_string_utils_split_by_char_does_split(MU_COUNT_ARRAY_ITEMS(expected_strings), expected_strings);

    // act
    RC_STRING_ARRAY* result = rc_string_utils_split_by_char(str, ',');

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    validate_split(result, MU_COUNT_ARRAY_ITEMS(expected_strings), expected_strings);

    // cleanup
    real_rc_string_array_destroy(result);
    THANDLE_ASSIGN(real_RC_STRING)(&str, NULL);
}

/*Tests_SRS_RC_STRING_UTILS_42_010: [ If there are any errors then rc_string_utils_split_by_char shall fail and return NULL. ]*/
TEST_FUNCTION(rc_string_utils_split_by_char_fails_when_underlying_functions_fail)
{
    // arrange
    THANDLE(RC_STRING) str = real_rc_string_create("value1,42");
    ASSERT_IS_NOT_NULL(str);

    const char* expected_strings[] = { "value1", "42" };

    expect_rc_string_utils_split_by_char_does_split(MU_COUNT_ARRAY_ITEMS(expected_strings), expected_strings);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            RC_STRING_ARRAY* result = rc_string_utils_split_by_char(str, ',');

            // assert
            ASSERT_IS_NULL(result, "On failed call %zu", i);
        }
    }

    // cleanup
    THANDLE_ASSIGN(real_RC_STRING)(&str, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)