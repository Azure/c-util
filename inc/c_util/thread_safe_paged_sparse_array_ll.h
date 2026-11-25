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
#include <stddef.h>
#include <string.h>
#endif // __cplusplus

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/srw_lock_ll.h"
#include "c_pal/thandle_ll.h"

#include "c_util/paged_sparse_array_ll.h"

#include "umock_c/umock_c_prod.h"

/*
 * THREAD_SAFE_PAGED_SPARSE_ARRAY wraps a PAGED_SPARSE_ARRAY (see paged_sparse_array_ll.h) so that ALLOCATE, RELEASE and ACQUIRE_ITEM can be
 * called concurrently from multiple threads.
 * The elements of the wrapped paged sparse array are slots that hold an item of type T. ALLOCATE copies the item of the caller in and ACQUIRE_ITEM
 * copies the item out to the caller, with the item copy function (for a THANDLE, an increment of its reference count), and RELEASE disposes the item
 * with the item dispose function. The caller never gets a pointer into a page, so a page can be freed while the caller uses the items it got.
 * The wrapped paged sparse array tracks the allocated elements of each page (the allocation bitmap and the allocated count of the page), and each page
 * has a lock that protects them:
 * - ALLOCATE and RELEASE hold the lock of the page in exclusive mode while they call PAGED_SPARSE_ARRAY_ALLOCATE and PAGED_SPARSE_ARRAY_RELEASE,
 *   which change the allocation bitmap and the allocated count of the page, and allocate or free the page.
 * - ACQUIRE_ITEM holds the lock of the page in shared mode while it calls PAGED_SPARSE_ARRAY_GET and copies the item out, so the ACQUIRE_ITEM calls
 *   of a page run concurrently, and the item is not released while it is copied.
 * - RELEASE moves the item out of the slot while it holds the lock, and disposes it after it releases the lock.
 */

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
    T item;                                                                                                                                                                \
} THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T);                                                                                                                     \
PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T))                                                                                 \
THANDLE_LL_TYPE_DECLARE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)), PAGED_SPARSE_ARRAY_TYPEDEF_NAME(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T))); \
typedef struct THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_TYPE_NAME_TAG(T)                                                                                                      \
{                                                                                                                                                                          \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_WRAPPED_ARRAY_LL(T) paged_sparse_array;                                                                                                 \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_COPY_FUNC(T) item_copy_func;                                                                                                    \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func;                                                                                              \
    SRW_LOCK_LL page_locks[]; /*one for each page of paged_sparse_array*/                                                                                                  \
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
            if (                                                                                                                                                           \
                (paged_sparse_array->item_dispose_func != NULL) &&                                                                                                         \
                (paged_sparse_array->paged_sparse_array->pages[page_index] != NULL)                                                                                        \
                )                                                                                                                                                          \
            {                                                                                                                                                              \
                uint64_t first_index = page_index * page_size;                                                                                                             \
                uint64_t end_index = THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_END_INDEX(first_index, page_size, max_size);                                                      \
                for (uint64_t index = first_index; index < end_index; index++)                                                                                             \
                {                                                                                                                                                          \
                    THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)* slot;                                                                                             \
                    if (PAGED_SPARSE_ARRAY_LL_GET(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(paged_sparse_array->paged_sparse_array, index, &slot) == PAGED_SPARSE_ARRAY_GET_OK) \
                    {                                                                                                                                                      \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_012: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call item_dispose_func with a pointer to the item of each allocated element. ]*/ \
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
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_013: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call srw_lock_ll_deinit for the lock of each page. ]*/   \
            srw_lock_ll_deinit(&paged_sparse_array->page_locks[page_index]);                                                                                               \
        }                                                                                                                                                                  \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_014: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall release the paged sparse array of slots. ]*/                 \
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
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_003: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX to allocate the array with a lock for each page. ]*/ \
            result = THANDLE_MALLOC_FLEX(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_NAME(C), page_count, sizeof(SRW_LOCK_LL)); \
            if (result == NULL)                                                                                                                                            \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [ If there are any other errors, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/ \
                LogError("failure in THANDLE_MALLOC_FLEX(" THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) ")(page_count=%" PRIu64 ", sizeof(SRW_LOCK_LL)=%zu), uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", item_copy_func=%p, item_dispose_func=%p", \
                    page_count, sizeof(SRW_LOCK_LL), max_size, page_size, item_copy_func, item_dispose_func);                                                              \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                uint64_t initialized_lock_count;                                                                                                                           \
                for (initialized_lock_count = 0; initialized_lock_count < page_count; initialized_lock_count++)                                                            \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_005: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call srw_lock_ll_init to initialize the lock of each page. ]*/ \
                    int lock_init_result = srw_lock_ll_init(&result->page_locks[initialized_lock_count]);                                                                  \
                    if (lock_init_result != 0)                                                                                                                             \
                    {                                                                                                                                                      \
                        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [ If there are any other errors, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/ \
                        LogError("failure in srw_lock_ll_init(&page_locks[%" PRIu64 "])=%d for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) ", uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", page_count=%" PRIu64 ", item_copy_func=%p, item_dispose_func=%p", \
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
                        srw_lock_ll_deinit(&result->page_locks[i]);                                                                                                        \
                    }                                                                                                                                                      \
                    THANDLE_FREE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(result);                                                                                  \
                    result = NULL;                                                                                                                                         \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_007: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store item_dispose_func. ]*/                      \
                    result->item_dispose_func = item_dispose_func;                                                                                                         \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_110: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store item_copy_func. ]*/                         \
                    result->item_copy_func = item_copy_func;                                                                                                               \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_008: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store the paged sparse array of slots. ]*/        \
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

