// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/uuid.h"
#include "c_pal/string_utils.h"

#include "c_util/uuid_string.h"

MU_DEFINE_ENUM_STRINGS(UUID_FROM_STRING_RESULT, UUID_FROM_STRING_RESULT_VALUES);

/* Codes_SRS_UUID_01_001: [ NIL_UUID shall contain all zeroes. ]*/
const UUID_T NIL_UUID = { 0 };

static bool isHexDigit(char c, uint8_t* value)
{
    int result;
    if (('0' <= c) && (c <= '9'))
    {
        *value = c - '0';
        result = true;
    }
    else
    {
        if (('a' <= c) && (c <= 'f'))
        {
            *value = 0xA + c - 'a';
            result = true;
        }
        else
        {
            if (('A' <= c) && (c <= 'F'))
            {
                *value = 0xA + c - 'A';
                result = true;
            }
            else
            {
                result = false;
            }
        }
    }
    return result;
}

static bool isHexByte(const char* s, uint8_t* value)
{
    bool result;
    size_t pos = 0;
    uint8_t high, low;
    if (!isHexDigit(s[pos], &high))
    {
        result = false;
    }
    else
    {
        pos++;
        if (!isHexDigit(s[pos], &low))
        {
            result = false;
        }
        else
        {
            *value = (high << 4) + low;
            result = true;
        }
    }
    return result;
}

static bool parseHexString(const char* s, uint8_t numbers, unsigned char* destination) /*numbers = how many hex number (0-0xFF) to parse*/
{
    bool result = true;
    size_t i = 0; /*where are we scanning in s*/
    while (
        (i < numbers) &&
        (result == true)
        )
    {
        result = isHexByte(s + i * 2, destination + i);
        i++;
    }
    
    return result;
}

UUID_FROM_STRING_RESULT uuid_from_string(const char* uuid_string, UUID_T uuid) /*uuid_string is not necessarily null terminated*/
{
    UUID_FROM_STRING_RESULT result;

    if (
        /*Codes_SRS_UUID_STRING_02_001: [ If uuid_string is NULL then uuid_from_string shall fail and return UUID_FROM_STRING_RESULT_INVALID_ARG. ]*/
        (uuid_string == NULL) ||
        /*Codes_SRS_UUID_STRING_02_002: [ If uuid is NULL then uuid_from_string shall fail and return UUID_FROM_STRING_RESULT_INVALID_ARG. ]*/
        (uuid == NULL)
        )
    {
        LogError("Invalid argument (uuid_string=%s, uuid=%p)", MU_P_OR_NULL(uuid_string), uuid);
        result = UUID_FROM_STRING_RESULT_INVALID_ARG;
    }
    else
    {
        /*scan until either all characters are converted and deposited into the UUID_T or found a non-expected character (that includes '\0')*/


        /*the below test shows where offsets are in a UUID representation as string*/
        /*             1         2         3        */
        /*   012345678901234567890123456789012345   */
        /*   8C9F1E63-3F22-4AFD-BC7D-8D1B20F968D6   */

        /*Codes_SRS_UUID_STRING_02_003: [ If any character of uuid_string doesn't match the string representation hhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhh then uuid_from_string shall succeed and return UUID_FROM_STRING_RESULT_INVALID_DATA. ]*/
        /*Codes_SRS_UUID_STRING_02_004: [ If any character of uuid_string is 0 instead of a hex digit then uuid_from_string shall succeed and return UUID_FROM_STRING_RESULT_INVALID_DATA.]*/
        /*Codes_SRS_UUID_STRING_02_005: [ If any character of uuid_string is 0 instead of a - then uuid_from_string shall succeed and return UUID_FROM_STRING_RESULT_INVALID_DATA.]*/
        if (
            (!parseHexString(uuid_string + 0, 4, uuid + 0)) ||
            (uuid_string[8] != '-') ||
            (!parseHexString(uuid_string + 9, 2, uuid + 4)) ||
            (uuid_string[13] != '-') ||
            (!parseHexString(uuid_string + 14, 2, uuid + 6)) ||
            (uuid_string[18] != '-') ||
            (!parseHexString(uuid_string + 19, 2, uuid + 8)) ||
            (uuid_string[23] != '-') ||
            (!parseHexString(uuid_string + 24, 6, uuid + 10))
            )
        {
            LogError("const char* uuid_string=%s cannot be parsed at UUID_T", uuid_string);
            result = UUID_FROM_STRING_RESULT_INVALID_DATA;
        }
        else
        {
            /*Codes_SRS_UUID_STRING_02_006: [ uuid_from_string shall convert the hex digits to the bytes of uuid, succeed and return UUID_FROM_STRING_RESULT_OK. ]*/
            result = UUID_FROM_STRING_RESULT_OK;
        }
    }

    return result;
}

char* uuid_to_string(const UUID_T uuid)
{
    char* result;

    /*Codes_SRS_UUID_STRING_02_007: [ If uuid is NULL then uuid_to_string shall fail and return NULL. ]*/
    if (uuid == NULL)
    {
        LogError("Invalid argument (const UUID_T uuid=%" PRI_UUID_T ")", UUID_T_VALUES_OR_NULL(uuid));
        result = NULL;
    }
    else
    {
        /*Codes_SRS_UUID_STRING_02_008: [ uuid_to_string shall output a 0 terminated string in format hhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhh where every h is a nibble of one the bytes in uuid.]*/
        result = sprintf_char("%" PRI_UUID_T "", UUID_T_VALUES(uuid));

        /*Codes_SRS_UUID_STRING_02_009: [ If there are any failures then uuid_to_string shall fail and return NULL. ]*/
        if (result == NULL)
        {
            /*return as is*/
            LogError("failure in sprintf_char(\"%%\" PRI_UUID_T \"\", UUID_T_VALUES(uuid=%" PRI_UUID_T ")", UUID_T_VALUES(uuid));
        }
    }

    return result;
}

