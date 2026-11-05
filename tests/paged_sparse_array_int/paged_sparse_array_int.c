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

/*
 * RING_ELEMENT - element type used by the producer/consumer ring buffer test with a thread-safe array
 */
typedef struct RING_ELEMENT_TAG
{
    volatile_atomic int64_t published_sequence_plus_one;
} RING_ELEMENT;

PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(RING_ELEMENT);
THANDLE_TYPE_DECLARE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(RING_ELEMENT));
THANDLE_TYPE_DEFINE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(RING_ELEMENT));

PAGED_SPARSE_ARRAY_TYPE_DECLARE(RING_ELEMENT);
PAGED_SPARSE_ARRAY_TYPE_DEFINE(RING_ELEMENT);

#define CHAOS_TEST_ACTION_VALUES \
    CHAOS_TEST_ACTION_ALLOCATE, \
    CHAOS_TEST_ACTION_RELEASE, \
    CHAOS_TEST_ACTION_GET

MU_DEFINE_ENUM(CHAOS_TEST_ACTION, CHAOS_TEST_ACTION_VALUES)
MU_DEFINE_ENUM_STRINGS(CHAOS_TEST_ACTION, CHAOS_TEST_ACTION_VALUES)

TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT_VALUES)

/*
 * Multi-threaded tests for arrays created with thread_safe set to true.
 * Asserts cannot be used from worker threads, so worker threads count failures and the test thread asserts on the count.
 */
#define CONCURRENT_THREAD_COUNT 8
#define CONCURRENT_OPERATIONS_PER_THREAD 20000
#define CONCURRENT_MAX_SIZE 256
#define CONCURRENT_PAGE_SIZE 16
#define CONCURRENT_MAX_HELD_ELEMENTS_PER_THREAD 4

static volatile_atomic int64_t g_dispose_count;

static void count_dispose(TEST_ELEMENT* item)
{
    (void)item;
    (void)interlocked_increment_64(&g_dispose_count);
}

static uint64_t test_next_random(uint64_t* state)
{
    /* 64-bit LCG, each thread has its own state so no locking is needed */
    *state = *state * 6364136223846793005ULL + 1442695040888963407ULL;
    return *state >> 33;
}

typedef struct CONCURRENT_TEST_CONTEXT_TAG
{
    const PAGED_SPARSE_ARRAY_TYPEDEF_NAME(TEST_ELEMENT)* psa;
    bool call_free_page_if_empty;
    volatile_atomic int32_t owners[CONCURRENT_MAX_SIZE];
    volatile_atomic int64_t successful_allocations;
    volatile_atomic int32_t failures;
} CONCURRENT_TEST_CONTEXT;

typedef struct CONCURRENT_THREAD_CONTEXT_TAG
{
    CONCURRENT_TEST_CONTEXT* test_context;
    int32_t thread_id;
} CONCURRENT_THREAD_CONTEXT;

typedef struct HELD_ELEMENT_TAG
{
    uint64_t index;
    TEST_ELEMENT* element;
    int64_t expected_value;
} HELD_ELEMENT;

static void concurrent_record_failure(CONCURRENT_TEST_CONTEXT* test_context)
{
    (void)interlocked_increment(&test_context->failures);
}

static void concurrent_release_held_element(CONCURRENT_TEST_CONTEXT* test_context, int32_t thread_id, const HELD_ELEMENT* held)
{
    /* the element is owned by this thread, so its value must not have been changed and its page must not have been freed */
    if (held->element->value != held->expected_value)
    {
        LogError("thread_id=%" PRId32 ": held element value changed, index=%" PRIu64 ", page_index=%" PRIu64 ", element=%p, value=%" PRId64 ", expected_value=%" PRId64 "",
            thread_id, held->index, held->index / CONCURRENT_PAGE_SIZE, (void*)held->element, held->element->value, held->expected_value);
        concurrent_record_failure(test_context);
    }
    else
    {
        TEST_ELEMENT* element_from_get = NULL;
        PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(test_context->psa, held->index, &element_from_get);
        if (
            (get_result != PAGED_SPARSE_ARRAY_GET_OK) ||
            (element_from_get != held->element)
            )
        {
            LogError("thread_id=%" PRId32 ": GET of a held element did not return the allocated pointer, index=%" PRIu64 ", page_index=%" PRIu64 ", get_result=%" PRI_MU_ENUM ", element_from_get=%p, allocated element=%p",
                thread_id, held->index, held->index / CONCURRENT_PAGE_SIZE, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, get_result), (void*)element_from_get, (void*)held->element);
            concurrent_record_failure(test_context);
        }
        else
        {
            /* do nothing, the element is as expected */
        }
    }

    int32_t owner = interlocked_exchange(&test_context->owners[held->index], 0);
    if (owner != thread_id)
    {
        LogError("thread_id=%" PRId32 ": element owner changed while held, index=%" PRIu64 ", page_index=%" PRIu64 ", owner=%" PRId32 "",
            thread_id, held->index, held->index / CONCURRENT_PAGE_SIZE, owner);
        concurrent_record_failure(test_context);
    }
    else
    {
        /* do nothing, ownership was exclusive */
    }

    PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT)(test_context->psa, held->index);
}

