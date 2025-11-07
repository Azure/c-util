// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/thandle.h"
#include "c_util/rc_string.h"
#include "c_util/rc_string_array.h"

#include "c_util/rc_string_utils.h"

RC_STRING_ARRAY* rc_string_utils_split_by_char(THANDLE(RC_STRING) str, char delimiter)
{
    RC_STRING_ARRAY* result;

    if (str == NULL)
    {
        /*Codes_SRS_RC_STRING_UTILS_42_001: [ If str is NULL then rc_string_utils_split_by_char shall fail and return NULL. ]*/
        LogError("Invalid argument: THANDLE(RC_STRING) str=%" PRI_RC_STRING ", char=%c",
            RC_STRING_VALUE_OR_NULL(str), delimiter);
        result = NULL;
    }
    else
    {
        size_t str_len = strlen(str->string);
        uint32_t num_splits = 0;

        /*Codes_SRS_RC_STRING_UTILS_42_002: [ rc_string_utils_split_by_char shall count the delimiter characters in str and add 1 to compute the number of resulting sub-strings. ]*/
        for (size_t i = 0; i < str_len; i++)
        {
            if (str->string[i] == delimiter)
            {
                num_splits++;
            }
        }
        num_splits++; // last item

        /*Codes_SRS_RC_STRING_UTILS_42_003: [ rc_string_utils_split_by_char shall call rc_string_array_create with the count of split strings. ]*/
        result = rc_string_array_create(num_splits);

        if (result == NULL)
        {
            /*Codes_SRS_RC_STRING_UTILS_42_010: [ If there are any errors then rc_string_utils_split_by_char shall fail and return NULL. ]*/
            LogError("rc_string_array_create failed");
        }
        else
        {
            if (num_splits == 1)
            {
                /*Codes_SRS_RC_STRING_UTILS_42_011: [ If only 1 sub-string is found (delimiter is not found in str) then rc_string_utils_split_by_char shall call THANDLE_ASSIGN(RC_STRING) to copy str into the allocated array. ]*/
                THANDLE_ASSIGN(RC_STRING)(&result->string_array[0], str);

                /*Codes_SRS_RC_STRING_UTILS_42_009: [ rc_string_utils_split_by_char shall return the allocated array. ]*/
            }
            else
            {
                size_t split_start = 0;
                uint32_t split_index = 0;
                size_t i = 0;

                /*Codes_SRS_RC_STRING_UTILS_42_004: [ For each delimiter found in the string, plus the null-terminator: ]*/
                for (i = 0; i < str_len + 1; i++)
                {
                    if (str->string[i] == delimiter || str->string[i] == '\0')
                    {
                        /*Codes_SRS_RC_STRING_UTILS_42_005: [ rc_string_utils_split_by_char shall allocate memory for the sub-string and a null-terminator. ]*/
                        char* temp_split = malloc(i - split_start + 1);
                        if (temp_split == NULL)
                        {
                            LogError("failed to malloc for split string item %zu",
                                i);
                            break;
                        }

                        /*Codes_SRS_RC_STRING_UTILS_42_006: [ rc_string_utils_split_by_char shall copy the sub-string into the allocated memory. ]*/
                        (void)memcpy(temp_split, str->string + split_start, i - split_start);
                        temp_split[i - split_start] = '\0';

                        /*Codes_SRS_RC_STRING_UTILS_42_007: [ rc_string_utils_split_by_char shall call rc_string_create_with_move_memory. ]*/
                        THANDLE(RC_STRING) split = rc_string_create_with_move_memory(temp_split);

                        if (split == NULL)
                        {
                            /*Codes_SRS_RC_STRING_UTILS_42_010: [ If there are any errors then rc_string_utils_split_by_char shall fail and return NULL. ]*/
                            LogError("rc_string_create_with_move_memory failed");
                            free(temp_split);
                            break;
                        }
                        else
                        {
                            /*Codes_SRS_RC_STRING_UTILS_42_008: [ rc_string_utils_split_by_char shall call THANDLE_MOVE(RC_STRING) to move the allocated string into the allocated array. ]*/
                            THANDLE_MOVE(RC_STRING)(&result->string_array[split_index], &split);
                            split_index++;
                            split_start = i + 1;
                        }
                    }
                }

                if (i < str_len + 1)
                {
                    rc_string_array_destroy(result);
                    result = NULL;
                }
                else
                {
                    /*Codes_SRS_RC_STRING_UTILS_42_009: [ rc_string_utils_split_by_char shall return the allocated array. ]*/
                }
            }
        }
    }

    return result;
}
