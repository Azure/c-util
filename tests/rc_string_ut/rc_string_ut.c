// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "rc_string_ut_pch.h"

#define ENABLE_MOCKS
#undef ENABLE_MOCKS_DECL
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, int, mocked_vsnprintf, void*, s, size_t, n, const char*, format, va_list, args)

// this is only here because umock is attempting to copy the argument s as a char*
int proxy_mocked_vsnprintf(char* s, size_t n, const char* format, va_list args)
{
    return mocked_vsnprintf(s, n, format, args);
}

MOCKABLE_FUNCTION(, size_t, mocked_strlen, const char*, s)
#undef ENABLE_MOCKS

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static int my_vsnprintf(void* s, size_t n, const char* format, va_list args)
{
    return vsnprintf(s, n, format, args);
}

static size_t my_strlen(const char* s)
{
    return strlen(s);
}

static unsigned char va_list_to_string[] = "some va_list stringification";
static char* umockvalue_stringify_va_list(const void* value)
{
    (void)value;
    char* result = malloc(sizeof(va_list_to_string));
    ASSERT_IS_NOT_NULL(result);
    memcpy(result, va_list_to_string, sizeof(va_list_to_string));
    return result;
}

static int umockvalue_are_equal_va_list(const void* left, const void* right)
{
    (void)memcmp(left, right, sizeof(va_list));
    return 0;
}

static int umockvalue_copy_va_list(void* destination, const void* source)
{
    (void)memcpy(destination, source, sizeof(va_list));
    return 0;
}

static void umockvalue_free_va_list(void* value)
{
    (void)value;
}

// this function is used for tests just to make sure we do not confuse it with regular malloc
MOCK_FUNCTION_WITH_CODE(, void*, test_malloc_func, size_t, size_needed)
MOCK_FUNCTION_END((unsigned char*)real_gballoc_hl_malloc(size_needed + 1) + 1)