static int concurrent_chaos_thread(void* arg)
{
    CONCURRENT_THREAD_CONTEXT* thread_context = arg;
    CONCURRENT_TEST_CONTEXT* test_context = thread_context->test_context;
    int32_t thread_id = thread_context->thread_id;
    uint64_t random_state = (uint64_t)thread_id;
    HELD_ELEMENT held[CONCURRENT_MAX_HELD_ELEMENTS_PER_THREAD];
    uint32_t held_count = 0;

    for (uint32_t operation = 0; operation < CONCURRENT_OPERATIONS_PER_THREAD; operation++)
    {
        uint64_t random_value = test_next_random(&random_state);
        if (
            (held_count < CONCURRENT_MAX_HELD_ELEMENTS_PER_THREAD) &&
            ((held_count == 0) || ((random_value % 2) == 0))
            )
        {
            uint64_t index = test_next_random(&random_state) % CONCURRENT_MAX_SIZE;
            TEST_ELEMENT* element;
            PAGED_SPARSE_ARRAY_ALLOCATE_RESULT allocate_result = PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(test_context->psa, index, &element);
            if (allocate_result == PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED)
            {
                /* do nothing, the element is owned by another thread (or by this one) */
            }
            else if (allocate_result != PAGED_SPARSE_ARRAY_ALLOCATE_OK)
            {
                LogError("thread_id=%" PRId32 ": ALLOCATE failed, operation=%" PRIu32 ", index=%" PRIu64 ", page_index=%" PRIu64 ", held_count=%" PRIu32 ", allocate_result=%" PRI_MU_ENUM "",
                    thread_id, operation, index, index / CONCURRENT_PAGE_SIZE, held_count, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, allocate_result));
                concurrent_record_failure(test_context);
            }
            else
            {
                (void)interlocked_increment_64(&test_context->successful_allocations);
                int32_t previous_owner = interlocked_compare_exchange(&test_context->owners[index], thread_id, 0);
                if (previous_owner != 0)
                {
                    LogError("thread_id=%" PRId32 ": ALLOCATE succeeded for an element owned by another thread, operation=%" PRIu32 ", index=%" PRIu64 ", page_index=%" PRIu64 ", previous_owner=%" PRId32 ", allocate_result=%" PRI_MU_ENUM "",
                        thread_id, operation, index, index / CONCURRENT_PAGE_SIZE, previous_owner, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, allocate_result));
                    concurrent_record_failure(test_context);
                }
                else
                {
                    /* do nothing, this thread is now the only owner */
                }
                held[held_count].index = index;
                held[held_count].element = element;
                held[held_count].expected_value = ((int64_t)thread_id << 32) | operation;
                element->value = held[held_count].expected_value;
                held_count++;
            }
        }
        else
        {
            uint32_t held_position = (uint32_t)(random_value % held_count);
            concurrent_release_held_element(test_context, thread_id, &held[held_position]);
            held_count--;
            held[held_position] = held[held_count];
        }

        /* elements of other threads are not read, only the result of GET is checked */
        uint64_t other_index = test_next_random(&random_state) % CONCURRENT_MAX_SIZE;
        TEST_ELEMENT* other_element;
        PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(test_context->psa, other_index, &other_element);
        if (
            (get_result != PAGED_SPARSE_ARRAY_GET_OK) &&
            (get_result != PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED)
            )
        {
            LogError("thread_id=%" PRId32 ": GET returned an unexpected result, operation=%" PRIu32 ", index=%" PRIu64 ", page_index=%" PRIu64 ", get_result=%" PRI_MU_ENUM "",
                thread_id, operation, other_index, other_index / CONCURRENT_PAGE_SIZE, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, get_result));
            concurrent_record_failure(test_context);
        }
        else
        {
            /* do nothing, both results are possible */
        }

        if (!test_context->call_free_page_if_empty)
        {
            /* do nothing */
        }
        else
        {
            uint64_t index = test_next_random(&random_state) % CONCURRENT_MAX_SIZE;
            PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT free_result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(TEST_ELEMENT)(test_context->psa, index);
            if (free_result == PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_INVALID_ARGS)
            {
                LogError("thread_id=%" PRId32 ": FREE_PAGE_IF_EMPTY returned an unexpected result, operation=%" PRIu32 ", index=%" PRIu64 ", page_index=%" PRIu64 ", free_result=%" PRI_MU_ENUM "",
                    thread_id, operation, index, index / CONCURRENT_PAGE_SIZE, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, free_result));
                concurrent_record_failure(test_context);
            }
            else
            {
                /* do nothing, FREED, NOT_ALLOCATED and NOT_EMPTY are all possible */
            }
        }
    }

    while (held_count > 0)
    {
        held_count--;
        concurrent_release_held_element(test_context, thread_id, &held[held_count]);
    }

    return 0;
}

