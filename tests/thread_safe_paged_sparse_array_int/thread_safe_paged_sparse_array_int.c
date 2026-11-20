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

/*TEST_ITEM - the items of the arrays of THANDLE(TEST_ITEM), they are not changed after they are created*/
typedef struct TEST_ITEM_TAG
{
    uint64_t index;
    int64_t value;
} TEST_ITEM;

THANDLE_TYPE_DECLARE(TEST_ITEM);
THANDLE_TYPE_DEFINE(TEST_ITEM);

THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(THANDLE(TEST_ITEM));
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(THANDLE(TEST_ITEM)));
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(THANDLE(TEST_ITEM)));

THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(THANDLE(TEST_ITEM));
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(THANDLE(TEST_ITEM));

/*RING_ELEMENT - element type used by the producer/consumer ring buffer test, it is copied by value*/
typedef struct RING_ELEMENT_TAG
{
    int64_t sequence;
    int64_t sequence_check; /*~sequence, an ACQUIRE_ITEM that copies a partly written element gets a sequence_check that does not match*/
} RING_ELEMENT;

THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(RING_ELEMENT);
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(RING_ELEMENT));
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(RING_ELEMENT));

THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(RING_ELEMENT);
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(RING_ELEMENT);

/*SLOW_COPY_ELEMENT - element type used by the test where RELEASE waits for an ACQUIRE_ITEM that copies the item*/
typedef struct SLOW_COPY_ELEMENT_TAG
{
    int64_t value;
} SLOW_COPY_ELEMENT;

THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(SLOW_COPY_ELEMENT);
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(SLOW_COPY_ELEMENT));
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(SLOW_COPY_ELEMENT));

THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(SLOW_COPY_ELEMENT);
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(SLOW_COPY_ELEMENT);

TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_RESULT_VALUES)
TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES)

static volatile_atomic int64_t g_created_item_count;
static volatile_atomic int64_t g_disposed_item_count;

static void test_item_dispose(TEST_ITEM* item)
{
    (void)item;
    (void)interlocked_increment_64(&g_disposed_item_count);
}

static void reset_item_counts(void)
{
    (void)interlocked_exchange_64(&g_created_item_count, 0);
    (void)interlocked_exchange_64(&g_disposed_item_count, 0);
}

/*returns a new item that the caller owns, or NULL*/
static THANDLE(TEST_ITEM) create_item(uint64_t index, int64_t value)
{
    TEST_ITEM* result = THANDLE_MALLOC(TEST_ITEM)(test_item_dispose);
    if (result != NULL)
    {
        result->index = index;
        result->value = value;
        (void)interlocked_increment_64(&g_created_item_count);
    }
    return result;
}

static void test_item_handle_copy(THANDLE(TEST_ITEM)* dst, THANDLE(TEST_ITEM)* src)
{
    THANDLE_INITIALIZE(TEST_ITEM)(dst, *src);
}

static void test_item_handle_dispose(THANDLE(TEST_ITEM)* item)
{
    THANDLE_ASSIGN(TEST_ITEM)(item, NULL);
}

/*allocates the element at index with a new item, the array holds the only reference to the item*/
static PAGED_SPARSE_ARRAY_ALLOCATE_RESULT allocate_new_item(THREAD_SAFE_PAGED_SPARSE_ARRAY(THANDLE(TEST_ITEM)) psa, uint64_t index, int64_t value)
{
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result;
    THANDLE(TEST_ITEM) item = create_item(index, value);
    if (item == NULL)
    {
        LogError("failure in create_item(index=%" PRIu64 ", value=%" PRId64 ")", index, value);
        result = PAGED_SPARSE_ARRAY_ALLOCATE_ERROR;
    }
    else
    {
        result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(THANDLE(TEST_ITEM))(psa, index, &item);
        THANDLE_ASSIGN(TEST_ITEM)(&item, NULL);
    }
    return result;
}

