// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_H
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_H

#ifdef __cplusplus
#include <cinttypes>
#include <cstddef>
#else // __cplusplus
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#endif // __cplusplus

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"
#include "c_pal/srw_lock_ll.h"
#include "c_pal/sync.h"
#include "c_pal/thandle_ll.h"

#include "c_util/paged_sparse_array_ll.h"

#include "umock_c/umock_c_prod.h"

/*
 * THREAD_SAFE_PAGED_SPARSE_ARRAY wraps a PAGED_SPARSE_ARRAY (see paged_sparse_array_ll.h) so that ALLOCATE, RELEASE and GET can be
 * called concurrently from multiple threads, without acquiring any lock when the page of the element is allocated.
 * The elements of the wrapped paged sparse array are slots. A slot holds an item of type T and the state of the item
 * (THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE), the state is changed only with interlocked operations.
 * Each page has a page guard: a use count, a page state (THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE) and a lock.
 * - The wrapped paged sparse array is changed (PAGED_SPARSE_ARRAY_ALLOCATE and PAGED_SPARSE_ARRAY_RELEASE) only to allocate a page,
 *   for all the indexes of the page while holding the lock of the page, and to free a page, for all the indexes of the page after its
 *   use count was changed from 0 to THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING. The lock is not held while the page is freed.
 * - ALLOCATE, RELEASE and GET of an element of an allocated page increment the use count of the page and call PAGED_SPARSE_ARRAY_GET
 *   (which only reads the wrapped paged sparse array). ALLOCATE and RELEASE change the state of the slot with interlocked operations,
 *   GET only reads it. A successful ALLOCATE keeps its use of the page for the allocated element, RELEASE ends it.
 * - A page is freed when its use count becomes 0.
 */

/*states of a slot*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_VALUES \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING

MU_DEFINE_ENUM(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_VALUES)

/*states of a page*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_VALUES \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED, \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING

MU_DEFINE_ENUM(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_VALUES)

/*added to the use count of a page while the page is being freed, it is larger than any count of allocated elements and calls that use the page*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING (INT64_C(1) << 62)

/*the array has one page guard for each page of the wrapped paged sparse array, the page guards exist as long as the array exists*/
typedef struct THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD_TAG
{
    /*count of allocated elements in the page plus count of calls that use the page, plus THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING while the page is being freed*/
    volatile_atomic int64_t use_count;
    /*THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE, the calls that allocate the page wait on it while the page is being freed*/
    /*it is 32 bits because wait_on_address_64 on Linux compares only 32 bits of the value, so a wait on the use count could miss its change*/
    volatile_atomic int32_t page_state;
    /*acquired in exclusive mode only to allocate the page or to start freeing it*/
    SRW_LOCK_LL lock;
} THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD;

/*the index after the last index of the page that starts at first_index, the last page of the array can have fewer than page_size indexes*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_END_INDEX(first_index, page_size, max_size) \
    ((((max_size) - (first_index)) < (page_size)) ? (max_size) : ((first_index) + (page_size)))

/*string literal that describes a thread safe paged sparse array type, used in logs*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) "THREAD_SAFE_PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) ")"

/*the elements of the wrapped paged sparse array are slots*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STRUCT_TYPE_NAME_TAG(T) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T), _TAG)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_, T)

/*the wrapped paged sparse array is a PAGED_SPARSE_ARRAY of slots*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_WRAPPED_ARRAY_LL(T) PAGED_SPARSE_ARRAY_LL(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T))

/*THREAD_SAFE_PAGED_SPARSE_ARRAY is backed by a THANDLE build on the structure below*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_TYPE_NAME_TAG(T) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T), _TAG)

#define THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_, T)

/*custom item dispose function type*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC_, T)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_ITEM_DISPOSE_FUNC_TYPE(T) typedef void (*THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T))(T* item)

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T) introduces the base type that holds the thread safe paged sparse array*/
/*it also introduces the slot type and the wrapped PAGED_SPARSE_ARRAY of slots, with its THANDLE declaration*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(T)                                                                                                               \
THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_ITEM_DISPOSE_FUNC_TYPE(T);                                                                                                           \
typedef struct THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STRUCT_TYPE_NAME_TAG(T)                                                                                                 \
{                                                                                                                                                                          \
    volatile_atomic int32_t state; /*THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE*/                                                                                           \
    T item;                                                                                                                                                                \
} THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T);                                                                                                                     \
PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T))                                                                                 \
THANDLE_LL_TYPE_DECLARE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)), PAGED_SPARSE_ARRAY_TYPEDEF_NAME(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T))); \
typedef struct THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_TYPE_NAME_TAG(T)                                                                                                      \
{                                                                                                                                                                          \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_WRAPPED_ARRAY_LL(T) wrapped_array;                                                                                                      \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func;                                                                                              \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD page_guards[]; /*one for each page of wrapped_array*/                                                                        \
} THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T);                                                                                                                          \