static void run_concurrent_chaos_test(bool auto_free_empty_pages, bool call_free_page_if_empty)
{
    //arrange
    PAGED_SPARSE_ARRAY_OPTIONS options;
    options.thread_safe = true;
    options.auto_free_empty_pages = auto_free_empty_pages;
    options.zero_new_pages = false;
    PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(TEST_ELEMENT)(CONCURRENT_MAX_SIZE, CONCURRENT_PAGE_SIZE, count_dispose, &options);
    ASSERT_IS_NOT_NULL(psa);

    CONCURRENT_TEST_CONTEXT* test_context = malloc(sizeof(CONCURRENT_TEST_CONTEXT));
    ASSERT_IS_NOT_NULL(test_context);
    test_context->psa = psa;
    test_context->call_free_page_if_empty = call_free_page_if_empty;
    for (uint32_t i = 0; i < CONCURRENT_MAX_SIZE; i++)
    {
        (void)interlocked_exchange(&test_context->owners[i], 0);
    }
    (void)interlocked_exchange_64(&test_context->successful_allocations, 0);
    (void)interlocked_exchange(&test_context->failures, 0);
    (void)interlocked_exchange_64(&g_dispose_count, 0);

    CONCURRENT_THREAD_CONTEXT thread_contexts[CONCURRENT_THREAD_COUNT];
    THREAD_HANDLE threads[CONCURRENT_THREAD_COUNT];
    int32_t threads_created = 0;
    bool thread_api_failed = false;

    //act
    /* no ASSERT until all the started threads are joined, since they use stack variables of this function */
    for (int32_t i = 0; i < CONCURRENT_THREAD_COUNT; i++)
    {
        thread_contexts[i].test_context = test_context;
        thread_contexts[i].thread_id = i + 1; /*0 means "no owner"*/
        if (ThreadAPI_Create(&threads[i], concurrent_chaos_thread, &thread_contexts[i]) != THREADAPI_OK)
        {
            thread_api_failed = true;
            break;
        }
        else
        {
            threads_created++;
        }
    }
    for (int32_t i = 0; i < threads_created; i++)
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
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&test_context->failures, 0));
    ASSERT_IS_TRUE(interlocked_add_64(&test_context->successful_allocations, 0) > 0);
    ASSERT_ARE_EQUAL(int64_t, interlocked_add_64(&test_context->successful_allocations, 0), interlocked_add_64(&g_dispose_count, 0));

    if (auto_free_empty_pages)
    {
        /* do nothing, every page was freed when its last element was released */
    }
    else
    {
        for (uint64_t page_index = 0; page_index < psa->page_count; page_index++)
        {
            PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT free_result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(TEST_ELEMENT)(psa, page_index * CONCURRENT_PAGE_SIZE);
            ASSERT_ARE_NOT_EQUAL(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_EMPTY, free_result);
            ASSERT_ARE_NOT_EQUAL(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_INVALID_ARGS, free_result);
        }
    }
    for (uint64_t page_index = 0; page_index < psa->page_count; page_index++)
    {
        ASSERT_IS_NULL(psa->pages[page_index]);
    }

    //cleanup
    free(test_context);
    PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

