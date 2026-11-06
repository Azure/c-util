// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

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

#include "c_util/thread_safe_paged_sparse_array.h"

typedef struct TEST_ELEMENT_TAG
{
    int64_t value;
} TEST_ELEMENT;


/*
 * THREAD_SAFE_PAGED_SPARSE_ARRAY(TEST_ELEMENT) uses the default options (AUTO_FREE_EMPTY_PAGES, DO_NOT_ZERO_NEW_PAGES),
 * THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_EMPTY_PAGES_TEST_ELEMENT) keeps the empty pages until FREE_EMPTY_PAGES is called
 */
THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(TEST_ELEMENT);
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(TEST_ELEMENT));
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(TEST_ELEMENT));

THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(TEST_ELEMENT);
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(TEST_ELEMENT);

typedef TEST_ELEMENT KEEP_EMPTY_PAGES_TEST_ELEMENT;

THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(KEEP_EMPTY_PAGES_TEST_ELEMENT);
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(KEEP_EMPTY_PAGES_TEST_ELEMENT));
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(KEEP_EMPTY_PAGES_TEST_ELEMENT));

THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(KEEP_EMPTY_PAGES_TEST_ELEMENT);
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE_WITH_OPTIONS(KEEP_EMPTY_PAGES_TEST_ELEMENT, KEEP_EMPTY_PAGES, DO_NOT_ZERO_NEW_PAGES);

/*
 * RING_ELEMENT - element type used by the producer/consumer ring buffer test, with the options a ring buffer uses:
 * thread-safe, empty pages are kept until FREE_EMPTY_PAGES is called and new pages are zeroed
 */
typedef struct RING_ELEMENT_TAG
{
    volatile_atomic int64_t published_sequence_plus_one;
} RING_ELEMENT;

THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(RING_ELEMENT);
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(RING_ELEMENT));
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(RING_ELEMENT));

THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(RING_ELEMENT);
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE_WITH_OPTIONS(RING_ELEMENT, KEEP_EMPTY_PAGES, ZERO_NEW_PAGES);

TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT_VALUES)

/*
 * Multi-threaded tests for THREAD_SAFE_PAGED_SPARSE_ARRAY.
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

/*the chaos test runs the same threads on arrays of different types (with different options) through these functions*/
typedef struct CONCURRENT_TEST_ARRAY_FUNCTIONS_TAG
{
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT (*allocate)(const void* psa, uint64_t index, TEST_ELEMENT** element);
    void (*release)(const void* psa, uint64_t index);
    PAGED_SPARSE_ARRAY_GET_RESULT (*get)(const void* psa, uint64_t index, TEST_ELEMENT** element);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT (*free_empty_pages)(const void* psa, uint64_t start_index, uint64_t end_index);
} CONCURRENT_TEST_ARRAY_FUNCTIONS;

#define DEFINE_CONCURRENT_TEST_ARRAY_FUNCTIONS(T)                                                                                                               \
static PAGED_SPARSE_ARRAY_ALLOCATE_RESULT MU_C2(concurrent_allocate_, T)(const void* psa, uint64_t index, TEST_ELEMENT** element)                               \
{                                                                                                                                                               \
    return THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)(psa, index, element);                                                                                     \
}                                                                                                                                                               \
static void MU_C2(concurrent_release_, T)(const void* psa, uint64_t index)                                                                                      \
{                                                                                                                                                               \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)(psa, index);                                                                                                      \
}                                                                                                                                                               \
static PAGED_SPARSE_ARRAY_GET_RESULT MU_C2(concurrent_get_, T)(const void* psa, uint64_t index, TEST_ELEMENT** element)                                         \
{                                                                                                                                                               \
    return THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)(psa, index, element);                                                                                          \
}                                                                                                                                                               \
static THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT MU_C2(concurrent_free_empty_pages_, T)(const void* psa, uint64_t start_index, uint64_t end_index) \
{                                                                                                                                                               \
    return THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T)(psa, start_index, end_index);                                                                     \
}                                                                                                                                                               \
static const CONCURRENT_TEST_ARRAY_FUNCTIONS MU_C2(concurrent_functions_, T) =                                                                                  \
{                                                                                                                                                               \
    MU_C2(concurrent_allocate_, T),                                                                                                                             \
    MU_C2(concurrent_release_, T),                                                                                                                              \
    MU_C2(concurrent_get_, T),                                                                                                                                  \
    MU_C2(concurrent_free_empty_pages_, T)                                                                                                                      \
};

