// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_logging/logger.h"

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
TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_RESULT_VALUES)

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
    for (uint64_t i = 0; i < MAX_SIZE; i++)
    {
        TEST_ELEMENT* elem;
        PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, i, &elem);
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result);
        ASSERT_IS_NOT_NULL(elem);
        elem->value = i;
    }

    //assert
    for (uint64_t i = 0; i < MAX_SIZE; i++)
    {
        TEST_ELEMENT* elem = NULL;
        PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(psa, i, &elem);
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, get_result);
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
    for (uint64_t i = 0; i < MAX_SIZE; i++)
    {
        TEST_ELEMENT* elem;
        PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, i, &elem);
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result);
        ASSERT_IS_NOT_NULL(elem);
        elem->value = i;
    }

    //act - release all elements
    for (uint64_t i = 0; i < MAX_SIZE; i++)
    {
        PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT)(psa, i);
    }

    //assert - all pages should be freed, all gets should fail
    for (uint64_t i = 0; i < MAX_SIZE; i++)
    {
        TEST_ELEMENT* elem = NULL;
        PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(psa, i, &elem);
        ASSERT_ARE_NOT_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, get_result);
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
    for (uint64_t i = 0; i < MAX_SIZE; i += 100)
    {
        TEST_ELEMENT* elem;
        PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, i, &elem);
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result);
        ASSERT_IS_NOT_NULL(elem);
        elem->value = i * 10;
    }

    //act & assert - verify only allocated elements are accessible
    for (uint64_t i = 0; i < MAX_SIZE; i++)
    {
        TEST_ELEMENT* elem = NULL;
        PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(psa, i, &elem);
        if (i % 100 == 0)
        {
            ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, get_result);
            ASSERT_IS_NOT_NULL(elem);
            ASSERT_ARE_EQUAL(int64_t, i * 10, elem->value);
        }
        else
        {
            // Element not allocated - either page is NULL or element not marked as allocated
            // GET should return NOT_ALLOCATED
            ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, get_result);
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
    for (uint64_t i = 0; i < PAGE_SIZE; i++)
    {
        TEST_ELEMENT* elem;
        PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, i, &elem);
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result);
        ASSERT_IS_NOT_NULL(elem);
    }
    ASSERT_IS_NOT_NULL(psa->pages[0]);

    //act - release all elements in first page
    for (uint64_t i = 0; i < PAGE_SIZE; i++)
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
        uint64_t rand_index = rand() % MAX_SIZE;
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
            TEST_ELEMENT* elem = NULL;
            PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(psa, rand_index, &elem);
            if (element_state[rand_index])
            {
                // Was allocated, should succeed
                ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, get_result);
                ASSERT_IS_NOT_NULL(elem);
                ASSERT_ARE_EQUAL(int64_t, rand_index, elem->value);
            }
            else
            {
                // Was not allocated, should fail
                ASSERT_ARE_NOT_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, get_result);
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
    for (uint64_t i = 0; i < 10; i++)
    {
        TEST_ELEMENT_WITH_MALLOC* elem;
        PAGED_SPARSE_ARRAY_ALLOCATE_RESULT alloc_result = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT_WITH_MALLOC)(psa, i, &elem);
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, alloc_result);
        ASSERT_IS_NOT_NULL(elem);
        
        elem->id = i;
        // Allocate a string for each element
        elem->data = malloc(32);
        ASSERT_IS_NOT_NULL(elem->data);
        (void)sprintf(elem->data, "element_%" PRIu64 "", i);
    }

    //assert - verify elements are accessible
    for (uint64_t i = 0; i < 10; i++)
    {
        TEST_ELEMENT_WITH_MALLOC* elem = NULL;
        PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT_WITH_MALLOC)(psa, i, &elem);
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, get_result);
        ASSERT_IS_NOT_NULL(elem);
        ASSERT_ARE_EQUAL(int64_t, i, elem->id);
        ASSERT_IS_NOT_NULL(elem->data);
    }

    // verify unallocated elements are not accessible
    for (uint64_t i = 10; i < MALLOC_MAX_SIZE; i++)
    {
        TEST_ELEMENT_WITH_MALLOC* elem = NULL;
        PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT_WITH_MALLOC)(psa, i, &elem);
        ASSERT_ARE_NOT_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, get_result);
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
    TEST_ELEMENT_WITH_MALLOC* elem_after = NULL;
    PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT_WITH_MALLOC)(psa, 5, &elem_after);
    ASSERT_ARE_NOT_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, get_result);

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
        uint64_t rand_index = rand() % MALLOC_MAX_SIZE;

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
                (void)sprintf(elem->data, "e%" PRIu64 "", rand_index);
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
            TEST_ELEMENT_WITH_MALLOC* elem = NULL;
            PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT_WITH_MALLOC)(psa, rand_index, &elem);
            if (element_state[rand_index])
            {
                ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, get_result);
                ASSERT_IS_NOT_NULL(elem);
                ASSERT_ARE_EQUAL(int64_t, rand_index, elem->id);
            }
            else
            {
                ASSERT_ARE_NOT_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, get_result);
            }
            break;
        }
        }
    }

    //assert - VLD will catch any memory leaks from unreleased elements

    //cleanup - dispose function should free all remaining malloc'd data
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT_WITH_MALLOC)(&psa, NULL);
}