/*
 * Producer/consumer ring buffer over a thread-safe array, the same usage pattern as an ordered writer:
 * producers allocate sequence % RING_MAX_SIZE (at most RING_MAX_SIZE sequences ahead of the tail),
 * a single consumer releases the elements in sequence order and asks the array to free a page when the tail leaves it.
 * A page can then still hold elements of the next lap (allocated by producers), in which case it is not freed.
 */
#define RING_PRODUCER_COUNT 4
#define RING_TOTAL_SEQUENCES 200000
#define RING_MAX_SIZE 2048
#define RING_PAGE_SIZE 256

typedef struct RING_TEST_CONTEXT_TAG
{
    const PAGED_SPARSE_ARRAY_TYPEDEF_NAME(RING_ELEMENT)* psa;
    volatile_atomic int64_t next_sequence;
    volatile_atomic int64_t tail;
    volatile_atomic int32_t failures; /*once not 0, the producers and the consumer stop instead of waiting for each other*/
} RING_TEST_CONTEXT;

static bool ring_has_failed(RING_TEST_CONTEXT* test_context)
{
    return interlocked_add(&test_context->failures, 0) != 0;
}

static int ring_producer_thread(void* arg)
{
    RING_TEST_CONTEXT* test_context = arg;
    int64_t sequence;
    while (
        ((sequence = interlocked_increment_64(&test_context->next_sequence) - 1) < RING_TOTAL_SEQUENCES) &&
        (!ring_has_failed(test_context))
        )
    {
        /* wait until the sequence is within the window */
        while (
            (sequence - interlocked_add_64(&test_context->tail, 0) >= RING_MAX_SIZE) &&
            (!ring_has_failed(test_context))
            )
        {
            ThreadAPI_Sleep(0);
        }

        if (ring_has_failed(test_context))
        {
            /* do nothing, the loop ends */
        }
        else
        {
            RING_ELEMENT* element;
            PAGED_SPARSE_ARRAY_ALLOCATE_RESULT allocate_result = PAGED_SPARSE_ARRAY_ALLOCATE(RING_ELEMENT)(test_context->psa, (uint64_t)sequence % RING_MAX_SIZE, &element);
            if (allocate_result != PAGED_SPARSE_ARRAY_ALLOCATE_OK)
            {
                uint64_t index = (uint64_t)sequence % RING_MAX_SIZE;
                LogError("producer ALLOCATE failed, sequence=%" PRId64 ", index=%" PRIu64 ", page_index=%" PRIu64 ", tail=%" PRId64 ", allocate_result=%" PRI_MU_ENUM "",
                    sequence, index, index / RING_PAGE_SIZE, interlocked_add_64(&test_context->tail, 0), MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, allocate_result));
                (void)interlocked_increment(&test_context->failures);
            }
            else
            {
                (void)interlocked_exchange_64(&element->published_sequence_plus_one, sequence + 1);
            }
        }
    }
    return 0;
}