/*once all the elements are released, ACQUIRE_ITEM returns PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED for every index*/
static void assert_all_items_are_released(THREAD_SAFE_PAGED_SPARSE_ARRAY(THANDLE(TEST_ITEM)) psa, uint64_t max_size)
{
    for (uint64_t index = 0; index < max_size; index++)
    {
        THANDLE(TEST_ITEM) item = NULL;
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(THANDLE(TEST_ITEM))(psa, index, &item), "index=%" PRIu64 "", index);
    }
}

static void assert_all_ring_elements_are_released(THREAD_SAFE_PAGED_SPARSE_ARRAY(RING_ELEMENT) psa, uint64_t max_size)
{
    for (uint64_t index = 0; index < max_size; index++)
    {
        RING_ELEMENT element;
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(RING_ELEMENT)(psa, index, &element), "index=%" PRIu64 "", index);
    }
}

/*
 * Multi-threaded tests for THREAD_SAFE_PAGED_SPARSE_ARRAY.
 * Asserts cannot be used from worker threads, so worker threads count failures and the test thread asserts on the count.
 */
#define CONCURRENT_THREAD_COUNT 8
#define CONCURRENT_OPERATIONS_PER_THREAD 20000
#define CONCURRENT_MAX_SIZE 250 /*not a multiple of CONCURRENT_PAGE_SIZE, so the last page has fewer indexes than the other pages*/
#define CONCURRENT_PAGE_SIZE 16
#define CONCURRENT_MAX_HELD_ELEMENTS_PER_THREAD 4

static uint64_t test_next_random(uint64_t* state)
{
    /* 64-bit LCG, each thread has its own state so no locking is needed */
    *state = *state * 6364136223846793005ULL + 1442695040888963407ULL;
    return *state >> 33;
}

typedef struct CONCURRENT_TEST_CONTEXT_TAG
{
    const THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(THANDLE(TEST_ITEM))* psa;
    volatile_atomic int32_t owners[CONCURRENT_MAX_SIZE];
    volatile_atomic int64_t successful_allocations;
    volatile_atomic int64_t successful_gets_of_other_elements;
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
    int64_t expected_value;
} HELD_ELEMENT;

static void concurrent_record_failure(CONCURRENT_TEST_CONTEXT* test_context)
{
    (void)interlocked_increment(&test_context->failures);
}

