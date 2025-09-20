// Copyright (c) Microsoft. All rights reserved.



#include "filename_helper_ut_pch.h"

char* sprintf_char_function(const char* format, ...)
{
    char* result;
    va_list va;
    va_start(va, format);
    result = vsprintf_char(format, va);
    va_end(va);
    return result;
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


#define DOT "."
#define EXTENSION "bsdl"

#define FILENAME_NO_DOT "a"

#define FILENAME "a"
#define DOT "."
#define EXTENSION "bsdl"
#define FULL_FILENAME_NO_BACKSLASH FILENAME DOT EXTENSION

#define FILENAME_DOT_BEFORE_BACKSLASH "maybeThisIsAFolder.exe\\andSomeOtherFolder.bat\\a"

#define PATH_DOT_AFTER_BACKSLASH "c:\\program files\\nothing\\"
#define FILENAME_DOT_AFTER_BACKSLASH "a"
#define FULL_FILENAME_DOT_AFTER_BACKSLASH PATH_DOT_AFTER_BACKSLASH FILENAME_DOT_AFTER_BACKSLASH DOT EXTENSION

#define PATH_DOT_AFTER_BACKSLASH_2 "c:\\program files.exe\\nothing.Q\\"
#define FILENAME_DOT_AFTER_BACKSLASH_2 "abcdfe"
#define FULL_FILENAME_DOT_AFTER_BACKSLASH_2 PATH_DOT_AFTER_BACKSLASH_2 FILENAME_DOT_AFTER_BACKSLASH_2 DOT EXTENSION

#define DEFAULT_SUFFIX "_suffix"

// This ignore is due to gcc treating the STRICT_EXPECTED_CALL as a 
// string function and complaining that it is 'accessing 24 bytes in a region of size 21'
// This is not the case and cannot be fixed without making the test fail
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");

    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types());

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_STRING_UTILS_GLOBAL_MOCK_HOOK();

    //linux says these days that sizeof(va_list) is 24, so REGISTER_UMOCK_ALIAS_TYPE(va_list, void*); would not work.
    REGISTER_UMOCK_VALUE_TYPE(va_list, umockvalue_stringify_va_list, umockvalue_are_equal_va_list, umockvalue_copy_va_list, umockvalue_free_va_list);
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

/*Tests_SRS_FILENAME_HELPER_42_001: [ If filename is NULL then filename_append_suffix shall fail and return NULL. ]*/
TEST_FUNCTION(filename_append_suffix_with_filename_NULL_fails)
{
    ///arrange
    const char suffix[] = DEFAULT_SUFFIX;

    ///act
    char* result = filename_append_suffix(NULL, suffix);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FILENAME_HELPER_42_002: [ If suffix is NULL then filename_append_suffix shall fail and return NULL. ]*/
TEST_FUNCTION(filename_append_suffix_with_suffix_NULL_fails)
{
    ///arrange
    const char filename[] = FILENAME_NO_DOT;

    ///act
    char* result = filename_append_suffix(filename, NULL);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FILENAME_HELPER_42_003: [ If filename does not contain a . then filename_append_suffix shall return filename + suffix. ]*/
TEST_FUNCTION(filename_append_suffix_without_dot_creates_a_new_string_and_succeeds)
{
    ///arrange
    const char filename[] = FILENAME_NO_DOT;
    const char suffix[] = DEFAULT_SUFFIX;

    STRICT_EXPECTED_CALL(vsprintf_char("%s%s", IGNORED_ARG));

    ///act
    char* result = filename_append_suffix(filename, suffix);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, FILENAME_NO_DOT DEFAULT_SUFFIX, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_FILENAME_HELPER_42_004: [ If filename does not contain a \ then filename_append_suffix shall return the name of the file + suffix + . + extension. ]*/
TEST_FUNCTION(filename_append_suffix_without_backslash_creates_a_new_string_and_succeeds)
{
    ///arrange
    const char filename[] = FULL_FILENAME_NO_BACKSLASH;
    const char suffix[] = DEFAULT_SUFFIX;

    STRICT_EXPECTED_CALL(vsprintf_char("%.*s%s%s", IGNORED_ARG));

    ///act
    char* result = filename_append_suffix(filename, suffix);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, FILENAME DEFAULT_SUFFIX DOT EXTENSION, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_FILENAME_HELPER_42_005: [ If filename contains last . before last \ then filename_append_suffix shall return filename + suffix. ]*/
TEST_FUNCTION(filename_append_suffix_with_last_dot_before_backslash_creates_a_new_string_and_succeeds)
{
    ///arrange
    const char filename[] = FILENAME_DOT_BEFORE_BACKSLASH;
    const char suffix[] = DEFAULT_SUFFIX;

    STRICT_EXPECTED_CALL(vsprintf_char("%s%s", IGNORED_ARG));

    ///act
    char* result = filename_append_suffix(filename, suffix);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, FILENAME_DOT_BEFORE_BACKSLASH DEFAULT_SUFFIX, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_FILENAME_HELPER_42_006: [ If filename contains last . after last \ then filename_append_suffix shall return the name of the file + suffix + . + extension. ]*/
TEST_FUNCTION(filename_append_suffix_with_dot_after_backslash_creates_a_new_string_and_succeeds)
{
    ///arrange
    const char filename[] = FULL_FILENAME_DOT_AFTER_BACKSLASH;
    const char suffix[] = DEFAULT_SUFFIX;

    STRICT_EXPECTED_CALL(vsprintf_char("%.*s%s%s", IGNORED_ARG));

    ///act
    char* result = filename_append_suffix(filename, suffix);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, PATH_DOT_AFTER_BACKSLASH FILENAME_DOT_AFTER_BACKSLASH DEFAULT_SUFFIX DOT EXTENSION, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_FILENAME_HELPER_42_006: [ If filename contains last . after last \ then filename_append_suffix shall return the name of the file + suffix + . + extension. ]*/
TEST_FUNCTION(filename_append_suffix_creates_a_new_string_and_succeeds_5)
{
    ///arrange
    const char filename[] = FULL_FILENAME_DOT_AFTER_BACKSLASH_2;
    const char suffix[] = DEFAULT_SUFFIX;

    STRICT_EXPECTED_CALL(vsprintf_char("%.*s%s%s", IGNORED_ARG));

    ///act
    char* result = filename_append_suffix(filename, suffix);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, PATH_DOT_AFTER_BACKSLASH_2 FILENAME_DOT_AFTER_BACKSLASH_2 DEFAULT_SUFFIX DOT EXTENSION, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_FILENAME_HELPER_42_007: [ If there are any failures then filename_append_suffix shall return NULL. ]*/
TEST_FUNCTION(filename_append_suffix_fails_when_malloc_fails_1)
{
    ///arrange
    const char filename[] = FULL_FILENAME_DOT_AFTER_BACKSLASH;
    const char suffix[] = DEFAULT_SUFFIX;

    STRICT_EXPECTED_CALL(vsprintf_char("%.*s%s%s", IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    char* result = filename_append_suffix(filename, suffix);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FILENAME_HELPER_42_007: [ If there are any failures then filename_append_suffix shall return NULL. ]*/
TEST_FUNCTION(filename_append_suffix_fails_when_malloc_fails_2)
{
    ///arrange
    const char filename[] = FILENAME_NO_DOT;
    const char suffix[] = DEFAULT_SUFFIX;

    STRICT_EXPECTED_CALL(vsprintf_char("%s%s", IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    char* result = filename_append_suffix(filename, suffix);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)