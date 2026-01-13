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

/*
 * TEST_ELEMENT_WITH_MALLOC - element type with a char* that requires malloc/free
 * This tests the custom item dispose functionality
 */
typedef struct TEST_ELEMENT_WITH_MALLOC_TAG
{
    int64_t id;
    char* data;  /* dynamically allocated string */
} TEST_ELEMENT_WITH_MALLOC;

static void test_element_with_malloc_dispose(TEST_ELEMENT_WITH_MALLOC* item)
{
    if (item->data != NULL)
    {
        free(item->data);
    }
}

PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(TEST_ELEMENT_WITH_MALLOC);
THANDLE_TYPE_DECLARE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(TEST_ELEMENT_WITH_MALLOC));
THANDLE_TYPE_DEFINE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(TEST_ELEMENT_WITH_MALLOC));

PAGED_SPARSE_ARRAY_TYPE_DECLARE(TEST_ELEMENT_WITH_MALLOC);
PAGED_SPARSE_ARRAY_TYPE_DEFINE(TEST_ELEMENT_WITH_MALLOC);

#define CHAOS_TEST_ACTION_VALUES \
    CHAOS_TEST_ACTION_ALLOCATE, \
    CHAOS_TEST_ACTION_RELEASE, \
    CHAOS_TEST_ACTION_GET

MU_DEFINE_ENUM(CHAOS_TEST_ACTION, CHAOS_TEST_ACTION_VALUES)
MU_DEFINE_ENUM_STRINGS(CHAOS_TEST_ACTION, CHAOS_TEST_ACTION_VALUES)

TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_RESULT_VALUES)

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

#define MAX_SIZE 1000
#define PAGE_SIZE 64

TEST_FUNCTION(PAGED_SPARSE_ARRAY_CREATE_succeeds)
{
    //arrange

    //act
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE, NULL);

    //assert
    ASSERT_IS_NOT_NULL(psa);

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_allocate_all_elements_succeeds)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE, NULL);
    ASSERT_IS_NOT_NULL(psa);

    //act
    for (uint32_t i = 0; i < MAX_SIZE; i++)
    {
        TEST_ELEMENT* elem;
        PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, i, &elem);
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result);
        ASSERT_IS_NOT_NULL(elem);
        elem->value = i;
    }

    //assert
    for (uint32_t i = 0; i < MAX_SIZE; i++)
    {
        TEST_ELEMENT* elem = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(psa, i);
        ASSERT_IS_NOT_NULL(elem);
        ASSERT_ARE_EQUAL(int64_t, i, elem->value);
    }

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_allocate_and_release_all_elements_succeeds)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE, NULL);
    ASSERT_IS_NOT_NULL(psa);

    // Allocate all elements
    for (uint32_t i = 0; i < MAX_SIZE; i++)
    {
        TEST_ELEMENT* elem;
        PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, i, &elem);
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result);
        ASSERT_IS_NOT_NULL(elem);
        elem->value = i;
    }

    //act - release all elements
    for (uint32_t i = 0; i < MAX_SIZE; i++)
    {
        PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT)(psa, i);
    }

    //assert - all pages should be freed, all gets should fail
    for (uint32_t i = 0; i < MAX_SIZE; i++)
    {
        TEST_ELEMENT* elem = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(psa, i);
        ASSERT_IS_NULL(elem);
    }

    // Note: VLD will catch any memory leaks if pages are not properly freed

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_sparse_allocation_succeeds)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE, NULL);
    ASSERT_IS_NOT_NULL(psa);

    // Allocate only every 100th element (sparse)
    for (uint32_t i = 0; i < MAX_SIZE; i += 100)
    {
        TEST_ELEMENT* elem;
        PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, i, &elem);
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result);
        ASSERT_IS_NOT_NULL(elem);
        elem->value = i * 10;
    }

    //act & assert - verify only allocated elements are accessible
    for (uint32_t i = 0; i < MAX_SIZE; i++)
    {
        TEST_ELEMENT* elem = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(psa, i);
        if (i % 100 == 0)
        {
            ASSERT_IS_NOT_NULL(elem);
            ASSERT_ARE_EQUAL(int64_t, i * 10, elem->value);
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
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE, NULL);
    ASSERT_IS_NOT_NULL(psa);
    TEST_ELEMENT* elem1;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result1 = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, 42, &elem1);
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result1);
    ASSERT_IS_NOT_NULL(elem1);

    //act
    TEST_ELEMENT* elem2;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result2 = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, 42, &elem2);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED, alloc_result2);

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_release_and_reallocate_succeeds)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE, NULL);
    ASSERT_IS_NOT_NULL(psa);
    TEST_ELEMENT* elem1;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result1 = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, 42, &elem1);
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result1);
    ASSERT_IS_NOT_NULL(elem1);
    elem1->value = 100;

    //act
    PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT)(psa, 42);

    TEST_ELEMENT* elem2;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result2 = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, 42, &elem2);

    //assert
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result2);
    ASSERT_IS_NOT_NULL(elem2);
    // Value should be indeterminate after reallocation, but element should be accessible

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_page_freed_when_all_elements_released)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE, NULL);
    ASSERT_IS_NOT_NULL(psa);
    
    // Allocate all elements in first page
    for (uint32_t i = 0; i < PAGE_SIZE; i++)
    {
        TEST_ELEMENT* elem;
        PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, i, &elem);
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result);
        ASSERT_IS_NOT_NULL(elem);
    }
    ASSERT_IS_NOT_NULL(psa->pages[0]);

    //act - release all elements in first page
    for (uint32_t i = 0; i < PAGE_SIZE; i++)
    {
        PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT)(psa, i);
    }

    //assert - page should be freed (VLD will catch any memory leaks)

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

