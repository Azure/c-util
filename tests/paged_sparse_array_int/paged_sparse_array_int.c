// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/interlocked.h"
#include "c_pal/thandle.h"
#include "c_pal/threadapi.h"

#include "c_util/paged_sparse_array.h"

typedef struct TEST_ELEMENT_TAG
{
    int64_t value;
} TEST_ELEMENT;

PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(TEST_ELEMENT);
THANDLE_TYPE_DECLARE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(TEST_ELEMENT));
THANDLE_TYPE_DEFINE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(TEST_ELEMENT));

PAGED_SPARSE_ARRAY_TYPE_DECLARE(TEST_ELEMENT);
PAGED_SPARSE_ARRAY_TYPE_DEFINE(TEST_ELEMENT);

#define CHAOS_TEST_ACTION_VALUES \
    CHAOS_TEST_ACTION_ALLOCATE, \
    CHAOS_TEST_ACTION_RELEASE, \
    CHAOS_TEST_ACTION_ALLOCATE_OR_GET, \
    CHAOS_TEST_ACTION_GET

MU_DEFINE_ENUM(CHAOS_TEST_ACTION, CHAOS_TEST_ACTION_VALUES)
MU_DEFINE_ENUM_STRINGS(CHAOS_TEST_ACTION, CHAOS_TEST_ACTION_VALUES)

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    time_t seed = time(NULL);
    LogInfo("Test using random seed=%u", (unsigned int)seed);
    srand((unsigned int)seed);
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

TEST_FUNCTION(PAGED_SPARSE_ARRAY_CREATE_succeeds)
{
    //arrange

    //act
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(1000, 64);

    //assert
    ASSERT_IS_NOT_NULL(psa);

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

#define MAX_SIZE 1000
#define PAGE_SIZE 64

TEST_FUNCTION(PAGED_SPARSE_ARRAY_allocate_all_elements_succeeds)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE);
    ASSERT_IS_NOT_NULL(psa);

    //act
    for (uint32_t i = 0; i < MAX_SIZE; i++)
    {
        TEST_ELEMENT* elem = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, i);
        ASSERT_IS_NOT_NULL(elem);
        elem->value = (int64_t)i;
    }

    //assert
    for (uint32_t i = 0; i < MAX_SIZE; i++)
    {
        TEST_ELEMENT* elem = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(psa, i);
        ASSERT_IS_NOT_NULL(elem);
        ASSERT_ARE_EQUAL(int64_t, (int64_t)i, elem->value);
    }

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_allocate_and_release_all_elements_succeeds)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE);
    ASSERT_IS_NOT_NULL(psa);

    // Allocate all elements
    for (uint32_t i = 0; i < MAX_SIZE; i++)
    {
        TEST_ELEMENT* elem = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, i);
        ASSERT_IS_NOT_NULL(elem);
        elem->value = (int64_t)i;
    }

    //act - release all elements
    for (uint32_t i = 0; i < MAX_SIZE; i++)
    {
        int result = PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT)(psa, i);
        ASSERT_ARE_EQUAL(int, 0, result);
    }

    //assert - all pages should be freed, all gets should fail
    for (uint32_t i = 0; i < MAX_SIZE; i++)
    {
        TEST_ELEMENT* elem = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(psa, i);
        ASSERT_IS_NULL(elem);
    }

    // All pages should be NULL
    for (uint32_t i = 0; i < psa->page_count; i++)
    {
        ASSERT_IS_NULL(psa->pages[i]);
    }

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_sparse_allocation_succeeds)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE);
    ASSERT_IS_NOT_NULL(psa);

    // Allocate only every 100th element (sparse)
    for (uint32_t i = 0; i < MAX_SIZE; i += 100)
    {
        TEST_ELEMENT* elem = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, i);
        ASSERT_IS_NOT_NULL(elem);
        elem->value = (int64_t)i * 10;
    }

    //act & assert - verify only allocated elements are accessible
    for (uint32_t i = 0; i < MAX_SIZE; i++)
    {
        TEST_ELEMENT* elem = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(psa, i);
        if (i % 100 == 0)
        {
            ASSERT_IS_NOT_NULL(elem);
            ASSERT_ARE_EQUAL(int64_t, (int64_t)i * 10, elem->value);
        }
        else
        {
            // Element not allocated - either page is NULL or element not marked as allocated
            // GET should return NULL
            ASSERT_IS_NULL(elem);
        }
    }

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_allocate_already_allocated_fails)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE);
    ASSERT_IS_NOT_NULL(psa);
    TEST_ELEMENT* elem1 = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, 42);
    ASSERT_IS_NOT_NULL(elem1);

    //act
    TEST_ELEMENT* elem2 = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, 42);

    //assert
    ASSERT_IS_NULL(elem2);

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_allocate_or_get_returns_same_element)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE);
    ASSERT_IS_NOT_NULL(psa);
    TEST_ELEMENT* elem1 = PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(TEST_ELEMENT)(psa, 42);
    ASSERT_IS_NOT_NULL(elem1);
    elem1->value = 12345;

    //act
    TEST_ELEMENT* elem2 = PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(TEST_ELEMENT)(psa, 42);

    //assert
    ASSERT_IS_NOT_NULL(elem2);
    ASSERT_ARE_EQUAL(void_ptr, elem1, elem2);
    ASSERT_ARE_EQUAL(int64_t, 12345, elem2->value);

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_release_and_reallocate_succeeds)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE);
    ASSERT_IS_NOT_NULL(psa);
    TEST_ELEMENT* elem1 = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, 42);
    ASSERT_IS_NOT_NULL(elem1);
    elem1->value = 100;

    //act
    int release_result = PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT)(psa, 42);
    ASSERT_ARE_EQUAL(int, 0, release_result);

    TEST_ELEMENT* elem2 = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, 42);

    //assert
    ASSERT_IS_NOT_NULL(elem2);
    // Value should be indeterminate after reallocation, but element should be accessible

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_page_freed_when_all_elements_released)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE);
    ASSERT_IS_NOT_NULL(psa);
    
    // Allocate all elements in first page
    for (uint32_t i = 0; i < PAGE_SIZE; i++)
    {
        TEST_ELEMENT* elem = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, i);
        ASSERT_IS_NOT_NULL(elem);
    }
    ASSERT_IS_NOT_NULL(psa->pages[0]);

    //act - release all elements in first page
    for (uint32_t i = 0; i < PAGE_SIZE; i++)
    {
        int result = PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT)(psa, i);
        ASSERT_ARE_EQUAL(int, 0, result);
    }

    //assert - page should be freed
    ASSERT_IS_NULL(psa->pages[0]);

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