static void concurrent_release_held_element(CONCURRENT_TEST_CONTEXT* test_context, int32_t thread_id, const HELD_ELEMENT* held)
{
    /* the element is owned by this thread, so ACQUIRE_ITEM returns the item that this thread allocated */
    THANDLE(TEST_ITEM) item = NULL;
    PAGED_SPARSE_ARRAY_GET_RESULT get_result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(THANDLE(TEST_ITEM))(test_context->psa, held->index, &item);
    if (get_result != PAGED_SPARSE_ARRAY_GET_OK)
    {
        LogError("thread_id=%" PRId32 ": ACQUIRE_ITEM of a held element failed, index=%" PRIu64 ", page_index=%" PRIu64 ", get_result=%" PRI_MU_ENUM "",
            thread_id, held->index, held->index / CONCURRENT_PAGE_SIZE, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, get_result));
        concurrent_record_failure(test_context);
    }
    else
    {
        if (
            (item->index != held->index) ||
            (item->value != held->expected_value)
            )
        {
            LogError("thread_id=%" PRId32 ": ACQUIRE_ITEM of a held element returned another item, index=%" PRIu64 ", page_index=%" PRIu64 ", item index=%" PRIu64 ", item value=%" PRId64 ", expected_value=%" PRId64 "",
                thread_id, held->index, held->index / CONCURRENT_PAGE_SIZE, item->index, item->value, held->expected_value);
            concurrent_record_failure(test_context);
        }
        else
        {
            /* do nothing, the item is as expected */
        }
        THANDLE_ASSIGN(TEST_ITEM)(&item, NULL);
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

    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(THANDLE(TEST_ITEM))(test_context->psa, held->index);
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
            int64_t value = ((int64_t)thread_id << 32) | operation;
            PAGED_SPARSE_ARRAY_ALLOCATE_RESULT allocate_result = allocate_new_item(test_context->psa, index, value);
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
                    held[held_count].expected_value = value;
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

        /* the item of an element of another thread can be read, even if the other thread releases the element at the same time */
        uint64_t other_index = test_next_random(&random_state) % CONCURRENT_MAX_SIZE;
        THANDLE(TEST_ITEM) other_item = NULL;
        PAGED_SPARSE_ARRAY_GET_RESULT get_result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(THANDLE(TEST_ITEM))(test_context->psa, other_index, &other_item);
        if (get_result == PAGED_SPARSE_ARRAY_GET_OK)
        {
            if (other_item->index != other_index)
            {
                LogError("thread_id=%" PRId32 ": ACQUIRE_ITEM returned the item of another index, operation=%" PRIu32 ", index=%" PRIu64 ", page_index=%" PRIu64 ", item index=%" PRIu64 ", item value=%" PRId64 "",
                    thread_id, operation, other_index, other_index / CONCURRENT_PAGE_SIZE, other_item->index, other_item->value);
                concurrent_record_failure(test_context);
            }
            else
            {
                (void)interlocked_increment_64(&test_context->successful_gets_of_other_elements);
            }
            THANDLE_ASSIGN(TEST_ITEM)(&other_item, NULL);
        }
        else
        {
            if (get_result != PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED)
            {
                LogError("thread_id=%" PRId32 ": ACQUIRE_ITEM returned an unexpected result, operation=%" PRIu32 ", index=%" PRIu64 ", page_index=%" PRIu64 ", get_result=%" PRI_MU_ENUM "",
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
 * Producer/consumer ring buffer over a thread safe paged sparse array of elements copied by value:
 * producers allocate sequence % RING_MAX_SIZE (less than RING_WINDOW sequences ahead of the tail) with an element that has the sequence.
 * A single consumer polls the element of the tail sequence with ACQUIRE_ITEM until its producer allocated it, the way the EBS writer polls the slot
 * of its head sequence, and releases the elements in sequence order. ACQUIRE_ITEM does not copy an element that ALLOCATE did not copy completely.
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
            RING_ELEMENT element;
            element.sequence = sequence;
            element.sequence_check = ~sequence;
            PAGED_SPARSE_ARRAY_ALLOCATE_RESULT allocate_result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(RING_ELEMENT)(test_context->psa, index, &element);
            if (allocate_result != PAGED_SPARSE_ARRAY_ALLOCATE_OK)
            {
                LogError("producer ALLOCATE failed, sequence=%" PRId64 ", index=%" PRIu64 ", page_index=%" PRIu64 ", tail=%" PRId64 ", allocate_result=%" PRI_MU_ENUM "",
                    sequence, index, index / RING_PAGE_SIZE, interlocked_add_64(&test_context->tail, 0), MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, allocate_result));
                (void)interlocked_increment(&test_context->failures);
            }
            else
            {
                /* do nothing, the consumer releases the element */
            }
        }
    }
    return 0;
}

static void run_ring_buffer_test(void)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(RING_ELEMENT) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(RING_ELEMENT)(RING_MAX_SIZE, RING_PAGE_SIZE, NULL, NULL);
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
        bool consumed = false;

        /* poll the element of the tail sequence until its producer allocated it */
        while (
            (!consumed) &&
            (!ring_has_failed(test_context))
            )
        {
            RING_ELEMENT element;
            PAGED_SPARSE_ARRAY_GET_RESULT get_result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(RING_ELEMENT)(psa, index, &element);
            if (get_result == PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED)
            {
                /* the producer did not allocate the element yet */
                ThreadAPI_Sleep(0);
            }
            else
            {
                if (get_result != PAGED_SPARSE_ARRAY_GET_OK)
                {
                    LogError("consumer ACQUIRE_ITEM failed, tail=%" PRId64 ", index=%" PRIu64 ", page_index=%" PRIu64 ", get_result=%" PRI_MU_ENUM "",
                        tail, index, index / RING_PAGE_SIZE, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, get_result));
                    (void)interlocked_increment(&test_context->failures);
                }
                else
                {
                    if (
                        (element.sequence != tail) ||
                        (element.sequence_check != ~tail)
                        )
                    {
                        LogError("consumer got an element that is not the one of the tail sequence, tail=%" PRId64 ", index=%" PRIu64 ", page_index=%" PRIu64 ", element sequence=%" PRId64 ", element sequence_check=%" PRId64 "",
                            tail, index, index / RING_PAGE_SIZE, element.sequence, element.sequence_check);
                        (void)interlocked_increment(&test_context->failures);
                    }
                    else
                    {
                        consumed = true;
                    }
                }
            }
        }

        if (!consumed)
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
    assert_all_ring_elements_are_released(psa, RING_MAX_SIZE);

    //cleanup
    free(test_context);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(RING_ELEMENT)(&psa, NULL);
}