DEFINE_CONCURRENT_TEST_ARRAY_FUNCTIONS(TEST_ELEMENT)
DEFINE_CONCURRENT_TEST_ARRAY_FUNCTIONS(KEEP_EMPTY_PAGES_TEST_ELEMENT)

typedef struct CONCURRENT_TEST_CONTEXT_TAG
{
    const void* psa;
    const CONCURRENT_TEST_ARRAY_FUNCTIONS* functions;
    bool call_free_empty_pages;
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
        PAGED_SPARSE_ARRAY_GET_RESULT get_result = test_context->functions->get(test_context->psa, held->index, &element_from_get);
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

    test_context->functions->release(test_context->psa, held->index);
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
            PAGED_SPARSE_ARRAY_ALLOCATE_RESULT allocate_result = test_context->functions->allocate(test_context->psa, index, &element);
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
        PAGED_SPARSE_ARRAY_GET_RESULT get_result = test_context->functions->get(test_context->psa, other_index, &other_element);
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

        if (!test_context->call_free_empty_pages)
        {
            /* do nothing */
        }
        else
        {
            /* a range of up to 3 pages, which can contain pages that are not allocated, empty, used by other threads or with allocated elements */
            uint64_t start_index = test_next_random(&random_state) % CONCURRENT_MAX_SIZE;
            uint64_t end_index = start_index + test_next_random(&random_state) % (3 * CONCURRENT_PAGE_SIZE);
            if (end_index >= CONCURRENT_MAX_SIZE)
            {
                end_index = CONCURRENT_MAX_SIZE - 1;
            }
            else
            {
                /* do nothing, the range is within the array */
            }
            THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT free_result = test_context->functions->free_empty_pages(test_context->psa, start_index, end_index);
            if (free_result != THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK)
            {
                LogError("thread_id=%" PRId32 ": FREE_EMPTY_PAGES failed, operation=%" PRIu32 ", start_index=%" PRIu64 ", end_index=%" PRIu64 ", first page_index=%" PRIu64 ", last page_index=%" PRIu64 ", free_result=%" PRI_MU_ENUM "",
                    thread_id, operation, start_index, end_index, start_index / CONCURRENT_PAGE_SIZE, end_index / CONCURRENT_PAGE_SIZE, MU_ENUM_VALUE(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, free_result));
                concurrent_record_failure(test_context);
            }
            else
            {
                /* do nothing, the pages of the range that were empty and not used were freed */
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

/*once all the elements are released and all the calls returned, no page is allocated and no page is used*/
static void assert_all_pages_are_freed_and_not_used(const THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT* page_slots, uint64_t page_count)
{
    for (uint64_t page_index = 0; page_index < page_count; page_index++)
    {
        ASSERT_IS_NULL(page_slots[page_index].page, "page_index=%" PRIu64 "", page_index);
        ASSERT_ARE_EQUAL(int64_t, 0, interlocked_add_64((volatile_atomic int64_t*)&page_slots[page_index].use_count, 0), "page_index=%" PRIu64 "", page_index);
    }
}

static void run_concurrent_chaos_test(const void* psa, const CONCURRENT_TEST_ARRAY_FUNCTIONS* functions, bool call_free_empty_pages)
{
    //arrange
    CONCURRENT_TEST_CONTEXT* test_context = malloc(sizeof(CONCURRENT_TEST_CONTEXT));
    ASSERT_IS_NOT_NULL(test_context);
    test_context->psa = psa;
    test_context->functions = functions;
    test_context->call_free_empty_pages = call_free_empty_pages;
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

    //cleanup
    free(test_context);
}

/*
 * Producer/consumer ring buffer over a thread-safe array:
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
    const THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(RING_ELEMENT)* psa;
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
            PAGED_SPARSE_ARRAY_ALLOCATE_RESULT allocate_result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(RING_ELEMENT)(test_context->psa, (uint64_t)sequence % RING_MAX_SIZE, &element);
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
    THREAD_SAFE_PAGED_SPARSE_ARRAY(RING_ELEMENT) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(RING_ELEMENT)(RING_MAX_SIZE, RING_PAGE_SIZE, NULL);
    ASSERT_IS_NOT_NULL(psa);

    RING_TEST_CONTEXT test_context;
    test_context.psa = psa;
    (void)interlocked_exchange_64(&test_context.next_sequence, 0);
    (void)interlocked_exchange_64(&test_context.tail, 0);
    (void)interlocked_exchange(&test_context.failures, 0);

    THREAD_HANDLE producers[RING_PRODUCER_COUNT];
    uint32_t producers_created = 0;
    bool thread_api_failed = false;
    uint32_t free_requests = 0;

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
                (THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(RING_ELEMENT)(psa, index, &element) != PAGED_SPARSE_ARRAY_GET_OK) ||
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
            THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(RING_ELEMENT)(psa, index);
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
                THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT free_result = THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(RING_ELEMENT)(psa, index, index);
                if (free_result != THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK)
                {
                    LogError("consumer FREE_EMPTY_PAGES failed, tail=%" PRId64 ", index=%" PRIu64 ", page_index=%" PRIu64 ", free_result=%" PRI_MU_ENUM "",
                        tail, index, index / RING_PAGE_SIZE, MU_ENUM_VALUE(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, free_result));
                    (void)interlocked_increment(&test_context.failures);
                }
                else
                {
                    /* the page is freed, unless producers already allocated elements of the next lap in it or are using it */
                    free_requests++;
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
    LogInfo("ring buffer test: producers=%" PRIu32 "/%" PRIu32 ", sequences=%" PRIu32 ", consumed=%" PRId64 ", max_size=%" PRIu32 ", page_size=%" PRIu32 ", free_requests=%" PRIu32 ", failures=%" PRId32 ", thread_api_failed=%" PRI_BOOL "",
        producers_created, (uint32_t)RING_PRODUCER_COUNT, (uint32_t)RING_TOTAL_SEQUENCES, interlocked_add_64(&test_context.tail, 0), (uint32_t)RING_MAX_SIZE, (uint32_t)RING_PAGE_SIZE,
        free_requests, interlocked_add(&test_context.failures, 0), MU_BOOL_VALUE(thread_api_failed));
    ASSERT_IS_FALSE(thread_api_failed);
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&test_context.failures, 0));
    ASSERT_ARE_EQUAL(uint32_t, RING_TOTAL_SEQUENCES / RING_PAGE_SIZE, free_requests);

    /* every element was released, so every page that is still allocated is empty */
    ASSERT_ARE_EQUAL(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(RING_ELEMENT)(psa, 0, RING_MAX_SIZE - 1));
    assert_all_pages_are_freed_and_not_used(psa->page_slots, psa->page_count);

    //cleanup
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(RING_ELEMENT)(&psa, NULL);
}

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

