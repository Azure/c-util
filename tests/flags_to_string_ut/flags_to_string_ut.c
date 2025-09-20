// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "flags_to_string_ut_pch.h"

char* sprintf_char_function(const char* format, ...)
{
    char* result;
    va_list va;
    va_start(va, format);
    result = vsprintf_char(format, va);
    va_end(va);
    return result;
}


MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
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

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types failed");

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
    umock_c_negative_tests_init();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

// This ignore is due to gcc treating the STRICT_EXPECTED_CALL as a 
// string function and complaining that it is 'accessing 24 bytes in a region of size 21'
// This is not the case and cannot be fixed without making the test fail
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

/*Tests_SRS_FLAGS_TO_STRING_02_007: [ If following the reset of the known flags, there are no bits set in argument, then FLAGS_TO_STRING(X) shall prepare a string using the format "%s%s...%s", where each pair of %s%s corresponds to a pair of separator_N/value_N; ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_008: [ FLAGS_TO_STRING(X) shall succeed and return a non-NULL string. ]*/
TEST_FUNCTION(FLAGS_TO_STRING_with_0_flags_succeeds)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG));

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(0);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_FLAGS_TO_STRING_02_006: [ If following the reset of the known flags, there are still bits set in argument, then FLAGS_TO_STRING(X) shall prepare a string using the format "%s%s...%sUNKNOWN_FLAG(%#.8" PRIx32 ")", where each pair of %s%s corresponds to a pair of separator_N/value_N; the last %s corresponds to either "" or " | " depeding on argument not containing or containing any known flags; %PRIx32 corresponds to the remaining (unknown) flags; ]*/
TEST_FUNCTION(FLAGS_TO_STRING_with_1_UNKNOWN_flag_succeeds)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s%sUNKNOWN_FLAG(%#.8x)";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG));

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(0x02000000U);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "UNKNOWN_FLAG(0x02000000)", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_FLAGS_TO_STRING_02_001: [ For each of the known flags FLAGS_TO_STRING(X) shall define 2 variables: separator_N and value_N both of them of type const char*. N is a decreasing number (all the way to 1) for every of the predefined flags. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_001: [ For each of the known flags FLAGS_TO_STRING(X) shall define 2 variables: separator_N and value_N both of them of type const char*. N is a decreasing number (all the way to 1) for every of the predefined flags. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_002: [ separator_N shall contain either "" (empty string) or " | ", depending on whether this is the first known flag or not. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_003: [ If the flag is set then value_N shall contain the stringification of the flag. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_004: [ If the flag is not set then value_N shall contain "" (empty string). ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_005: [ FLAGS_TO_STRING(X) shall reset (set to 0) all the used known flags in argument. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_006: [ If following the reset of the known flags, there are still bits set in argument, then FLAGS_TO_STRING(X) shall prepare a string using the format "%s%s...%sUNKNOWN_FLAG(%#.8" PRIx32 ")", where each pair of %s%s corresponds to a pair of separator_N/value_N; the last %s corresponds to either "" or " | " depeding on argument not containing or containing any known flags; %PRIx32 corresponds to the remaining (unknown) flags; ]*/
TEST_FUNCTION(FLAGS_TO_STRING_with_1_known_flag_succeeds)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG));

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(0x40000000 /*CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG*/);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_FLAGS_TO_STRING_02_001: [ For each of the known flags FLAGS_TO_STRING(X) shall define 2 variables: separator_N and value_N both of them of type const char*. N is a decreasing number (all the way to 1) for every of the predefined flags. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_002: [ separator_N shall contain either "" (empty string) or " | ", depending on whether this is the first known flag or not. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_003: [ If the flag is set then value_N shall contain the stringification of the flag. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_004: [ If the flag is not set then value_N shall contain "" (empty string). ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_005: [ FLAGS_TO_STRING(X) shall reset (set to 0) all the used known flags in argument. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_006: [ If following the reset of the known flags, there are still bits set in argument, then FLAGS_TO_STRING(X) shall prepare a string using the format "%s%s...%sUNKNOWN_FLAG(%#.8" PRIx32 ")", where each pair of %s%s corresponds to a pair of separator_N/value_N; the last %s corresponds to either "" or " | " depeding on argument not containing or containing any known flags; %PRIx32 corresponds to the remaining (unknown) flags; ]*/
TEST_FUNCTION(FLAGS_TO_STRING_with_1_known_1_unknown_flag_succeeds)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s%sUNKNOWN_FLAG(%#.8x)";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG));

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(0x40000000 /*CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG*/ | 0x02000000);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG | UNKNOWN_FLAG(0x02000000)", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_FLAGS_TO_STRING_02_001: [ For each of the known flags FLAGS_TO_STRING(X) shall define 2 variables: separator_N and value_N both of them of type const char*. N is a decreasing number (all the way to 1) for every of the predefined flags. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_002: [ separator_N shall contain either "" (empty string) or " | ", depending on whether this is the first known flag or not. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_003: [ If the flag is set then value_N shall contain the stringification of the flag. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_004: [ If the flag is not set then value_N shall contain "" (empty string). ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_005: [ FLAGS_TO_STRING(X) shall reset (set to 0) all the used known flags in argument. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_007: [ If following the reset of the known flags, there are no bits set in argument, then FLAGS_TO_STRING(X) shall prepare a string using the format "%s%s...%s", where each pair of %s%s corresponds to a pair of separator_N/value_N; ]*/
TEST_FUNCTION(FLAGS_TO_STRING_with_2_known_flag_succeeds)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG));

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(0x40000000 /*"CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG"*/ | 0x80000000 /*"CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG"*/);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG | CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_FLAGS_TO_STRING_02_001: [ For each of the known flags FLAGS_TO_STRING(X) shall define 2 variables: separator_N and value_N both of them of type const char*. N is a decreasing number (all the way to 1) for every of the predefined flags. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_002: [ separator_N shall contain either "" (empty string) or " | ", depending on whether this is the first known flag or not. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_003: [ If the flag is set then value_N shall contain the stringification of the flag. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_004: [ If the flag is not set then value_N shall contain "" (empty string). ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_005: [ FLAGS_TO_STRING(X) shall reset (set to 0) all the used known flags in argument. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_007: [ If following the reset of the known flags, there are no bits set in argument, then FLAGS_TO_STRING(X) shall prepare a string using the format "%s%s...%s", where each pair of %s%s corresponds to a pair of separator_N/value_N; ]*/
TEST_FUNCTION(FLAGS_TO_STRING_with_3_known_flag_succeeds)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG));

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(0x40000000 /*"CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG"*/ | 0x80000000 /*"CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG"*/ | 0x00000002  /*"CRYPT_OID_USE_PUBKEY_PARA_FOR_PKCS7_FLAG"*/);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG | CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG | CRYPT_OID_USE_PUBKEY_PARA_FOR_PKCS7_FLAG", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_FLAGS_TO_STRING_02_001: [ For each of the known flags FLAGS_TO_STRING(X) shall define 2 variables: separator_N and value_N both of them of type const char*. N is a decreasing number (all the way to 1) for every of the predefined flags. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_002: [ separator_N shall contain either "" (empty string) or " | ", depending on whether this is the first known flag or not. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_003: [ If the flag is set then value_N shall contain the stringification of the flag. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_004: [ If the flag is not set then value_N shall contain "" (empty string). ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_005: [ FLAGS_TO_STRING(X) shall reset (set to 0) all the used known flags in argument. ]*/
/*Tests_SRS_FLAGS_TO_STRING_02_006: [ If following the reset of the known flags, there are still bits set in argument, then FLAGS_TO_STRING(X) shall prepare a string using the format "%s%s...%sUNKNOWN_FLAG(%#.8" PRIx32 ")", where each pair of %s%s corresponds to a pair of separator_N/value_N; the last %s corresponds to either "" or " | " depeding on argument not containing or containing any known flags; %PRIx32 corresponds to the remaining (unknown) flags; ]*/
TEST_FUNCTION(FLAGS_TO_STRING_with_2_known_flag_1_unknown_succeeds)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s%sUNKNOWN_FLAG(%#.8x)";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG));

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(0x40000000 /*"CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG"*/ | 0x80000000 /*"CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG"*/ | 0x02000000);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG | CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG | UNKNOWN_FLAG(0x02000000)", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_FLAGS_TO_STRING_02_009: [ If there are any failures then FLAGS_TO_STRING(X) shall fails and return NULL. ]*/
TEST_FUNCTION(FLAGS_TO_STRING_with_2_known_flag_1_unknown_fails_when_vsprintf_char_fails)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s%sUNKNOWN_FLAG(%#.8x)";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(0x40000000 /*"CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG"*/ | 0x80000000 /*"CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG"*/ | 0x02000000);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_FLAGS_TO_STRING_02_009: [ If there are any failures then FLAGS_TO_STRING(X) shall fails and return NULL. ]*/
TEST_FUNCTION(FLAGS_TO_STRING_with_2_known_flag_fails_when_vsprintf_char_fails)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(0x40000000 /*"CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG"*/ | 0x80000000 /*"CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG"*/);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)