static void run_ring_buffer_test(void)
{
    //arrange
    PAGED_SPARSE_ARRAY_OPTIONS options;
    options.thread_safe = true;
    options.auto_free_empty_pages = false;
    options.zero_new_pages = true;
    PAGED_SPARSE_ARRAY(RING_ELEMENT) psa = PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(RING_ELEMENT)(RING_MAX_SIZE, RING_PAGE_SIZE, NULL, &options);
    ASSERT_IS_NOT_NULL(psa);

    RING_TEST_CONTEXT test_context;
    test_context.psa = psa;
    (void)interlocked_exchange_64(&test_context.next_sequence, 0);
    (void)interlocked_exchange_64(&test_context.tail, 0);
    (void)interlocked_exchange(&test_context.failures, 0);

    THREAD_HANDLE producers[RING_PRODUCER_COUNT];
    uint32_t producers_created = 0;
    bool thread_api_failed = false;
    uint32_t pages_freed = 0;
    uint32_t pages_kept_not_empty = 0;

    //act
    /* no ASSERT until all the started producers are joined, since they use stack variables of this function */
    for (uint32_t i = 0; i < RING_PRODUCER_COUNT; i++)
    {
        if (ThreadAPI_Create(&producers[i], ring_producer_thread, &test_context) != THREADAPI_OK)
        {
            thread_api_failed = true;
            (void)interlocked_increment(&test_context.failures);
            break;
        }
        else
        {
            producers_created++;
        }
    }

    for (int64_t tail = 0; (tail < RING_TOTAL_SEQUENCES) && (!ring_has_failed(&test_context)); tail++)
    {
        uint64_t index = (uint64_t)tail % RING_MAX_SIZE;
        RING_ELEMENT* element;

        /* wait until a producer allocated the element and published this sequence in it */
        while (
            (!ring_has_failed(&test_context)) &&
            (
                (PAGED_SPARSE_ARRAY_GET(RING_ELEMENT)(psa, index, &element) != PAGED_SPARSE_ARRAY_GET_OK) ||
                (interlocked_add_64(&element->published_sequence_plus_one, 0) != tail + 1)
            )
            )
        {
            ThreadAPI_Sleep(0);
        }

        if (ring_has_failed(&test_context))
        {
            /* do nothing, the loop ends */
        }
        else
        {
            PAGED_SPARSE_ARRAY_RELEASE(RING_ELEMENT)(psa, index);
            (void)interlocked_exchange_64(&test_context.tail, tail + 1);

            if (
                ((index + 1) % RING_PAGE_SIZE != 0) &&
                (index + 1 != RING_MAX_SIZE)
                )
            {
                /* do nothing, the tail did not leave the page */
            }
            else
            {
                PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT free_result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(RING_ELEMENT)(psa, index);
                if (free_result == PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_FREED)
                {
                    pages_freed++;
                }
                else if (free_result == PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_EMPTY)
                {
                    /* producers already allocated elements of the next lap in this page */
                    pages_kept_not_empty++;
                }
                else
                {
                    LogError("consumer got an unexpected FREE_PAGE_IF_EMPTY result, tail=%" PRId64 ", index=%" PRIu64 ", page_index=%" PRIu64 ", free_result=%" PRI_MU_ENUM "",
                        tail, index, index / RING_PAGE_SIZE, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, free_result));
                    (void)interlocked_increment(&test_context.failures);
                }
            }
        }
    }

    for (uint32_t i = 0; i < producers_created; i++)
    {
        int thread_result;
        if (
            (ThreadAPI_Join(producers[i], &thread_result) != THREADAPI_OK) ||
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
    LogInfo("ring buffer test: producers=%" PRIu32 "/%" PRIu32 ", sequences=%" PRIu32 ", consumed=%" PRId64 ", max_size=%" PRIu32 ", page_size=%" PRIu32 ", pages_freed=%" PRIu32 ", pages_kept_not_empty=%" PRIu32 ", failures=%" PRId32 ", thread_api_failed=%" PRI_BOOL "",
        producers_created, (uint32_t)RING_PRODUCER_COUNT, (uint32_t)RING_TOTAL_SEQUENCES, interlocked_add_64(&test_context.tail, 0), (uint32_t)RING_MAX_SIZE, (uint32_t)RING_PAGE_SIZE,
        pages_freed, pages_kept_not_empty, interlocked_add(&test_context.failures, 0), MU_BOOL_VALUE(thread_api_failed));
    ASSERT_IS_FALSE(thread_api_failed);
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&test_context.failures, 0));
    ASSERT_ARE_EQUAL(uint32_t, RING_TOTAL_SEQUENCES / RING_PAGE_SIZE, pages_freed + pages_kept_not_empty);

    /* every element was released, so every page that is still allocated is empty */
    for (uint64_t page_index = 0; page_index < psa->page_count; page_index++)
    {
        PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT free_result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(RING_ELEMENT)(psa, page_index * RING_PAGE_SIZE);
        ASSERT_ARE_NOT_EQUAL(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_EMPTY, free_result);
        ASSERT_IS_NULL(psa->pages[page_index]);
    }

    //cleanup
    PAGED_SPARSE_ARRAY_ASSIGN(RING_ELEMENT)(&psa, NULL);
}

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
 * Tests for arrays created with options
 */

TEST_FUNCTION(PAGED_SPARSE_ARRAY_thread_safe_chaos_with_auto_free_empty_pages)
{
    run_concurrent_chaos_test(true, false);
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_thread_safe_chaos_with_free_page_if_empty)
{
    run_concurrent_chaos_test(false, true);
}

TEST_FUNCTION(PAGED_SPARSE_ARRAY_thread_safe_ring_buffer_with_free_page_if_empty_when_tail_leaves_a_page)
{
    run_ring_buffer_test();
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