/*THREAD_SAFE_PAGED_SPARSE_ARRAY is-a THANDLE*/
/*given a type "T" THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T) expands to the name of the type. */
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T) THANDLE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T))

/*because THREAD_SAFE_PAGED_SPARSE_ARRAY is a THANDLE, all THANDLE's macro APIs are useable with THREAD_SAFE_PAGED_SPARSE_ARRAY.*/
/*the below are just shortcuts of THANDLE's public ones*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INITIALIZE(T) THANDLE_INITIALIZE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T))
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ASSIGN(T) THANDLE_ASSIGN(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T))
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_MOVE(T) THANDLE_MOVE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T))
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INITIALIZE_MOVE(T) THANDLE_INITIALIZE_MOVE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T))

/*introduces a new name for a function that returns a THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T)*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE_, C)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE(C) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE_NAME(C)

/*introduces a name for the dispose function that is called when THREAD_SAFE_PAGED_SPARSE_ARRAY ref count goes to 0*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_, C)

/*introduces a name for the allocate function*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_, C)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE(C) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_NAME(C)

/*introduces a name for the release function*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE_, C)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE(C) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE_NAME(C)

/*introduces a name for the get function*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_GET_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_GET_, C)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_GET(C) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_GET_NAME(C)

/*introduces names for the internal functions*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_BEGIN_PAGE_USE_INTERNAL_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_BEGIN_PAGE_USE_INTERNAL_, C)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_, C)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TRY_FREE_PAGE_INTERNAL_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TRY_FREE_PAGE_INTERNAL_, C)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_, C)

/*introduces a function declaration for thread_safe_paged_sparse_array_create*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T), THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE(C), uint64_t, max_size, uint64_t, page_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T), item_dispose_func);

/*introduces a function declaration for thread_safe_paged_sparse_array_allocate*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T), paged_sparse_array, uint64_t, index, T**, allocated_ptr);

/*introduces a function declaration for thread_safe_paged_sparse_array_release*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, void, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T), paged_sparse_array, uint64_t, index);

/*introduces a function declaration for thread_safe_paged_sparse_array_get*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_GET_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, PAGED_SPARSE_ARRAY_GET_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_GET(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T), paged_sparse_array, uint64_t, index, T**, item);

/*introduces the function declarations of the wrapped PAGED_SPARSE_ARRAY of slots*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_WRAPPED_ARRAY_DECLARE(C, T) \
    PAGED_SPARSE_ARRAY_LL_CREATE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)) \
    PAGED_SPARSE_ARRAY_LL_ALLOCATE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)) \
    PAGED_SPARSE_ARRAY_LL_RELEASE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)) \
    PAGED_SPARSE_ARRAY_LL_GET_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)) \

/*introduces the function definitions of the wrapped PAGED_SPARSE_ARRAY of slots, including its THANDLE definition*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_WRAPPED_ARRAY_DEFINE(C, T)                                                                                                       \
    THANDLE_LL_TYPE_DEFINE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C)), PAGED_SPARSE_ARRAY_TYPEDEF_NAME(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T))); \
    PAGED_SPARSE_ARRAY_LL_DISPOSE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T))                         \
    PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T))          \
    PAGED_SPARSE_ARRAY_LL_CREATE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T))                          \
    PAGED_SPARSE_ARRAY_LL_ALLOCATE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T))                        \
    PAGED_SPARSE_ARRAY_LL_RELEASE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T))                         \
    PAGED_SPARSE_ARRAY_LL_GET_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T))                             \

/*introduces a function definition for freeing the allocated resources for a THREAD_SAFE_PAGED_SPARSE_ARRAY*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_DEFINE(C, T)                                                                                                             \
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_NAME(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* paged_sparse_array)                                          \
{                                                                                                                                                                          \
    if (paged_sparse_array == NULL)                                                                                                                                        \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_011: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall return. ]*/                   \
        LogError("invalid arguments " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p",                                                           \
            paged_sparse_array);                                                                                                                                           \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        uint64_t max_size = paged_sparse_array->wrapped_array->max_size;                                                                                                   \
        uint64_t page_size = paged_sparse_array->wrapped_array->page_size;                                                                                                 \
        uint64_t page_count = paged_sparse_array->wrapped_array->page_count;                                                                                               \
        for (uint64_t page_index = 0; page_index < page_count; page_index++)                                                                                               \
        {                                                                                                                                                                  \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD* page_guard = &paged_sparse_array->page_guards[page_index];                                                          \
            if (                                                                                                                                                           \
                (paged_sparse_array->item_dispose_func != NULL) &&                                                                                                         \
                (interlocked_add(&page_guard->page_state, 0) == THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED)                                                       \
                )                                                                                                                                                          \
            {                                                                                                                                                              \
                uint64_t first_index = page_index * page_size;                                                                                                             \
                uint64_t end_index = THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_END_INDEX(first_index, page_size, max_size);                                                      \
                for (uint64_t index = first_index; index < end_index; index++)                                                                                             \
                {                                                                                                                                                          \
                    THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)* slot = NULL;                                                                                      \
                    if (                                                                                                                                                   \
                        (PAGED_SPARSE_ARRAY_LL_GET(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(paged_sparse_array->wrapped_array, index, &slot) == PAGED_SPARSE_ARRAY_GET_OK) && \
                        (interlocked_add(&slot->state, 0) == THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED)                                                          \
                        )                                                                                                                                                  \
                    {                                                                                                                                                      \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_012: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call item_dispose_func with a pointer to the item of each slot whose state is THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED in the pages whose page state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED. ]*/ \
                        paged_sparse_array->item_dispose_func(&slot->item);                                                                                                \
                    }                                                                                                                                                      \
                    else                                                                                                                                                   \
                    {                                                                                                                                                      \
                        /* do nothing, the element is not allocated */                                                                                                     \
                    }                                                                                                                                                      \
                }                                                                                                                                                          \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                /* do nothing, there is no item to dispose in the page */                                                                                                  \
            }                                                                                                                                                              \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_013: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call srw_lock_ll_deinit for the lock of each page guard. ]*/ \
            srw_lock_ll_deinit(&page_guard->lock);                                                                                                                         \
        }                                                                                                                                                                  \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_014: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall release the paged sparse array. ]*/                          \
        PAGED_SPARSE_ARRAY_LL_ASSIGN(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(&paged_sparse_array->wrapped_array, NULL);                                       \
    }                                                                                                                                                                      \
}

/*introduces a function definition for thread_safe_paged_sparse_array_create*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE_DEFINE(C, T)                                                                                                              \
THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE(C)(uint64_t max_size, uint64_t page_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func) \
{                                                                                                                                                                          \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* result;                                                                                                                \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_001: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call PAGED_SPARSE_ARRAY_CREATE(T) with max_size, page_size and NULL as item_dispose_func to create the paged sparse array of slots. ]*/ \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_WRAPPED_ARRAY_LL(T) wrapped_array = PAGED_SPARSE_ARRAY_LL_CREATE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(max_size, page_size, NULL); \
    if (wrapped_array == NULL)                                                                                                                                             \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_002: [ If PAGED_SPARSE_ARRAY_CREATE(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/ \
        LogError("failure in PAGED_SPARSE_ARRAY_CREATE(" MU_TOSTRING(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)) ")(uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", NULL) for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) ", item_dispose_func=%p", \
            max_size, page_size, item_dispose_func);                                                                                                                       \
        result = NULL;                                                                                                                                                     \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        uint64_t page_count = wrapped_array->page_count;                                                                                                                   \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_003: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX to allocate memory for the thread safe paged sparse array with one page guard for each page of the paged sparse array. ]*/ \
        result = THANDLE_MALLOC_FLEX(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_NAME(C), page_count, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD)); \
        if (result == NULL)                                                                                                                                                \
        {                                                                                                                                                                  \
            LogError("failure in THANDLE_MALLOC_FLEX(" THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) ")(page_count=%" PRIu64 ", sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD)=%zu), uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", item_dispose_func=%p", \
                page_count, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD), max_size, page_size, item_dispose_func);                                                    \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            uint64_t initialized_lock_count;                                                                                                                               \
            for (initialized_lock_count = 0; initialized_lock_count < page_count; initialized_lock_count++)                                                                \
            {                                                                                                                                                              \
                THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD* page_guard = &result->page_guards[initialized_lock_count];                                                      \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_004: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall set the use count of each page guard to 0 and the page state of each page guard to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED using interlocked operations. ]*/ \
                (void)interlocked_exchange_64(&page_guard->use_count, 0);                                                                                                  \
                (void)interlocked_exchange(&page_guard->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED);                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_005: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call srw_lock_ll_init to initialize the lock of each page guard. ]*/ \
                int lock_init_result = srw_lock_ll_init(&page_guard->lock);                                                                                                \
                if (lock_init_result != 0)                                                                                                                                 \
                {                                                                                                                                                          \
                    LogError("failure in srw_lock_ll_init(&page_guards[%" PRIu64 "].lock)=%d for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) ", uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", page_count=%" PRIu64 ", item_dispose_func=%p", \
                        initialized_lock_count, lock_init_result, max_size, page_size, page_count, item_dispose_func);                                                     \
                    break;                                                                                                                                                 \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    /* do nothing */                                                                                                                                       \
                }                                                                                                                                                          \
            }                                                                                                                                                              \
            if (initialized_lock_count < page_count)                                                                                                                       \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_006: [ If srw_lock_ll_init fails: ]*/                                                                       \
                for (uint64_t i = 0; i < initialized_lock_count; i++)                                                                                                      \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_080: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call srw_lock_ll_deinit for each lock that was initialized. ]*/ \
                    srw_lock_ll_deinit(&result->page_guards[i].lock);                                                                                                      \
                }                                                                                                                                                          \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_081: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall free the memory of the thread safe paged sparse array. ]*/ \
                THANDLE_FREE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(result);                                                                                      \
                result = NULL;                                                                                                                                             \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_007: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store item_dispose_func in the thread safe paged sparse array. ]*/ \
                result->item_dispose_func = item_dispose_func;                                                                                                             \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_008: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store the paged sparse array in the thread safe paged sparse array. ]*/ \
                PAGED_SPARSE_ARRAY_LL_INITIALIZE_MOVE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(&result->wrapped_array, &wrapped_array);                        \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_009: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/              \
            }                                                                                                                                                              \
        }                                                                                                                                                                  \
        if (result == NULL)                                                                                                                                                \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [ If there are any other errors: ]*/                                                                       \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_082: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall release the paged sparse array. ]*/                       \
            PAGED_SPARSE_ARRAY_LL_ASSIGN(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(&wrapped_array, NULL);                                                       \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_083: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/                                 \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* do nothing, the paged sparse array was moved to result */                                                                                                   \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}