MOCK_FUNCTION_WITH_CODE(, void, test_free_func, void*, context)
    real_gballoc_hl_free((unsigned char*)context - 1);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, test_free_func_do_nothing, void*, context)
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, test_func_that_takes_charptr, const char*, str)
MOCK_FUNCTION_END()

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_initialize)
{

    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types());

    REGISTER_GLOBAL_MOCK_HOOK(mocked_vsnprintf, my_vsnprintf);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_vsnprintf, -100);

    REGISTER_GLOBAL_MOCK_HOOK(mocked_strlen, my_strlen);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_strlen, 0);

    REGISTER_UMOCK_VALUE_TYPE(va_list, umockvalue_stringify_va_list, umockvalue_are_equal_va_list, umockvalue_copy_va_list, umockvalue_free_va_list);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(function_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(function_cleanup)
{
    umock_c_negative_tests_deinit();
}

/* RC_STRING_VALUE */

/*Tests_SRS_RC_STRING_01_021: [ RC_STRING_VALUE shall print the string field of rc. ]*/
TEST_FUNCTION(RC_STRING_VALUE_works)
{
    // arrange
    THANDLE(RC_STRING) a = rc_string_create("Lagavulin");
    ASSERT_IS_NOT_NULL(a);
    char result[256];

    // act
    (void)sprintf(result, "I love %" PRI_RC_STRING "", RC_STRING_VALUE(a));

    // assert
    ASSERT_ARE_EQUAL(char_ptr, "I love Lagavulin", result);

    // cleanup
    THANDLE_ASSIGN(RC_STRING)(&a, NULL);
}

/* RC_STRING_AS_CHARPTR */

/*Tests_SRS_RC_STRING_42_001: [ If rc is NULL then RC_STRING_AS_CHARPTR shall return NULL. ]*/
TEST_FUNCTION(RC_STRING_AS_CHARPTR_with_NULL_works)
{
    // arrange
    THANDLE(RC_STRING) null_string = NULL;

    STRICT_EXPECTED_CALL(test_func_that_takes_charptr(NULL));

    // act
    test_func_that_takes_charptr(RC_STRING_AS_CHARPTR(null_string));

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_RC_STRING_42_002: [ If rc is non-NULL then RC_STRING_AS_CHARPTR shall return the string field of rc. ]*/
TEST_FUNCTION(RC_STRING_AS_CHARPTR_works)
{
    // arrange
    THANDLE(RC_STRING) a = rc_string_create("this is a string");
    ASSERT_IS_NOT_NULL(a);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_func_that_takes_charptr("this is a string"));

    // act
    test_func_that_takes_charptr(RC_STRING_AS_CHARPTR(a));

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(RC_STRING)(&a, NULL);
}

/* RC_STRING_VALUE_OR_NULL */

/*Tests_SRS_RC_STRING_01_022: [ If rc is NULL, RC_STRING_VALUE_OR_NULL shall print NULL. ]*/
TEST_FUNCTION(RC_STRING_VALUE_OR_NULL_with_NULL_yields_NULL)
{
    // arrange
    THANDLE(RC_STRING) a = NULL;
    char result[256];

    // act
    (void)sprintf(result, "I do not like %" PRI_RC_STRING "", RC_STRING_VALUE_OR_NULL(a));

    // assert
    ASSERT_ARE_EQUAL(char_ptr, "I do not like NULL", result);
}

/*Tests_SRS_RC_STRING_01_023: [ If rc is non NULL, RC_STRING_VALUE_OR_NULL shall print the string field of rc. ]*/
TEST_FUNCTION(RC_STRING_VALUE_OR_NULL_works)
{
    // arrange
    THANDLE(RC_STRING) a = rc_string_create("Laphroaig");
    ASSERT_IS_NOT_NULL(a);
    char result[256];

    // act
    (void)sprintf(result, "I love %" PRI_RC_STRING "", RC_STRING_VALUE_OR_NULL(a));

    // assert
    ASSERT_ARE_EQUAL(char_ptr, "I love Laphroaig", result);

    // cleanup
    THANDLE_ASSIGN(RC_STRING)(&a, NULL);
}

/* rc_string_create */

/*Tests_SRS_RC_STRING_01_001: [ If string is NULL, rc_string_create shall fail and return NULL. ]*/
TEST_FUNCTION(rc_string_create_with_NULL_fails)
{
    // arrange

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(rc_string);
}

/* Tests_SRS_RC_STRING_07_010: [ If the resulting memory size requested for the `THANDLE(RC_STRING)`and `string` results in an size_t overflow, `rc_string_create` shall failand return `NULL`. ]*/
TEST_FUNCTION(rc_string_create_with_overflow_memory_allocation_size_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_strlen(IGNORED_ARG))
        .SetReturn(SIZE_MAX - 24);

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create("grogu");

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(rc_string);
}

/* Tests_SRS_RC_STRING_01_002: [ Otherwise, rc_string_create shall determine the length of string. ]*/
/* Tests_SRS_RC_STRING_01_003: [ rc_string_create shall allocate memory for the THANDLE(RC_STRING), ensuring all the bytes in string can be copied (including the zero terminator). ]*/
/* Tests_SRS_RC_STRING_01_004: [ rc_string_create shall copy the string memory (including the NULL terminator). ]*/
/* Tests_SRS_RC_STRING_01_005: [ rc_string_create shall succeed and return a non-NULL handle. ]*/
TEST_FUNCTION(rc_string_create_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_strlen(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create("grogu");

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(rc_string);
    ASSERT_ARE_EQUAL(char_ptr, "grogu", rc_string->string);

    // cleanup
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);
}

/* Tests_SRS_RC_STRING_01_002: [ Otherwise, rc_string_create shall determine the length of string. ]*/
/* Tests_SRS_RC_STRING_01_003: [ rc_string_create shall allocate memory for the THANDLE(RC_STRING), ensuring all the bytes in string can be copied (including the zero terminator). ]*/
/* Tests_SRS_RC_STRING_01_004: [ rc_string_create shall copy the string memory (including the NULL terminator). ]*/
/* Tests_SRS_RC_STRING_01_005: [ rc_string_create shall succeed and return a non-NULL handle. ]*/
TEST_FUNCTION(rc_string_create_with_empty_string_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_strlen(""));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create("");

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(rc_string);
    ASSERT_ARE_EQUAL(char_ptr, "", rc_string->string);

    // cleanup
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);
}

