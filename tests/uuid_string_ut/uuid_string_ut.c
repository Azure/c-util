// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "uuid_string_ut_pch.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(UUID_FROM_STRING_RESULT, UUID_FROM_STRING_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(UUID_FROM_STRING_RESULT, UUID_FROM_STRING_RESULT_VALUES);

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

    REGISTER_TYPE(UUID_FROM_STRING_RESULT, UUID_FROM_STRING_RESULT);

    REGISTER_UUID_GLOBAL_MOCK_HOOK();
    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    umock_c_reset_all_calls();
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    umock_c_negative_tests_deinit();
}

/*Tests_SRS_UUID_STRING_02_001: [ If uuid_string is NULL then uuid_from_string shall fail and return UUID_FROM_STRING_RESULT_INVALID_ARG. ]*/
TEST_FUNCTION(uuid_from_string_with_uuid_string_NULL_returns_UUID_FROM_STRING_RESULT_INVALID_ARG)
{
    ///arrange
    UUID_FROM_STRING_RESULT result;
    UUID_T out;

    ///act
    result = uuid_from_string(NULL, &out);

    ///arrange
    ASSERT_ARE_EQUAL(UUID_FROM_STRING_RESULT, UUID_FROM_STRING_RESULT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_UUID_STRING_02_010: [ If uuid is NULL then uuid_from_string shall fail and return UUID_FROM_STRING_RESULT_INVALID_ARG. ]*/
TEST_FUNCTION(uuid_from_string_with_uuid_NULL_returns_UUID_FROM_STRING_RESULT_INVALID_ARG)
{
    ///arrange
    UUID_FROM_STRING_RESULT result;
    const char* uuid_string = "0f102132-4354-6576-8798-a9bacbdcedfe";

    ///act
    result = uuid_from_string(uuid_string, NULL);

    ///arrange
    ASSERT_ARE_EQUAL(UUID_FROM_STRING_RESULT, UUID_FROM_STRING_RESULT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

static bool isHexDigit(char c)
{
    return (('a' <= c) && (c <= 'f')) ||
        (('A' <= c) && (c <= 'F')) ||
        (('0' <= c) && (c <= '9'));
}

/*Tests_SRS_UUID_STRING_02_003: [ If any character of uuid_string doesn't match the string representation hhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhh then uuid_from_string shall succeed and return UUID_FROM_STRING_RESULT_INVALID_DATA. ]*/
/*Tests_SRS_UUID_STRING_02_004: [ If any character of uuid_string is \0 instead of a hex digit then uuid_from_string shall succeed and return UUID_FROM_STRING_RESULT_INVALID_DATA.]*/ /*note: this happens when int_result is 0*/
/*Tests_SRS_UUID_STRING_02_005: [ If any character of uuid_string is \0 instead of a - then uuid_from_string shall succeed and return UUID_FROM_STRING_RESULT_INVALID_DATA.]*/ /*note: this happens when int_result is 0*/
TEST_FUNCTION(uuid_from_string_with_invalid_characters_fails)
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
                    umock_c_reset_all_calls();
                    source[pos] = replace;

                    ///act + assert all in one!
                    ASSERT_ARE_EQUAL(UUID_FROM_STRING_RESULT, UUID_FROM_STRING_RESULT_INVALID_DATA, uuid_from_string(source, &destination));
                    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
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
                    umock_c_reset_all_calls();

                    source[pos] = replace;

                    ///act + assert all in one!
                    ASSERT_ARE_EQUAL(UUID_FROM_STRING_RESULT, UUID_FROM_STRING_RESULT_INVALID_DATA, uuid_from_string(source, &destination));
                    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
                }
            }
        }
        else /*any other character is invalid and can replace both '-' and hex digits in source*/
        {
            for (size_t pos = 0; pos < UUID_T_STRING_LENGTH; pos++) /*trying to replace the character at position "pos" with "replace"*/
            {
                (void)memcpy(source, SOURCE, UUID_T_STRING_LENGTH + 1);
                umock_c_reset_all_calls();

                source[pos] = replace;

                ///act + assert all in one!
                ASSERT_ARE_EQUAL(UUID_FROM_STRING_RESULT, UUID_FROM_STRING_RESULT_INVALID_DATA, uuid_from_string(source, &destination));
                ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
            }
        }
    }
}

/*Tests_SRS_UUID_STRING_02_006: [ uuid_from_string shall convert the hex digits to the bytes of uuid, succeed and return UUID_FROM_STRING_RESULT_OK. ]*/
TEST_FUNCTION(uuid_from_string_succeeds)
{
    ///arrange
    const char* s = "0f102132-4354-6576-8798-a9bacbdcedfe";
    UUID_FROM_STRING_RESULT result;
    UUID_T destination = { 0 };

    ///act
    result = uuid_from_string(s, &destination);

    ///assert
    ASSERT_ARE_EQUAL(UUID_FROM_STRING_RESULT, UUID_FROM_STRING_RESULT_OK, result);
    ASSERT_ARE_EQUAL(uint8_t, 0x0f, destination.bytes[0]);
    ASSERT_ARE_EQUAL(uint8_t, 0x10, destination.bytes[1]);
    ASSERT_ARE_EQUAL(uint8_t, 0x21, destination.bytes[2]);
    ASSERT_ARE_EQUAL(uint8_t, 0x32, destination.bytes[3]);
    ASSERT_ARE_EQUAL(uint8_t, 0x43, destination.bytes[4]);
    ASSERT_ARE_EQUAL(uint8_t, 0x54, destination.bytes[5]);
    ASSERT_ARE_EQUAL(uint8_t, 0x65, destination.bytes[6]);
    ASSERT_ARE_EQUAL(uint8_t, 0x76, destination.bytes[7]);
    ASSERT_ARE_EQUAL(uint8_t, 0x87, destination.bytes[8]);
    ASSERT_ARE_EQUAL(uint8_t, 0x98, destination.bytes[9]);
    ASSERT_ARE_EQUAL(uint8_t, 0xA9, destination.bytes[10]);
    ASSERT_ARE_EQUAL(uint8_t, 0xBA, destination.bytes[11]);
    ASSERT_ARE_EQUAL(uint8_t, 0xCB, destination.bytes[12]);
    ASSERT_ARE_EQUAL(uint8_t, 0xDC, destination.bytes[13]);
    ASSERT_ARE_EQUAL(uint8_t, 0xED, destination.bytes[14]);
    ASSERT_ARE_EQUAL(uint8_t, 0xFE, destination.bytes[15]);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_UUID_STRING_02_008: [ uuid_to_string shall output a \0 terminated string in format hhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhh where every h is a nibble of one the bytes in uuid.]*/

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
    result = uuid_to_string(source);

    //assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "0f102132-4354-6576-8798-a9bacbdcedfe", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    free(result);
}

/*Tests_SRS_UUID_STRING_02_009: [ If there are any failures then uuid_to_string shall fail and return NULL. ]*/
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
    result = uuid_to_string(source);

    //assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