#define N_CHAOS_API_CALLS 10000
TEST_FUNCTION(PAGED_SPARSE_ARRAY_chaos)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE);
    ASSERT_IS_NOT_NULL(psa);

    //act
    uint32_t iteration_count = 0;
    bool element_state[MAX_SIZE] = { false };
    while (iteration_count < N_CHAOS_API_CALLS)
    {
        iteration_count++;
        // perform one of the several actions
        CHAOS_TEST_ACTION action = (CHAOS_TEST_ACTION)((rand() * (MU_ENUM_VALUE_COUNT(CHAOS_TEST_ACTION_VALUES) - 1) / RAND_MAX) + 1);
        uint32_t rand_index = rand() % MAX_SIZE;
        switch (action)
        {
        default:
            LogError("Invalid action: %" PRI_MU_ENUM "", MU_ENUM_VALUE(CHAOS_TEST_ACTION, action));
            break;
        case CHAOS_TEST_ACTION_ALLOCATE:
        {
            TEST_ELEMENT* elem = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, rand_index);
            if (element_state[rand_index])
            {
                // Already allocated, should fail
                ASSERT_IS_NULL(elem);
            }
            else
            {
                // Not allocated, should succeed
                ASSERT_IS_NOT_NULL(elem);
                elem->value = (int64_t)rand_index;
                element_state[rand_index] = true;
            }
            break;
        }
        case CHAOS_TEST_ACTION_RELEASE:
        {
            int result = PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT)(psa, rand_index);
            if (element_state[rand_index])
            {
                // Was allocated, should succeed
                ASSERT_ARE_EQUAL(int, 0, result);
                element_state[rand_index] = false;
            }
            else
            {
                // Was not allocated, should fail
                ASSERT_ARE_NOT_EQUAL(int, 0, result);
            }
            break;
        }
        case CHAOS_TEST_ACTION_ALLOCATE_OR_GET:
        {
            TEST_ELEMENT* elem = PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(TEST_ELEMENT)(psa, rand_index);
            // Should always succeed
            ASSERT_IS_NOT_NULL(elem);
            if (!element_state[rand_index])
            {
                // Was not allocated, now it is
                elem->value = (int64_t)rand_index;
                element_state[rand_index] = true;
            }
            else
            {
                // Was already allocated, verify value
                ASSERT_ARE_EQUAL(int64_t, (int64_t)rand_index, elem->value);
            }
            break;
        }
        case CHAOS_TEST_ACTION_GET:
        {
            TEST_ELEMENT* elem = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(psa, rand_index);
            if (element_state[rand_index])
            {
                // Was allocated, should succeed
                ASSERT_IS_NOT_NULL(elem);
                ASSERT_ARE_EQUAL(int64_t, (int64_t)rand_index, elem->value);
            }
            else
            {
                // Was not allocated, should fail
                ASSERT_IS_NULL(elem);
            }
            break;
        }
        }
    }

    // assert
    // Test verifies that nothing crashes and state remains consistent

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
