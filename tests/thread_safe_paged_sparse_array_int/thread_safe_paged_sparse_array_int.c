// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_logging/logger.h"

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

THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(TEST_ELEMENT);
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(TEST_ELEMENT));
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(TEST_ELEMENT));

THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(TEST_ELEMENT);
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(TEST_ELEMENT);

/*RING_ELEMENT - element type used by the producer/consumer ring buffer test*/
typedef struct RING_ELEMENT_TAG
{
    volatile_atomic int64_t published_sequence_plus_one; /*0 until the producer wrote the element*/
    int64_t sequence;
} RING_ELEMENT;

THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(RING_ELEMENT);
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(RING_ELEMENT));
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(RING_ELEMENT));

THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(RING_ELEMENT);
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(RING_ELEMENT);

TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES)

/*
 * once all the elements are released and all the calls returned, every page is freed. This is checked with the API only: a page that was freed
 * is allocated again with all the bytes of its items set to 0, while a page that was not freed still has the items that the test wrote
 */
#define DEFINE_ASSERT_ALL_PAGES_ARE_FREED(T)                                                                                                                               \
static void MU_C2(assert_all_pages_are_freed_, T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) psa, uint64_t max_size)                                                                \
{                                                                                                                                                                          \
    T zero_item;                                                                                                                                                           \
    (void)memset(&zero_item, 0, sizeof(zero_item));                                                                                                                        \
    for (uint64_t index = 0; index < max_size; index++)                                                                                                                    \
    {                                                                                                                                                                      \
        T* item;                                                                                                                                                           \
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)(psa, index, &item), "index=%" PRIu64 "", index); \
        ASSERT_ARE_EQUAL(int, 0, memcmp(item, &zero_item, sizeof(zero_item)), "index=%" PRIu64 "", index);                                                                 \
    }                                                                                                                                                                      \
    for (uint64_t index = 0; index < max_size; index++)                                                                                                                    \
    {                                                                                                                                                                      \
        THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)(psa, index);                                                                                                             \
    }                                                                                                                                                                      \
}

DEFINE_ASSERT_ALL_PAGES_ARE_FREED(TEST_ELEMENT)
DEFINE_ASSERT_ALL_PAGES_ARE_FREED(RING_ELEMENT)

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