/*introduces a function definition for thread_safe_paged_sparse_array_allocate*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_DEFINE(C, T)                                                                                                            \
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
            SRW_LOCK_LL* page_lock = &array->page_locks[page_index];                                                                                                       \
            /* PAGED_SPARSE_ARRAY_ALLOCATE(T) changes the allocation bitmap and the allocated count of the page, and allocates the page if it is not allocated */          \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_030: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall acquire the lock of the page of index in exclusive mode. ]*/ \
            srw_lock_ll_acquire_exclusive(page_lock);                                                                                                                      \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)* slot;                                                                                                     \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_132: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call PAGED_SPARSE_ARRAY_ALLOCATE(T) to allocate the slot of the element. ]*/ \
            PAGED_SPARSE_ARRAY_ALLOCATE_RESULT allocate_result = PAGED_SPARSE_ARRAY_LL_ALLOCATE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->paged_sparse_array, index, &slot); \
            if (allocate_result != PAGED_SPARSE_ARRAY_ALLOCATE_OK)                                                                                                         \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_133: [ If PAGED_SPARSE_ARRAY_ALLOCATE(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return the result of PAGED_SPARSE_ARRAY_ALLOCATE(T). ]*/ \
                result = allocate_result;                                                                                                                                  \
                LogError("failure in PAGED_SPARSE_ARRAY_ALLOCATE(" MU_TOSTRING(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)) ")(index=%" PRIu64 ")=%" PRI_MU_ENUM " for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, page_index=%" PRIu64 ", max_size=%" PRIu64 ", page_size=%" PRIu64 ", " MU_TOSTRING(T) "* item=%p, result=%" PRI_MU_ENUM "", \
                    index, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, allocate_result), paged_sparse_array, page_index, max_size, page_size, item, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result)); \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                if (array->item_copy_func != NULL)                                                                                                                         \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_113: [ If item_copy_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call item_copy_func with a pointer to the item of the slot as dst and item as src. ]*/ \
                    array->item_copy_func(&slot->item, item);                                                                                                              \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_114: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall copy item to the item of the slot. ]*/ \
                    (void)memcpy((void*)&slot->item, (void*)item, sizeof(T));                                                                                              \
                }                                                                                                                                                          \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_116: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall succeed and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/ \
                result = PAGED_SPARSE_ARRAY_ALLOCATE_OK;                                                                                                                   \
            }                                                                                                                                                              \
            /* ACQUIRE_ITEM does not copy the item before it is completely copied in */                                                                                    \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the page lock in exclusive mode before returning. ]*/ \
            srw_lock_ll_release_exclusive(page_lock);                                                                                                                      \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}