/*
 * PAGED_SPARSE_ARRAY is not thread-safe, but calls for indexes of different pages may be made concurrently, and calls of GET may be made
 * concurrently also for the same page (see the Threading Model in paged_sparse_array_requirements.md). No lock is used in this test:
 * - page 0 is shared: its elements are allocated before the worker threads start and released after they end, and every worker thread
 *   gets them, so GET is called concurrently for the same page.
 * - the other pages belong to the worker threads: each thread allocates, gets and releases only the elements of its own pages, so its pages
 *   are allocated and freed again and again while the other threads use their own pages. The pages of the threads are interleaved, so the
 *   neighbor pages of a page belong to other threads.
 * Asserts cannot be used from worker threads, so worker threads count failures and the test thread asserts on the count.
 */
#define PAGE_INDEPENDENCE_THREAD_COUNT 8
#define PAGE_INDEPENDENCE_PAGES_PER_THREAD 4
#define PAGE_INDEPENDENCE_PAGE_SIZE 4
#define PAGE_INDEPENDENCE_ELEMENTS_PER_THREAD (PAGE_INDEPENDENCE_PAGES_PER_THREAD * PAGE_INDEPENDENCE_PAGE_SIZE)
#define PAGE_INDEPENDENCE_MAX_SIZE ((1 + PAGE_INDEPENDENCE_THREAD_COUNT * PAGE_INDEPENDENCE_PAGES_PER_THREAD) * PAGE_INDEPENDENCE_PAGE_SIZE)
#define PAGE_INDEPENDENCE_OPERATIONS_PER_THREAD 100000

static volatile_atomic int64_t g_page_independence_dispose_count;

static void page_independence_count_dispose(TEST_ELEMENT* item)
{
    (void)item;
    (void)interlocked_increment_64(&g_page_independence_dispose_count);
}

static uint64_t page_independence_next_random(uint64_t* state)
{
    /* 64-bit LCG, each thread has its own state so no locking is needed */
    *state = *state * 6364136223846793005ULL + 1442695040888963407ULL;
    return *state >> 33;
}

static uint64_t page_independence_index_of_owned_element(uint32_t thread_index, uint32_t owned_element)
{
    /* page 1 belongs to thread 0, page 2 to thread 1, ..., page PAGE_INDEPENDENCE_THREAD_COUNT + 1 to thread 0 again */
    uint64_t page_index = 1 + thread_index + (uint64_t)(owned_element / PAGE_INDEPENDENCE_PAGE_SIZE) * PAGE_INDEPENDENCE_THREAD_COUNT;
    return page_index * PAGE_INDEPENDENCE_PAGE_SIZE + owned_element % PAGE_INDEPENDENCE_PAGE_SIZE;
}

typedef struct PAGE_INDEPENDENCE_TEST_CONTEXT_TAG
{
    const PAGED_SPARSE_ARRAY_TYPEDEF_NAME(TEST_ELEMENT)* psa;
    volatile_atomic int64_t successful_allocations;
    volatile_atomic int32_t failures;
} PAGE_INDEPENDENCE_TEST_CONTEXT;

typedef struct PAGE_INDEPENDENCE_THREAD_CONTEXT_TAG
{
    PAGE_INDEPENDENCE_TEST_CONTEXT* test_context;
    uint32_t thread_index;
} PAGE_INDEPENDENCE_THREAD_CONTEXT;

