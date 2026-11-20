// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_H
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_H

#ifdef __cplusplus
#include <cinttypes>
#include <cstddef>
#include <cstring>
#else // __cplusplus
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
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
 * THREAD_SAFE_PAGED_SPARSE_ARRAY wraps a PAGED_SPARSE_ARRAY (see paged_sparse_array_ll.h) so that ALLOCATE, RELEASE and ACQUIRE_ITEM can be
 * called concurrently from multiple threads.
 * The elements of the wrapped paged sparse array are slots. A slot holds an item of type T and the state of the slot, the state is changed only
 * with interlocked operations. ALLOCATE copies the item of the caller in and ACQUIRE_ITEM copies the item out to the caller, with the item copy function
 * (for a THANDLE, an increment of its reference count), and RELEASE disposes the item with the item dispose function. The caller never gets a
 * pointer into a page, so a page can be freed while the caller uses the items it got.
 * Each page has a page guard: a count of the allocated elements of the page, a page state (THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE) and a lock.
 * - The lock of the page protects the page: ALLOCATE and ACQUIRE_ITEM hold it in shared mode while they look at a slot of the page, and it is held
 *   in exclusive mode to allocate the page and to start freeing it, so the page is not freed while a call looks at one of its slots.
 * - The state of the slot protects the item: ALLOCATE changes it from NOT_ALLOCATED to ALLOCATING, copies the item in and then sets it to ALLOCATED,
 *   so ACQUIRE_ITEM does not copy an item that is not completely copied in. ACQUIRE_ITEM adds 1 to the state of an allocated slot while it copies the item out, and
 *   RELEASE adds RELEASING to it and waits for the ACQUIRE_ITEM calls that copy the item out before it disposes the item, so an item is not disposed while it is
 *   copied.
 * - RELEASE releases the element without acquiring the lock: the released element keeps the page allocated until RELEASE decrements the element
 *   count, which is the last time RELEASE uses the page. The RELEASE that makes the element count 0 frees the page, which acquires the lock in
 *   exclusive mode.
 * - A page is allocated by ALLOCATE while holding the lock of the page in exclusive mode, and the element is allocated before the lock is released.
 */

/*states of a slot*/
/*the state of an allocated slot is THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED plus the number of ACQUIRE_ITEM calls that copy its item out*/
/*RELEASE adds THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING to it, and ACQUIRE_ITEM does not copy the item of a slot that has it*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED INT32_C(0)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATING INT32_C(-1)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED INT32_C(1)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING (INT32_C(1) << 30)

/*states of a page*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_VALUES \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED, \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING

MU_DEFINE_ENUM(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_VALUES)

/*the array has one page guard for each page of the wrapped paged sparse array, the page guards exist as long as the array exists*/
typedef struct THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD_TAG
{
    /*count of allocated elements in the page, incremented by ALLOCATE (holding the lock) and decremented by RELEASE (without the lock)*/
    volatile_atomic int64_t element_count;
    /*THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE, the calls that allocate the page wait on it while the page is being freed*/
    volatile_atomic int32_t page_state;
    /*acquired in shared mode by ALLOCATE and ACQUIRE_ITEM of an element of the page, and in exclusive mode to allocate the page or to start freeing it*/
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

/*custom item copy function type, dst is not initialized when it is called (for a THANDLE, the function calls THANDLE_INITIALIZE)*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_COPY_FUNC(T) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_COPY_FUNC_, T)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_ITEM_COPY_FUNC_TYPE(T) typedef void (*THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_COPY_FUNC(T))(T* dst, T* src)

/*custom item dispose function type (for a THANDLE, the function calls THANDLE_ASSIGN with NULL)*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC_, T)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_ITEM_DISPOSE_FUNC_TYPE(T) typedef void (*THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T))(T* item)

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T) introduces the base type that holds the thread safe paged sparse array*/
/*it also introduces the slot type and the wrapped PAGED_SPARSE_ARRAY of slots, with its THANDLE declaration*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(T)                                                                                                               \
THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_ITEM_COPY_FUNC_TYPE(T);                                                                                                              \
THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_ITEM_DISPOSE_FUNC_TYPE(T);                                                                                                           \
typedef struct THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STRUCT_TYPE_NAME_TAG(T)                                                                                                 \
{                                                                                                                                                                          \
    volatile_atomic int32_t state; /*THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_...*/                                                                                       \
    T item;                                                                                                                                                                \
} THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T);                                                                                                                     \
PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T))                                                                                 \
THANDLE_LL_TYPE_DECLARE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)), PAGED_SPARSE_ARRAY_TYPEDEF_NAME(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T))); \
typedef struct THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_TYPE_NAME_TAG(T)                                                                                                      \
{                                                                                                                                                                          \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_WRAPPED_ARRAY_LL(T) paged_sparse_array;                                                                                                 \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_COPY_FUNC(T) item_copy_func;                                                                                                    \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func;                                                                                              \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD page_guards[]; /*one for each page of paged_sparse_array*/                                                                   \
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
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ACQUIRE_ITEM_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ACQUIRE_ITEM_, C)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ACQUIRE_ITEM(C) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ACQUIRE_ITEM_NAME(C)