/* Tests_SRS_RC_STRING_01_006: [ If any error occurs, rc_string_create shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_rc_string_create_also_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE(RC_STRING) rc_string = rc_string_create("gogu");

            ///assert
            ASSERT_IS_NULL(rc_string, "On failed call %zu", i);
        }
    }
}

/* rc_string_create_with_format */

/* Tests_SRS_RC_STRING_07_001: [If format is NULL, rc_string_create_with_format shall fail and return NULL.]*/
TEST_FUNCTION(rc_string_create_with_format_format_NULL_fails)
{
    // arrange

    // act
#ifdef WIN32
    THANDLE(RC_STRING) rc_string = rc_string_create_with_format(NULL);
#else
    // On Linux, printf(NULL) generates a compiler warning, so instead just call the function that doesn't do the printf validation
    THANDLE(RC_STRING) rc_string = rc_string_create_with_format_function(NULL);
#endif

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(rc_string);
}

/* Tests_SRS_RC_STRING_07_002: [ Otherwise `rc_string_create_with_format` shall determine the total number of characters written using `vsnprintf` with the variable number of arguments. ]*/
/* Tests_SRS_RC_STRING_07_004: [ `rc_string_create_with_format` shall allocate memory for the `THANDLE(RC_STRING)`and the number of bytes for the resulting formatted string. ]*/
/* Tests_SRS_RC_STRING_07_005: [ `rc_string_create_with_format` shall fill in the bytes of the string by using `vsnprintf`. ]*/
/* Tests_SRS_RC_STRING_07_007: [ `rc_string_create_with_format` shall succeed and return a non - `NULL` handle. ]*/
TEST_FUNCTION(rc_string_create_with_format_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_format("hell%c", 'o');

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(rc_string);
    ASSERT_ARE_EQUAL(char_ptr, "hello", rc_string->string);

    // cleanup
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);
}

/* Tests_SRS_RC_STRING_07_002: [ Otherwise `rc_string_create_with_format` shall determine the total number of characters written using `vsnprintf` with the variable number of arguments. ]*/
/* Tests_SRS_RC_STRING_07_004: [ `rc_string_create_with_format` shall allocate memory for the `THANDLE(RC_STRING)`and the number of bytes for the resulting formatted string. ]*/
/* Tests_SRS_RC_STRING_07_005: [ `rc_string_create_with_format` shall fill in the bytes of the string by using `vsnprintf`. ]*/
/* Tests_SRS_RC_STRING_07_007: [ `rc_string_create_with_format` shall succeed and return a non - `NULL` handle. ]*/
TEST_FUNCTION(rc_string_create_with_format_succeeds_with_percentage_sign_front)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_format("%cello", 'h');

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(rc_string);
    ASSERT_ARE_EQUAL(char_ptr, "hello", rc_string->string);

    // cleanup
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);
}

/* Tests_SRS_RC_STRING_07_002: [ Otherwise `rc_string_create_with_format` shall determine the total number of characters written using `vsnprintf` with the variable number of arguments. ]*/
/* Tests_SRS_RC_STRING_07_004: [ `rc_string_create_with_format` shall allocate memory for the `THANDLE(RC_STRING)`and the number of bytes for the resulting formatted string. ]*/
/* Tests_SRS_RC_STRING_07_005: [ `rc_string_create_with_format` shall fill in the bytes of the string by using `vsnprintf`. ]*/
/* Tests_SRS_RC_STRING_07_007: [ `rc_string_create_with_format` shall succeed and return a non - `NULL` handle. ]*/
TEST_FUNCTION(rc_string_create_with_format_succeeds_with_longer_string_input)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_format("The %s of %d and %d is %d", "sum", 3, 7, 10);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(rc_string);
    ASSERT_ARE_EQUAL(char_ptr, "The sum of 3 and 7 is 10", rc_string->string);

    // cleanup
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);
}

/*Tests_SRS_RC_STRING_07_003: [ If `vsnprintf` failed to determine the total number of characters written, `rc_string_create_with_format` shall fail and return `NULL`. ]*/
TEST_FUNCTION(when_vsnprintf_determine_written_characters_length_fails_with_return_value_negative_one_rc_string_create_with_format_also_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(-1);

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_format("hell%c", 'o');

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(rc_string);

}

