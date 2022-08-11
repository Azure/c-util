// Copyright (c) Microsoft. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_windows.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_bool.h"

#define ENABLE_MOCKS
#include "c_pal/string_utils.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#undef ENABLE_MOCKS

#include "real_string_utils.h"
#include "real_gballoc_hl.h"

#include "flags_to_string_helper.h"

static TEST_MUTEX_HANDLE test_serialize_mutex;

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

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_IS_NOT_NULL(test_serialize_mutex = TEST_MUTEX_CREATE());

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_STRING_UTILS_GLOBAL_MOCK_HOOK();

    REGISTER_UMOCK_ALIAS_TYPE(va_list, void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(test_serialize_mutex);

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }

    umock_c_negative_tests_init();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_044: [ If following the reset of the known flags, there are no bits set in argument, then FLAG_to_string shall prepare a string using the format "%s%s%s%s%s%s%s%s%s%s", where each pair of %s%s corresponds to a pair of separator_N/value_N; ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_037: [ FLAG_to_string shall succeed and return a non-NULL string. ]*/
TEST_FUNCTION(FLAG_to_string_with_0_flags_succeeds)
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

/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_043: [ If following the reset of the known flags, there are still bits set in argument, then FLAG_to_string shall prepare a string using the format "%s%s%s%s%s%s%s%s%s%s%sUNKNOWN_FLAG(%#.8" PRIx32 ")", where each pair of %s%s corresponds to a pair of separator_N/value_N; the last %s corresponds to either "" or " | " depeding on argument not containing or containing any known flags; %PRIx32 corresponds to the remaining (unknown) flags; ]*/
TEST_FUNCTION(FLAG_to_string_with_1_UNKNOWN_flag_succeeds)
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

/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_038: [ For each of the known flags FLAG_to_string shall define 2 variables: separator_N and value_N both of them of type const char*. N is a decreasing number (all the way to 1) for every of the predefined flags. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_039: [ separator_N shall contain either "" (empty string) or " | ", depending on whether this is the first known flag or not. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_040: [ If the flag is set then value_N shall contain the stringification of the flag. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_041: [ If the flag is not set then value_N shall contain "" (empty string). ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_042: [ FLAG_to_string shall reset (set to 0) all the used known flags in argument. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_044: [ If following the reset of the known flags, there are no bits set in argument, then FLAG_to_string shall prepare a string using the format "%s%s%s%s%s%s%s%s%s%s", where each pair of %s%s corresponds to a pair of separator_N/value_N; ]*/
TEST_FUNCTION(FLAG_to_string_with_1_known_flag_succeeds)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG));

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}


/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_038: [ For each of the known flags FLAG_to_string shall define 2 variables: separator_N and value_N both of them of type const char*. N is a decreasing number (all the way to 1) for every of the predefined flags. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_039: [ separator_N shall contain either "" (empty string) or " | ", depending on whether this is the first known flag or not. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_040: [ If the flag is set then value_N shall contain the stringification of the flag. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_041: [ If the flag is not set then value_N shall contain "" (empty string). ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_042: [ FLAG_to_string shall reset (set to 0) all the used known flags in argument. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_043: [ If following the reset of the known flags, there are still bits set in argument, then FLAG_to_string shall prepare a string using the format "%s%s%s%s%s%s%s%s%s%s%sUNKNOWN_FLAG(%#.8" PRIx32 ")", where each pair of %s%s corresponds to a pair of separator_N/value_N; the last %s corresponds to either "" or " | " depeding on argument not containing or containing any known flags; %PRIx32 corresponds to the remaining (unknown) flags; ]*/
TEST_FUNCTION(FLAG_to_string_with_1_known_1_unknown_flag_succeeds)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s%sUNKNOWN_FLAG(%#.8x)";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG));

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG | 0x02000000 );

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG | UNKNOWN_FLAG(0x02000000)", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_038: [ For each of the known flags FLAG_to_string shall define 2 variables: separator_N and value_N both of them of type const char*. N is a decreasing number (all the way to 1) for every of the predefined flags. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_039: [ separator_N shall contain either "" (empty string) or " | ", depending on whether this is the first known flag or not. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_040: [ If the flag is set then value_N shall contain the stringification of the flag. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_041: [ If the flag is not set then value_N shall contain "" (empty string). ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_042: [ FLAG_to_string shall reset (set to 0) all the used known flags in argument. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_044: [ If following the reset of the known flags, there are no bits set in argument, then FLAG_to_string shall prepare a string using the format "%s%s%s%s%s%s%s%s%s%s", where each pair of %s%s corresponds to a pair of separator_N/value_N; ]*/
TEST_FUNCTION(FLAG_to_string_with_2_known_flag_succeeds)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG));

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG | CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG | CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_038: [ For each of the known flags FLAG_to_string shall define 2 variables: separator_N and value_N both of them of type const char*. N is a decreasing number (all the way to 1) for every of the predefined flags. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_039: [ separator_N shall contain either "" (empty string) or " | ", depending on whether this is the first known flag or not. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_040: [ If the flag is set then value_N shall contain the stringification of the flag. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_041: [ If the flag is not set then value_N shall contain "" (empty string). ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_042: [ FLAG_to_string shall reset (set to 0) all the used known flags in argument. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_044: [ If following the reset of the known flags, there are no bits set in argument, then FLAG_to_string shall prepare a string using the format "%s%s%s%s%s%s%s%s%s%s", where each pair of %s%s corresponds to a pair of separator_N/value_N; ]*/
TEST_FUNCTION(FLAG_to_string_with_3_known_flag_succeeds)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG));

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG | CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG | CRYPT_OID_USE_PUBKEY_PARA_FOR_PKCS7_FLAG);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG | CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG | CRYPT_OID_USE_PUBKEY_PARA_FOR_PKCS7_FLAG", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_038: [ For each of the known flags FLAG_to_string shall define 2 variables: separator_N and value_N both of them of type const char*. N is a decreasing number (all the way to 1) for every of the predefined flags. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_039: [ separator_N shall contain either "" (empty string) or " | ", depending on whether this is the first known flag or not. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_040: [ If the flag is set then value_N shall contain the stringification of the flag. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_041: [ If the flag is not set then value_N shall contain "" (empty string). ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_042: [ FLAG_to_string shall reset (set to 0) all the used known flags in argument. ]*/
/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_043: [ If following the reset of the known flags, there are still bits set in argument, then FLAG_to_string shall prepare a string using the format "%s%s%s%s%s%s%s%s%s%s%sUNKNOWN_FLAG(%#.8" PRIx32 ")", where each pair of %s%s corresponds to a pair of separator_N/value_N; the last %s corresponds to either "" or " | " depeding on argument not containing or containing any known flags; %PRIx32 corresponds to the remaining (unknown) flags; ]*/
TEST_FUNCTION(FLAG_to_string_with_2_known_flag_1_unknown_succeeds)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s%sUNKNOWN_FLAG(%#.8x)";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG));

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG | CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG | 0x02000000);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG | CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG | UNKNOWN_FLAG(0x02000000)", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    real_gballoc_hl_free(result);
}

/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_045: [ If there are any failures then FLAG_to_string shall fails and return NULL. ]*/
TEST_FUNCTION(FLAG_to_string_with_2_known_flag_1_unknown_fails_when_vsprintf_char_fails)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s%sUNKNOWN_FLAG(%#.8x)";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG | CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG | 0x02000000);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_TLS_SECURITY_SCHANNEL_HELPER_02_045: [ If there are any failures then FLAG_to_string shall fails and return NULL. ]*/
TEST_FUNCTION(FLAG_to_string_with_2_known_flag_fails_when_vsprintf_char_fails)
{
    ///arrange
    char* result;

    const char* format;

    format = "%s%s%s%s%s%s%s%s%s%s";
    STRICT_EXPECTED_CALL(vsprintf_char(format, IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    result = FLAGS_TO_STRING(MYCOLLECTION)(CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG | CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