/*introduces names for the internal functions*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_ELEMENT_INTERNAL_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_ELEMENT_INTERNAL_, C)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TRY_FREE_PAGE_INTERNAL_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TRY_FREE_PAGE_INTERNAL_, C)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_, C)

/*introduces a function declaration for thread_safe_paged_sparse_array_create*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T), THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE(C), uint64_t, max_size, uint64_t, page_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_COPY_FUNC(T), item_copy_func, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T), item_dispose_func);

/*introduces a function declaration for thread_safe_paged_sparse_array_allocate*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T), paged_sparse_array, uint64_t, index, T*, item);

/*introduces a function declaration for thread_safe_paged_sparse_array_release*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, void, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T), paged_sparse_array, uint64_t, index);

/*introduces a function declaration for thread_safe_paged_sparse_array_acquire_item*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ACQUIRE_ITEM_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, PAGED_SPARSE_ARRAY_GET_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ACQUIRE_ITEM(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T), paged_sparse_array, uint64_t, index, T*, item);

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
        uint64_t max_size = paged_sparse_array->paged_sparse_array->max_size;                                                                                              \
        uint64_t page_size = paged_sparse_array->paged_sparse_array->page_size;                                                                                            \
        uint64_t page_count = paged_sparse_array->paged_sparse_array->page_count;                                                                                          \
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
                    THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)* slot;                                                                                             \
                    /* all the indexes of an allocated page are allocated in the paged sparse array, so PAGED_SPARSE_ARRAY_GET(T) does not fail */                         \
                    (void)PAGED_SPARSE_ARRAY_LL_GET(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(paged_sparse_array->paged_sparse_array, index, &slot);            \
                    if (interlocked_add(&slot->state, 0) == THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED)                                                           \
                    {                                                                                                                                                      \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_012: [ If paged_sparse_array->item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call paged_sparse_array->item_dispose_func with a pointer to the item of each slot whose state is THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED in the pages whose page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED. ]*/ \
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
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_013: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call srw_lock_ll_deinit for lock in each page guard of paged_sparse_array->page_guards. ]*/ \
            srw_lock_ll_deinit(&page_guard->lock);                                                                                                                         \
        }                                                                                                                                                                  \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_014: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall release paged_sparse_array->paged_sparse_array. ]*/          \
        PAGED_SPARSE_ARRAY_LL_ASSIGN(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(&paged_sparse_array->paged_sparse_array, NULL);                                  \
    }                                                                                                                                                                      \
}

