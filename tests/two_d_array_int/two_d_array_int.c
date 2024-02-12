// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/interlocked.h"
#include "c_pal/thandle.h"
#include "c_pal/threadapi.h"

#include "c_util/two_d_array.h"

typedef struct TEST_THANDLE_TAG
{
    int64_t a;
} TEST_THANDLE;

TWO_D_ARRAY_DEFINE_STRUCT_TYPE(TEST_THANDLE);
THANDLE_TYPE_DECLARE(TWO_D_ARRAY_TYPEDEF_NAME(TEST_THANDLE));
THANDLE_TYPE_DEFINE(TWO_D_ARRAY_TYPEDEF_NAME(TEST_THANDLE));

TWO_D_ARRAY_TYPE_DECLARE(TEST_THANDLE);
TWO_D_ARRAY_TYPE_DEFINE(TEST_THANDLE);

#define CHAOS_TEST_ACTION_VALUES \
    CHAOS_TEST_ACTION_FREE_ROW, \
    CHAOS_TEST_ACTION_ALLOCATE_NEW_ROW, \
    CHAOS_TEST_ACTION_GET_ROW \

MU_DEFINE_ENUM(CHAOS_TEST_ACTION, CHAOS_TEST_ACTION_VALUES)
MU_DEFINE_ENUM_STRINGS(CHAOS_TEST_ACTION, CHAOS_TEST_ACTION_VALUES)

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
}

