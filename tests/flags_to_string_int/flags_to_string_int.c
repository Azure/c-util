// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdlib.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"



#include "c_util/flags_to_string.h"

#include "collection_one.h"

static TEST_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(it_does_something)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(f)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }
}

TEST_FUNCTION_CLEANUP(cleans)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

TEST_FUNCTION(collection_one_flag_one)
{
    ///arrange

    ///act
    char* result = FLAGS_TO_STRING(COLLECTION_ONE)(1);
    
    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "FLAG_ONE", result);

    ///clean
    free(result);
}

TEST_FUNCTION(collection_one_flag_two)
{
    ///arrange

    ///act
    char* result = FLAGS_TO_STRING(COLLECTION_ONE)(2);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "FLAG_TWO", result);

    ///clean
    free(result);
}

TEST_FUNCTION(collection_one_flag_one_and_two)
{
    ///arrange

    ///act
    char* result = FLAGS_TO_STRING(COLLECTION_ONE)(1 | 2);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "FLAG_ONE | FLAG_TWO", result);

    ///clean
    free(result);
}

TEST_FUNCTION(collection_one_zero)
{
    ///arrange

    ///act
    char* result = FLAGS_TO_STRING(COLLECTION_ONE)(0);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "", result);

    ///clean
    free(result);
}

TEST_FUNCTION(collection_one_unknown_flag)
{
    ///arrange

    ///act
    char* result = FLAGS_TO_STRING(COLLECTION_ONE)(4);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "UNKNOWN_FLAG(0x00000004)", result);

    ///clean
    free(result);
}

TEST_FUNCTION(collection_one_known_and_unknown_flag)
{
    ///arrange

    ///act
    char* result = FLAGS_TO_STRING(COLLECTION_ONE)(13);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "FLAG_ONE | UNKNOWN_FLAG(0x0000000c)", result);

    ///clean
    free(result);
}

TEST_FUNCTION(collection_one_all_known_and_unknown_flag)
{
    ///arrange

    ///act
    char* result = FLAGS_TO_STRING(COLLECTION_ONE)(15);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, "FLAG_ONE | FLAG_TWO | UNKNOWN_FLAG(0x0000000c)", result);

    ///clean
    free(result);
}


END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