/*introduces a function definition for thread_safe_paged_sparse_array_create*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE_DEFINE(C, T)                                                                                                              \
THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE(C)(uint64_t max_size, uint64_t page_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_COPY_FUNC(T) item_copy_func, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func) \
{                                                                                                                                                                          \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* result;                                                                                                                \
    if ((item_copy_func == NULL) != (item_dispose_func == NULL))                                                                                                           \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_109: [ If only one of item_copy_func and item_dispose_func is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/ \
        LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", item_copy_func=%p, item_dispose_func=%p", \
            max_size, page_size, item_copy_func, item_dispose_func);                                                                                                       \
        result = NULL;                                                                                                                                                     \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_001: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call PAGED_SPARSE_ARRAY_CREATE(T) with max_size, page_size and NULL as item_dispose_func to create the paged sparse array of slots. ]*/ \
        THREAD_SAFE_PAGED_SPARSE_ARRAY_WRAPPED_ARRAY_LL(T) paged_sparse_array = PAGED_SPARSE_ARRAY_LL_CREATE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(max_size, page_size, NULL); \
        if (paged_sparse_array == NULL)                                                                                                                                    \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_002: [ If PAGED_SPARSE_ARRAY_CREATE(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/ \
            LogError("failure in PAGED_SPARSE_ARRAY_CREATE(" MU_TOSTRING(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)) ")(uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", NULL) for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) ", item_copy_func=%p, item_dispose_func=%p", \
                max_size, page_size, item_copy_func, item_dispose_func);                                                                                                   \
            result = NULL;                                                                                                                                                 \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            uint64_t page_count = paged_sparse_array->page_count;                                                                                                          \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_003: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX to allocate result with one page guard in result->page_guards for each page of the paged sparse array. ]*/ \
            result = THANDLE_MALLOC_FLEX(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_NAME(C), page_count, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD)); \
            if (result == NULL)                                                                                                                                            \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [ If there are any other errors, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/ \
                LogError("failure in THANDLE_MALLOC_FLEX(" THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) ")(page_count=%" PRIu64 ", sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD)=%zu), uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", item_copy_func=%p, item_dispose_func=%p", \
                    page_count, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD), max_size, page_size, item_copy_func, item_dispose_func);                                \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                uint64_t initialized_lock_count;                                                                                                                           \
                for (initialized_lock_count = 0; initialized_lock_count < page_count; initialized_lock_count++)                                                            \
                {                                                                                                                                                          \
                    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD* page_guard = &result->page_guards[initialized_lock_count];                                                  \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_004: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall set element_count to 0 and page_state to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED in each page guard of result->page_guards. ]*/ \
                    (void)interlocked_exchange_64(&page_guard->element_count, 0);                                                                                          \
                    (void)interlocked_exchange(&page_guard->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED);                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_005: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call srw_lock_ll_init to initialize lock in each page guard of result->page_guards. ]*/ \
                    int lock_init_result = srw_lock_ll_init(&page_guard->lock);                                                                                            \
                    if (lock_init_result != 0)                                                                                                                             \
                    {                                                                                                                                                      \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [ If there are any other errors, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/ \
                        LogError("failure in srw_lock_ll_init(&page_guards[%" PRIu64 "].lock)=%d for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) ", uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", page_count=%" PRIu64 ", item_copy_func=%p, item_dispose_func=%p", \
                            initialized_lock_count, lock_init_result, max_size, page_size, page_count, item_copy_func, item_dispose_func);                                 \
                        break;                                                                                                                                             \
                    }                                                                                                                                                      \
                    else                                                                                                                                                   \
                    {                                                                                                                                                      \
                        /* do nothing */                                                                                                                                   \
                    }                                                                                                                                                      \
                }                                                                                                                                                          \
                if (initialized_lock_count < page_count)                                                                                                                   \
                {                                                                                                                                                          \
                    for (uint64_t i = 0; i < initialized_lock_count; i++)                                                                                                  \
                    {                                                                                                                                                      \
                        srw_lock_ll_deinit(&result->page_guards[i].lock);                                                                                                  \
                    }                                                                                                                                                      \
                    THANDLE_FREE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(result);                                                                                  \
                    result = NULL;                                                                                                                                         \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_007: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store item_dispose_func in result->item_dispose_func. ]*/ \
                    result->item_dispose_func = item_dispose_func;                                                                                                         \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_110: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store item_copy_func in result->item_copy_func. ]*/ \
                    result->item_copy_func = item_copy_func;                                                                                                               \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_008: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store the paged sparse array in result->paged_sparse_array. ]*/ \
                    PAGED_SPARSE_ARRAY_LL_INITIALIZE_MOVE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(&result->paged_sparse_array, &paged_sparse_array);          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_009: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/          \
                }                                                                                                                                                          \
            }                                                                                                                                                              \
            if (result == NULL)                                                                                                                                            \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [ If there are any other errors, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/ \
                PAGED_SPARSE_ARRAY_LL_ASSIGN(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(&paged_sparse_array, NULL);                                              \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                /* do nothing, the paged sparse array was moved to result */                                                                                               \
            }                                                                                                                                                              \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}