/*
 * Tests for THREAD_SAFE_PAGED_SPARSE_ARRAY
 */

TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_chaos_with_auto_free_empty_pages)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(CONCURRENT_MAX_SIZE, CONCURRENT_PAGE_SIZE, count_dispose);
    ASSERT_IS_NOT_NULL(psa);

    //act
    run_concurrent_chaos_test(psa, &concurrent_functions_TEST_ELEMENT, false);

    //assert
    /* every page was freed when its last use ended */
    assert_all_pages_are_freed_and_not_used(psa->page_slots, psa->page_count);

    //cleanup
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_chaos_with_free_empty_pages)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(KEEP_EMPTY_PAGES_TEST_ELEMENT) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(KEEP_EMPTY_PAGES_TEST_ELEMENT)(CONCURRENT_MAX_SIZE, CONCURRENT_PAGE_SIZE, count_dispose);
    ASSERT_IS_NOT_NULL(psa);

    //act
    run_concurrent_chaos_test(psa, &concurrent_functions_KEEP_EMPTY_PAGES_TEST_ELEMENT, true);

    //assert
    /* every element was released, so every page that is still allocated is empty */
    ASSERT_ARE_EQUAL(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(KEEP_EMPTY_PAGES_TEST_ELEMENT)(psa, 0, CONCURRENT_MAX_SIZE - 1));
    assert_all_pages_are_freed_and_not_used(psa->page_slots, psa->page_count);

    //cleanup
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(KEEP_EMPTY_PAGES_TEST_ELEMENT)(&psa, NULL);
}

TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ring_buffer_with_free_empty_pages_when_tail_leaves_a_page)
{
    run_ring_buffer_test();
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