/*Tests_SRS_RC_STRING_07_003: [ If `vsnprintf` failed to determine the total number of characters written, `rc_string_create_with_format` shall fail and return `NULL`. ]*/
TEST_FUNCTION(when_vsnprintf_determine_written_characters_length_fails_with_return_value_negative_two_rc_string_create_with_format_also_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(-2);

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_format("hell%c", 'o');

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(rc_string);

}

/* Tests_SRS_RC_STRING_07_002: [ Otherwise `rc_string_create_with_format` shall determine the total number of characters written using `vsnprintf` with the variable number of arguments. ]*/
/* Tests_SRS_RC_STRING_07_004: [ `rc_string_create_with_format` shall allocate memory for the `THANDLE(RC_STRING)`and the number of bytes for the resulting formatted string. ]*/
/* Tests_SRS_RC_STRING_07_005: [ `rc_string_create_with_format` shall fill in the bytes of the string by using `vsnprintf`. ]*/
/* Tests_SRS_RC_STRING_07_007: [ `rc_string_create_with_format` shall succeed and return a non - `NULL` handle. ]*/
TEST_FUNCTION(rc_string_create_with_format_with_empty_string_succeeds)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    // act
#ifdef WIN32
    THANDLE(RC_STRING) rc_string = rc_string_create_with_format("");
#else
    // On Linux, printf("") generates a compiler warning, so instead just call the function that doesn't do the printf validation
    THANDLE(RC_STRING) rc_string = rc_string_create_with_format_function("");
#endif

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(rc_string);
    ASSERT_ARE_EQUAL(char_ptr, "", rc_string->string);

    // cleanup
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);
}

/*Tests_SRS_RC_STRING_07_009: [ If the resulting memory size requested for the `THANDLE(RC_STRING)`and the resulting formatted string results in an size_t overflow in `malloc_flex`, `rc_string_create_with_format` shall failand return `NULL`. ]*/
TEST_FUNCTION(rc_string_create_with_format_with_overflow_for_memory_allocation_size_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(INT_MAX);

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_format("hell%c", 'o');

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(rc_string);

}

/*Tests_SRS_RC_STRING_07_006: [ If `vsnprintf` failed to construct the resulting formatted string, `rc_string_create_with_format` shall fail and return `NULL`. ]*/
TEST_FUNCTION(when_vsnprintf_get_resulting_formatted_string_fails_with_return_value_negative_one_rc_string_create_with_format_also_fails)
{
    //// arrange
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)).SetReturn(-1);
    STRICT_EXPECTED_CALL(gballoc_hl_free(IGNORED_ARG));

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_format("hell%c", 'o');

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(rc_string);
}

/*Tests_SRS_RC_STRING_07_006: [ If `vsnprintf` failed to construct the resulting formatted string, `rc_string_create_with_format` shall fail and return `NULL`. ]*/
TEST_FUNCTION(when_vsnprintf_get_resulting_formatted_string_fails_with_return_value_negative_two_rc_string_create_with_format_also_fails)
{
    //// arrange
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)).SetReturn(-2);
    STRICT_EXPECTED_CALL(gballoc_hl_free(IGNORED_ARG));

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_format("hell%c", 'o');

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(rc_string);
}

/* Tests_SRS_RC_STRING_07_008: [ If any error occurs, `rc_string_create_with_format` shall fail and return `NULL`. ]*/
TEST_FUNCTION(when_underlying_calls_fail_rc_string_create_with_format_also_fails)
{
    // arrange
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));
    STRICT_EXPECTED_CALL(mocked_vsnprintf(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE(RC_STRING) rc_string = rc_string_create_with_format("hell%c", 'o');

            ///assert
            ASSERT_IS_NULL(rc_string, "On failed call %zu", i);
        }
    }
}

/* rc_string_create_with_move_memory */