/*introduces the definitions of the internal functions, they are used by ALLOCATE and RELEASE*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INTERNAL_DEFINE(C, T)                                                                                                            \
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TRY_FREE_PAGE_INTERNAL_NAME(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array, uint64_t page_index)                   \
{                                                                                                                                                                          \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD* page_guard = &array->page_guards[page_index];                                                                               \
    bool is_freeing;                                                                                                                                                       \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_021: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall acquire page_guard->lock in exclusive mode. ]*/            \
    srw_lock_ll_acquire_exclusive(&page_guard->lock);                                                                                                                      \
    /* the calls that allocate an element hold the lock, so the element count can only decrease while it is checked, */                                                    \
    /* and every RELEASE that makes it 0 calls THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) after that */                                                               \
    if (                                                                                                                                                                   \
        (interlocked_add(&page_guard->page_state, 0) == THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED) &&                                                            \
        (interlocked_add_64(&page_guard->element_count, 0) == 0)                                                                                                           \
        )                                                                                                                                                                  \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_022: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED and page_guard->element_count is 0: ]*/ \
        /* the calls that acquire the lock from now on see THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING and do not use the wrapped paged sparse array */              \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_024: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall set page_guard->page_state to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING. ]*/ \
        (void)interlocked_exchange(&page_guard->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING);                                                            \
        is_freeing = true;                                                                                                                                                 \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* do nothing, another call freed the page, is freeing it or allocated an element in it */                                                                         \
        is_freeing = false;                                                                                                                                                \
    }                                                                                                                                                                      \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release page_guard->lock. ]*/                              \
    srw_lock_ll_release_exclusive(&page_guard->lock);                                                                                                                      \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_026: [ If page_guard->page_state was set to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING: ]*/                      \
    if (is_freeing)                                                                                                                                                        \
    {                                                                                                                                                                      \
        uint64_t page_size = array->paged_sparse_array->page_size;                                                                                                         \
        uint64_t first_index = page_index * page_size;                                                                                                                     \
        uint64_t end_index = THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_END_INDEX(first_index, page_size, array->paged_sparse_array->max_size);                                   \
        for (uint64_t index = first_index; index < end_index; index++)                                                                                                     \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_027: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall call PAGED_SPARSE_ARRAY_RELEASE(T) on array->paged_sparse_array for each index of the page. ]*/ \
            PAGED_SPARSE_ARRAY_LL_RELEASE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->paged_sparse_array, index);                                          \
        }                                                                                                                                                                  \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_077: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall call InterlockedHL_SetAndWakeAll to set page_guard->page_state to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED and wake the calls waiting for it to change. ]*/ \
        (void)InterlockedHL_SetAndWakeAll(&page_guard->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED);                                               \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* do nothing, the page is not allocated or has allocated elements */                                                                                              \
    }                                                                                                                                                                      \
}                                                                                                                                                                          \
static PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_ELEMENT_INTERNAL_NAME(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array, uint64_t page_index, uint64_t index, T* item) \
{                                                                                                                                                                          \
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result;                                                                                                                             \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD* page_guard = &array->page_guards[page_index];                                                                               \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)* slot;                                                                                                             \
    /* all the indexes of an allocated page are allocated in the paged sparse array, so PAGED_SPARSE_ARRAY_GET(T) does not fail */                                         \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_049: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall call PAGED_SPARSE_ARRAY_GET(T) with array->paged_sparse_array and index to get the slot of the element. ]*/ \
    (void)PAGED_SPARSE_ARRAY_LL_GET(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->paged_sparse_array, index, &slot);                                         \
    /* ACQUIRE_ITEM and RELEASE do not use the item of the slot while its state is THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATING */                                 \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_112: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall change the state of the slot from THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATING using interlocked_compare_exchange. ]*/ \
    int32_t slot_state = interlocked_compare_exchange(&slot->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATING, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED); \
    if (slot_state != THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED)                                                                                             \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_052: [ If the state of the slot was not THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED. ]*/ \
        result = PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED;                                                                                                            \
        LogError("Element is already allocated in " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " array=%p, uint64_t index=%" PRIu64 ", page_index=%" PRIu64 ", slot_state=%" PRId32 ", result=%" PRI_MU_ENUM "", \
            array, index, page_index, slot_state, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result));                                                              \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        if (array->item_copy_func != NULL)                                                                                                                                 \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_113: [ If array->item_copy_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall call array->item_copy_func with a pointer to the item of the slot as dst and item as src. ]*/ \
            array->item_copy_func(&slot->item, item);                                                                                                                      \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_114: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall copy item to the item of the slot. ]*/ \
            (void)memcpy((void*)&slot->item, (void*)item, sizeof(T));                                                                                                      \
        }                                                                                                                                                                  \
        /* the page is not freed while the lock is held, and from now on the allocated element keeps the page allocated */                                                 \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_098: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall increment page_guard->element_count. ]*/            \
        (void)interlocked_increment_64(&page_guard->element_count);                                                                                                        \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_115: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall set the state of the slot to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED. ]*/ \
        (void)interlocked_exchange(&slot->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED);                                                                     \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_116: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) shall succeed and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/ \
        result = PAGED_SPARSE_ARRAY_ALLOCATE_OK;                                                                                                                           \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}                                                                                                                                                                          \
static PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array, uint64_t page_index, uint64_t index, T* item) \
{                                                                                                                                                                          \
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result;                                                                                                                             \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD* page_guard = &array->page_guards[page_index];                                                                               \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_030: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall acquire page_guard->lock in exclusive mode. ]*/            \
    srw_lock_ll_acquire_exclusive(&page_guard->lock);                                                                                                                      \
    /* a call that started to free the page before the lock was acquired frees it without holding the lock */                                                              \
    INTERLOCKED_HL_RESULT wait_result = INTERLOCKED_HL_OK;                                                                                                                 \
    int32_t page_state = interlocked_add(&page_guard->page_state, 0);                                                                                                      \
    if (page_state == THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING)                                                                                                   \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_031: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call InterlockedHL_WaitForNotValue to wait for page_guard->page_state to change. ]*/ \
        wait_result = InterlockedHL_WaitForNotValue(&page_guard->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING, UINT32_MAX);                               \
        page_state = interlocked_add(&page_guard->page_state, 0);                                                                                                          \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* do nothing, the page is not being freed */                                                                                                                      \
    }                                                                                                                                                                      \
    if (wait_result != INTERLOCKED_HL_OK)                                                                                                                                  \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_032: [ If InterlockedHL_WaitForNotValue fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/ \
        result = PAGED_SPARSE_ARRAY_ALLOCATE_ERROR;                                                                                                                        \
        LogError("failure in InterlockedHL_WaitForNotValue(&page_guards[%" PRIu64 "].page_state, %" PRI_MU_ENUM ", UINT32_MAX)=%" PRI_MU_ENUM " for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) ", page_state=%" PRI_MU_ENUM ", element_count=%" PRId64 ", index=%" PRIu64 ", page_size=%" PRIu64 ", result=%" PRI_MU_ENUM "", \
            page_index, MU_ENUM_VALUE(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING), MU_ENUM_VALUE(INTERLOCKED_HL_RESULT, wait_result), \
            MU_ENUM_VALUE(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE, (THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE)page_state), interlocked_add_64(&page_guard->element_count, 0), \
            index, array->paged_sparse_array->page_size, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result));                                                       \
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
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_033: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED: ]*/                \
            uint64_t page_size = array->paged_sparse_array->page_size;                                                                                                     \
            uint64_t first_index = page_index * page_size;                                                                                                                 \
            uint64_t end_index = THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_END_INDEX(first_index, page_size, array->paged_sparse_array->max_size);                               \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)* slot;                                                                                                     \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_034: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call PAGED_SPARSE_ARRAY_ALLOCATE(T) on array->paged_sparse_array for each index of the page. ]*/ \
            PAGED_SPARSE_ARRAY_ALLOCATE_RESULT allocate_result = PAGED_SPARSE_ARRAY_LL_ALLOCATE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->paged_sparse_array, first_index, &slot); \
            if (allocate_result != PAGED_SPARSE_ARRAY_ALLOCATE_OK)                                                                                                         \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_036: [ If PAGED_SPARSE_ARRAY_ALLOCATE(T) fails for the first index of the page, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/ \
                result = PAGED_SPARSE_ARRAY_ALLOCATE_ERROR;                                                                                                                \
                LogError("failure in PAGED_SPARSE_ARRAY_ALLOCATE(" MU_TOSTRING(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)) ")(index=%" PRIu64 ")=%" PRI_MU_ENUM " for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) ", page_index=%" PRIu64 ", first_index=%" PRIu64 ", end_index=%" PRIu64 ", index=%" PRIu64 ", result=%" PRI_MU_ENUM "", \
                    first_index, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, allocate_result), page_index, first_index, end_index, index, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result)); \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                for (uint64_t slot_index = first_index; slot_index < end_index; slot_index++)                                                                              \
                {                                                                                                                                                          \
                    if (slot_index == first_index)                                                                                                                         \
                    {                                                                                                                                                      \
                        /* do nothing, slot is the slot of the first index */                                                                                              \
                    }                                                                                                                                                      \
                    else                                                                                                                                                   \
                    {                                                                                                                                                      \
                        /* the first index allocated the page of the paged sparse array, so PAGED_SPARSE_ARRAY_ALLOCATE(T) does not fail for the other indexes of the page */ \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_034: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call PAGED_SPARSE_ARRAY_ALLOCATE(T) on array->paged_sparse_array for each index of the page. ]*/ \
                        (void)PAGED_SPARSE_ARRAY_LL_ALLOCATE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->paged_sparse_array, slot_index, &slot);           \
                    }                                                                                                                                                      \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_035: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall set the state of each slot of the page to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED. ]*/ \
                    (void)interlocked_exchange(&slot->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED);                                                     \
                }                                                                                                                                                          \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_038: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall set page_guard->page_state to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED. ]*/ \
                (void)interlocked_exchange(&page_guard->page_state, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED);                                                  \
                result = PAGED_SPARSE_ARRAY_ALLOCATE_OK;                                                                                                                   \
            }                                                                                                                                                              \
        }                                                                                                                                                                  \
        if (result != PAGED_SPARSE_ARRAY_ALLOCATE_OK)                                                                                                                      \
        {                                                                                                                                                                  \
            /* do nothing, the error was logged */                                                                                                                         \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* the element is allocated before the lock is released, so a new page is not freed before the caller uses it */                                               \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_099: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) and return its result. ]*/ \
            result = THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_ELEMENT_INTERNAL_NAME(C)(array, page_index, index, item);                                                  \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [ In all cases, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) shall release page_guard->lock before returning. ]*/ \
    srw_lock_ll_release_exclusive(&page_guard->lock);                                                                                                                      \
    return result;                                                                                                                                                         \
}

/*introduces a function definition for thread_safe_paged_sparse_array_allocate*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_DEFINE(C, T)                                                                                                            \
/*hint: have THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INTERNAL_DEFINE(C, T) before THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_DEFINE(C, T)*/                                      \
PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint64_t index, T* item)         \
{                                                                                                                                                                          \
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result;                                                                                                                             \
    if (                                                                                                                                                                   \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_041: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/ \
        (paged_sparse_array == NULL) ||                                                                                                                                    \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_111: [ If item is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/ \
        (item == NULL)                                                                                                                                                     \
        )                                                                                                                                                                  \
    {                                                                                                                                                                      \
        result = PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS;                                                                                                                 \
        LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, uint64_t index=%" PRIu64 ", " MU_TOSTRING(T) "* item=%p, result=%" PRI_MU_ENUM "", \
            paged_sparse_array, index, item, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result));                                                                   \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array);                         \
        uint64_t max_size = array->paged_sparse_array->max_size;                                                                                                           \
        uint64_t page_size = array->paged_sparse_array->page_size;                                                                                                         \
        if (index >= max_size)                                                                                                                                             \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_043: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/ \
            result = PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS;                                                                                                             \
            LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, uint64_t index=%" PRIu64 " out of bound, max_size=%" PRIu64 ", page_size=%" PRIu64 ", " MU_TOSTRING(T) "* item=%p, result=%" PRI_MU_ENUM "", \
                paged_sparse_array, index, max_size, page_size, item, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result));                                          \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            uint64_t page_index = index / page_size;                                                                                                                       \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD* page_guard = &array->page_guards[page_index];                                                                       \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_093: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall acquire page_guard->lock in shared mode, where page_guard is the page guard of the page with index index / page_size. ]*/ \
            srw_lock_ll_acquire_shared(&page_guard->lock);                                                                                                                 \
            if (interlocked_add(&page_guard->page_state, 0) == THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED)                                                        \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_094: [ If page_guard->page_state is THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED: ]*/                \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_095: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T) and return its result. ]*/ \
                result = THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_ELEMENT_INTERNAL_NAME(C)(array, page_index, index, item);                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_096: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release page_guard->lock in shared mode. ]*/        \
                srw_lock_ll_release_shared(&page_guard->lock);                                                                                                             \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_045: [ If page_guard->page_state is not THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED: ]*/            \
                /* the lock cannot be changed from shared mode to exclusive mode, so it is released and acquired again in exclusive mode */                                \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_097: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release page_guard->lock in shared mode. ]*/        \
                srw_lock_ll_release_shared(&page_guard->lock);                                                                                                             \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_047: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T) and return its result. ]*/ \
                result = THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C)(array, page_index, index, item);                                                 \
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
        uint64_t max_size = array->paged_sparse_array->max_size;                                                                                                           \
        uint64_t page_size = array->paged_sparse_array->page_size;                                                                                                         \
        if (index >= max_size)                                                                                                                                             \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_055: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/ \
            LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, uint64_t index=%" PRIu64 " out of bound, max_size=%" PRIu64 ", page_size=%" PRIu64 "", \
                paged_sparse_array, index, max_size, page_size);                                                                                                           \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            uint64_t page_index = index / page_size;                                                                                                                       \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD* page_guard = &array->page_guards[page_index];                                                                       \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)* slot;                                                                                                     \
            /* the lock of the page is not acquired: the released element keeps the page allocated until the element count is decremented, */                              \
            /* which is the last time RELEASE uses the page */                                                                                                             \
            /* all the indexes of an allocated page are allocated in the paged sparse array, so PAGED_SPARSE_ARRAY_GET(T) does not fail */                                 \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_058: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call PAGED_SPARSE_ARRAY_GET(T) with paged_sparse_array->paged_sparse_array and index to get the slot of the element. ]*/ \
            (void)PAGED_SPARSE_ARRAY_LL_GET(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->paged_sparse_array, index, &slot);                                 \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_119: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall add THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING to the state of the slot using interlocked_compare_exchange if the state is greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED and lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING. ]*/ \
            int32_t slot_state = interlocked_add(&slot->state, 0);                                                                                                         \
            while (                                                                                                                                                        \
                (slot_state >= THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED) &&                                                                                     \
                (slot_state < THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING)                                                                                         \
                )                                                                                                                                                          \
            {                                                                                                                                                              \
                int32_t current_slot_state = interlocked_compare_exchange(&slot->state, slot_state + THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING, slot_state);     \
                if (current_slot_state == slot_state)                                                                                                                      \
                {                                                                                                                                                          \
                    break;                                                                                                                                                 \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    /* an ACQUIRE_ITEM started or ended copying the item, try again with the new state */                                                                  \
                    slot_state = current_slot_state;                                                                                                                       \
                }                                                                                                                                                          \
            }                                                                                                                                                              \
            if (                                                                                                                                                           \
                (slot_state < THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED) ||                                                                                      \
                (slot_state >= THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING)                                                                                        \
                )                                                                                                                                                          \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_124: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/                               \
                LogError("Element is not allocated, is being allocated or is being released by another call in " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, uint64_t index=%" PRIu64 ", page_index=%" PRIu64 ", slot_state=%" PRId32 "", \
                    paged_sparse_array, index, page_index, slot_state);                                                                                                    \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_121: [ If the state of the slot was greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED and lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING: ]*/ \
                INTERLOCKED_HL_RESULT wait_result;                                                                                                                         \
                if (slot_state == THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED)                                                                                     \
                {                                                                                                                                                          \
                    /* do nothing, no ACQUIRE_ITEM copies the item */                                                                                                      \
                    wait_result = INTERLOCKED_HL_OK;                                                                                                                       \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    /* each ACQUIRE_ITEM that copies the item subtracts 1 from the state when it is done, and the last one wakes this call */                              \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_122: [ If the state of the slot was not THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call InterlockedHL_WaitForValue to wait for the state of the slot to be THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING + THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED. ]*/ \
                    wait_result = InterlockedHL_WaitForValue(&slot->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING + THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, UINT32_MAX); \
                }                                                                                                                                                          \
                if (wait_result != INTERLOCKED_HL_OK)                                                                                                                      \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_123: [ If InterlockedHL_WaitForValue fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall subtract THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING from the state of the slot and return. ]*/ \
                    /* the element stays allocated */                                                                                                                      \
                    int32_t new_slot_state = interlocked_add(&slot->state, -THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING);                                          \
                    LogError("failure in InterlockedHL_WaitForValue(&slot->state, %" PRId32 ", UINT32_MAX)=%" PRI_MU_ENUM " for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, uint64_t index=%" PRIu64 ", page_index=%" PRIu64 ", slot_state=%" PRId32 ", new_slot_state=%" PRId32 "", \
                        THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING + THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, MU_ENUM_VALUE(INTERLOCKED_HL_RESULT, wait_result), \
                        paged_sparse_array, index, page_index, slot_state, new_slot_state);                                                                                \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    if (array->item_dispose_func != NULL)                                                                                                                  \
                    {                                                                                                                                                      \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [ If paged_sparse_array->item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call paged_sparse_array->item_dispose_func with a pointer to the item of the slot. ]*/ \
                        array->item_dispose_func(&slot->item);                                                                                                             \
                    }                                                                                                                                                      \
                    else                                                                                                                                                   \
                    {                                                                                                                                                      \
                        /* do nothing, there is no item dispose function */                                                                                                \
                    }                                                                                                                                                      \
                    /* the slot is not allocated again before its item is disposed */                                                                                      \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_063: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall set the state of the slot to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED. ]*/ \
                    (void)interlocked_exchange(&slot->state, THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED);                                                     \
                    /* this is the last time this call uses the page without holding the lock */                                                                           \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_101: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall decrement page_guard->element_count, where page_guard is the page guard of the page with index index / page_size. ]*/ \
                    if (interlocked_decrement_64(&page_guard->element_count) == 0)                                                                                         \
                    {                                                                                                                                                      \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_103: [ If the element_count became 0, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T). ]*/ \
                        THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TRY_FREE_PAGE_INTERNAL_NAME(C)(array, page_index);                                                               \
                    }                                                                                                                                                      \
                    else                                                                                                                                                   \
                    {                                                                                                                                                      \
                        /* do nothing, the page has other allocated elements */                                                                                            \
                    }                                                                                                                                                      \
                }                                                                                                                                                          \
            }                                                                                                                                                              \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
}