/*
 * RELEASE does not dispose an item while ACQUIRE_ITEM calls copy it: item_copy_func of each ACQUIRE_ITEM waits until the RELEASE of the element started,
 * then the copies end one after the other and each one checks that the item is not disposed before it ends.
 */
#define SLOW_COPY_INDEX 3
#define SLOW_COPY_WAIT_FOR_RELEASE_MS 10000
#define SLOW_COPY_RELEASE_TIME_MS 200
#define SLOW_COPY_MAX_READER_COUNT 3

typedef struct SLOW_COPY_TEST_CONTEXT_TAG
{
    const THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(SLOW_COPY_ELEMENT)* psa;
    volatile_atomic int32_t copy_is_slow; /*1 while the copies of the ACQUIRE_ITEM calls wait for RELEASE*/
    volatile_atomic int32_t copy_started_count;
    volatile_atomic int32_t release_started;
    volatile_atomic int32_t release_returned;
    volatile_atomic int32_t dispose_count;
    volatile_atomic int32_t copies_that_ended_after_dispose;
} SLOW_COPY_TEST_CONTEXT;

typedef struct SLOW_COPY_READER_CONTEXT_TAG
{
    SLOW_COPY_TEST_CONTEXT* test_context;
    PAGED_SPARSE_ARRAY_GET_RESULT get_result;
    SLOW_COPY_ELEMENT got_element;
} SLOW_COPY_READER_CONTEXT;

static SLOW_COPY_TEST_CONTEXT* g_slow_copy_test_context;

static void slow_copy_element_copy(SLOW_COPY_ELEMENT* dst, SLOW_COPY_ELEMENT* src)
{
    SLOW_COPY_TEST_CONTEXT* test_context = g_slow_copy_test_context;
    if (interlocked_add(&test_context->copy_is_slow, 0) == 0)
    {
        /* do nothing, ALLOCATE copies the element */
    }
    else
    {
        int32_t copy_order = interlocked_increment(&test_context->copy_started_count);
        for (uint32_t waited_ms = 0; (waited_ms < SLOW_COPY_WAIT_FOR_RELEASE_MS) && (interlocked_add(&test_context->release_started, 0) == 0); waited_ms++)
        {
            ThreadAPI_Sleep(1);
        }
        /* RELEASE has time to dispose the item if it does not wait for all the ACQUIRE_ITEM calls, the copies end one after the other */
        ThreadAPI_Sleep(SLOW_COPY_RELEASE_TIME_MS * (uint32_t)copy_order);
    }
    dst->value = src->value;
    if (interlocked_add(&test_context->dispose_count, 0) != 0)
    {
        (void)interlocked_increment(&test_context->copies_that_ended_after_dispose);
    }
    else
    {
        /* do nothing, the item is not disposed */
    }
}

static void slow_copy_element_dispose(SLOW_COPY_ELEMENT* item)
{
    (void)item;
    (void)interlocked_increment(&g_slow_copy_test_context->dispose_count);
}

static int slow_copy_get_thread(void* arg)
{
    SLOW_COPY_READER_CONTEXT* reader_context = arg;
    reader_context->get_result = THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(SLOW_COPY_ELEMENT)(reader_context->test_context->psa, SLOW_COPY_INDEX, &reader_context->got_element);
    return 0;
}

