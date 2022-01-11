// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/uuid.h"
#undef ENABLE_MOCKS

#include "c_pal/umocktypes_uuid_t.h"

#include "real_uuid.h" /*the one from c_pal*/
#include "real_gballoc_hl.h"

#include "c_util/uuid.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(UUID_T_FROM_STRING_RESULT, UUID_T_FROM_STRING_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(UUID_T_FROM_STRING_RESULT, UUID_T_FROM_STRING_RESULT_VALUES);

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));

    ASSERT_ARE_EQUAL(int, 0, umocktypes_UUID_T_register_types());

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    REGISTER_TYPE(UUID_T_FROM_STRING_RESULT, UUID_T_FROM_STRING_RESULT);

    REGISTER_UUID_GLOBAL_MOCK_HOOK();
    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(g_testByTest);

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest) != 0)
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    umock_c_negative_tests_deinit();
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_UUID_02_001: [ If uuid_string is NULL then UUID_T_from_string shall fail and return UUID_T_FROM_STRING_RESULT_INVALID_ARG. ]*/
TEST_FUNCTION(UUID_T_from_string_with_uuid_string_NULL_returns_UUID_T_FROM_STRING_RESULT_INVALID_ARG)
{
    ///arrange
    UUID_T_FROM_STRING_RESULT result;
    UUID_T out;

    ///act
    result = UUID_T_from_string(NULL, out);

    ///arrange
    ASSERT_ARE_EQUAL(UUID_T_FROM_STRING_RESULT, UUID_T_FROM_STRING_RESULT_INVALID_ARG, result);
}

/*Tests_SRS_UUID_02_002: [ If uuid is NULL then UUID_T_from_string shall fail and return UUID_T_FROM_STRING_RESULT_INVALID_ARG. ]*/
TEST_FUNCTION(UUID_T_from_string_with_uuid_NULL_returns_UUID_T_FROM_STRING_RESULT_INVALID_ARG)
{
    ///arrange
    UUID_T_FROM_STRING_RESULT result;
    const char* s = "0f102132-4354-6576-8798-a9bacbdcedfe";

    ///act
    result = UUID_T_from_string(s, NULL);

    ///arrange
    ASSERT_ARE_EQUAL(UUID_T_FROM_STRING_RESULT, UUID_T_FROM_STRING_RESULT_INVALID_ARG, result);
}

static bool isHexDigit(char c)
{
    return (('a' <= c) && (c <= 'f')) ||
        (('A' <= c) && (c <= 'F')) ||
        (('0' <= c) && (c <= '9'));
}

/*Tests_SRS_UUID_02_003: [ If any character of uuid_string doesn't match the string representation hhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhh then UUID_T_from_string shall succeed and return UUID_T_FROM_STRING_RESULT_INVALID_DATA. ]*/
TEST_FUNCTION(UUID_T_from_string_with_invalid_characters_fails)
{
    ///arrange
    const char* SOURCE = "0f102132-4354-6576-8798-a9bacbdcedfe"; /*gets modified at every turn*/
    char source[UUID_T_STRING_LENGTH + 1];
    UUID_T destination;

    /*try placing any character except [a-f], [A-F], [0-9] results in a INVALID_DATA*/
    for (int int_replace = 0; int_replace <= UINT8_MAX; int_replace++)
    {
        char replace = (char)int_replace;
        if (isHexDigit(replace))
        {
            /*hex digits can only replace '-', when they replace any other hex digit it would result in a valid UUID_T*/
            for (size_t pos = 0; pos < UUID_T_STRING_LENGTH; pos++) /*trying to replace the character at position "pos" with "replace"*/
            {
                if (SOURCE[pos] == '-')
                {
                    (void)memcpy(source, SOURCE, UUID_T_STRING_LENGTH + 1);
                    source[pos] = replace;

                    ///act + assert all in one!
                    ASSERT_ARE_EQUAL(UUID_T_FROM_STRING_RESULT, UUID_T_FROM_STRING_RESULT_INVALID_DATA, UUID_T_from_string(source, destination));
                }
            }
        }
        else if (replace == '-') /*minus can replace everything, except itself*/
        {
            /*minux can only replace hex digits*/
            for (size_t pos = 0; pos < UUID_T_STRING_LENGTH; pos++) /*trying to replace the character at position "pos" with "replace"*/
            {
                if (SOURCE[pos] != '-')
                {
                    (void)memcpy(source, SOURCE, UUID_T_STRING_LENGTH + 1);

                    source[pos] = replace;

                    ///act + assert all in one!
                    ASSERT_ARE_EQUAL(UUID_T_FROM_STRING_RESULT, UUID_T_FROM_STRING_RESULT_INVALID_DATA, UUID_T_from_string(source, destination));
                }
            }
        }
        else /*any other character is invalid and can replace both '-' and hex digits in source*/
        {
            for (size_t pos = 0; pos < UUID_T_STRING_LENGTH; pos++) /*trying to replace the character at position "pos" with "replace"*/
            {
                (void)memcpy(source, SOURCE, UUID_T_STRING_LENGTH + 1);

                source[pos] = replace;

                ///act + assert all in one!
                ASSERT_ARE_EQUAL(UUID_T_FROM_STRING_RESULT, UUID_T_FROM_STRING_RESULT_INVALID_DATA, UUID_T_from_string(source, destination));
            }
        }
    }
}