/*introduces a function definition for thread_safe_paged_sparse_array_acquire_item*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ACQUIRE_ITEM_DEFINE(C, T)                                                                                                        \
PAGED_SPARSE_ARRAY_GET_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ACQUIRE_ITEM(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint64_t index, T* item)          \
{                                                                                                                                                                          \
    PAGED_SPARSE_ARRAY_GET_RESULT result;                                                                                                                                  \
    if (                                                                                                                                                                   \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_067: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/ \
        (paged_sparse_array == NULL) ||                                                                                                                                    \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_068: [ If item is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/ \
        (item == NULL)                                                                                                                                                     \
        )                                                                                                                                                                  \
    {                                                                                                                                                                      \
        result = PAGED_SPARSE_ARRAY_GET_INVALID_ARGS;                                                                                                                      \
        LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, uint64_t index=%" PRIu64 ", " MU_TOSTRING(T) "* item=%p, result=%" PRI_MU_ENUM "", \
            paged_sparse_array, index, item, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, result));                                                                        \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array);                         \
        uint64_t max_size = array->paged_sparse_array->max_size;                                                                                                           \
        uint64_t page_size = array->paged_sparse_array->page_size;                                                                                                         \
        if (index >= max_size)                                                                                                                                             \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_069: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/ \
            result = PAGED_SPARSE_ARRAY_GET_INVALID_ARGS;                                                                                                                  \
            LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, uint64_t index=%" PRIu64 " out of bound, max_size=%" PRIu64 ", page_size=%" PRIu64 ", " MU_TOSTRING(T) "* item=%p, result=%" PRI_MU_ENUM "", \
                paged_sparse_array, index, max_size, page_size, item, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, result));                                               \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_GUARD* page_guard = &array->page_guards[index / page_size];                                                                \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_104: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall acquire page_guard->lock in shared mode, where page_guard is the page guard of the page with index index / page_size. ]*/ \
            srw_lock_ll_acquire_shared(&page_guard->lock);                                                                                                                 \
            if (interlocked_add(&page_guard->page_state, 0) != THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED)                                                        \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_071: [ If page_guard->page_state is not THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/ \
                result = PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED;                                                                                                             \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)* slot;                                                                                                 \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_072: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall call PAGED_SPARSE_ARRAY_GET(T) with paged_sparse_array->paged_sparse_array and index to get the slot of the element. ]*/ \
                (void)PAGED_SPARSE_ARRAY_LL_GET(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->paged_sparse_array, index, &slot);                             \
                /* RELEASE does not dispose the item while the state counts this call */                                                                                   \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_125: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall add 1 to the state of the slot using interlocked_compare_exchange if the state is greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED and lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING. ]*/ \
                int32_t slot_state = interlocked_add(&slot->state, 0);                                                                                                     \
                while (                                                                                                                                                    \
                    (slot_state >= THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED) &&                                                                                 \
                    (slot_state < THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING)                                                                                     \
                    )                                                                                                                                                      \
                {                                                                                                                                                          \
                    int32_t current_slot_state = interlocked_compare_exchange(&slot->state, slot_state + 1, slot_state);                                                   \
                    if (current_slot_state == slot_state)                                                                                                                  \
                    {                                                                                                                                                      \
                        break;                                                                                                                                             \
                    }                                                                                                                                                      \
                    else                                                                                                                                                   \
                    {                                                                                                                                                      \
                        slot_state = current_slot_state;                                                                                                                   \
                    }                                                                                                                                                      \
                }                                                                                                                                                          \
                if (                                                                                                                                                       \
                    (slot_state < THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED) ||                                                                                  \
                    (slot_state >= THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING)                                                                                    \
                    )                                                                                                                                                      \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_074: [ If the state of the slot is lower than THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED or greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/ \
                    result = PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED;                                                                                                         \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    if (array->item_copy_func != NULL)                                                                                                                     \
                    {                                                                                                                                                      \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_126: [ If paged_sparse_array->item_copy_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall call paged_sparse_array->item_copy_func with item as dst and a pointer to the item of the slot as src. ]*/ \
                        array->item_copy_func(item, &slot->item);                                                                                                          \
                    }                                                                                                                                                      \
                    else                                                                                                                                                   \
                    {                                                                                                                                                      \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_127: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall copy the item of the slot to item. ]*/ \
                        (void)memcpy((void*)item, (void*)&slot->item, sizeof(T));                                                                                          \
                    }                                                                                                                                                      \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_128: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall subtract 1 from the state of the slot. ]*/  \
                    if (interlocked_decrement(&slot->state) == THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING + THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED)  \
                    {                                                                                                                                                      \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_129: [ If the state of the slot became THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING + THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall call wake_by_address_single on the state of the slot. ]*/ \
                        wake_by_address_single(&slot->state);                                                                                                              \
                    }                                                                                                                                                      \
                    else                                                                                                                                                   \
                    {                                                                                                                                                      \
                        /* do nothing, no RELEASE waits or other ACQUIRE_ITEM calls still copy the item */                                                                 \
                    }                                                                                                                                                      \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_130: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall succeed and return PAGED_SPARSE_ARRAY_GET_OK. ]*/ \
                    result = PAGED_SPARSE_ARRAY_GET_OK;                                                                                                                    \
                }                                                                                                                                                          \
            }                                                                                                                                                              \
            /* the copy belongs to the caller, so the lock is not held after ACQUIRE_ITEM returns */                                                                       \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_131: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall release page_guard->lock in shared mode before returning. ]*/ \
            srw_lock_ll_release_shared(&page_guard->lock);                                                                                                                 \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}

#endif /*THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_H*/