/* Tests_SRS_RC_STRING_01_007: [ If string is NULL, rc_string_create_with_move_memory shall fail and return NULL. ]*/
TEST_FUNCTION(rc_string_create_with_move_memory_with_NULL_fails)
{
    // arrange

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_move_memory(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(rc_string);
}

/* Tests_SRS_RC_STRING_01_008: [ Otherwise, rc_string_create_with_move_memory shall allocate memory for the THANDLE(RC_STRING). ]*/
/* Tests_SRS_RC_STRING_01_009: [ rc_string_create_with_move_memory shall associate string with the new handle. ]*/
/* Tests_SRS_RC_STRING_01_010: [ rc_string_create_with_move_memory shall succeed and return a non-NULL handle. ]*/
TEST_FUNCTION(rc_string_create_with_move_memory_succeeds)
{
    // arrange
    const char const_test_string[] = "goguletz";
    char* test_string = real_gballoc_hl_malloc(sizeof(const_test_string));
    ASSERT_IS_NOT_NULL(test_string);

    (void)memcpy(test_string, const_test_string, sizeof(const_test_string));

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_move_memory(test_string);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(rc_string);
    ASSERT_ARE_EQUAL(void_ptr, test_string, rc_string->string);

    // cleanup
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);
}

/* Tests_SRS_RC_STRING_01_008: [ Otherwise, rc_string_create_with_move_memory shall allocate memory for the THANDLE(RC_STRING). ]*/
/* Tests_SRS_RC_STRING_01_009: [ rc_string_create_with_move_memory shall associate string with the new handle. ]*/
/* Tests_SRS_RC_STRING_01_010: [ rc_string_create_with_move_memory shall succeed and return a non-NULL handle. ]*/
TEST_FUNCTION(rc_string_create_with_move_memory_with_empty_string_succeeds)
{
    // arrange
    const char const_test_string[] = "";
    char* test_string = real_gballoc_hl_malloc(sizeof(const_test_string));
    ASSERT_IS_NOT_NULL(test_string);

    (void)memcpy(test_string, const_test_string, sizeof(const_test_string));

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_move_memory(test_string);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(rc_string);
    ASSERT_ARE_EQUAL(void_ptr, test_string, rc_string->string);

    // cleanup
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);
}

/* Tests_SRS_RC_STRING_01_020: [ When the THANDLE(RC_STRING) reference count reaches 0, string shall be free with free. ]*/
TEST_FUNCTION(a_string_created_with_rc_string_create_with_move_memory_frees_the_original_memory_with_free)
{
    // arrange
    const char const_test_string[] = "goguletz";
    char* test_string = real_gballoc_hl_malloc(sizeof(const_test_string));
    ASSERT_IS_NOT_NULL(test_string);

    (void)memcpy(test_string, const_test_string, sizeof(const_test_string));

    THANDLE(RC_STRING) rc_string = rc_string_create_with_move_memory(test_string);
    ASSERT_IS_NOT_NULL(rc_string);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(free(test_string));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_RC_STRING_01_011: [ If any error occurs, rc_string_create_with_move_memory shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_rc_string_create_with_move_memory_also_fails)
{
    // arrange
    const char const_test_string[] = "";

    char* test_string = real_gballoc_hl_malloc(sizeof(const_test_string));
    ASSERT_IS_NOT_NULL(test_string);

    (void)memcpy(test_string, const_test_string, sizeof(const_test_string));

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE(RC_STRING) rc_string = rc_string_create_with_move_memory(test_string);

            ///assert
            ASSERT_IS_NULL(rc_string, "On failed call %zu", i);
        }
    }

    // cleanup
    real_gballoc_hl_free(test_string);
}

/* rc_string_create_with_custom_free */

/* Tests_SRS_RC_STRING_01_012: [ If string is NULL, rc_string_create_with_custom_free shall fail and return NULL. ]*/
TEST_FUNCTION(rc_string_create_with_custom_free_with_NULL_string_fails)
{
    // arrange

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_custom_free(NULL, test_free_func, (void*)4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(rc_string);
}

/* Tests_SRS_RC_STRING_01_013: [ If free_func is NULL, rc_string_create_with_custom_free shall fail and return NULL. ]*/
TEST_FUNCTION(rc_string_create_with_custom_free_with_NULL_free_func_fails)
{
    // arrange

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_custom_free(NULL, test_free_func, (void*)4242);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(rc_string);
}

/* Tests_SRS_RC_STRING_01_015: [ rc_string_create_with_custom_free shall allocate memory for the THANDLE(RC_STRING). ]*/
/* Tests_SRS_RC_STRING_01_016: [ rc_string_create_with_custom_free shall associate string, free_func and free_func_context with the new handle. ]*/
/* Tests_SRS_RC_STRING_01_017: [ rc_string_create_with_custom_free shall succeed and return a non-NULL handle. ]*/
TEST_FUNCTION(rc_string_create_with_custom_free_succeeds)
{
    // arrange
    const char const_test_string[] = "goguletz";
    char* test_string = test_malloc_func(sizeof(const_test_string));
    ASSERT_IS_NOT_NULL(test_string);

    (void)memcpy(test_string, const_test_string, sizeof(const_test_string));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_custom_free(test_string, test_free_func, test_string);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(rc_string);
    ASSERT_ARE_EQUAL(void_ptr, test_string, rc_string->string);

    // cleanup
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);
}

