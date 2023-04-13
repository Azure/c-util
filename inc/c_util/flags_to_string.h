// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef FLAGS_TO_STRING_H
#define FLAGS_TO_STRING_H

#include <inttypes.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/string_utils.h"

/*introduce a name for function that can be called. The name is based on "X", User code might look like FLAGS_TO_STRING(ISC_REQ)(value)*/
#define FLAGS_TO_STRING(X) MU_C2(FLAGS_TO_STRING_,X)

/*this macro declares a function to be used in a header*/
#define FLAGS_TO_STRING_DECLARE_FUNCTION(X) MOCKABLE_FUNCTION(, char*, FLAGS_TO_STRING(X), uint32_t, argument)

/*this macro defined a function to be used in a .c file. ... is a list of pairs of FLAG_VALUE and FLAG_NAME. FLAG_VALUE is a number and FLAG_NAME is a string*/
#define FLAGS_TO_STRING_DEFINE_FUNCTION(X, ...) \
char* FLAGS_TO_STRING(X)(uint32_t argument)                                                                                                                             \
{                                                                                                                                                                       \
    char* result;                                                                                                                                                       \
                                                                                                                                                                        \
    bool firstFlag = true;                                                                                                                                              \
                                                                                                                                                                        \
    /*Codes_SRS_FLAGS_TO_STRING_02_001: [ For each of the known flags FLAGS_TO_STRING(X) shall define 2 variables: separator_N and value_N both of them of type const char*. N is a decreasing number (all the way to 1) for every of the predefined flags. ]*/ \
    /*Codes_SRS_FLAGS_TO_STRING_02_002: [ separator_N shall contain either "" (empty string) or " | ", depending on whether this is the first known flag or not. ]*/    \
    /*Codes_SRS_FLAGS_TO_STRING_02_003: [ If the flag is set then value_N shall contain the stringification of the flag. ]*/                                            \
    /*Codes_SRS_FLAGS_TO_STRING_02_004: [ If the flag is not set then value_N shall contain "" (empty string). ]*/                                                      \
    /*Codes_SRS_FLAGS_TO_STRING_02_005: [ FLAGS_TO_STRING(X) shall reset (set to 0) all the used known flags in argument. ]*/                                           \
    MU_FOR_EACH_2_COUNTED(MAKE_SEPARATOR_AND_VALUE, __VA_ARGS__);                                                                                                       \
                                                                                                                                                                        \
    if (argument != 0)                                                                                                                                                  \
    {                                                                                                                                                                   \
                                                                                                                                                                        \
        /*Codes_SRS_FLAGS_TO_STRING_02_006: [ If following the reset of the known flags, there are still bits set in argument, then FLAGS_TO_STRING(X) shall prepare a string using the format "%s%s...%sUNKNOWN_FLAG(%#.8" PRIx32 ")", where each pair of %s%s corresponds to a pair of separator_N/value_N; the last %s corresponds to either "" or " | " depeding on argument not containing or containing any known flags; %PRIx32 corresponds to the remaining (unknown) flags; ]*/ \
        const char* unknownFlag_separator = firstFlag ? (firstFlag = false, "") : " | ";                                                                                \
                                                                                                                                                                        \
        result = sprintf_char(MU_FOR_EACH_2_COUNTED(MAKE_SEPARATOR_AND_VALUE_FORMAT, __VA_ARGS__) "%sUNKNOWN_FLAG(%#.8" PRIx32 ")",                                     \
            MU_FOR_EACH_2_COUNTED(USE_SEPARATOR_AND_VALUE, __VA_ARGS__),                                                                                                \
            unknownFlag_separator,                                                                                                                                      \
            argument);                                                                                                                                                  \
        if (result == NULL)                                                                                                                                             \
        {                                                                                                                                                               \
            /*Codes_SRS_FLAGS_TO_STRING_02_009: [ If there are any failures then FLAGS_TO_STRING(X) shall fails and return NULL. ]*/                                    \
            LogError("failure in sprintf_char(MU_FOR_EACH_2_COUNTED(MAKE_SEPARATOR_AND_VALUE_FORMAT, __VA_ARGS__) \"%%sUNKNOWN_FLAG(%%#.8\" PRIx32 \")\", MU_FOR_EACH_2_COUNTED(USE_SEPARATOR_AND_VALUE, __VA_ARGS__), unknownFlag_separator=%s, argument=%" PRIu32 ");", \
                unknownFlag_separator, argument);                                                                                                                       \
            /*return as is*/                                                                                                                                            \
        }                                                                                                                                                               \
        else                                                                                                                                                            \
        {                                                                                                                                                               \
            /*Codes_SRS_FLAGS_TO_STRING_02_008: [ FLAGS_TO_STRING(X) shall succeed and return a non-NULL string. ]*/                                                    \
            /*return as is*/                                                                                                                                            \
        }                                                                                                                                                               \
                                                                                                                                                                        \
    }                                                                                                                                                                   \
    else                                                                                                                                                                \
    {                                                                                                                                                                   \
        /*Codes_SRS_FLAGS_TO_STRING_02_007: [ If following the reset of the known flags, there are no bits set in argument, then FLAGS_TO_STRING(X) shall prepare a string using the format "%s%s...%s", where each pair of %s%s corresponds to a pair of separator_N/value_N; ]*/ \
        result = sprintf_char(MU_FOR_EACH_2_COUNTED(MAKE_SEPARATOR_AND_VALUE_FORMAT, __VA_ARGS__),                                                                      \
            MU_FOR_EACH_2_COUNTED(USE_SEPARATOR_AND_VALUE, __VA_ARGS__));                                                                                               \
        if (result == NULL)                                                                                                                                             \
        {                                                                                                                                                               \
            /*Codes_SRS_FLAGS_TO_STRING_02_009: [ If there are any failures then FLAGS_TO_STRING(X) shall fails and return NULL. ]*/                                    \
            LogError("failure in sprintf_char(MU_FOR_EACH_2_COUNTED(MAKE_SEPARATOR_AND_VALUE_FORMAT, __VA_ARGS__), MU_FOR_EACH_2_COUNTED(USE_SEPARATOR_AND_VALUE, __VA_ARGS__));"); \
            /*return as is*/                                                                                                                                            \
        }                                                                                                                                                               \
        else                                                                                                                                                            \
        {                                                                                                                                                               \
            /*Codes_SRS_FLAGS_TO_STRING_02_008: [ FLAGS_TO_STRING(X) shall succeed and return a non-NULL string. ]*/                                                    \
            /*return as is*/                                                                                                                                            \
        }                                                                                                                                                               \
                                                                                                                                                                        \
    }                                                                                                                                                                   \
    return result;                                                                                                                                                      \
}                                                                                                                                                                       \

/*

END OF PUBLIC EXPOSED MACROS

*/

/*internal helper macro - comma operator used to generate separators and value for each of the flags. Note: ?: operator are used here abundantly*/
#define MAKE_SEPARATOR_AND_VALUE(N, FLAG, STRING)                                                                                   \
    const char* MU_C2(separator_, MU_DIV2(N)) = ((argument & FLAG) == FLAG) ? (firstFlag ? (firstFlag = false, "") : " | ") : "";   \
    const char* MU_C2(value_, MU_DIV2(N)) = ((argument & FLAG) == FLAG) ? (argument&=~FLAG, STRING) : "";                           \

/*internal helper macro*/
#define USE_SEPARATOR_AND_VALUE(N, FLAG, STRING)                                                                 \
    MU_C2(separator_, MU_DIV2(N)),                                                                               \
    MU_C2(value_, MU_DIV2(N)) MU_IFCOMMALOGIC(MU_DEC(MU_DEC(N)))                                                 \

/*internal helper macro*/
#define MAKE_SEPARATOR_AND_VALUE_FORMAT(N, FLAG, STRING)                                                         \
    "%s%s"

#endif /*FLAGS_TO_STRING_H*/