/*introduces a function definition for thread_safe_paged_sparse_array_release*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE_DEFINE(C, T)                                                                                                             \
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
            SRW_LOCK_LL* page_lock = &array->page_locks[page_index];                                                                                                       \
            /* PAGED_SPARSE_ARRAY_RELEASE(T) changes the allocation bitmap and the allocated count of the page, and frees the page if it has no other elements */          \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_021: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall acquire the lock of the page of index in exclusive mode. ]*/ \
            srw_lock_ll_acquire_exclusive(page_lock);                                                                                                                      \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)* slot;                                                                                                     \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_058: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call PAGED_SPARSE_ARRAY_GET(T) to get the slot of the element. ]*/ \
            PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_LL_GET(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->paged_sparse_array, index, &slot); \
            if (get_result != PAGED_SPARSE_ARRAY_GET_OK)                                                                                                                   \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_134: [ If PAGED_SPARSE_ARRAY_GET(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall release the page lock and return. ]*/ \
                srw_lock_ll_release_exclusive(page_lock);                                                                                                                  \
                LogError("failure in PAGED_SPARSE_ARRAY_GET(" MU_TOSTRING(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)) ")(index=%" PRIu64 ")=%" PRI_MU_ENUM ", the element is not allocated in " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T) " paged_sparse_array=%p, page_index=%" PRIu64 ", max_size=%" PRIu64 ", page_size=%" PRIu64 "", \
                    index, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, get_result), paged_sparse_array, page_index, max_size, page_size);                                 \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                /* the item is disposed after the lock is released, and the slot is not used after PAGED_SPARSE_ARRAY_RELEASE(T) */                                        \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_135: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall copy the item of the slot to a local item. ]*/       \
                T item = slot->item;                                                                                                                                       \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_136: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call PAGED_SPARSE_ARRAY_RELEASE(T) to release the slot of the element. ]*/ \
                PAGED_SPARSE_ARRAY_LL_RELEASE(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->paged_sparse_array, index);                                      \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall release the page lock. ]*/                           \
                srw_lock_ll_release_exclusive(page_lock);                                                                                                                  \
                if (array->item_dispose_func != NULL)                                                                                                                      \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call item_dispose_func with a pointer to the local item. ]*/ \
                    array->item_dispose_func(&item);                                                                                                                       \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    /* do nothing, there is no item dispose function */                                                                                                    \
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
            SRW_LOCK_LL* page_lock = &array->page_locks[index / page_size];                                                                                                \
            /* ALLOCATE and RELEASE do not change the page while the lock is held in shared mode */                                                                        \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_104: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall acquire the lock of the page of index in shared mode. ]*/ \
            srw_lock_ll_acquire_shared(page_lock);                                                                                                                         \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(T)* slot;                                                                                                     \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_072: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall call PAGED_SPARSE_ARRAY_GET(T) to get the slot of the element. ]*/ \
            PAGED_SPARSE_ARRAY_GET_RESULT get_result = PAGED_SPARSE_ARRAY_LL_GET(THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_TYPEDEF_NAME(C))(array->paged_sparse_array, index, &slot); \
            if (get_result != PAGED_SPARSE_ARRAY_GET_OK)                                                                                                                   \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_137: [ If PAGED_SPARSE_ARRAY_GET(T) fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall fail and return the result of PAGED_SPARSE_ARRAY_GET(T). ]*/ \
                result = get_result;                                                                                                                                       \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                if (array->item_copy_func != NULL)                                                                                                                         \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_126: [ If item_copy_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall call item_copy_func with item as dst and a pointer to the item of the slot as src. ]*/ \
                    array->item_copy_func(item, &slot->item);                                                                                                              \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_127: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall copy the item of the slot to item. ]*/ \
                    (void)memcpy((void*)item, (void*)&slot->item, sizeof(T));                                                                                              \
                }                                                                                                                                                          \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_130: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall succeed and return PAGED_SPARSE_ARRAY_GET_OK. ]*/ \
                result = PAGED_SPARSE_ARRAY_GET_OK;                                                                                                                        \
            }                                                                                                                                                              \
            /* the copy belongs to the caller, so the lock is not held after ACQUIRE_ITEM returns */                                                                       \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_131: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T) shall release the page lock in shared mode before returning. ]*/ \
            srw_lock_ll_release_shared(page_lock);                                                                                                                         \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}

#endif /*THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_H*/