/* Tests_SRS_RC_STRING_01_015: [ rc_string_create_with_custom_free shall allocate memory for the THANDLE(RC_STRING). ]*/
/* Tests_SRS_RC_STRING_01_016: [ rc_string_create_with_custom_free shall associate string, free_func and free_func_context with the new handle. ]*/
/* Tests_SRS_RC_STRING_01_017: [ rc_string_create_with_custom_free shall succeed and return a non-NULL handle. ]*/
TEST_FUNCTION(rc_string_create_with_custom_free_with_empty_string_succeeds)
{
    // arrange
    const char const_test_string[] = "";
    char* test_string = test_malloc_func(sizeof(const_test_string));
    ASSERT_IS_NOT_NULL(test_string);

    (void)memcpy(test_string, const_test_string, sizeof(const_test_string));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_custom_free(test_string, test_free_func, test_string);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(rc_string);
    ASSERT_ARE_EQUAL(void_ptr, test_string, rc_string->string);

    // cleanup
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);
}

/* Tests_SRS_RC_STRING_01_014: [ free_func_context shall be allowed to be NULL. ]*/
TEST_FUNCTION(rc_string_create_with_custom_free_with_do_nothing_free_and_NULL_context_succeeds)
{
    // arrange
    const char const_test_string[] = "";
    char* test_string = test_malloc_func(sizeof(const_test_string));
    ASSERT_IS_NOT_NULL(test_string);

    (void)memcpy(test_string, const_test_string, sizeof(const_test_string));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));

    // act
    THANDLE(RC_STRING) rc_string = rc_string_create_with_custom_free(test_string, test_free_func_do_nothing, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(rc_string);
    ASSERT_ARE_EQUAL(void_ptr, test_string, rc_string->string);

    // cleanup
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);
    // because the free function does nothing, we need to free this here
    test_free_func(test_string);
}

/* Tests_SRS_RC_STRING_01_019: [ If any error occurs, rc_string_create_with_custom_free shall fail and return NULL. ]*/
TEST_FUNCTION(when_underlying_calls_fail_rc_string_create_with_custom_free_also_fails)
{
    // arrange
    const char const_test_string[] = "";
    char* test_string = test_malloc_func(sizeof(const_test_string));
    ASSERT_IS_NOT_NULL(test_string);

    (void)memcpy(test_string, const_test_string, sizeof(const_test_string));

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE(RC_STRING) rc_string = rc_string_create_with_custom_free(test_string, test_free_func, test_string);

            ///assert
            ASSERT_IS_NULL(rc_string, "On failed call %zu", i);
        }
    }

    // cleanup
    test_free_func(test_string);
}

/* Tests_SRS_RC_STRING_01_018: [ When the THANDLE(RC_STRING) reference count reaches 0, free_func shall be called with free_func_context to free the memory used by string. ]*/
TEST_FUNCTION(when_reference_count_reaches_0_the_custom_free_function_is_called)
{
    // arrange
    const char const_test_string[] = "";
    char* test_string = test_malloc_func(sizeof(const_test_string));
    ASSERT_IS_NOT_NULL(test_string);

    (void)memcpy(test_string, const_test_string, sizeof(const_test_string));

    THANDLE(RC_STRING) rc_string = rc_string_create_with_custom_free(test_string, test_free_func, test_string);
    ASSERT_IS_NOT_NULL(rc_string);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_free_func(test_string));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_RC_STRING_01_018: [ When the THANDLE(RC_STRING) reference count reaches 0, free_func shall be called with free_func_context to free the memory used by string. ]*/