typedef struct CONCURRENT_TEST_CONTEXT_TAG
{
    const THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(TEST_ELEMENT)* psa;
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
        PAGED_SPARSE_ARRAY_GET_RESULT get_result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(test_context->psa, held->index, &element_from_get);
        if (get_result != PAGED_SPARSE_ARRAY_GET_OK)
        {
            LogError("thread_id=%" PRId32 ": GET of a held element failed, index=%" PRIu64 ", page_index=%" PRIu64 ", get_result=%" PRI_MU_ENUM ", allocated element=%p",
                thread_id, held->index, held->index / CONCURRENT_PAGE_SIZE, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, get_result), (void*)held->element);
            concurrent_record_failure(test_context);
        }
        else
        {
            if (element_from_get != held->element)
            {
                LogError("thread_id=%" PRId32 ": GET of a held element did not return the allocated pointer, index=%" PRIu64 ", page_index=%" PRIu64 ", element_from_get=%p, allocated element=%p",
                    thread_id, held->index, held->index / CONCURRENT_PAGE_SIZE, (void*)element_from_get, (void*)held->element);
                concurrent_record_failure(test_context);
            }
            else
            {
                /* do nothing, the element is as expected */
            }
            /* END_GET is called before RELEASE: no other call of the array is made while GET holds the page */
            THREAD_SAFE_PAGED_SPARSE_ARRAY_END_GET(TEST_ELEMENT)(test_context->psa, held->index);
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

    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT)(test_context->psa, held->index);
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
            PAGED_SPARSE_ARRAY_ALLOCATE_RESULT allocate_result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(test_context->psa, index, &element);
            if (allocate_result == PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED)
            {
                /* do nothing, the element is owned by another thread (or by this one) */
            }
            else
            {
                if (allocate_result != PAGED_SPARSE_ARRAY_ALLOCATE_OK)
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
        PAGED_SPARSE_ARRAY_GET_RESULT get_result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(test_context->psa, other_index, &other_element);
        if (get_result == PAGED_SPARSE_ARRAY_GET_OK)
        {
            THREAD_SAFE_PAGED_SPARSE_ARRAY_END_GET(TEST_ELEMENT)(test_context->psa, other_index);
        }
        else
        {
            if (get_result != PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED)
            {
                LogError("thread_id=%" PRId32 ": GET returned an unexpected result, operation=%" PRIu32 ", index=%" PRIu64 ", page_index=%" PRIu64 ", get_result=%" PRI_MU_ENUM "",
                    thread_id, operation, other_index, other_index / CONCURRENT_PAGE_SIZE, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, get_result));
                concurrent_record_failure(test_context);
            }
            else
            {
                /* do nothing, the element is not allocated */
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

/*
 * Producer/consumer ring buffer over a thread safe paged sparse array:
 * producers allocate sequence % RING_MAX_SIZE (less than RING_WINDOW sequences ahead of the tail), write the sequence in the element
 * and then publish it in the element. A single consumer polls the element of the tail sequence with GET until it is published, the way
 * the EBS writer polls the slot of its head sequence, and releases the elements in sequence order.
 * The published sequence of an element that the producer did not write yet is 0: the items of a new page are set to 0, and the item dispose
 * function sets it back to 0 when the element is released, because an element that is allocated again keeps its item.
 * RING_WINDOW keeps a page between the producers and the tail, so the page that the tail leaves has no element of the next lap: it is freed
 * when its last element is released and it is allocated again in the next lap.
 */
#define RING_PRODUCER_COUNT 4
#define RING_TOTAL_SEQUENCES 200000
#define RING_MAX_SIZE 2048
#define RING_PAGE_SIZE 256
#define RING_WINDOW (RING_MAX_SIZE - RING_PAGE_SIZE)

typedef struct RING_TEST_CONTEXT_TAG
{
    const THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(RING_ELEMENT)* psa;
    volatile_atomic int64_t next_sequence;
    volatile_atomic int64_t tail;
    volatile_atomic int32_t failures; /*once not 0, the producers and the consumer stop instead of waiting for each other*/
} RING_TEST_CONTEXT;

static void ring_element_dispose(RING_ELEMENT* element)
{
    (void)interlocked_exchange_64(&element->published_sequence_plus_one, 0);
}

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
            (sequence - interlocked_add_64(&test_context->tail, 0) >= RING_WINDOW) &&
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
            uint64_t index = (uint64_t)sequence % RING_MAX_SIZE;
            RING_ELEMENT* element;
            PAGED_SPARSE_ARRAY_ALLOCATE_RESULT allocate_result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(RING_ELEMENT)(test_context->psa, index, &element);
            if (allocate_result != PAGED_SPARSE_ARRAY_ALLOCATE_OK)
            {
                LogError("producer ALLOCATE failed, sequence=%" PRId64 ", index=%" PRIu64 ", page_index=%" PRIu64 ", tail=%" PRId64 ", allocate_result=%" PRI_MU_ENUM "",
                    sequence, index, index / RING_PAGE_SIZE, interlocked_add_64(&test_context->tail, 0), MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, allocate_result));
                (void)interlocked_increment(&test_context->failures);
            }
            else
            {
                element->sequence = sequence;
                (void)interlocked_exchange_64(&element->published_sequence_plus_one, sequence + 1);
            }
        }
    }
    return 0;
}

