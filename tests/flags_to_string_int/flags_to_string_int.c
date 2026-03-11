// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/flags_to_string.h"

#include "collection_one.h"
#include "collection_two.h"

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(it_does_something)
{
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
}

TEST_FUNCTION_INITIALIZE(f)
{
}

TEST_FUNCTION_CLEANUP(cleans)
{
}

PARAMETERIZED_TEST_FUNCTION(collection_one_flags_to_string,
    ARGS(uint32_t, flags_value, const char*, expected_string),
    CASE((1, "FLAG_ONE"), flag_one),
    CASE((2, "FLAG_TWO"), flag_two),
    CASE((3, "FLAG_ONE | FLAG_TWO"), flag_one_and_two),
    CASE((0, ""), zero),
    CASE((4, "UNKNOWN_FLAG(0x00000004)"), unknown_flag),
    CASE((13, "FLAG_ONE | UNKNOWN_FLAG(0x0000000c)"), known_and_unknown_flag),
    CASE((15, "FLAG_ONE | FLAG_TWO | UNKNOWN_FLAG(0x0000000c)"), all_known_and_unknown_flag))
{
    ///arrange

    ///act
    char* result = FLAGS_TO_STRING(COLLECTION_ONE)(flags_value);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, expected_string, result);

    ///clean
    free(result);
}

TEST_FUNCTION(collection_two_can_print_all_bits)
{
    ///arrange

    ///act
    char* result = FLAGS_TO_STRING(COLLECTION_BETA)(UINT32_MAX);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "bit_00 | bit_01 | bit_02 | bit_03 | bit_04 | bit_05 | bit_06 | bit_07 | bit_08 | bit_09 | bit_10 | bit_11 | bit_12 | bit_13 | bit_14 | bit_15 | bit_16 | bit_17 | bit_18 | bit_19 | bit_20 | bit_21 | bit_22 | bit_23 | bit_24 | bit_25 | bit_26 | bit_27 | bit_28 | bit_29 | bit_30 | bit_31", result);

    ///clean
    free(result);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