TEST_FUNCTION(when_reference_count_reaches_0_the_custom_free_function_is_called_context_NULL)
{
    // arrange
    const char const_test_string[] = "";
    char* test_string = test_malloc_func(sizeof(const_test_string));
    ASSERT_IS_NOT_NULL(test_string);

    (void)memcpy(test_string, const_test_string, sizeof(const_test_string));

    THANDLE(RC_STRING) rc_string = rc_string_create_with_custom_free(test_string, test_free_func_do_nothing, NULL);
    ASSERT_IS_NOT_NULL(rc_string);

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(test_free_func_do_nothing(NULL));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    test_free_func(test_string);
}

/*Tests_SRS_RC_STRING_02_001: [ If self is NULL then rc_string_recreate shall return. ]*/
TEST_FUNCTION(rc_string_recreate_with_self_NULL_returns)
{
    ///arrange

    ///act
    rc_string_recreate(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_RC_STRING_02_002: [ rc_string_recreate shall perform same steps as rc_string_create to return a THANDLE(RC_STRING) with the same content as source. ]*/
TEST_FUNCTION(rc_string_recreate_succeeds_1)
{
    ///arrange
    const char source[] = "bla";

    STRICT_EXPECTED_CALL(mocked_strlen(source));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));

    THANDLE(RC_STRING) rc_string = rc_string_create(source);
    ASSERT_IS_NOT_NULL(rc_string);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, source, rc_string->string);

    STRICT_EXPECTED_CALL(mocked_strlen(source));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));

    ///act
    THANDLE(RC_STRING) same = rc_string_recreate(rc_string);

    ///assert
    ASSERT_IS_NOT_NULL(same);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, source, same->string);

    ///clean
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);
    THANDLE_ASSIGN(RC_STRING)(&same, NULL);
}

/*Tests_SRS_RC_STRING_02_002: [ rc_string_recreate shall perform same steps as rc_string_create to return a THANDLE(RC_STRING) with the same content as source. ]*/
TEST_FUNCTION(rc_string_recreate_fails_when_malloc_fails)
{
    ///arrange
    const char source[] = "bla";

    STRICT_EXPECTED_CALL(mocked_strlen(source));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));

    THANDLE(RC_STRING) rc_string = rc_string_create(source);
    ASSERT_IS_NOT_NULL(rc_string);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, source, rc_string->string);

    STRICT_EXPECTED_CALL(mocked_strlen(source));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)))
        .SetReturn(NULL);

    ///act
    THANDLE(RC_STRING) same = rc_string_recreate(rc_string);

    ///assert
    ASSERT_IS_NULL(same);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, source, rc_string->string);

    ///clean
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);
}

/*Tests_SRS_RC_STRING_02_002: [ rc_string_recreate shall perform same steps as rc_string_create to return a THANDLE(RC_STRING) with the same content as source. ]*/
/*tests wants to see that a recreation can be done on a inc_ref'd handle*/
TEST_FUNCTION(rc_string_recreate_succeeds_2)
{
    ///arrange
    const char source[] = "bla2";

    STRICT_EXPECTED_CALL(mocked_strlen(source));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));

    THANDLE(RC_STRING) rc_string = rc_string_create(source);
    ASSERT_IS_NOT_NULL(rc_string);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, source, rc_string->string);

    THANDLE(RC_STRING) rc_string_2 = NULL;
    THANDLE_INITIALIZE(RC_STRING)(&rc_string_2, rc_string);

    STRICT_EXPECTED_CALL(mocked_strlen(source));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, sizeof(char)));

    ///act
    THANDLE(RC_STRING) same = rc_string_recreate(rc_string);

    ///assert
    ASSERT_IS_NOT_NULL(same);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(char_ptr, same->string, rc_string->string);

    ///clean
    THANDLE_ASSIGN(RC_STRING)(&rc_string_2, NULL);
    THANDLE_ASSIGN(RC_STRING)(&rc_string, NULL);
    THANDLE_ASSIGN(RC_STRING)(&same, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)