static int slow_copy_release_thread(void* arg)
{
    SLOW_COPY_TEST_CONTEXT* test_context = arg;
    (void)interlocked_exchange(&test_context->release_started, 1);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(SLOW_COPY_ELEMENT)(test_context->psa, SLOW_COPY_INDEX);
    (void)interlocked_exchange(&test_context->release_returned, 1);
    return 0;
}

#define SLOW_COPY_ITEM_VALUE 42

static void run_slow_copy_test(uint32_t reader_count)
{
    //arrange
    ASSERT_IS_TRUE(reader_count <= SLOW_COPY_MAX_READER_COUNT);
    SLOW_COPY_TEST_CONTEXT test_context;
    (void)memset(&test_context, 0, sizeof(test_context));
    g_slow_copy_test_context = &test_context;
    THREAD_SAFE_PAGED_SPARSE_ARRAY(SLOW_COPY_ELEMENT) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(SLOW_COPY_ELEMENT)(CONCURRENT_MAX_SIZE, CONCURRENT_PAGE_SIZE, slow_copy_element_copy, slow_copy_element_dispose);
    ASSERT_IS_NOT_NULL(psa);
    test_context.psa = psa;
    SLOW_COPY_ELEMENT element = { SLOW_COPY_ITEM_VALUE };
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(SLOW_COPY_ELEMENT)(psa, SLOW_COPY_INDEX, &element));
    (void)interlocked_exchange(&test_context.copy_is_slow, 1);

    /* no ASSERT until all the started threads are joined, since they use stack variables of this function */
    SLOW_COPY_READER_CONTEXT reader_contexts[SLOW_COPY_MAX_READER_COUNT];
    THREAD_HANDLE reader_threads[SLOW_COPY_MAX_READER_COUNT];
    uint32_t readers_created = 0;
    THREADAPI_RESULT reader_create_result = THREADAPI_OK;
    for (uint32_t i = 0; i < reader_count; i++)
    {
        reader_contexts[i].test_context = &test_context;
        reader_contexts[i].get_result = PAGED_SPARSE_ARRAY_GET_INVALID_ARGS;
        reader_contexts[i].got_element.value = 0;
        reader_create_result = ThreadAPI_Create(&reader_threads[i], slow_copy_get_thread, &reader_contexts[i]);
        if (reader_create_result != THREADAPI_OK)
        {
            break;
        }
        else
        {
            readers_created++;
        }
    }
    /* RELEASE starts once every ACQUIRE_ITEM copies the item */
    for (uint32_t waited_ms = 0; (waited_ms < SLOW_COPY_WAIT_FOR_RELEASE_MS) && (interlocked_add(&test_context.copy_started_count, 0) < (int32_t)readers_created); waited_ms++)
    {
        ThreadAPI_Sleep(1);
    }

    //act
    THREAD_HANDLE release_thread;
    THREADAPI_RESULT release_create_result = ThreadAPI_Create(&release_thread, slow_copy_release_thread, &test_context);
    THREADAPI_RESULT reader_join_result = THREADAPI_OK;
    for (uint32_t i = 0; i < readers_created; i++)
    {
        int reader_thread_result;
        THREADAPI_RESULT join_result = ThreadAPI_Join(reader_threads[i], &reader_thread_result);
        if (join_result != THREADAPI_OK)
        {
            reader_join_result = join_result;
        }
        else
        {
            /* do nothing */
        }
    }
    int release_thread_result = 0;
    THREADAPI_RESULT release_join_result = (release_create_result != THREADAPI_OK) ? release_create_result : ThreadAPI_Join(release_thread, &release_thread_result);

    //assert
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, reader_create_result);
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, release_create_result);
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, reader_join_result);
    ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, release_join_result);
    ASSERT_ARE_EQUAL(int32_t, (int32_t)reader_count, interlocked_add(&test_context.copy_started_count, 0));
    for (uint32_t i = 0; i < reader_count; i++)
    {
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, reader_contexts[i].get_result, "reader %" PRIu32 "", i);
        ASSERT_ARE_EQUAL(int64_t, SLOW_COPY_ITEM_VALUE, reader_contexts[i].got_element.value, "reader %" PRIu32 "", i);
    }
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&test_context.copies_that_ended_after_dispose, 0));
    ASSERT_ARE_EQUAL(int32_t, 1, interlocked_add(&test_context.release_returned, 0));
    ASSERT_ARE_EQUAL(int32_t, 1, interlocked_add(&test_context.dispose_count, 0));

    //cleanup
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(SLOW_COPY_ELEMENT)(&psa, NULL);
    g_slow_copy_test_context = NULL;
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
    reset_item_counts();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