/*introduces the definitions of the internal functions, they are used by ALLOCATE, RELEASE and GET*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INTERNAL_DEFINE(C, T)                                                                                                            \
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TRY_FREE_PAGE_INTERNAL_NAME(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array, uint64_t page_index)                   \
{                                                                                                                                                                          \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD* page_guard = &array->page_guards[page_index];                                                                               \
    bool is_freeing;                                                                                                                                                       \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_021: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall acquire the lock of the page guard in exclusive mode. ]*/  \
    srw_lock_ll_acquire_exclusive(&page_guard->lock);                                                                                                                      \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_022: [ If the page state of the page guard is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED: ]*/                  \
    if (interlocked_add(&page_guard->page_state, 0) != THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED)                                                                \
    {                                                                                                                                                                      \
        /* do nothing, another call freed the page or is freeing it */                                                                                                     \
        is_freeing = false;                                                                                                                                                \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_023: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall change the use count of the page guard from 0 to THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING using interlocked_compare_exchange_64. ]*/ \
        if (interlocked_compare_exchange_64(&page_guard->use_count, THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING, 0) != 0)                                             \
        {                                                                                                                                                                  \
            /* do nothing, another call uses the page */                                                                                                                   \
            is_freeing = false;                                                                                                                                            \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* the calls that begin to use the page from now on see THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING and do not use the wrapped paged sparse array */      \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_024: [ If the use count was 0, THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall set the page state of the page guard to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING using interlocked operations. ]*/ \
            (void)interlocked_exchange(&page_guard->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING);                                                        \
            is_freeing = true;                                                                                                                                             \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release the lock of the page guard. ]*/                    \
    srw_lock_ll_release_exclusive(&page_guard->lock);                                                                                                                      \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_026: [ If the use count was changed to THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING: ]*/                            \
    if (is_freeing)                                                                                                                                                        \
    {                                                                                                                                                                      \
        uint64_t page_size = array->wrapped_array->page_size;                                                                                                              \
        uint64_t first_index = page_index * page_size;                                                                                                                     \
        uint64_t end_index = THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_END_INDEX(first_index, page_size, array->wrapped_array->max_size);                                        \
        for (uint64_t index = first_index; index < end_index; index++)                                                                                                     \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_027: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall call PAGED_SPARSE_ARRAY_RELEASE(T) for each index of the page. ]*/ \
            PAGED_SPARSE_ARRAY_LL_RELEASE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->wrapped_array, index);                                               \
        }                                                                                                                                                                  \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_028: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall subtract THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING from the use count of the page guard using interlocked operations. ]*/ \
        (void)interlocked_add_64(&page_guard->use_count, -THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING);                                                               \
        /* the use count is lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING before the calls waiting for the page state see it change */                       \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_077: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall set the page state of the page guard to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED using interlocked operations. ]*/ \
        (void)interlocked_exchange(&page_guard->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED);                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_029: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall call wake_by_address_all for the page state of the page guard. ]*/ \
        wake_by_address_all(&page_guard->page_state);                                                                                                                      \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* do nothing, the page is not allocated or is used */                                                                                                             \
    }                                                                                                                                                                      \
}                                                                                                                                                                          \
static bool THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_BEGIN_PAGE_USE_INTERNAL_NAME(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array, uint64_t page_index)                  \
{                                                                                                                                                                          \
    bool result;                                                                                                                                                           \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD* page_guard = &array->page_guards[page_index];                                                                               \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_015: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall increment the use count of the page guard using interlocked operations. ]*/ \
    if (interlocked_increment_64(&page_guard->use_count) >= THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING)                                                              \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_016: [ If the use count is greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING, THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall return false. ]*/ \
        result = false;                                                                                                                                                    \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        if (interlocked_add(&page_guard->page_state, 0) != THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED)                                                            \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_017: [ If the page state of the page guard is not THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall return false. ]*/ \
            result = false;                                                                                                                                                \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* the page is not freed while it is used */                                                                                                                   \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_018: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall return true. ]*/                       \
            result = true;                                                                                                                                                 \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}                                                                                                                                                                          \
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array, uint64_t page_index, int64_t ended_use_count) \
{                                                                                                                                                                          \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD* page_guard = &array->page_guards[page_index];                                                                               \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_019: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) shall subtract ended_use_count from the use count of the page guard using interlocked operations. ]*/ \
    int64_t use_count = interlocked_add_64(&page_guard->use_count, -ended_use_count);                                                                                      \
    if (                                                                                                                                                                   \
        (use_count == 0) &&                                                                                                                                                \
        (interlocked_add(&page_guard->page_state, 0) == THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED)                                                               \
        )                                                                                                                                                                  \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_020: [ If the use count became 0 and the page state of the page guard is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T). ]*/ \
        THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TRY_FREE_PAGE_INTERNAL_NAME(C)(array, page_index);                                                                               \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* do nothing, the page has allocated elements, is used by another call, is being freed or is not allocated */                                                     \
    }                                                                                                                                                                      \
}                                                                                                                                                                          \
static PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array, uint64_t page_index) \
{                                                                                                                                                                          \
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result;                                                                                                                             \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD* page_guard = &array->page_guards[page_index];                                                                               \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_030: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall acquire the lock of the page guard in exclusive mode. ]*/  \
    srw_lock_ll_acquire_exclusive(&page_guard->lock);                                                                                                                      \
    /* a call that started to free the page before the lock was acquired frees it without holding the lock */                                                              \
    /* it subtracts THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING from the use count before it changes the page state, so the use count is lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING after the wait */ \
    INTERLOCKED_HL_RESULT wait_result = INTERLOCKED_HL_OK;                                                                                                                 \
    int32_t page_state = interlocked_add(&page_guard->page_state, 0);                                                                                                      \
    while (                                                                                                                                                                \
        (page_state == THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING) &&                                                                                               \
        (wait_result == INTERLOCKED_HL_OK)                                                                                                                                 \
        )                                                                                                                                                                  \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_031: [ While the page state of the page guard is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call InterlockedHL_WaitForNotValue to wait for the page state to change. ]*/ \
        wait_result = InterlockedHL_WaitForNotValue(&page_guard->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING, UINT32_MAX);                               \
        page_state = interlocked_add(&page_guard->page_state, 0);                                                                                                          \
    }                                                                                                                                                                      \
    if (wait_result != INTERLOCKED_HL_OK)                                                                                                                                  \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_032: [ If InterlockedHL_WaitForNotValue fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/ \
        result = PAGED_SPARSE_ARRAY_ALLOCATE_ERROR;                                                                                                                        \
        LogError("failure in InterlockedHL_WaitForNotValue(&page_guards[%" PRIu64 "].page_state, %" PRI_MU_ENUM ", UINT32_MAX)=%" PRI_MU_ENUM " for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) ", page_state=%" PRI_MU_ENUM ", use_count=%" PRId64 ", page_size=%" PRIu64 ", result=%" PRI_MU_ENUM "", \
            page_index, MU_ENUM_VALUE(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING), MU_ENUM_VALUE(INTERLOCKED_HL_RESULT, wait_result), \
            MU_ENUM_VALUE(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE, (THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE)page_state), interlocked_add_64(&page_guard->use_count, 0), \
            array->wrapped_array->page_size, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result));                                                                   \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        if (page_state == THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED)                                                                                             \
        {                                                                                                                                                                  \
            /* do nothing, another call allocated the page before the lock was acquired */                                                                                 \
            result = PAGED_SPARSE_ARRAY_ALLOCATE_OK;                                                                                                                       \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_033: [ If the page state of the page guard is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED: ]*/      \
            uint64_t page_size = array->wrapped_array->page_size;                                                                                                          \
            uint64_t first_index = page_index * page_size;                                                                                                                 \
            uint64_t end_index = THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_END_INDEX(first_index, page_size, array->wrapped_array->max_size);                                    \
            uint64_t index;                                                                                                                                                \
            result = PAGED_SPARSE_ARRAY_ALLOCATE_OK;                                                                                                                       \
            for (index = first_index; index < end_index; index++)                                                                                                          \
            {                                                                                                                                                              \
                THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)* slot;                                                                                                 \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_034: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call PAGED_SPARSE_ARRAY_ALLOCATE(T) for each index of the page. ]*/ \
                result = PAGED_SPARSE_ARRAY_LL_ALLOCATE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->wrapped_array, index, &slot);                          \
                if (result != PAGED_SPARSE_ARRAY_ALLOCATE_OK)                                                                                                              \
                {                                                                                                                                                          \
                    LogError("failure in PAGED_SPARSE_ARRAY_ALLOCATE(" MU_TOSTRING(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)) ")(index=%" PRIu64 ")=%" PRI_MU_ENUM " for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) ", page_index=%" PRIu64 ", first_index=%" PRIu64 ", end_index=%" PRIu64 "", \
                        index, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result), page_index, first_index, end_index);                                             \
                    break;                                                                                                                                                 \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_035: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall set the state of each slot of the page to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED using interlocked operations. ]*/ \
                    (void)interlocked_exchange(&slot->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED);                                                     \
                }                                                                                                                                                          \
            }                                                                                                                                                              \
            if (result != PAGED_SPARSE_ARRAY_ALLOCATE_OK)                                                                                                                  \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_036: [ If PAGED_SPARSE_ARRAY_ALLOCATE(T) fails: ]*/                                                         \
                for (uint64_t allocated_index = first_index; allocated_index < index; allocated_index++)                                                                   \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_078: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call PAGED_SPARSE_ARRAY_RELEASE(T) for each index of the page that was allocated. ]*/ \
                    PAGED_SPARSE_ARRAY_LL_RELEASE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->wrapped_array, allocated_index);                             \
                }                                                                                                                                                          \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_079: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/ \
                result = PAGED_SPARSE_ARRAY_ALLOCATE_ERROR;                                                                                                                \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                /* do nothing, all the indexes of the page are allocated in the paged sparse array */                                                                      \
            }                                                                                                                                                              \
        }                                                                                                                                                                  \
        if (result != PAGED_SPARSE_ARRAY_ALLOCATE_OK)                                                                                                                      \
        {                                                                                                                                                                  \
            /* do nothing, the error was logged */                                                                                                                         \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* the use count is incremented before the page state is set, so the page is not freed before the caller uses it */                                            \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_037: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall increment the use count of the page guard using interlocked operations. ]*/ \
            (void)interlocked_increment_64(&page_guard->use_count);                                                                                                        \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_038: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall set the page state of the page guard to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED using interlocked operations. ]*/ \
            (void)interlocked_exchange(&page_guard->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED);                                                      \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [ In all cases, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall release the lock of the page guard before returning. ]*/ \
    srw_lock_ll_release_exclusive(&page_guard->lock);                                                                                                                      \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_040: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall succeed and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/     \
    return result;                                                                                                                                                         \
}