/*Tests_SRS_UUID_02_006: [ UUID_T_from_string shall convert the hex digits to the bytes of uuid, succeed and return UUID_T_FROM_STRING_RESULT_OK. ]*/
TEST_FUNCTION(UUID_T_from_string_succeeds)
{
    ///arrange
    const char* s = "0f102132-4354-6576-8798-a9bacbdcedfe";
    UUID_T_FROM_STRING_RESULT result;
    UUID_T destination = { 0 };

    ///act
    result = UUID_T_from_string(s, destination);

    ///assert
    ASSERT_ARE_EQUAL(UUID_T_FROM_STRING_RESULT, UUID_T_FROM_STRING_RESULT_OK, result);
    ASSERT_ARE_EQUAL(uint8_t, 0x0f, destination[0]);
    ASSERT_ARE_EQUAL(uint8_t, 0x10, destination[1]);
    ASSERT_ARE_EQUAL(uint8_t, 0x21, destination[2]);
    ASSERT_ARE_EQUAL(uint8_t, 0x32, destination[3]);
    ASSERT_ARE_EQUAL(uint8_t, 0x43, destination[4]);
    ASSERT_ARE_EQUAL(uint8_t, 0x54, destination[5]);
    ASSERT_ARE_EQUAL(uint8_t, 0x65, destination[6]);
    ASSERT_ARE_EQUAL(uint8_t, 0x76, destination[7]);
    ASSERT_ARE_EQUAL(uint8_t, 0x87, destination[8]);
    ASSERT_ARE_EQUAL(uint8_t, 0x98, destination[9]);
    ASSERT_ARE_EQUAL(uint8_t, 0xA9, destination[10]);
    ASSERT_ARE_EQUAL(uint8_t, 0xBA, destination[11]);
    ASSERT_ARE_EQUAL(uint8_t, 0xCB, destination[12]);
    ASSERT_ARE_EQUAL(uint8_t, 0xDC, destination[13]);
    ASSERT_ARE_EQUAL(uint8_t, 0xED, destination[14]);
    ASSERT_ARE_EQUAL(uint8_t, 0xFE, destination[15]);
}

// Tests_SRS_UUID_09_011: [ If uuid is NULL, UUID_to_string shall return a non-zero value ]
TEST_FUNCTION(UUID_to_string_NULL_uuid)
{
    //arrange

    //act
    char* result = UUID_T_to_string(NULL);

    //assert
    ASSERT_IS_NULL(result);
}

TEST_FUNCTION(UUID_to_string_succeed)
{
    //arrange
    char* result;
    UUID_T source = {
        0x0F,
        0x10,
        0x21,
        0x32,
        0x43,
        0x54,
        0x65,
        0x76,
        0x87,
        0x98,
        0xA9,
        0xBA,
        0xCB,
        0xDC,
        0xED,
        0xFE
    };

    STRICT_EXPECTED_CALL(malloc(UUID_T_STRING_LENGTH + 1));

    //act
    result = UUID_T_to_string(source);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "0f102132-4354-6576-8798-a9bacbdcedfe", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    free(result);
}

TEST_FUNCTION(UUID_to_string_fails_when_sprintf_char_fails)
{
    //arrange
    char* result;
    UUID_T source = {
        0x0F,
        0x10,
        0x21,
        0x32,
        0x43,
        0x54,
        0x65,
        0x76,
        0x87,
        0x98,
        0xA9,
        0xBA,
        0xCB,
        0xDC,
        0xED,
        0xFE
    };

    STRICT_EXPECTED_CALL(malloc(UUID_T_STRING_LENGTH + 1))
        .SetReturn(NULL);

    //act
    result = UUID_T_to_string(source);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
