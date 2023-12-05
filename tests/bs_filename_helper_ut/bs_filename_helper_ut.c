// Copyright (c) Microsoft. All rights reserved.


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>


#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"

#include "c_pal/interlocked.h" /*included for mocking reasons - it will prohibit creation of mocks belonging to interlocked.h - at the moment verified through int tests - this is porting legacy code, temporary solution*/

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/string_utils.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_string_utils.h"

#include "c_util/bs_filename_helper.h"

/*following function cannot be mocked because of variable number of arguments:( so it is copy&pasted here*/
char* sprintf_char_function(const char* format, ...)
{
    char* result;
    va_list va;
    va_start(va, format);
    result = vsprintf_char(format, va);
    va_end(va);
    return result;
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

    REGISTER_UMOCK_ALIAS_TYPE(va_list, void*);
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

/*Tests_SRS_BS_FILENAME_HELPER_42_001: [ If filename is NULL then bs_filename_append_suffix shall fail and return NULL. ]*/
TEST_FUNCTION(bs_filename_append_suffix_with_filename_NULL_fails)
{
    ///arrange
    const char suffix[] = DEFAULT_SUFFIX;

    ///act
    char* result = bs_filename_append_suffix(NULL, suffix);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_BS_FILENAME_HELPER_42_002: [ If suffix is NULL then bs_filename_append_suffix shall fail and return NULL. ]*/
TEST_FUNCTION(bs_filename_append_suffix_with_suffix_NULL_fails)
{
    ///arrange
    const char filename[] = FILENAME_NO_DOT;

    ///act
    char* result = bs_filename_append_suffix(filename, NULL);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_BS_FILENAME_HELPER_42_003: [ If filename does not contain a . then bs_filename_append_suffix shall return filename + suffix. ]*/
TEST_FUNCTION(bs_filename_append_suffix_without_dot_creates_a_new_string_and_succeeds)
{
    ///arrange
    const char filename[] = FILENAME_NO_DOT;
    const char suffix[] = DEFAULT_SUFFIX;

    STRICT_EXPECTED_CALL(vsprintf_char("%s%s", IGNORED_ARG));

    ///act
    char* result = bs_filename_append_suffix(filename, suffix);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, FILENAME_NO_DOT DEFAULT_SUFFIX, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_BS_FILENAME_HELPER_42_004: [ If filename does not contain a \ then bs_filename_append_suffix shall return the name of the file + suffix + . + extension. ]*/
TEST_FUNCTION(bs_filename_append_suffix_without_backslash_creates_a_new_string_and_succeeds)
{
    ///arrange
    const char filename[] = FULL_FILENAME_NO_BACKSLASH;
    const char suffix[] = DEFAULT_SUFFIX;

    STRICT_EXPECTED_CALL(vsprintf_char("%.*s%s%s", IGNORED_ARG));

    ///act
    char* result = bs_filename_append_suffix(filename, suffix);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, FILENAME DEFAULT_SUFFIX DOT EXTENSION, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_BS_FILENAME_HELPER_42_005: [ If filename contains last . before last \ then bs_filename_append_suffix shall return filename + suffix. ]*/
TEST_FUNCTION(bs_filename_append_suffix_with_last_dot_before_backslash_creates_a_new_string_and_succeeds)
{
    ///arrange
    const char filename[] = FILENAME_DOT_BEFORE_BACKSLASH;
    const char suffix[] = DEFAULT_SUFFIX;

    STRICT_EXPECTED_CALL(vsprintf_char("%s%s", IGNORED_ARG));

    ///act
    char* result = bs_filename_append_suffix(filename, suffix);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, FILENAME_DOT_BEFORE_BACKSLASH DEFAULT_SUFFIX, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_BS_FILENAME_HELPER_42_006: [ If filename contains last . after last \ then bs_filename_append_suffix shall return the name of the file + suffix + . + extension. ]*/
TEST_FUNCTION(bs_filename_append_suffix_with_dot_after_backslash_creates_a_new_string_and_succeeds)
{
    ///arrange
    const char filename[] = FULL_FILENAME_DOT_AFTER_BACKSLASH;
    const char suffix[] = DEFAULT_SUFFIX;

    STRICT_EXPECTED_CALL(vsprintf_char("%.*s%s%s", IGNORED_ARG));

    ///act
    char* result = bs_filename_append_suffix(filename, suffix);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, PATH_DOT_AFTER_BACKSLASH FILENAME_DOT_AFTER_BACKSLASH DEFAULT_SUFFIX DOT EXTENSION, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_BS_FILENAME_HELPER_42_006: [ If filename contains last . after last \ then bs_filename_append_suffix shall return the name of the file + suffix + . + extension. ]*/
TEST_FUNCTION(bs_filename_append_suffix_creates_a_new_string_and_succeeds_5)
{
    ///arrange
    const char filename[] = FULL_FILENAME_DOT_AFTER_BACKSLASH_2;
    const char suffix[] = DEFAULT_SUFFIX;

    STRICT_EXPECTED_CALL(vsprintf_char("%.*s%s%s", IGNORED_ARG));

    ///act
    char* result = bs_filename_append_suffix(filename, suffix);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, PATH_DOT_AFTER_BACKSLASH_2 FILENAME_DOT_AFTER_BACKSLASH_2 DEFAULT_SUFFIX DOT EXTENSION, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_BS_FILENAME_HELPER_42_007: [ If there are any failures then bs_filename_append_suffix shall return NULL. ]*/
TEST_FUNCTION(bs_filename_append_suffix_fails_when_malloc_fails_1)
{
    ///arrange
    const char filename[] = FULL_FILENAME_DOT_AFTER_BACKSLASH;
    const char suffix[] = DEFAULT_SUFFIX;

    STRICT_EXPECTED_CALL(vsprintf_char("%.*s%s%s", IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    char* result = bs_filename_append_suffix(filename, suffix);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_BS_FILENAME_HELPER_42_007: [ If there are any failures then bs_filename_append_suffix shall return NULL. ]*/
TEST_FUNCTION(bs_filename_append_suffix_fails_when_malloc_fails_2)
{
    ///arrange
    const char filename[] = FILENAME_NO_DOT;
    const char suffix[] = DEFAULT_SUFFIX;

    STRICT_EXPECTED_CALL(vsprintf_char("%s%s", IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    char* result = bs_filename_append_suffix(filename, suffix);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