/*introduces a function definition for thread_safe_paged_sparse_array_allocate*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_DEFINE(C, T)                                                                                                            \
/*hint: have THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INTERNAL_DEFINE(C, T) before THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_DEFINE(C, T)*/                                      \
PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint64_t index, T** allocated_ptr) \
{                                                                                                                                                                          \
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result;                                                                                                                             \
    if (                                                                                                                                                                   \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_041: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/ \
        (paged_sparse_array == NULL) ||                                                                                                                                    \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_042: [ If allocated_ptr is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/ \
        (allocated_ptr == NULL)                                                                                                                                            \
        )                                                                                                                                                                  \
    {                                                                                                                                                                      \
        result = PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS;                                                                                                                 \
        LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, uint64_t index=%" PRIu64 ", " MU_TOSTRING(T) "** allocated_ptr=%p, result=%" PRI_MU_ENUM "", \
            paged_sparse_array, index, allocated_ptr, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result));                                                          \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array);                         \
        uint64_t max_size = array->wrapped_array->max_size;                                                                                                                \
        uint64_t page_size = array->wrapped_array->page_size;                                                                                                              \
        if (index >= max_size)                                                                                                                                             \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_043: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/ \
            result = PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS;                                                                                                             \
            LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, uint64_t index=%" PRIu64 " out of bound, max_size=%" PRIu64 ", page_size=%" PRIu64 ", " MU_TOSTRING(T) "** allocated_ptr=%p, result=%" PRI_MU_ENUM "", \
                paged_sparse_array, index, max_size, page_size, allocated_ptr, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result));                                 \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            uint64_t page_index = index / page_size;                                                                                                                       \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_044: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) for the page with index index / page_size. ]*/ \
            if (THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_BEGIN_PAGE_USE_INTERNAL_NAME(C)(array, page_index))                                                                      \
            {                                                                                                                                                              \
                /* do nothing, the page is allocated and it is not freed while it is used */                                                                               \
                result = PAGED_SPARSE_ARRAY_ALLOCATE_OK;                                                                                                                   \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_045: [ If THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) returns false: ]*/                               \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_046: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/ \
                THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 1);                                                                     \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_047: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T). ]*/ \
                result = THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C)(array, page_index);                                                              \
            }                                                                                                                                                              \
            if (result != PAGED_SPARSE_ARRAY_ALLOCATE_OK)                                                                                                                  \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_048: [ If THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/ \
                result = PAGED_SPARSE_ARRAY_ALLOCATE_ERROR;                                                                                                                \
                LogError("failure allocating the page of " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, uint64_t index=%" PRIu64 ", page_index=%" PRIu64 ", page_size=%" PRIu64 ", max_size=%" PRIu64 ", result=%" PRI_MU_ENUM "", \
                    paged_sparse_array, index, page_index, page_size, max_size, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result));                                \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)* slot;                                                                                                 \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_049: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call PAGED_SPARSE_ARRAY_GET(T) with the paged sparse array and index to get the slot of the element. ]*/ \
                PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_LL_GET(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->wrapped_array, index, &slot); \
                if (get_result != PAGED_SPARSE_ARRAY_GET_OK)                                                                                                               \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_050: [ If PAGED_SPARSE_ARRAY_GET(T) fails: ]*/                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_084: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/ \
                    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 1);                                                                 \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_085: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/ \
                    result = PAGED_SPARSE_ARRAY_ALLOCATE_ERROR;                                                                                                            \
                    LogError("failure in PAGED_SPARSE_ARRAY_GET(" MU_TOSTRING(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)) ")(index=%" PRIu64 ")=%" PRI_MU_ENUM " for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, page_index=%" PRIu64 ", result=%" PRI_MU_ENUM "", \
                        index, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, get_result), paged_sparse_array, page_index, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result)); \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_051: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall change the state of the slot from THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED using interlocked_compare_exchange. ]*/ \
                    int32_t slot_state = interlocked_compare_exchange(&slot->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED); \
                    if (slot_state != THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED)                                                                             \
                    {                                                                                                                                                      \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_052: [ If the state of the slot was not THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED: ]*/ \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_086: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/ \
                        THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 1);                                                             \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_087: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED. ]*/ \
                        result = PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED;                                                                                            \
                        LogError("Element is already allocated in " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, uint64_t index=%" PRIu64 ", page_index=%" PRIu64 ", slot_state=%" PRI_MU_ENUM ", result=%" PRI_MU_ENUM "", \
                            paged_sparse_array, index, page_index, MU_ENUM_VALUE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE, (THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE)slot_state), MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result)); \
                    }                                                                                                                                                      \
                    else                                                                                                                                                   \
                    {                                                                                                                                                      \
                        /* the use of the page is not ended, from now on it is the use of the allocated element */                                                         \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_053: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall store a pointer to the item of the slot in allocated_ptr and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/ \
                        *allocated_ptr = &slot->item;                                                                                                                      \
                        result = PAGED_SPARSE_ARRAY_ALLOCATE_OK;                                                                                                           \
                    }                                                                                                                                                      \
                }                                                                                                                                                          \
            }                                                                                                                                                              \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}