static int page_independence_thread(void* arg)
{
    PAGE_INDEPENDENCE_THREAD_CONTEXT* thread_context = arg;
    PAGE_INDEPENDENCE_TEST_CONTEXT* test_context = thread_context->test_context;
    uint32_t thread_index = thread_context->thread_index;
    uint64_t random_state = thread_index;
    TEST_ELEMENT* owned_elements[PAGE_INDEPENDENCE_ELEMENTS_PER_THREAD] = { NULL };
    int64_t expected_values[PAGE_INDEPENDENCE_ELEMENTS_PER_THREAD] = { 0 };

    for (uint32_t operation = 0; operation < PAGE_INDEPENDENCE_OPERATIONS_PER_THREAD; operation++)
    {
        uint64_t random_value = page_independence_next_random(&random_state);
        uint32_t owned_element = (uint32_t)(random_value % PAGE_INDEPENDENCE_ELEMENTS_PER_THREAD);
        uint64_t index = page_independence_index_of_owned_element(thread_index, owned_element);
        TEST_ELEMENT* element = NULL;
        PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(test_context->psa, index, &element);
        if (owned_elements[owned_element] == NULL)
        {
            if (get_result != PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED)
            {
                LogError("thread_index=%" PRIu32 ": GET of an element that is not allocated did not return NOT_ALLOCATED, operation=%" PRIu32 ", index=%" PRIu64 ", page_index=%" PRIu64 ", get_result=%" PRI_MU_ENUM ", element=%p",
                    thread_index, operation, index, index / PAGE_INDEPENDENCE_PAGE_SIZE, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, get_result), (void*)element);
                (void)interlocked_increment(&test_context->failures);
            }
            else
            {
                PAGED_SPARSE_ARRAY_ALLOCATE_RESULT allocate_result = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(test_context->psa, index, &element);
                if (allocate_result != PAGED_SPARSE_ARRAY_ALLOCATE_OK)
                {
                    LogError("thread_index=%" PRIu32 ": ALLOCATE failed, operation=%" PRIu32 ", index=%" PRIu64 ", page_index=%" PRIu64 ", allocate_result=%" PRI_MU_ENUM "",
                        thread_index, operation, index, index / PAGE_INDEPENDENCE_PAGE_SIZE, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, allocate_result));
                    (void)interlocked_increment(&test_context->failures);
                }
                else
                {
                    (void)interlocked_increment_64(&test_context->successful_allocations);
                    expected_values[owned_element] = ((int64_t)thread_index << 32) | operation;
                    element->value = expected_values[owned_element];
                    owned_elements[owned_element] = element;
                }
            }
        }
        else
        {
            if (get_result != PAGED_SPARSE_ARRAY_GET_OK)
            {
                LogError("thread_index=%" PRIu32 ": GET of an allocated element failed, operation=%" PRIu32 ", index=%" PRIu64 ", page_index=%" PRIu64 ", get_result=%" PRI_MU_ENUM ", allocated element=%p",
                    thread_index, operation, index, index / PAGE_INDEPENDENCE_PAGE_SIZE, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, get_result), (void*)owned_elements[owned_element]);
                (void)interlocked_increment(&test_context->failures);
            }
            else
            {
                if (
                    (element != owned_elements[owned_element]) ||
                    (element->value != expected_values[owned_element])
                    )
                {
                    LogError("thread_index=%" PRIu32 ": GET of an allocated element did not return it, operation=%" PRIu32 ", index=%" PRIu64 ", page_index=%" PRIu64 ", element=%p, allocated element=%p, value=%" PRId64 ", expected_value=%" PRId64 "",
                        thread_index, operation, index, index / PAGE_INDEPENDENCE_PAGE_SIZE, (void*)element, (void*)owned_elements[owned_element], element->value, expected_values[owned_element]);
                    (void)interlocked_increment(&test_context->failures);
                }
                else
                {
                    /* do nothing, the element is as expected */
                }
            }
            PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT)(test_context->psa, index);
            owned_elements[owned_element] = NULL;
        }

        /* the elements of the shared page are allocated during the whole test and are not changed */
        uint64_t shared_index = (random_value / PAGE_INDEPENDENCE_ELEMENTS_PER_THREAD) % PAGE_INDEPENDENCE_PAGE_SIZE;
        TEST_ELEMENT* shared_element = NULL;
        PAGED_SPARSE_ARRAY_GET_RESULT shared_get_result = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(test_context->psa, shared_index, &shared_element);
        if (shared_get_result != PAGED_SPARSE_ARRAY_GET_OK)
        {
            LogError("thread_index=%" PRIu32 ": GET of an element of the shared page failed, operation=%" PRIu32 ", shared_index=%" PRIu64 ", get_result=%" PRI_MU_ENUM "",
                thread_index, operation, shared_index, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, shared_get_result));
            (void)interlocked_increment(&test_context->failures);
        }
        else
        {
            if (shared_element->value != (int64_t)shared_index)
            {
                LogError("thread_index=%" PRIu32 ": element of the shared page changed, operation=%" PRIu32 ", shared_index=%" PRIu64 ", shared_element=%p, value=%" PRId64 "",
                    thread_index, operation, shared_index, (void*)shared_element, shared_element->value);
                (void)interlocked_increment(&test_context->failures);
            }
            else
            {
                /* do nothing, the element is as expected */
            }
        }
    }

    /* release the remaining elements, so the pages of this thread are freed */
    for (uint32_t owned_element = 0; owned_element < PAGE_INDEPENDENCE_ELEMENTS_PER_THREAD; owned_element++)
    {
        if (owned_elements[owned_element] != NULL)
        {
            PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT)(test_context->psa, page_independence_index_of_owned_element(thread_index, owned_element));
        }
        else
        {
            /* do nothing, the element is not allocated */
        }
    }

    return 0;
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_calls_for_different_pages_and_GET_for_the_same_page_run_concurrently)
{
    //arrange
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(PAGE_INDEPENDENCE_MAX_SIZE, PAGE_INDEPENDENCE_PAGE_SIZE, page_independence_count_dispose);
    ASSERT_IS_NOT_NULL(psa);
    (void)interlocked_exchange_64(&g_page_independence_dispose_count, 0);
    for (uint64_t shared_index = 0; shared_index < PAGE_INDEPENDENCE_PAGE_SIZE; shared_index++)
    {
        TEST_ELEMENT* shared_element;
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, shared_index, &shared_element));
        shared_element->value = (int64_t)shared_index;
    }

    PAGE_INDEPENDENCE_TEST_CONTEXT test_context;
    test_context.psa = psa;
    (void)interlocked_exchange_64(&test_context.successful_allocations, 0);
    (void)interlocked_exchange(&test_context.failures, 0);

    PAGE_INDEPENDENCE_THREAD_CONTEXT thread_contexts[PAGE_INDEPENDENCE_THREAD_COUNT];
    THREAD_HANDLE threads[PAGE_INDEPENDENCE_THREAD_COUNT];
    uint32_t threads_created = 0;
    bool thread_api_failed = false;

    //act
    /* no ASSERT until all the started threads are joined, since they use stack variables of this function */
    for (uint32_t i = 0; i < PAGE_INDEPENDENCE_THREAD_COUNT; i++)
    {
        thread_contexts[i].test_context = &test_context;
        thread_contexts[i].thread_index = i;
        if (ThreadAPI_Create(&threads[i], page_independence_thread, &thread_contexts[i]) != THREADAPI_OK)
        {
            thread_api_failed = true;
            break;
        }
        else
        {
            threads_created++;
        }
    }
    for (uint32_t i = 0; i < threads_created; i++)
    {
        int thread_result;
        if (
            (ThreadAPI_Join(threads[i], &thread_result) != THREADAPI_OK) ||
            (thread_result != 0)
            )
        {
            thread_api_failed = true;
        }
        else
        {
            /* do nothing */
        }
    }

    //assert
    ASSERT_IS_FALSE(thread_api_failed);
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&test_context.failures, 0));
    ASSERT_IS_TRUE(interlocked_add_64(&test_context.successful_allocations, 0) > 0);
    /* every element that a worker thread allocated was released once */
    ASSERT_ARE_EQUAL(int64_t, interlocked_add_64(&test_context.successful_allocations, 0), interlocked_add_64(&g_page_independence_dispose_count, 0));
    for (uint64_t index = PAGE_INDEPENDENCE_PAGE_SIZE; index < PAGE_INDEPENDENCE_MAX_SIZE; index++)
    {
        TEST_ELEMENT* element;
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(psa, index, &element));
    }
    for (uint64_t shared_index = 0; shared_index < PAGE_INDEPENDENCE_PAGE_SIZE; shared_index++)
    {
        TEST_ELEMENT* shared_element;
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(psa, shared_index, &shared_element));
        ASSERT_ARE_EQUAL(int64_t, (int64_t)shared_index, shared_element->value);
    }

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