/*
 * Tests for THREAD_SAFE_PAGED_SPARSE_ARRAY
 */

TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_chaos_allocate_release_and_acquire_item_from_multiple_threads)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(THANDLE(TEST_ITEM)) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(THANDLE(TEST_ITEM))(CONCURRENT_MAX_SIZE, CONCURRENT_PAGE_SIZE, test_item_handle_copy, test_item_handle_dispose);
    ASSERT_IS_NOT_NULL(psa);

    CONCURRENT_TEST_CONTEXT* test_context = malloc(sizeof(CONCURRENT_TEST_CONTEXT));
    ASSERT_IS_NOT_NULL(test_context);
    test_context->psa = psa;
    for (uint32_t i = 0; i < CONCURRENT_MAX_SIZE; i++)
    {
        (void)interlocked_exchange(&test_context->owners[i], 0);
    }
    (void)interlocked_exchange_64(&test_context->successful_allocations, 0);
    (void)interlocked_exchange_64(&test_context->successful_gets_of_other_elements, 0);
    (void)interlocked_exchange(&test_context->failures, 0);

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
    LogInfo("chaos test: successful_allocations=%" PRId64 ", successful_gets_of_other_elements=%" PRId64 ", created items=%" PRId64 ", disposed items=%" PRId64 ", failures=%" PRId32 "",
        interlocked_add_64(&test_context->successful_allocations, 0), interlocked_add_64(&test_context->successful_gets_of_other_elements, 0),
        interlocked_add_64(&g_created_item_count, 0), interlocked_add_64(&g_disposed_item_count, 0), interlocked_add(&test_context->failures, 0));
    ASSERT_IS_FALSE(thread_api_failed);
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&test_context->failures, 0));
    ASSERT_IS_TRUE(interlocked_add_64(&test_context->successful_allocations, 0) > 0);
    ASSERT_IS_TRUE(interlocked_add_64(&test_context->successful_gets_of_other_elements, 0) > 0);
    /* every element was released and every item that ACQUIRE_ITEM returned was released, so every item was disposed */
    ASSERT_ARE_EQUAL(int64_t, interlocked_add_64(&g_created_item_count, 0), interlocked_add_64(&g_disposed_item_count, 0));
    assert_all_items_are_released(psa, CONCURRENT_MAX_SIZE);

    //cleanup
    free(test_context);
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(THANDLE(TEST_ITEM))(&psa, NULL);
}

TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_ring_buffer_with_multiple_producers_and_one_consumer)
{
    run_ring_buffer_test();
}

/*
 * The item that ACQUIRE_ITEM returned does not depend on the array: it stays valid after the element is released and its page is freed.
 */
#define OUTLIVING_ITEM_INDEX 3
#define OUTLIVING_ITEM_VALUE 42

TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_item_that_ACQUIRE_ITEM_returned_stays_valid_after_RELEASE_of_the_last_element_of_its_page)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(THANDLE(TEST_ITEM)) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(THANDLE(TEST_ITEM))(CONCURRENT_MAX_SIZE, CONCURRENT_PAGE_SIZE, test_item_handle_copy, test_item_handle_dispose);
    ASSERT_IS_NOT_NULL(psa);
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, allocate_new_item(psa, OUTLIVING_ITEM_INDEX, OUTLIVING_ITEM_VALUE));
    THANDLE(TEST_ITEM) item = NULL;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(THANDLE(TEST_ITEM))(psa, OUTLIVING_ITEM_INDEX, &item));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(THANDLE(TEST_ITEM))(psa, OUTLIVING_ITEM_INDEX);

    //assert
    assert_all_items_are_released(psa, CONCURRENT_MAX_SIZE);
    ASSERT_ARE_EQUAL(int64_t, 0, interlocked_add_64(&g_disposed_item_count, 0));
    ASSERT_ARE_EQUAL(uint64_t, OUTLIVING_ITEM_INDEX, item->index);
    ASSERT_ARE_EQUAL(int64_t, OUTLIVING_ITEM_VALUE, item->value);
    THANDLE_ASSIGN(TEST_ITEM)(&item, NULL);
    ASSERT_ARE_EQUAL(int64_t, 1, interlocked_add_64(&g_created_item_count, 0));
    ASSERT_ARE_EQUAL(int64_t, 1, interlocked_add_64(&g_disposed_item_count, 0));

    //cleanup
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(THANDLE(TEST_ITEM))(&psa, NULL);
}

/*
 * The dispose of the array disposes the items of the elements that are still allocated, in every page, including the last page that has fewer indexes.
 * An item that ACQUIRE_ITEM returned stays valid after the array is disposed.
 */
#define DISPOSE_TEST_ACQUIRED_INDEX (CONCURRENT_PAGE_SIZE + 1)

TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_dispose_of_the_array_disposes_the_items_of_the_allocated_elements)
{
    //arrange
    THREAD_SAFE_PAGED_SPARSE_ARRAY(THANDLE(TEST_ITEM)) psa = THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(THANDLE(TEST_ITEM))(CONCURRENT_MAX_SIZE, CONCURRENT_PAGE_SIZE, test_item_handle_copy, test_item_handle_dispose);
    ASSERT_IS_NOT_NULL(psa);
    uint64_t allocated_indexes[] = { 0, 1, DISPOSE_TEST_ACQUIRED_INDEX, CONCURRENT_MAX_SIZE - 1 };
    for (size_t i = 0; i < MU_COUNT_ARRAY_ITEMS(allocated_indexes); i++)
    {
        ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_OK, allocate_new_item(psa, allocated_indexes[i], (int64_t)allocated_indexes[i]), "index=%" PRIu64 "", allocated_indexes[i]);
    }
    THANDLE(TEST_ITEM) item = NULL;
    ASSERT_ARE_EQUAL(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_OK, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(THANDLE(TEST_ITEM))(psa, DISPOSE_TEST_ACQUIRED_INDEX, &item));

    //act
    THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(THANDLE(TEST_ITEM))(&psa, NULL);

    //assert
    ASSERT_ARE_EQUAL(int64_t, (int64_t)MU_COUNT_ARRAY_ITEMS(allocated_indexes), interlocked_add_64(&g_created_item_count, 0));
    ASSERT_ARE_EQUAL(int64_t, (int64_t)MU_COUNT_ARRAY_ITEMS(allocated_indexes) - 1, interlocked_add_64(&g_disposed_item_count, 0));
    ASSERT_ARE_EQUAL(uint64_t, DISPOSE_TEST_ACQUIRED_INDEX, item->index);
    ASSERT_ARE_EQUAL(int64_t, (int64_t)DISPOSE_TEST_ACQUIRED_INDEX, item->value);
    THANDLE_ASSIGN(TEST_ITEM)(&item, NULL);
    ASSERT_ARE_EQUAL(int64_t, (int64_t)MU_COUNT_ARRAY_ITEMS(allocated_indexes), interlocked_add_64(&g_disposed_item_count, 0));
}

TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_waits_for_the_ACQUIRE_ITEM_that_copies_the_item_before_disposing_the_item)
{
    run_slow_copy_test(1);
}

TEST_FUNCTION(THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE_waits_for_all_the_ACQUIRE_ITEM_calls_that_copy_the_item_before_disposing_the_item)
{
    run_slow_copy_test(SLOW_COPY_MAX_READER_COUNT);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