TEST_FUNCTION_INITIALIZE(method_init)
{
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

TEST_FUNCTION(TWO_D_ARRAY_CREATE_create_succeeds)
{
    //arrange

    //act
    TWO_D_ARRAY(TEST_THANDLE) tdarr = TWO_D_ARRAY_CREATE(TEST_THANDLE)(5, 5);

    //assert
    ASSERT_IS_NOT_NULL(tdarr);

    //cleanup
    TWO_D_ARRAY_ASSIGN(TEST_THANDLE)(&tdarr, NULL);
}

#define ROW_NUM 1000

TEST_FUNCTION(TWO_D_ARRAY_create_and_allocate_succeeds)
{
    //arrange
    TWO_D_ARRAY(TEST_THANDLE) tdarr = TWO_D_ARRAY_CREATE(TEST_THANDLE)(ROW_NUM, 5);
    ASSERT_IS_NOT_NULL(tdarr);

    //act
    for (uint32_t i = 0; i < ROW_NUM; i++)
    {
        TWO_D_ARRAY_ALLOCATE_NEW_ROW(TEST_THANDLE)(tdarr, i);
    }

    //assert
    for (uint32_t i = 0; i < ROW_NUM; i++)
    {
        ASSERT_IS_NOT_NULL(tdarr->row_arrays[i]);
    }

    //cleanup
    TWO_D_ARRAY_ASSIGN(TEST_THANDLE)(&tdarr, NULL);
}

TEST_FUNCTION(TWO_D_ARRAY_allocate_row_and_free_row_succeeds)
{
    //arrange
    TWO_D_ARRAY(TEST_THANDLE) tdarr = TWO_D_ARRAY_CREATE(TEST_THANDLE)(ROW_NUM, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    for (uint32_t i = 0; i < ROW_NUM; i++)
    {
        TWO_D_ARRAY_ALLOCATE_NEW_ROW(TEST_THANDLE)(tdarr, i);
    }

    //act
    for (uint32_t i = 0; i < ROW_NUM; i++)
    {
        TWO_D_ARRAY_FREE_ROW(TEST_THANDLE)(tdarr, i);
    }

    //assert
    for (uint32_t i = 0; i < ROW_NUM; i++)
    {
        ASSERT_IS_NULL(tdarr->row_arrays[i]);
    }

    //cleanup
    TWO_D_ARRAY_ASSIGN(TEST_THANDLE)(&tdarr, NULL);
}

TEST_FUNCTION(TWO_D_ARRAY_allocate_a_row_already_been_allocated)
{
    //arrange
    TWO_D_ARRAY(TEST_THANDLE) tdarr = TWO_D_ARRAY_CREATE(TEST_THANDLE)(ROW_NUM, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    int result;
    result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(TEST_THANDLE)(tdarr, 0);
    ASSERT_ARE_EQUAL(int, result, 0);

    //act
    result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(TEST_THANDLE)(tdarr, 0);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    //cleanup
    TWO_D_ARRAY_ASSIGN(TEST_THANDLE)(&tdarr, NULL);
}

TEST_FUNCTION(TWO_D_ARRAY_GET_ROW_succeeds)
{
    //arrange
    TEST_THANDLE* row_result;
    TWO_D_ARRAY(TEST_THANDLE) tdarr = TWO_D_ARRAY_CREATE(TEST_THANDLE)(5, 5);
    int add_row_result = TWO_D_ARRAY_ALLOCATE_NEW_ROW(TEST_THANDLE)(tdarr, 0);

    ASSERT_IS_NOT_NULL(tdarr);
    ASSERT_ARE_EQUAL(int, add_row_result, 0);
    ASSERT_IS_NOT_NULL(tdarr->row_arrays[0]);
    for (unsigned int i = 0; i < 5; i++)
    {
        tdarr->row_arrays[0][i].a = i;
    }

    //act
    row_result = TWO_D_ARRAY_GET_ROW(TEST_THANDLE)(tdarr, 0);

    //assert
    ASSERT_IS_NOT_NULL(row_result);
    for (unsigned int i = 0; i < 5; i++)
    {
        ASSERT_ARE_EQUAL(int64_t, row_result[i].a, i);
    }

    //clean
    TWO_D_ARRAY_ASSIGN(TEST_THANDLE)(&tdarr, NULL);
}

TEST_FUNCTION(TWO_D_ARRAY_get_a_row_not_allocated_yet)
{
    //arrange
    TEST_THANDLE* row_result;
    TWO_D_ARRAY(TEST_THANDLE) tdarr = TWO_D_ARRAY_CREATE(TEST_THANDLE)(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);

    //act
    row_result = TWO_D_ARRAY_GET_ROW(TEST_THANDLE)(tdarr, 0);

    //assert
    ASSERT_IS_NULL(row_result);

    //clean
    TWO_D_ARRAY_ASSIGN(TEST_THANDLE)(&tdarr, NULL);
}

TEST_FUNCTION(TWO_D_ARRAY_free_a_row_not_allocated_yet)
{
    //arrange
    int result;
    TWO_D_ARRAY(TEST_THANDLE) tdarr = TWO_D_ARRAY_CREATE(TEST_THANDLE)(5, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    for (uint32_t i = 0; i < 5; i++)
    {
        ASSERT_IS_NULL(tdarr->row_arrays[i]);
    }

    //act
    result = TWO_D_ARRAY_FREE_ROW(TEST_THANDLE)(tdarr, 0);

    //assert
    ASSERT_ARE_NOT_EQUAL(int, result, 0);

    //clean
    TWO_D_ARRAY_ASSIGN(TEST_THANDLE)(&tdarr, NULL);
}

#define N_CHAOS_API_CALLS 5000
TEST_FUNCTION(TWO_D_ARRAY_chaos)
{
    //arrange
    TWO_D_ARRAY(TEST_THANDLE) tdarr = TWO_D_ARRAY_CREATE(TEST_THANDLE)(ROW_NUM, 5);
    ASSERT_IS_NOT_NULL(tdarr);
    for (uint32_t i = 0; i < ROW_NUM; i++)
    {
        TWO_D_ARRAY_ALLOCATE_NEW_ROW(TEST_THANDLE)(tdarr, i);
    }

    //action
    uint32_t run_time = 0;
    while (run_time < N_CHAOS_API_CALLS)
    {
        run_time++;
        // perform one of the several actions
        CHAOS_TEST_ACTION action = (CHAOS_TEST_ACTION)((rand() * (MU_ENUM_VALUE_COUNT(CHAOS_TEST_ACTION_VALUES) - 1) / RAND_MAX) + 1);
        uint32_t rand_row_index = rand() % ROW_NUM;
        switch (action)
        {
        default:
            LogError("Invalid action: %" PRI_MU_ENUM "", MU_ENUM_VALUE(CHAOS_TEST_ACTION, action));
            break;
        case CHAOS_TEST_ACTION_FREE_ROW:
        {
            TWO_D_ARRAY_FREE_ROW(TEST_THANDLE)(tdarr, rand_row_index);
        }
        case CHAOS_TEST_ACTION_ALLOCATE_NEW_ROW:
        {
            TWO_D_ARRAY_ALLOCATE_NEW_ROW(TEST_THANDLE)(tdarr, rand_row_index);
        }
        case CHAOS_TEST_ACTION_GET_ROW:
        {
            TWO_D_ARRAY_GET_ROW(TEST_THANDLE)(tdarr, rand_row_index);
        }
        }
    }

    //cleanup
    TWO_D_ARRAY_ASSIGN(TEST_THANDLE)(&tdarr, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