/*introduces a function definition for thread_safe_paged_sparse_array_release*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE_DEFINE(C, T)                                                                                                             \
/*hint: have THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INTERNAL_DEFINE(C, T) before THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE_DEFINE(C, T)*/                                       \
void THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint64_t index)                                                 \
{                                                                                                                                                                          \
    if (paged_sparse_array == NULL)                                                                                                                                        \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_054: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/                   \
        LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, uint64_t index=%" PRIu64 "",                              \
            paged_sparse_array, index);                                                                                                                                    \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array);                         \
        uint64_t max_size = array->wrapped_array->max_size;                                                                                                                \
        uint64_t page_size = array->wrapped_array->page_size;                                                                                                              \
        if (index >= max_size)                                                                                                                                             \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_055: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/ \
            LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, uint64_t index=%" PRIu64 " out of bound, max_size=%" PRIu64 ", page_size=%" PRIu64 "", \
                paged_sparse_array, index, max_size, page_size);                                                                                                           \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            uint64_t page_index = index / page_size;                                                                                                                       \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_056: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) for the page with index index / page_size. ]*/ \
            if (THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_BEGIN_PAGE_USE_INTERNAL_NAME(C)(array, page_index))                                                                      \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_057: [ If THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) returns true: ]*/                                \
                THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)* slot;                                                                                                 \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_058: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call PAGED_SPARSE_ARRAY_GET(T) with the paged sparse array and index to get the slot of the element. ]*/ \
                PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_LL_GET(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->wrapped_array, index, &slot); \
                if (get_result != PAGED_SPARSE_ARRAY_GET_OK)                                                                                                               \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_059: [ If PAGED_SPARSE_ARRAY_GET(T) fails: ]*/                                                          \
                    LogError("failure in PAGED_SPARSE_ARRAY_GET(" MU_TOSTRING(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)) ")(index=%" PRIu64 ")=%" PRI_MU_ENUM " for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, page_index=%" PRIu64 "", \
                        index, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, get_result), paged_sparse_array, page_index);                                                  \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_088: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/ \
                    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 1);                                                                 \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_089: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/                                      \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_060: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall change the state of the slot from THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING using interlocked_compare_exchange. ]*/ \
                    if (interlocked_compare_exchange(&slot->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED) == THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED) \
                    {                                                                                                                                                      \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_061: [ If the state of the slot was THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED: ]*/        \
                        if (array->item_dispose_func != NULL)                                                                                                              \
                        {                                                                                                                                                  \
                            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call item_dispose_func with a pointer to the item of the slot. ]*/ \
                            array->item_dispose_func(&slot->item);                                                                                                         \
                        }                                                                                                                                                  \
                        else                                                                                                                                               \
                        {                                                                                                                                                  \
                            /* do nothing, there is no item dispose function */                                                                                            \
                        }                                                                                                                                                  \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_063: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall set the state of the slot to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED using interlocked operations. ]*/ \
                        (void)interlocked_exchange(&slot->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED);                                                 \
                        /* the use of the page by the element ends together with the use of the page by this call */                                                       \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_064: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 2. ]*/ \
                        THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 2);                                                             \
                    }                                                                                                                                                      \
                    else                                                                                                                                                   \
                    {                                                                                                                                                      \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_065: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/ \
                        THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 1);                                                             \
                    }                                                                                                                                                      \
                }                                                                                                                                                          \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_066: [ If THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) returns false, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/ \
                THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 1);                                                                     \
            }                                                                                                                                                              \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
}