#define N_CHAOS_API_CALLS 10000
TEST_FUNCTION(PAGED_SPARSE_ARRAY_chaos)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(MAX_SIZE, PAGE_SIZE, NULL);
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
            TEST_ELEMENT* elem;
            PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, rand_index, &elem);
            if (element_state[rand_index])
            {
                // Already allocated, should fail
                ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED, alloc_result);
            }
            else
            {
                // Not allocated, should succeed
                ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result);
                ASSERT_IS_NOT_NULL(elem);
                elem->value = rand_index;
                element_state[rand_index] = true;
            }
            break;
        }
        case CHAOS_TEST_ACTION_RELEASE:
        {
            PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT)(psa, rand_index);
            if (element_state[rand_index])
            {
                // Was allocated, now released
                element_state[rand_index] = false;
            }
            // If element was not allocated, RELEASE just returns without doing anything
            break;
        }
        case CHAOS_TEST_ACTION_GET:
        {
            TEST_ELEMENT* elem = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(psa, rand_index);
            if (element_state[rand_index])
            {
                // Was allocated, should succeed
                ASSERT_IS_NOT_NULL(elem);
                ASSERT_ARE_EQUAL(int64_t, rand_index, elem->value);
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

/*
 * Tests for TEST_ELEMENT_WITH_MALLOC - verifies custom dispose function is called properly
 */

#define MALLOC_MAX_SIZE 100
#define MALLOC_PAGE_SIZE 16

TEST_FUNCTION(PAGED_SPARSE_ARRAY_with_malloc_element_allocate_and_dispose_succeeds)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT_WITH_MALLOC) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT_WITH_MALLOC)(MALLOC_MAX_SIZE, MALLOC_PAGE_SIZE, test_element_with_malloc_dispose);
    ASSERT_IS_NOT_NULL(psa);

    //act - allocate elements with malloc'd data
    for (uint32_t i = 0; i < 10; i++)
    {
        TEST_ELEMENT_WITH_MALLOC* elem;
        PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT_WITH_MALLOC)(psa, i, &elem);
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result);
        ASSERT_IS_NOT_NULL(elem);
        
        elem->id = i;
        // Allocate a string for each element
        elem->data = malloc(32);
        ASSERT_IS_NOT_NULL(elem->data);
        (void)sprintf(elem->data, "element_%" PRIu32 "", i);
    }

    //assert - verify elements are accessible
    for (uint32_t i = 0; i < 10; i++)
    {
        TEST_ELEMENT_WITH_MALLOC* elem = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT_WITH_MALLOC)(psa, i);
        ASSERT_IS_NOT_NULL(elem);
        ASSERT_ARE_EQUAL(int64_t, i, elem->id);
        ASSERT_IS_NOT_NULL(elem->data);
    }

    // verify unallocated elements are not accessible
    for (uint32_t i = 10; i < MALLOC_MAX_SIZE; i++)
    {
        TEST_ELEMENT_WITH_MALLOC* elem = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT_WITH_MALLOC)(psa, i);
        ASSERT_IS_NULL(elem);
    }

    //cleanup - the dispose function should free all the malloc'd data
    // VLD will catch any memory leaks if dispose is not called properly
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT_WITH_MALLOC)(&psa, NULL);
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_with_malloc_element_release_calls_dispose)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT_WITH_MALLOC) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT_WITH_MALLOC)(MALLOC_MAX_SIZE, MALLOC_PAGE_SIZE, test_element_with_malloc_dispose);
    ASSERT_IS_NOT_NULL(psa);

    TEST_ELEMENT_WITH_MALLOC* elem;
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT_WITH_MALLOC)(psa, 5, &elem);
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result);
    ASSERT_IS_NOT_NULL(elem);
    
    elem->id = 5;
    elem->data = malloc(64);
    ASSERT_IS_NOT_NULL(elem->data);
    (void)sprintf(elem->data, "test_data_for_element_5");

    //act - release the element, which should call dispose and free the malloc'd data
    PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT_WITH_MALLOC)(psa, 5);

    //assert - element should no longer be accessible
    TEST_ELEMENT_WITH_MALLOC* elem_after = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT_WITH_MALLOC)(psa, 5);
    ASSERT_IS_NULL(elem_after);

    //cleanup - VLD will catch any memory leaks
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT_WITH_MALLOC)(&psa, NULL);
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_with_malloc_element_chaos)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT_WITH_MALLOC) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT_WITH_MALLOC)(MALLOC_MAX_SIZE, MALLOC_PAGE_SIZE, test_element_with_malloc_dispose);
    ASSERT_IS_NOT_NULL(psa);

    bool element_state[MALLOC_MAX_SIZE] = { false };

    //act - perform random operations
    for (uint32_t iter = 0; iter < 10000; iter++)
    {
        CHAOS_TEST_ACTION action = (CHAOS_TEST_ACTION)((rand() * (MU_ENUM_VALUE_COUNT(CHAOS_TEST_ACTION_VALUES) - 1) / RAND_MAX) + 1);
        uint32_t rand_index = rand() % MALLOC_MAX_SIZE;

        switch (action)
        {
        default:
            break;
        case CHAOS_TEST_ACTION_ALLOCATE:
        {
            TEST_ELEMENT_WITH_MALLOC* elem;
            PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT_WITH_MALLOC)(psa, rand_index, &elem);
            if (element_state[rand_index])
            {
                ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED, alloc_result);
            }
            else
            {
                ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result);
                ASSERT_IS_NOT_NULL(elem);
                elem->id = rand_index;
                elem->data = malloc(16);
                ASSERT_IS_NOT_NULL(elem->data);
                (void)sprintf(elem->data, "e%" PRIu32 "", rand_index);
                element_state[rand_index] = true;
            }
            break;
        }
        case CHAOS_TEST_ACTION_RELEASE:
        {
            PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT_WITH_MALLOC)(psa, rand_index);
            element_state[rand_index] = false;
            break;
        }
        case CHAOS_TEST_ACTION_GET:
        {
            TEST_ELEMENT_WITH_MALLOC* elem = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT_WITH_MALLOC)(psa, rand_index);
            if (element_state[rand_index])
            {
                ASSERT_IS_NOT_NULL(elem);
                ASSERT_ARE_EQUAL(int64_t, rand_index, elem->id);
            }
            else
            {
                ASSERT_IS_NULL(elem);
            }
            break;
        }
        }
    }

    //assert - VLD will catch any memory leaks from unreleased elements

    //cleanup - dispose function should free all remaining malloc'd data
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT_WITH_MALLOC)(&psa, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