static void run_ring_buffer_test(void)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(RING_ELEMENT) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(RING_ELEMENT)(RING_MAX_SIZE, RING_PAGE_SIZE, ring_element_dispose);
    ASSERT_IS_NOT_NULL(psa);

    RING_TEST_CONTEXT* test_context = malloc(sizeof(RING_TEST_CONTEXT));
    ASSERT_IS_NOT_NULL(test_context);
    test_context->psa = psa;
    (void)interlocked_exchange_64(&test_context->next_sequence, 0);
    (void)interlocked_exchange_64(&test_context->tail, 0);
    (void)interlocked_exchange(&test_context->failures, 0);

    THREAD_HANDLE producers[RING_PRODUCER_COUNT];
    uint32_t producers_created = 0;
    bool thread_api_failed = false;

    //act
    for (uint32_t i = 0; i < RING_PRODUCER_COUNT; i++)
    {
        if (ThreadAPI_Create(&producers[i], ring_producer_thread, test_context) != THREADAPI_OK)
        {
            thread_api_failed = true;
            (void)interlocked_increment(&test_context->failures);
            break;
        }
        else
        {
            producers_created++;
        }
    }

    for (int64_t tail = 0; (tail < RING_TOTAL_SEQUENCES) && (!ring_has_failed(test_context)); tail++)
    {
        uint64_t index = (uint64_t)tail % RING_MAX_SIZE;
        bool published = false;

        /* poll the element of the tail sequence until its producer allocated it and published this sequence */
        while (
            (!published) &&
            (!ring_has_failed(test_context))
            )
        {
            RING_ELEMENT* element = NULL;
            PAGED_SPARSE_ARRAY_GET_RESULT get_result = THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(RING_ELEMENT)(psa, index, &element);
            if (get_result == PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED)
            {
                /* the producer did not allocate the element yet */
                ThreadAPI_Sleep(0);
            }
            else
            {
                if (get_result != PAGED_SPARSE_ARRAY_GET_OK)
                {
                    LogError("consumer GET failed, tail=%" PRId64 ", index=%" PRIu64 ", page_index=%" PRIu64 ", get_result=%" PRI_MU_ENUM "",
                        tail, index, index / RING_PAGE_SIZE, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, get_result));
                    (void)interlocked_increment(&test_context->failures);
                }
                else
                {
                    int64_t published_sequence_plus_one = interlocked_add_64(&element->published_sequence_plus_one, 0);
                    int64_t element_sequence = element->sequence;
                    /* END_GET is called before RELEASE: no other call of the array is made while GET holds the page */
                    THREAD_SAFE_PAGED_SPARSE_ARRAY_END_GET(RING_ELEMENT)(psa, index);
                    if (published_sequence_plus_one == 0)
                    {
                        /* the producer allocated the element but did not publish it yet */
                        ThreadAPI_Sleep(0);
                    }
                    else
                    {
                        if (
                            (published_sequence_plus_one != tail + 1) ||
                            (element_sequence != tail)
                            )
                        {
                            LogError("consumer got an element that is not the one of the tail sequence, tail=%" PRId64 ", index=%" PRIu64 ", page_index=%" PRIu64 ", published_sequence_plus_one=%" PRId64 ", element sequence=%" PRId64 "",
                                tail, index, index / RING_PAGE_SIZE, published_sequence_plus_one, element_sequence);
                            (void)interlocked_increment(&test_context->failures);
                        }
                        else
                        {
                            published = true;
                        }
                    }
                }
            }
        }

        if (!published)
        {
            /* do nothing, the test failed and the loop ends */
        }
        else
        {
            THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(RING_ELEMENT)(psa, index);
            (void)interlocked_exchange_64(&test_context->tail, tail + 1);
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
    LogInfo("ring buffer test: producers=%" PRIu32 "/%" PRIu32 ", sequences=%" PRIu32 ", consumed=%" PRId64 ", max_size=%" PRIu32 ", page_size=%" PRIu32 ", failures=%" PRId32 ", thread_api_failed=%" PRI_BOOL "",
        producers_created, (uint32_t)RING_PRODUCER_COUNT, (uint32_t)RING_TOTAL_SEQUENCES, interlocked_add_64(&test_context->tail, 0), (uint32_t)RING_MAX_SIZE, (uint32_t)RING_PAGE_SIZE,
        interlocked_add(&test_context->failures, 0), MU_BOOL_VALUE(thread_api_failed));
    ASSERT_IS_FALSE(thread_api_failed);
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&test_context->failures, 0));
    ASSERT_ARE_EQUAL(int64_t, RING_TOTAL_SEQUENCES, interlocked_add_64(&test_context->tail, 0));
    /* every element was released, so every page was freed when its last element was released */
    assert_all_pages_are_freed_RING_ELEMENT(psa, RING_MAX_SIZE);

    //cleanup
    free(test_context);
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

TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_chaos_allocate_release_and_get_from_multiple_threads)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(CONCURRENT_MAX_SIZE, CONCURRENT_PAGE_SIZE, count_dispose);
    ASSERT_IS_NOT_NULL(psa);

    CONCURRENT_TEST_CONTEXT* test_context = malloc(sizeof(CONCURRENT_TEST_CONTEXT));
    ASSERT_IS_NOT_NULL(test_context);
    test_context->psa = psa;
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
    /* every element was released, so every page was freed when its last element was released */
    assert_all_pages_are_freed_TEST_ELEMENT(psa, CONCURRENT_MAX_SIZE);

    //cleanup
    free(test_context);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ring_buffer_with_multiple_producers_and_one_consumer)
{
    run_ring_buffer_test();
}

/*
 * A call that got an element with GET holds the page of the element until END_GET: the RELEASE of the last element of the page
 * by another thread does not free the page (and does not return) before END_GET.
 */
#define HELD_PAGE_INDEX 3
#define HELD_PAGE_RELEASE_WAIT_MS 200

typedef struct HELD_PAGE_RELEASE_CONTEXT_TAG
{
    const THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(TEST_ELEMENT)* psa;
    volatile_atomic int32_t release_returned;
} HELD_PAGE_RELEASE_CONTEXT;

static int held_page_release_thread(void* arg)
{
    HELD_PAGE_RELEASE_CONTEXT* release_context = arg;
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(TEST_ELEMENT)(release_context->psa, HELD_PAGE_INDEX);
    (void)interlocked_exchange(&release_context->release_returned, 1);
    return 0;
}

TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_of_the_last_element_of_a_page_does_not_free_the_page_before_END_GET)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(TEST_ELEMENT) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(TEST_ELEMENT)(CONCURRENT_MAX_SIZE, CONCURRENT_PAGE_SIZE, NULL);
    ASSERT_IS_NOT_NULL(psa);
    TEST_ELEMENT* allocated;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(TEST_ELEMENT)(psa, HELD_PAGE_INDEX, &allocated));
    allocated->value = 42;
    TEST_ELEMENT* element;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(TEST_ELEMENT)(psa, HELD_PAGE_INDEX, &element));
    HELD_PAGE_RELEASE_CONTEXT release_context;
    release_context.psa = psa;
    (void)interlocked_exchange(&release_context.release_returned, 0);
    THREAD_HANDLE release_thread;
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&release_thread, held_page_release_thread, &release_context));

    //act
    ThreadAPI_Sleep(HELD_PAGE_RELEASE_WAIT_MS);
    int32_t release_returned_before_end_get = interlocked_add(&release_context.release_returned, 0);
    int64_t value_before_end_get = element->value;
    THREAD_SAFE_PAGED_SPARSE_ARRAY_END_GET(TEST_ELEMENT)(psa, HELD_PAGE_INDEX);
    int thread_result;
    THREADAPI_RESULT join_result = ThreadAPI_Join(release_thread, &thread_result);

    //assert
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, join_result);
    ASSERT_ARE_EQUAL(int, 0, thread_result);
    ASSERT_ARE_EQUAL(int32_t, 0, release_returned_before_end_get);
    ASSERT_ARE_EQUAL(int64_t, 42, value_before_end_get);
    ASSERT_ARE_EQUAL(int32_t, 1, interlocked_add(&release_context.release_returned, 0));
    assert_all_pages_are_freed_TEST_ELEMENT(psa, CONCURRENT_MAX_SIZE);

    //cleanup
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(TEST_ELEMENT)(&psa, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