/*introduces a function definition for thread_safe_paged_sparse_array_get*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_GET_DEFINE(C, T)                                                                                                                 \
/*hint: have THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INTERNAL_DEFINE(C, T) before THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_GET_DEFINE(C, T)*/                                           \
PAGED_SPARSE_ARRAY_GET_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_GET(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint64_t index, T** item)                  \
{                                                                                                                                                                          \
    PAGED_SPARSE_ARRAY_GET_RESULT result;                                                                                                                                  \
    if (                                                                                                                                                                   \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_067: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/ \
        (paged_sparse_array == NULL) ||                                                                                                                                    \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_068: [ If item is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/ \
        (item == NULL)                                                                                                                                                     \
        )                                                                                                                                                                  \
    {                                                                                                                                                                      \
        result = PAGED_SPARSE_ARRAY_GET_INVALID_ARGS;                                                                                                                      \
        LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, uint64_t index=%" PRIu64 ", " MU_TOSTRING(T) "** item=%p, result=%" PRI_MU_ENUM "", \
            paged_sparse_array, index, item, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, result));                                                                        \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array);                         \
        uint64_t max_size = array->wrapped_array->max_size;                                                                                                                \
        uint64_t page_size = array->wrapped_array->page_size;                                                                                                              \
        if (index >= max_size)                                                                                                                                             \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_069: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/ \
            result = PAGED_SPARSE_ARRAY_GET_INVALID_ARGS;                                                                                                                  \
            LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, uint64_t index=%" PRIu64 " out of bound, max_size=%" PRIu64 ", page_size=%" PRIu64 ", " MU_TOSTRING(T) "** item=%p, result=%" PRI_MU_ENUM "", \
                paged_sparse_array, index, max_size, page_size, item, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, result));                                               \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            uint64_t page_index = index / page_size;                                                                                                                       \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_070: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) for the page with index index / page_size. ]*/ \
            if (!THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_BEGIN_PAGE_USE_INTERNAL_NAME(C)(array, page_index))                                                                     \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_071: [ If THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) returns false, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/ \
                result = PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED;                                                                                                             \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)* slot;                                                                                                 \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_072: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call PAGED_SPARSE_ARRAY_GET(T) with the paged sparse array and index to get the slot of the element. ]*/ \
                result = PAGED_SPARSE_ARRAY_LL_GET(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->wrapped_array, index, &slot);                               \
                if (result != PAGED_SPARSE_ARRAY_GET_OK)                                                                                                                   \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_073: [ If PAGED_SPARSE_ARRAY_GET(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall return the result of PAGED_SPARSE_ARRAY_GET(T). ]*/ \
                    LogError("failure in PAGED_SPARSE_ARRAY_GET(" MU_TOSTRING(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)) ")(index=%" PRIu64 ") for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, page_index=%" PRIu64 ", result=%" PRI_MU_ENUM "", \
                        index, paged_sparse_array, page_index, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, result));                                                      \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    if (interlocked_add(&slot->state, 0) != THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED)                                                           \
                    {                                                                                                                                                      \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_074: [ If the state of the slot is not THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/ \
                        result = PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED;                                                                                                     \
                    }                                                                                                                                                      \
                    else                                                                                                                                                   \
                    {                                                                                                                                                      \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_075: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall store a pointer to the item of the slot in item and return PAGED_SPARSE_ARRAY_GET_OK. ]*/ \
                        *item = &slot->item;                                                                                                                               \
                        result = PAGED_SPARSE_ARRAY_GET_OK;                                                                                                                \
                    }                                                                                                                                                      \
                }                                                                                                                                                          \
            }                                                                                                                                                              \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_076: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 before returning, also when THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) returns false. ]*/ \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 1);                                                                         \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}

#endif /*THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_H*/
