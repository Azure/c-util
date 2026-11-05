// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PAGED_SPARSE_ARRAY_LL_H
#define PAGED_SPARSE_ARRAY_LL_H

#ifdef __cplusplus
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#else // __cplusplus
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#endif // __cplusplus

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/srw_lock_ll.h"
#include "c_pal/thandle_ll.h"

#include "umock_c/umock_c_prod.h"

/*result codes for PAGED_SPARSE_ARRAY_ALLOCATE*/
#define PAGED_SPARSE_ARRAY_ALLOCATE_RESULT_VALUES \
    PAGED_SPARSE_ARRAY_ALLOCATE_OK, \
    PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, \
    PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED, \
    PAGED_SPARSE_ARRAY_ALLOCATE_ERROR

MU_DEFINE_ENUM(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_RESULT_VALUES)

/*result codes for PAGED_SPARSE_ARRAY_GET*/
#define PAGED_SPARSE_ARRAY_GET_RESULT_VALUES \
    PAGED_SPARSE_ARRAY_GET_OK, \
    PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, \
    PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED

MU_DEFINE_ENUM(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_RESULT_VALUES)

/*result codes for PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY*/
#define PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT_VALUES \
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_FREED, \
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_INVALID_ARGS, \
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_ALLOCATED, \
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_EMPTY

MU_DEFINE_ENUM(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT_VALUES)

/*options for PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS*/
typedef struct PAGED_SPARSE_ARRAY_OPTIONS_TAG
{
    bool thread_safe;           /*ALLOCATE, RELEASE, GET and FREE_PAGE_IF_EMPTY can be called concurrently*/
    bool auto_free_empty_pages; /*RELEASE frees a page when the last allocated element in it is released*/
    bool zero_new_pages;        /*all the items of a newly allocated page are set to zero*/
} PAGED_SPARSE_ARRAY_OPTIONS;

/*format and values to log a PAGED_SPARSE_ARRAY_OPTIONS, e.g. LogInfo("options=%" PRI_PAGED_SPARSE_ARRAY_OPTIONS "", PAGED_SPARSE_ARRAY_OPTIONS_VALUES(options))*/
#define PRI_PAGED_SPARSE_ARRAY_OPTIONS "s{ thread_safe=%" PRI_BOOL ", auto_free_empty_pages=%" PRI_BOOL ", zero_new_pages=%" PRI_BOOL " }"
#define PAGED_SPARSE_ARRAY_OPTIONS_VALUES(options) "", MU_BOOL_VALUE((options).thread_safe), MU_BOOL_VALUE((options).auto_free_empty_pages), MU_BOOL_VALUE((options).zero_new_pages)

/*PAGED_SPARSE_ARRAY is backed by a THANDLE build on the structure below*/
#define PAGED_SPARSE_ARRAY_STRUCT_TYPE_NAME_TAG(T) MU_C2(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T), _TAG)

#define PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T) MU_C2(PAGED_SPARSE_ARRAY_STRUCT_, T)

/*custom item dispose function type*/
#define PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) MU_C2(PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC_, T)
#define PAGED_SPARSE_ARRAY_DEFINE_ITEM_DISPOSE_FUNC_TYPE(T) typedef void (*PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T))(T* item)

/*PAGE_STRUCT holds a single page with elements and allocation bitmap*/
#define PAGED_SPARSE_ARRAY_PAGE_STRUCT_TYPE_NAME_TAG(T) MU_C2(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T), _TAG)
#define PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T) MU_C2(PAGED_SPARSE_ARRAY_PAGE_STRUCT_, T)

/*PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T) introduces the base type that holds the paged sparse array*/
#define PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(T)                                                                        \
/*define the item dispose function type*/                                                                              \
PAGED_SPARSE_ARRAY_DEFINE_ITEM_DISPOSE_FUNC_TYPE(T);                                                                    \
/*define the page structure first - bitmap is embedded after items[] in the same allocation*/                          \
/*when thread_safe is true, allocated_count and the bitmap bytes are only accessed with interlocked operations*/       \
typedef struct PAGED_SPARSE_ARRAY_PAGE_STRUCT_TYPE_NAME_TAG(T)                                                          \
{                                                                                                                       \
    uint64_t allocated_count;                                                                                           \
    T items[];  /*followed by bitmap bytes at offset page_size * sizeof(T)*/                                            \
} PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T);                                                                              \
/*forward define the typedef of the PAGED_SPARSE_ARRAY struct so that it can be used for a function pointer definition*/\
typedef struct PAGED_SPARSE_ARRAY_STRUCT_TYPE_NAME_TAG(T) PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T);                           \
struct PAGED_SPARSE_ARRAY_STRUCT_TYPE_NAME_TAG(T)                                                                       \
{                                                                                                                       \
    uint64_t max_size;                                                                                                  \
    uint64_t page_size;                                                                                                 \
    uint64_t page_count;                                                                                                \
    PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func;                                                       \
    PAGED_SPARSE_ARRAY_OPTIONS options;                                                                                 \
    SRW_LOCK_LL lock; /*only initialized and used when options.thread_safe is true*/                                    \
    PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)* pages[];                                                                   \
};                                                                                                                      \

/*PAGED_SPARSE_ARRAY is-a THANDLE*/
/*given a type "T" PAGED_SPARSE_ARRAY_LL(T) expands to the name of the type. */
#define PAGED_SPARSE_ARRAY_LL(T) THANDLE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T))

/*because PAGED_SPARSE_ARRAY is a THANDLE, all THANDLE's macro APIs are useable with PAGED_SPARSE_ARRAY.*/
/*the below are just shortcuts of THANDLE's public ones*/
#define PAGED_SPARSE_ARRAY_LL_INITIALIZE(T) THANDLE_INITIALIZE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T))
#define PAGED_SPARSE_ARRAY_LL_ASSIGN(T) THANDLE_ASSIGN(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T))
#define PAGED_SPARSE_ARRAY_LL_MOVE(T) THANDLE_MOVE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T))
#define PAGED_SPARSE_ARRAY_LL_INITIALIZE_MOVE(T) THANDLE_INITIALIZE_MOVE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T))

/*introduces a new name for a function that returns a PAGED_SPARSE_ARRAY_LL(T)*/
#define PAGED_SPARSE_ARRAY_LL_CREATE_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_CREATE_, C)
#define PAGED_SPARSE_ARRAY_LL_CREATE(C) PAGED_SPARSE_ARRAY_LL_CREATE_NAME(C)

/*introduces a new name for a function that returns a PAGED_SPARSE_ARRAY_LL(T) created with options*/
#define PAGED_SPARSE_ARRAY_LL_CREATE_WITH_OPTIONS_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_CREATE_WITH_OPTIONS_, C)
#define PAGED_SPARSE_ARRAY_LL_CREATE_WITH_OPTIONS(C) PAGED_SPARSE_ARRAY_LL_CREATE_WITH_OPTIONS_NAME(C)

/*introduces a name for the internal function used by both CREATE and CREATE_WITH_OPTIONS*/
#define PAGED_SPARSE_ARRAY_LL_CREATE_INTERNAL_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_CREATE_INTERNAL_, C)

/*introduces a name for the dispose function that is called when PAGED_SPARSE_ARRAY ref count goes to 0*/
#define PAGED_SPARSE_ARRAY_LL_DISPOSE_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_DISPOSE_, C)

/*introduces a name for the allocate function*/
#define PAGED_SPARSE_ARRAY_LL_ALLOCATE_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_ALLOCATE_, C)
#define PAGED_SPARSE_ARRAY_LL_ALLOCATE(C) PAGED_SPARSE_ARRAY_LL_ALLOCATE_NAME(C)

/*introduces a name for the internal allocate function used when thread_safe is true*/
#define PAGED_SPARSE_ARRAY_LL_ALLOCATE_THREAD_SAFE_INTERNAL_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_ALLOCATE_THREAD_SAFE_INTERNAL_, C)

/*introduces a name for the release function*/
#define PAGED_SPARSE_ARRAY_LL_RELEASE_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_RELEASE_, C)
#define PAGED_SPARSE_ARRAY_LL_RELEASE(C) PAGED_SPARSE_ARRAY_LL_RELEASE_NAME(C)

/*introduces a name for the internal release function used when thread_safe is true*/
#define PAGED_SPARSE_ARRAY_LL_RELEASE_THREAD_SAFE_INTERNAL_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_RELEASE_THREAD_SAFE_INTERNAL_, C)

/*introduces a name for the get function*/
#define PAGED_SPARSE_ARRAY_LL_GET_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_GET_, C)
#define PAGED_SPARSE_ARRAY_LL_GET(C) PAGED_SPARSE_ARRAY_LL_GET_NAME(C)

/*introduces a name for the internal get function used when thread_safe is true*/
#define PAGED_SPARSE_ARRAY_LL_GET_THREAD_SAFE_INTERNAL_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_GET_THREAD_SAFE_INTERNAL_, C)

/*introduces a name for the free page if empty function*/
#define PAGED_SPARSE_ARRAY_LL_FREE_PAGE_IF_EMPTY_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_FREE_PAGE_IF_EMPTY_, C)
#define PAGED_SPARSE_ARRAY_LL_FREE_PAGE_IF_EMPTY(C) PAGED_SPARSE_ARRAY_LL_FREE_PAGE_IF_EMPTY_NAME(C)

/*introduces a name for the internal function that frees a page if it is empty, used by RELEASE and FREE_PAGE_IF_EMPTY*/
#define PAGED_SPARSE_ARRAY_LL_FREE_PAGE_IF_EMPTY_INTERNAL_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_FREE_PAGE_IF_EMPTY_INTERNAL_, C)

/*introduces a function declaration for paged_sparse_array_create*/
#define PAGED_SPARSE_ARRAY_LL_CREATE_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, PAGED_SPARSE_ARRAY_LL(T), PAGED_SPARSE_ARRAY_LL_CREATE(C), uint64_t, max_size, uint64_t, page_size, PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T), item_dispose_func);

/*introduces a function declaration for paged_sparse_array_create_with_options*/
#define PAGED_SPARSE_ARRAY_LL_CREATE_WITH_OPTIONS_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, PAGED_SPARSE_ARRAY_LL(T), PAGED_SPARSE_ARRAY_LL_CREATE_WITH_OPTIONS(C), uint64_t, max_size, uint64_t, page_size, PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T), item_dispose_func, const PAGED_SPARSE_ARRAY_OPTIONS*, options);

/*introduces a function declaration for paged_sparse_array_allocate*/
#define PAGED_SPARSE_ARRAY_LL_ALLOCATE_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_LL_ALLOCATE(C), PAGED_SPARSE_ARRAY_LL(T), paged_sparse_array, uint64_t, index, T**, allocated_ptr);

/*introduces a function declaration for paged_sparse_array_release*/
#define PAGED_SPARSE_ARRAY_LL_RELEASE_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, void, PAGED_SPARSE_ARRAY_LL_RELEASE(C), PAGED_SPARSE_ARRAY_LL(T), paged_sparse_array, uint64_t, index);

/*introduces a function declaration for paged_sparse_array_get*/
#define PAGED_SPARSE_ARRAY_LL_GET_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_LL_GET(C), PAGED_SPARSE_ARRAY_LL(T), paged_sparse_array, uint64_t, index, T**, item);

/*introduces a function declaration for paged_sparse_array_free_page_if_empty*/
#define PAGED_SPARSE_ARRAY_LL_FREE_PAGE_IF_EMPTY_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, PAGED_SPARSE_ARRAY_LL_FREE_PAGE_IF_EMPTY(C), PAGED_SPARSE_ARRAY_LL(T), paged_sparse_array, uint64_t, index);

/*helper to compute bitmap size in bytes (1 bit per element, rounded up to nearest byte)*/
#define PAGED_SPARSE_ARRAY_BITMAP_SIZE(page_size) (((page_size) + 7) / 8)

/*helper to get the allocation bitmap from a page - bitmap is stored after items[] in the same allocation*/
#define PAGED_SPARSE_ARRAY_GET_BITMAP(page, page_size) ((uint8_t*)((page)->items + (page_size)))

/*helper to check if an element is allocated in the bitmap*/
#define PAGED_SPARSE_ARRAY_IS_ALLOCATED(bitmap, index_in_page) \
    ((bitmap)[(index_in_page) / 8] & (1 << ((index_in_page) % 8)))

/*helper to set an element as allocated in the bitmap*/
#define PAGED_SPARSE_ARRAY_SET_ALLOCATED(bitmap, index_in_page) \
    ((bitmap)[(index_in_page) / 8] |= (1 << ((index_in_page) % 8)))

/*helper to clear an element as not allocated in the bitmap*/
#define PAGED_SPARSE_ARRAY_CLEAR_ALLOCATED(bitmap, index_in_page) \
    ((bitmap)[(index_in_page) / 8] &= ~(1 << ((index_in_page) % 8)))

/*the helpers below are used when thread_safe is true, where allocated_count and the bitmap bytes are only accessed with interlocked operations*/
#define PAGED_SPARSE_ARRAY_ALLOCATED_COUNT_ATOMIC(page) ((volatile_atomic int64_t*)&(page)->allocated_count)

#define PAGED_SPARSE_ARRAY_BITMAP_BYTE_ATOMIC(bitmap, index_in_page) ((volatile_atomic int8_t*)&(bitmap)[(index_in_page) / 8])

#define PAGED_SPARSE_ARRAY_BITMAP_MASK(index_in_page) ((int8_t)(1 << ((index_in_page) % 8)))

/*helper to atomically check if an element is allocated in the bitmap*/
#define PAGED_SPARSE_ARRAY_IS_ALLOCATED_ATOMIC(bitmap, index_in_page) \
    ((interlocked_or_8(PAGED_SPARSE_ARRAY_BITMAP_BYTE_ATOMIC(bitmap, index_in_page), 0) & PAGED_SPARSE_ARRAY_BITMAP_MASK(index_in_page)) != 0)

/*helper to atomically set an element as allocated in the bitmap, evaluates to true if the element was already allocated*/
#define PAGED_SPARSE_ARRAY_TEST_AND_SET_ALLOCATED_ATOMIC(bitmap, index_in_page) \
    ((interlocked_or_8(PAGED_SPARSE_ARRAY_BITMAP_BYTE_ATOMIC(bitmap, index_in_page), PAGED_SPARSE_ARRAY_BITMAP_MASK(index_in_page)) & PAGED_SPARSE_ARRAY_BITMAP_MASK(index_in_page)) != 0)

/*helper to atomically clear an element as not allocated in the bitmap, evaluates to true if the element was allocated*/
#define PAGED_SPARSE_ARRAY_TEST_AND_CLEAR_ALLOCATED_ATOMIC(bitmap, index_in_page) \
    ((interlocked_and_8(PAGED_SPARSE_ARRAY_BITMAP_BYTE_ATOMIC(bitmap, index_in_page), (int8_t)~PAGED_SPARSE_ARRAY_BITMAP_MASK(index_in_page)) & PAGED_SPARSE_ARRAY_BITMAP_MASK(index_in_page)) != 0)

/*introduces a function definition for freeing the allocated resources for a PAGED_SPARSE_ARRAY*/
#define PAGED_SPARSE_ARRAY_LL_DISPOSE_DEFINE(C, T)                                                                              \
static void PAGED_SPARSE_ARRAY_LL_DISPOSE_NAME(C)(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* paged_sparse_array)                      \
{                                                                                                                              \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_009: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_DISPOSE(T) shall return. ]*/   \
    if (paged_sparse_array == NULL)                                                                                            \
    {                                                                                                                          \
        LogError("invalid arguments " MU_TOSTRING(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)) "* paged_sparse_array=%p",               \
            paged_sparse_array);                                                                                               \
    }                                                                                                                          \
    else                                                                                                                       \
    {                                                                                                                          \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_010: [ PAGED_SPARSE_ARRAY_DISPOSE(T) shall free all pages that are non-NULL. ]*/    \
        for (uint64_t i = 0; i < paged_sparse_array->page_count; i++)                                                          \
        {                                                                                                                      \
            if (paged_sparse_array->pages[i] != NULL)                                                                          \
            {                                                                                                                  \
                /* Codes_SRS_PAGED_SPARSE_ARRAY_88_043: [ If item_dispose_func is not NULL, PAGED_SPARSE_ARRAY_DISPOSE(T) shall call item_dispose_func for each element that is still allocated. ]*/ \
                if (paged_sparse_array->item_dispose_func != NULL)                                                             \
                {                                                                                                              \
                    uint8_t* bitmap = PAGED_SPARSE_ARRAY_GET_BITMAP(paged_sparse_array->pages[i], paged_sparse_array->page_size); \
                    for (uint64_t j = 0; j < paged_sparse_array->page_size; j++)                                               \
                    {                                                                                                          \
                        if (PAGED_SPARSE_ARRAY_IS_ALLOCATED(bitmap, j))                                                        \
                        {                                                                                                      \
                            paged_sparse_array->item_dispose_func(&paged_sparse_array->pages[i]->items[j]);                    \
                        }                                                                                                      \
                    }                                                                                                          \
                }                                                                                                              \
                /* bitmap is embedded in same allocation as page, so only one free needed */                                  \
                free((void*)paged_sparse_array->pages[i]);                                                                     \
            }                                                                                                                  \
        }                                                                                                                      \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_052: [ If thread_safe is true, PAGED_SPARSE_ARRAY_DISPOSE(T) shall call srw_lock_ll_deinit. ]*/ \
        if (!paged_sparse_array->options.thread_safe)                                                                          \
        {                                                                                                                      \
            /* do nothing, the lock was not initialized */                                                                     \
        }                                                                                                                      \
        else                                                                                                                   \
        {                                                                                                                      \
            srw_lock_ll_deinit(&paged_sparse_array->lock);                                                                     \
        }                                                                                                                      \
    }                                                                                                                          \
}

#define PAGED_SPARSE_ARRAY_LL_CREATE_DEFINE(C, T)                                                                                                                         \
static PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* PAGED_SPARSE_ARRAY_LL_CREATE_INTERNAL_NAME(C)(uint64_t max_size, uint64_t page_size, PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func, const PAGED_SPARSE_ARRAY_OPTIONS* options) \
{                                                                                                                                                                         \
    PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* result;                                                                                                                           \
    if (                                                                                                                                                                  \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_001: [ If max_size is zero, PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/                                       \
        (max_size == 0) ||                                                                                                                                                \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_002: [ If page_size is zero, PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/                                      \
        (page_size == 0) ||                                                                                                                                               \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_040: [ If max_size + page_size - 1 would overflow, PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/                \
        (max_size > UINT64_MAX - (page_size - 1))                                                                                                                         \
        )                                                                                                                                                                 \
    {                                                                                                                                                                     \
        LogError("Invalid arguments: PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) "), uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", PAGED_SPARSE_ARRAY_ITEM_DISPOSE_FUNC(" MU_TOSTRING(T) ") item_dispose_func=%p, const PAGED_SPARSE_ARRAY_OPTIONS* options=%p (%" PRI_PAGED_SPARSE_ARRAY_OPTIONS ")", \
            max_size, page_size, item_dispose_func, options, PAGED_SPARSE_ARRAY_OPTIONS_VALUES(*options));                                                               \
    }                                                                                                                                                                     \
    else                                                                                                                                                                  \
    {                                                                                                                                                                     \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_003: [ PAGED_SPARSE_ARRAY_CREATE(T) shall compute the number of pages as (max_size + page_size - 1) / page_size. ]*/           \
        uint64_t page_count = (max_size + page_size - 1) / page_size;                                                                                                     \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_004: [ PAGED_SPARSE_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX to allocate memory for the paged sparse array with the number of pages. ]*/ \
        result = THANDLE_MALLOC_FLEX(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(PAGED_SPARSE_ARRAY_LL_DISPOSE_NAME(C), page_count, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)*)); \
        if (result == NULL)                                                                                                                                               \
        {                                                                                                                                                                 \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_007: [ If there are any errors, PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/                               \
            LogError("failure in THANDLE_MALLOC_FLEX(PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) ")) page_count=%" PRIu64 ", sizeof(PAGED_SPARSE_ARRAY_PAGE*)=%zu, uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", PAGED_SPARSE_ARRAY_ITEM_DISPOSE_FUNC(" MU_TOSTRING(T) ") item_dispose_func=%p, const PAGED_SPARSE_ARRAY_OPTIONS* options=%p (%" PRI_PAGED_SPARSE_ARRAY_OPTIONS ")", \
                page_count, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)*), max_size, page_size, item_dispose_func, options, PAGED_SPARSE_ARRAY_OPTIONS_VALUES(*options)); \
        }                                                                                                                                                                 \
        else                                                                                                                                                              \
        {                                                                                                                                                                 \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_006: [ PAGED_SPARSE_ARRAY_CREATE(T) shall store max_size, page_size, and item_dispose_func in the structure. ]*/           \
            result->max_size = max_size;                                                                                                                                  \
            result->page_size = page_size;                                                                                                                                \
            result->page_count = page_count;                                                                                                                              \
            result->item_dispose_func = item_dispose_func;                                                                                                                \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_049: [ PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T) shall store a copy of *options in the structure. ]*/                      \
            result->options = *options;                                                                                                                                   \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_005: [ PAGED_SPARSE_ARRAY_CREATE(T) shall set all page pointers to NULL. ]*/                                               \
            for (uint64_t i = 0; i < page_count; i++)                                                                                                                     \
            {                                                                                                                                                             \
                result->pages[i] = NULL;                                                                                                                                  \
            }                                                                                                                                                             \
            int lock_init_result;                                                                                                                                         \
            if (!options->thread_safe)                                                                                                                                    \
            {                                                                                                                                                             \
                /* do nothing, the lock is not used */                                                                                                                    \
                lock_init_result = 0;                                                                                                                                     \
            }                                                                                                                                                             \
            else                                                                                                                                                          \
            {                                                                                                                                                             \
                /* Codes_SRS_PAGED_SPARSE_ARRAY_88_050: [ If options->thread_safe is true, PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T) shall call srw_lock_ll_init to initialize the lock. ]*/ \
                lock_init_result = srw_lock_ll_init(&result->lock);                                                                                                       \
            }                                                                                                                                                             \
            if (lock_init_result != 0)                                                                                                                                    \
            {                                                                                                                                                             \
                /* Codes_SRS_PAGED_SPARSE_ARRAY_88_051: [ If srw_lock_ll_init fails, PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T) shall free the paged sparse array and return NULL. ]*/ \
                LogError("failure in srw_lock_ll_init(&result->lock)=%d for PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) "), uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", uint64_t page_count=%" PRIu64 ", const PAGED_SPARSE_ARRAY_OPTIONS* options=%p (%" PRI_PAGED_SPARSE_ARRAY_OPTIONS ")", \
                    lock_init_result, max_size, page_size, page_count, options, PAGED_SPARSE_ARRAY_OPTIONS_VALUES(*options));                                            \
                THANDLE_FREE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(result);                                                                                                 \
            }                                                                                                                                                             \
            else                                                                                                                                                          \
            {                                                                                                                                                             \
                /* Codes_SRS_PAGED_SPARSE_ARRAY_88_008: [ PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/                                     \
                goto all_ok;                                                                                                                                              \
            }                                                                                                                                                             \
        }                                                                                                                                                                 \
    }                                                                                                                                                                     \
    result = NULL;                                                                                                                                                        \
all_ok:                                                                                                                                                                   \
    return result;                                                                                                                                                        \
}                                                                                                                                                                         \
PAGED_SPARSE_ARRAY_LL(T) PAGED_SPARSE_ARRAY_LL_CREATE(C)(uint64_t max_size, uint64_t page_size, PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func)             \
{                                                                                                                                                                         \
    PAGED_SPARSE_ARRAY_OPTIONS default_options;                                                                                                                           \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_046: [ PAGED_SPARSE_ARRAY_CREATE(T) shall create the paged sparse array with thread_safe set to false, auto_free_empty_pages set to true and zero_new_pages set to false. ]*/ \
    default_options.thread_safe = false;                                                                                                                                  \
    default_options.auto_free_empty_pages = true;                                                                                                                         \
    default_options.zero_new_pages = false;                                                                                                                               \
    return PAGED_SPARSE_ARRAY_LL_CREATE_INTERNAL_NAME(C)(max_size, page_size, item_dispose_func, &default_options);                                                       \
}

/*internal helper to allocate a page*/
#define PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_, C)

#define PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_DEFINE(C, T)                                                                                                         \
static PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)* PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C)(uint64_t page_size, bool zero_new_pages)                             \
{                                                                                                                                                                         \
    PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)* page;                                                                                                                        \
    /* Allocate page struct + items array + bitmap in a single allocation */                                                                                             \
    size_t bitmap_size = PAGED_SPARSE_ARRAY_BITMAP_SIZE(page_size);                                                                                                       \
    size_t items_and_bitmap_size = (size_t)page_size * sizeof(T) + bitmap_size;                                                                                           \
    page = malloc_flex(sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)), items_and_bitmap_size, 1);                                                                        \
    if (page == NULL)                                                                                                                                                     \
    {                                                                                                                                                                     \
        LogError("failure in malloc_flex(sizeof(PAGED_SPARSE_ARRAY_PAGE(" MU_TOSTRING(T) "))=%zu, items_and_bitmap_size=%zu, 1) for page, page_size=%" PRIu64 ", sizeof(" MU_TOSTRING(T) ")=%zu, bitmap_size=%zu, zero_new_pages=%" PRI_BOOL "", \
            sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)), items_and_bitmap_size, page_size, sizeof(T), bitmap_size, MU_BOOL_VALUE(zero_new_pages));                     \
    }                                                                                                                                                                     \
    else                                                                                                                                                                  \
    {                                                                                                                                                                     \
        /* Initialize bitmap to all zeros (nothing allocated) - bitmap is at end of items array */                                                                       \
        uint8_t* bitmap = PAGED_SPARSE_ARRAY_GET_BITMAP(page, page_size);                                                                                              \
        (void)memset(bitmap, 0, bitmap_size);                                                                                                                             \
        page->allocated_count = 0;                                                                                                                                        \
        if (!zero_new_pages)                                                                                                                                              \
        {                                                                                                                                                                 \
            /* do nothing, items are left uninitialized */                                                                                                                \
        }                                                                                                                                                                 \
        else                                                                                                                                                              \
        {                                                                                                                                                                 \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_053: [ If zero_new_pages is true, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall set all the items of a newly allocated page to zero. ]*/ \
            (void)memset((void*)page->items, 0, (size_t)page_size * sizeof(T));                                                                                           \
        }                                                                                                                                                                 \
    }                                                                                                                                                                     \
    return page;                                                                                                                                                          \
}                                                                                                                                                                         \
MU_SUPPRESS_WARNING(4505) /*warning C4505: 'function': unreferenced function with internal linkage has been removed*/                                                     \
static PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT PAGED_SPARSE_ARRAY_LL_FREE_PAGE_IF_EMPTY_INTERNAL_NAME(C)(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array, uint64_t page_index) \
{                                                                                                                                                                         \
    /* when thread_safe is true, the caller holds the lock in exclusive mode */                                                                                          \
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT result;                                                                                                                  \
    PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)* page = array->pages[page_index];                                                                                             \
    if (page == NULL)                                                                                                                                                     \
    {                                                                                                                                                                     \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_070: [ If the page is not allocated, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall return PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_ALLOCATED. ]*/ \
        result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_ALLOCATED;                                                                                                     \
    }                                                                                                                                                                     \
    else                                                                                                                                                                  \
    {                                                                                                                                                                     \
        int64_t allocated_count;                                                                                                                                          \
        if (!array->options.thread_safe)                                                                                                                                  \
        {                                                                                                                                                                 \
            allocated_count = (int64_t)page->allocated_count;                                                                                                             \
        }                                                                                                                                                                 \
        else                                                                                                                                                              \
        {                                                                                                                                                                 \
            allocated_count = interlocked_add_64(PAGED_SPARSE_ARRAY_ALLOCATED_COUNT_ATOMIC(page), 0);                                                                     \
        }                                                                                                                                                                 \
        if (allocated_count != 0)                                                                                                                                         \
        {                                                                                                                                                                 \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_071: [ If the allocated count of the page is not 0, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall return PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_EMPTY. ]*/ \
            result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_EMPTY;                                                                                                     \
        }                                                                                                                                                                 \
        else                                                                                                                                                              \
        {                                                                                                                                                                 \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_072: [ Otherwise, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall free the page, set the page pointer to NULL and return PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_FREED. ]*/ \
            free((void*)page);                                                                                                                                            \
            array->pages[page_index] = NULL;                                                                                                                              \
            result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_FREED;                                                                                                         \
        }                                                                                                                                                                 \
    }                                                                                                                                                                     \
    return result;                                                                                                                                                        \
}                                                                                                                                                                         \
MU_UNSUPPRESS_WARNING(4505)                                                                                                                                               \

#define PAGED_SPARSE_ARRAY_LL_ALLOCATE_DEFINE(C, T)                                                                                                                        \
static PAGED_SPARSE_ARRAY_ALLOCATE_RESULT PAGED_SPARSE_ARRAY_LL_ALLOCATE_THREAD_SAFE_INTERNAL_NAME(C)(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array, uint64_t index, T** allocated_ptr) \
{                                                                                                                                                                          \
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result;                                                                                                                             \
    bool is_lock_exclusive;                                                                                                                                                \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_013: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall compute the page index as index / page_size. ]*/                                        \
    uint64_t page_index = index / array->page_size;                                                                                                                        \
    uint64_t index_in_page = index % array->page_size;                                                                                                                     \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_054: [ If thread_safe is true, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall acquire the lock in shared mode. ]*/                            \
    srw_lock_ll_acquire_shared(&array->lock);                                                                                                                              \
    PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)* page = array->pages[page_index];                                                                                              \
    if (page != NULL)                                                                                                                                                      \
    {                                                                                                                                                                      \
        is_lock_exclusive = false;                                                                                                                                         \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_055: [ If thread_safe is true and the page is not allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the shared lock and acquire the lock in exclusive mode. ]*/ \
        srw_lock_ll_release_shared(&array->lock);                                                                                                                          \
        srw_lock_ll_acquire_exclusive(&array->lock);                                                                                                                       \
        is_lock_exclusive = true;                                                                                                                                          \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_056: [ After acquiring the lock in exclusive mode, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall check again whether the page is allocated, since another thread may have allocated it in the meantime. ]*/ \
        page = array->pages[page_index];                                                                                                                                   \
        if (page != NULL)                                                                                                                                                  \
        {                                                                                                                                                                  \
            /* do nothing, another thread allocated the page while the lock was not held */                                                                                \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_014: [ If the page is not allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call malloc_flex to allocate memory for the page structure containing page_size elements of type T plus a uint8_t allocation bitmap of (page_size + 7) / 8 bytes, and initialize all bitmap bits to 0 (not allocated). ]*/ \
            page = PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C)(array->page_size, array->options.zero_new_pages);                                                  \
            if (page == NULL)                                                                                                                                              \
            {                                                                                                                                                              \
                LogError("failure allocating page for PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) ") array=%p, uint64_t index=%" PRIu64 ", page_index=%" PRIu64 ", page_size=%" PRIu64 ", page_count=%" PRIu64 ", options=%" PRI_PAGED_SPARSE_ARRAY_OPTIONS "", \
                    array, index, page_index, array->page_size, array->page_count, PAGED_SPARSE_ARRAY_OPTIONS_VALUES(array->options));                                    \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                array->pages[page_index] = page;                                                                                                                           \
            }                                                                                                                                                              \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
                                                                                                                                                                           \
    if (page == NULL)                                                                                                                                                      \
    {                                                                                                                                                                      \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_018: [ If malloc_flex fails, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/        \
        result = PAGED_SPARSE_ARRAY_ALLOCATE_ERROR;                                                                                                                        \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_057: [ If thread_safe is true, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall mark the element at index as allocated and increment the allocated count of the page using interlocked operations. ]*/ \
        if (PAGED_SPARSE_ARRAY_TEST_AND_SET_ALLOCATED_ATOMIC(PAGED_SPARSE_ARRAY_GET_BITMAP(page, array->page_size), index_in_page))                                        \
        {                                                                                                                                                                  \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_015: [ If the element at index is already allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED. ]*/ \
            result = PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED;                                                                                                        \
            LogError("Element is already allocated in PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) ") array=%p, uint64_t index=%" PRIu64 ", page_index=%" PRIu64 ", index_in_page=%" PRIu64 ", lock held exclusive=%" PRI_BOOL ", result=%" PRI_MU_ENUM "", \
                array, index, page_index, index_in_page, MU_BOOL_VALUE(is_lock_exclusive), MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result));                    \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            (void)interlocked_increment_64(PAGED_SPARSE_ARRAY_ALLOCATED_COUNT_ATOMIC(page));                                                                               \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_017: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall store a pointer to the element at index in allocated_ptr and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/ \
            *allocated_ptr = &page->items[index_in_page];                                                                                                                  \
            result = PAGED_SPARSE_ARRAY_ALLOCATE_OK;                                                                                                                       \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
                                                                                                                                                                           \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_058: [ If thread_safe is true, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the lock (shared or exclusive, whichever is held) before returning. ]*/ \
    if (is_lock_exclusive)                                                                                                                                                 \
    {                                                                                                                                                                      \
        srw_lock_ll_release_exclusive(&array->lock);                                                                                                                       \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        srw_lock_ll_release_shared(&array->lock);                                                                                                                          \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}                                                                                                                                                                          \
PAGED_SPARSE_ARRAY_ALLOCATE_RESULT PAGED_SPARSE_ARRAY_LL_ALLOCATE(C)(PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint64_t index, T** allocated_ptr)                       \
{                                                                                                                                                                          \
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result;                                                                                                                             \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_011: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/ \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_041: [ If allocated_ptr is NULL, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/ \
    if (                                                                                                                                                                   \
        (paged_sparse_array == NULL) ||                                                                                                                                    \
        (allocated_ptr == NULL)                                                                                                                                            \
        )                                                                                                                                                                  \
    {                                                                                                                                                                      \
        LogError("Invalid arguments: PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) ") paged_sparse_array=%p, " MU_TOSTRING(T) "** allocated_ptr=%p", paged_sparse_array, allocated_ptr); \
        result = PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS;                                                                                                                 \
    }                                                                                                                                                                      \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_012: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/ \
    else if (index >= paged_sparse_array->max_size)                                                                                                                        \
    {                                                                                                                                                                      \
        LogError("Invalid arguments: uint64_t index=%" PRIu64 " out of bound, max_size=%" PRIu64 "", index, paged_sparse_array->max_size);                                 \
        result = PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS;                                                                                                                 \
    }                                                                                                                                                                      \
    else if (paged_sparse_array->options.thread_safe)                                                                                                                      \
    {                                                                                                                                                                      \
        result = PAGED_SPARSE_ARRAY_LL_ALLOCATE_THREAD_SAFE_INTERNAL_NAME(C)(THANDLE_GET_T(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array), index, allocated_ptr);  \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_013: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall compute the page index as index / page_size. ]*/                                    \
        uint64_t page_index = index / paged_sparse_array->page_size;                                                                                                       \
        PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array);                                                 \
                                                                                                                                                                           \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_014: [ If the page is not allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call malloc_flex to allocate memory for the page containing page_size elements and an allocation bitmap, and initialize all elements as not allocated. ]*/ \
        if (                                                                                                                                                               \
            (array->pages[page_index] == NULL) &&                                                                                                                          \
            ((array->pages[page_index] = PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C)(paged_sparse_array->page_size, paged_sparse_array->options.zero_new_pages)) == NULL) \
            )                                                                                                                                                              \
        {                                                                                                                                                                  \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_018: [ If page allocation fails, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/ \
            LogError("failure allocating page at page_index=%" PRIu64 "", page_index);                                                                                     \
            result = PAGED_SPARSE_ARRAY_ALLOCATE_ERROR;                                                                                                                    \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            uint64_t index_in_page = index % paged_sparse_array->page_size;                                                                                                \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_015: [ If the element at index is already allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED. ]*/ \
            if (PAGED_SPARSE_ARRAY_IS_ALLOCATED(PAGED_SPARSE_ARRAY_GET_BITMAP(array->pages[page_index], paged_sparse_array->page_size), index_in_page))                    \
            {                                                                                                                                                              \
                LogError("Element at index=%" PRIu64 " is already allocated", index);                                                                                      \
                result = PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED;                                                                                                    \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                /* Codes_SRS_PAGED_SPARSE_ARRAY_88_016: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall mark the element at index as allocated. ]*/                                 \
                PAGED_SPARSE_ARRAY_SET_ALLOCATED(PAGED_SPARSE_ARRAY_GET_BITMAP(array->pages[page_index], paged_sparse_array->page_size), index_in_page);                   \
                array->pages[page_index]->allocated_count++;                                                                                                               \
                /* Codes_SRS_PAGED_SPARSE_ARRAY_88_017: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall store a pointer to the element at index in allocated_ptr and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/ \
                *allocated_ptr = &array->pages[page_index]->items[index_in_page];                                                                                          \
                result = PAGED_SPARSE_ARRAY_ALLOCATE_OK;                                                                                                                   \
            }                                                                                                                                                              \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}

#define PAGED_SPARSE_ARRAY_LL_RELEASE_DEFINE(C, T)                                                                                                                          \
static void PAGED_SPARSE_ARRAY_LL_RELEASE_THREAD_SAFE_INTERNAL_NAME(C)(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array, uint64_t index)                                           \
{                                                                                                                                                                           \
    bool try_free_page;                                                                                                                                                     \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_021: [ PAGED_SPARSE_ARRAY_RELEASE(T) shall compute the page index as index / page_size. ]*/                                          \
    uint64_t page_index = index / array->page_size;                                                                                                                         \
    uint64_t index_in_page = index % array->page_size;                                                                                                                      \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_059: [ If thread_safe is true, PAGED_SPARSE_ARRAY_RELEASE(T) shall acquire the lock in shared mode. ]*/                              \
    srw_lock_ll_acquire_shared(&array->lock);                                                                                                                               \
    PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)* page = array->pages[page_index];                                                                                               \
    if (page == NULL)                                                                                                                                                       \
    {                                                                                                                                                                       \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_022: [ If the page is not allocated, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/                                             \
        try_free_page = false;                                                                                                                                              \
    }                                                                                                                                                                       \
    else                                                                                                                                                                    \
    {                                                                                                                                                                       \
        uint8_t* bitmap = PAGED_SPARSE_ARRAY_GET_BITMAP(page, array->page_size);                                                                                            \
        if (!PAGED_SPARSE_ARRAY_IS_ALLOCATED_ATOMIC(bitmap, index_in_page))                                                                                                 \
        {                                                                                                                                                                   \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_023: [ If the element at index is not allocated, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/                             \
            try_free_page = false;                                                                                                                                          \
        }                                                                                                                                                                   \
        else                                                                                                                                                                \
        {                                                                                                                                                                   \
            if (array->item_dispose_func == NULL)                                                                                                                           \
            {                                                                                                                                                               \
                /* do nothing */                                                                                                                                            \
            }                                                                                                                                                               \
            else                                                                                                                                                            \
            {                                                                                                                                                               \
                /* Codes_SRS_PAGED_SPARSE_ARRAY_88_044: [ If item_dispose_func is not NULL, PAGED_SPARSE_ARRAY_RELEASE(T) shall call item_dispose_func for the element being released. ]*/ \
                array->item_dispose_func(&page->items[index_in_page]);                                                                                                      \
            }                                                                                                                                                               \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_060: [ If thread_safe is true, PAGED_SPARSE_ARRAY_RELEASE(T) shall mark the element at index as not allocated using interlocked operations and, only if the element was still marked as allocated, decrement the allocated count of the page using interlocked operations. ]*/ \
            if (!PAGED_SPARSE_ARRAY_TEST_AND_CLEAR_ALLOCATED_ATOMIC(bitmap, index_in_page))                                                                                 \
            {                                                                                                                                                               \
                /* the element was released concurrently by another caller, which already decremented the allocated count */                                               \
                try_free_page = false;                                                                                                                                      \
            }                                                                                                                                                               \
            else                                                                                                                                                            \
            {                                                                                                                                                               \
                try_free_page = (interlocked_decrement_64(PAGED_SPARSE_ARRAY_ALLOCATED_COUNT_ATOMIC(page)) == 0) && array->options.auto_free_empty_pages;                   \
            }                                                                                                                                                               \
        }                                                                                                                                                                   \
    }                                                                                                                                                                       \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_061: [ If thread_safe is true, PAGED_SPARSE_ARRAY_RELEASE(T) shall release the shared lock. ]*/                                      \
    srw_lock_ll_release_shared(&array->lock);                                                                                                                               \
                                                                                                                                                                            \
    if (!try_free_page)                                                                                                                                                     \
    {                                                                                                                                                                       \
        /* do nothing */                                                                                                                                                    \
    }                                                                                                                                                                       \
    else                                                                                                                                                                    \
    {                                                                                                                                                                       \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_062: [ If thread_safe is true, auto_free_empty_pages is true and the allocated count of the page became 0, PAGED_SPARSE_ARRAY_RELEASE(T) shall acquire the lock in exclusive mode, free the page only if it is still allocated and its allocated count is still 0, and release the exclusive lock. ]*/ \
        srw_lock_ll_acquire_exclusive(&array->lock);                                                                                                                        \
        (void)PAGED_SPARSE_ARRAY_LL_FREE_PAGE_IF_EMPTY_INTERNAL_NAME(C)(array, page_index);                                                                                 \
        srw_lock_ll_release_exclusive(&array->lock);                                                                                                                        \
    }                                                                                                                                                                       \
}                                                                                                                                                                           \
void PAGED_SPARSE_ARRAY_LL_RELEASE(C)(PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint64_t index)                                                                          \
{                                                                                                                                                                           \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_019: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/                                                \
    if (paged_sparse_array == NULL)                                                                                                                                         \
    {                                                                                                                                                                       \
        LogError("Invalid arguments: PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) ") paged_sparse_array=%p", paged_sparse_array);                                                    \
    }                                                                                                                                                                       \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_020: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/                                \
    else if (index >= paged_sparse_array->max_size)                                                                                                                         \
    {                                                                                                                                                                       \
        LogError("Invalid arguments: uint64_t index=%" PRIu64 " out of bound, max_size=%" PRIu64 "", index, paged_sparse_array->max_size);                                  \
    }                                                                                                                                                                       \
    else if (paged_sparse_array->options.thread_safe)                                                                                                                       \
    {                                                                                                                                                                       \
        PAGED_SPARSE_ARRAY_LL_RELEASE_THREAD_SAFE_INTERNAL_NAME(C)(THANDLE_GET_T(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array), index);                           \
    }                                                                                                                                                                       \
    else                                                                                                                                                                    \
    {                                                                                                                                                                       \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_021: [ PAGED_SPARSE_ARRAY_RELEASE(T) shall compute the page index as index / page_size. ]*/                                      \
        uint64_t page_index = index / paged_sparse_array->page_size;                                                                                                        \
        PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array);                                                  \
                                                                                                                                                                            \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_022: [ If the page is not allocated, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/                                             \
        if (array->pages[page_index] == NULL)                                                                                                                               \
        {                                                                                                                                                                   \
            /* Page at page_index is not allocated, nothing to release */                                                                                                   \
        }                                                                                                                                                                   \
        else                                                                                                                                                                \
        {                                                                                                                                                                   \
            uint64_t index_in_page = index % paged_sparse_array->page_size;                                                                                                 \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_023: [ If the element at index is not allocated, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/                             \
            if (!PAGED_SPARSE_ARRAY_IS_ALLOCATED(PAGED_SPARSE_ARRAY_GET_BITMAP(array->pages[page_index], paged_sparse_array->page_size), index_in_page))                 \
            {                                                                                                                                                               \
                /* Element at index is not allocated, nothing to release */                                                                                                 \
            }                                                                                                                                                               \
            else                                                                                                                                                            \
            {                                                                                                                                                               \
                /* Codes_SRS_PAGED_SPARSE_ARRAY_88_044: [ If item_dispose_func is not NULL, PAGED_SPARSE_ARRAY_RELEASE(T) shall call item_dispose_func for the element being released. ]*/ \
                if (array->item_dispose_func != NULL)                                                                                                                       \
                {                                                                                                                                                           \
                    array->item_dispose_func(&array->pages[page_index]->items[index_in_page]);                                                                              \
                }                                                                                                                                                           \
                /* Codes_SRS_PAGED_SPARSE_ARRAY_88_024: [ PAGED_SPARSE_ARRAY_RELEASE(T) shall mark the element at index as not allocated. ]*/                               \
                PAGED_SPARSE_ARRAY_CLEAR_ALLOCATED(PAGED_SPARSE_ARRAY_GET_BITMAP(array->pages[page_index], paged_sparse_array->page_size), index_in_page);               \
                array->pages[page_index]->allocated_count--;                                                                                                                \
                                                                                                                                                                            \
                /* Codes_SRS_PAGED_SPARSE_ARRAY_88_025: [ If auto_free_empty_pages is true and all elements in the page are now not allocated, PAGED_SPARSE_ARRAY_RELEASE(T) shall free the page and set the page pointer to NULL. ]*/ \
                if (                                                                                                                                                        \
                    (array->options.auto_free_empty_pages) &&                                                                                                               \
                    (array->pages[page_index]->allocated_count == 0)                                                                                                        \
                    )                                                                                                                                                       \
                {                                                                                                                                                           \
                    /* bitmap is embedded in same allocation as page, so only one free needed */                                                                            \
                    free((void*)array->pages[page_index]);                                                                                                                  \
                    array->pages[page_index] = NULL;                                                                                                                        \
                }                                                                                                                                                           \
                else                                                                                                                                                        \
                {                                                                                                                                                           \
                    /* do nothing, the page is kept */                                                                                                                      \
                }                                                                                                                                                           \
            }                                                                                                                                                               \
        }                                                                                                                                                                   \
    }                                                                                                                                                                       \
}

#define PAGED_SPARSE_ARRAY_LL_GET_DEFINE(C, T)                                                                                                                               \
static PAGED_SPARSE_ARRAY_GET_RESULT PAGED_SPARSE_ARRAY_LL_GET_THREAD_SAFE_INTERNAL_NAME(C)(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array, uint64_t index, T** item)             \
{                                                                                                                                                                            \
    PAGED_SPARSE_ARRAY_GET_RESULT result;                                                                                                                                    \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_036: [ PAGED_SPARSE_ARRAY_GET(T) shall compute the page index as index / page_size. ]*/                                               \
    uint64_t page_index = index / array->page_size;                                                                                                                          \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_063: [ If thread_safe is true, PAGED_SPARSE_ARRAY_GET(T) shall acquire the lock in shared mode. ]*/                                   \
    srw_lock_ll_acquire_shared(&array->lock);                                                                                                                                \
    PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)* page = array->pages[page_index];                                                                                                \
    if (page == NULL)                                                                                                                                                        \
    {                                                                                                                                                                        \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_037: [ If the page is not allocated, PAGED_SPARSE_ARRAY_GET(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/              \
        result = PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED;                                                                                                                       \
    }                                                                                                                                                                        \
    else                                                                                                                                                                     \
    {                                                                                                                                                                        \
        uint64_t index_in_page = index % array->page_size;                                                                                                                   \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_064: [ If thread_safe is true, PAGED_SPARSE_ARRAY_GET(T) shall check whether the element at index is allocated using interlocked operations. ]*/ \
        if (!PAGED_SPARSE_ARRAY_IS_ALLOCATED_ATOMIC(PAGED_SPARSE_ARRAY_GET_BITMAP(page, array->page_size), index_in_page))                                                   \
        {                                                                                                                                                                    \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_038: [ If the element at index is not allocated, PAGED_SPARSE_ARRAY_GET(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/ \
            result = PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED;                                                                                                                   \
        }                                                                                                                                                                    \
        else                                                                                                                                                                 \
        {                                                                                                                                                                    \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_039: [ PAGED_SPARSE_ARRAY_GET(T) shall store in item a pointer to the element at index and return PAGED_SPARSE_ARRAY_GET_OK. ]*/ \
            *item = &page->items[index_in_page];                                                                                                                             \
            result = PAGED_SPARSE_ARRAY_GET_OK;                                                                                                                              \
        }                                                                                                                                                                    \
    }                                                                                                                                                                        \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_065: [ If thread_safe is true, PAGED_SPARSE_ARRAY_GET(T) shall release the shared lock before returning. ]*/                          \
    srw_lock_ll_release_shared(&array->lock);                                                                                                                                \
    return result;                                                                                                                                                           \
}                                                                                                                                                                            \
PAGED_SPARSE_ARRAY_GET_RESULT PAGED_SPARSE_ARRAY_LL_GET(C)(PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint64_t index, T** item)                                            \
{                                                                                                                                                                            \
    PAGED_SPARSE_ARRAY_GET_RESULT result;                                                                                                                                    \
    if (                                                                                                                                                         \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_034: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/ \
        paged_sparse_array == NULL ||                                                                                                                            \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_045: [ If item is NULL, PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/ \
        item == NULL ||                                                                                                                                          \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_035: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/ \
        index >= paged_sparse_array->max_size)                                                                                                                   \
    {                                                                                                                                                        \
        LogError("Invalid arguments: PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) ") paged_sparse_array=%p, item=%p, uint64_t index=%" PRIu64, paged_sparse_array, item, index); \
        result = PAGED_SPARSE_ARRAY_GET_INVALID_ARGS;                                                                                                        \
    }                                                                                                                                                        \
    else if (paged_sparse_array->options.thread_safe)                                                                                                                        \
    {                                                                                                                                                                        \
        result = PAGED_SPARSE_ARRAY_LL_GET_THREAD_SAFE_INTERNAL_NAME(C)(THANDLE_GET_T(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array), index, item);                 \
    }                                                                                                                                                                        \
    else                                                                                                                                                                     \
    {                                                                                                                                                                        \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_036: [ PAGED_SPARSE_ARRAY_GET(T) shall compute the page index as index / page_size. ]*/                                           \
        uint64_t page_index = index / paged_sparse_array->page_size;                                                                                                          \
                                                                                                                                                                              \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_037: [ If the page is not allocated, PAGED_SPARSE_ARRAY_GET(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/              \
        if (paged_sparse_array->pages[page_index] == NULL)                                                                                                                    \
        {                                                                                                                                                                    \
            result = PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED;                                                                                                                   \
        }                                                                                                                                                                    \
        else                                                                                                                                                                 \
        {                                                                                                                                                                    \
            uint64_t index_in_page = index % paged_sparse_array->page_size;                                                                                                   \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_038: [ If the element at index is not allocated, PAGED_SPARSE_ARRAY_GET(T) shall return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/ \
            if (!PAGED_SPARSE_ARRAY_IS_ALLOCATED(PAGED_SPARSE_ARRAY_GET_BITMAP(paged_sparse_array->pages[page_index], paged_sparse_array->page_size), index_in_page))      \
            {                                                                                                                                                                \
                result = PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED;                                                                                                               \
            }                                                                                                                                                                \
            else                                                                                                                                                             \
            {                                                                                                                                                                \
                /* Codes_SRS_PAGED_SPARSE_ARRAY_88_039: [ PAGED_SPARSE_ARRAY_GET(T) shall store in item a pointer to the element at index and return PAGED_SPARSE_ARRAY_GET_OK. ]*/ \
                *item = &paged_sparse_array->pages[page_index]->items[index_in_page];                                                                                         \
                result = PAGED_SPARSE_ARRAY_GET_OK;                                                                                                                          \
            }                                                                                                                                                                \
        }                                                                                                                                                                    \
    }                                                                                                                                                                        \
    return result;                                                                                                                                                           \
}

#define PAGED_SPARSE_ARRAY_LL_CREATE_WITH_OPTIONS_DEFINE(C, T)                                                                                                            \
/*hint: have PAGED_SPARSE_ARRAY_LL_CREATE_DEFINE(C, T) before PAGED_SPARSE_ARRAY_LL_CREATE_WITH_OPTIONS_DEFINE(C, T)*/                                                    \
PAGED_SPARSE_ARRAY_LL(T) PAGED_SPARSE_ARRAY_LL_CREATE_WITH_OPTIONS(C)(uint64_t max_size, uint64_t page_size, PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func, const PAGED_SPARSE_ARRAY_OPTIONS* options) \
{                                                                                                                                                                         \
    PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* result;                                                                                                                           \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_047: [ If options is NULL, PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T) shall fail and return NULL. ]*/                              \
    if (options == NULL)                                                                                                                                                  \
    {                                                                                                                                                                     \
        LogError("Invalid arguments: PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) "), uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", PAGED_SPARSE_ARRAY_ITEM_DISPOSE_FUNC(" MU_TOSTRING(T) ") item_dispose_func=%p, const PAGED_SPARSE_ARRAY_OPTIONS* options=%p", \
            max_size, page_size, item_dispose_func, options);                                                                                                             \
        result = NULL;                                                                                                                                                    \
    }                                                                                                                                                                     \
    else                                                                                                                                                                  \
    {                                                                                                                                                                     \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_048: [ PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T) shall validate max_size and page_size, and allocate and initialize the paged sparse array, in the same way as PAGED_SPARSE_ARRAY_CREATE(T). ]*/ \
        result = PAGED_SPARSE_ARRAY_LL_CREATE_INTERNAL_NAME(C)(max_size, page_size, item_dispose_func, options);                                                          \
    }                                                                                                                                                                     \
    return result;                                                                                                                                                        \
}

#define PAGED_SPARSE_ARRAY_LL_FREE_PAGE_IF_EMPTY_DEFINE(C, T)                                                                                                             \
/*hint: have PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_DEFINE(C, T) before PAGED_SPARSE_ARRAY_LL_FREE_PAGE_IF_EMPTY_DEFINE(C, T)*/                                     \
PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT PAGED_SPARSE_ARRAY_LL_FREE_PAGE_IF_EMPTY(C)(PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint64_t index)                    \
{                                                                                                                                                                         \
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT result;                                                                                                                  \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_066: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall fail and return PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_INVALID_ARGS. ]*/ \
    if (paged_sparse_array == NULL)                                                                                                                                       \
    {                                                                                                                                                                     \
        result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_INVALID_ARGS;                                                                                                      \
        LogError("Invalid arguments: PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) ") paged_sparse_array=%p, uint64_t index=%" PRIu64 ", result=%" PRI_MU_ENUM "",                  \
            paged_sparse_array, index, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, result));                                                              \
    }                                                                                                                                                                     \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_067: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall fail and return PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_INVALID_ARGS. ]*/ \
    else if (index >= paged_sparse_array->max_size)                                                                                                                       \
    {                                                                                                                                                                     \
        result = PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_INVALID_ARGS;                                                                                                      \
        LogError("Invalid arguments: PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) ") paged_sparse_array=%p, uint64_t index=%" PRIu64 " out of bound, max_size=%" PRIu64 ", page_size=%" PRIu64 ", result=%" PRI_MU_ENUM "", \
            paged_sparse_array, index, paged_sparse_array->max_size, paged_sparse_array->page_size, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, result)); \
    }                                                                                                                                                                     \
    else                                                                                                                                                                  \
    {                                                                                                                                                                     \
        PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array);                                                \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_069: [ PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall compute the page index as index / page_size. ]*/                           \
        uint64_t page_index = index / array->page_size;                                                                                                                   \
        if (!array->options.thread_safe)                                                                                                                                  \
        {                                                                                                                                                                 \
            result = PAGED_SPARSE_ARRAY_LL_FREE_PAGE_IF_EMPTY_INTERNAL_NAME(C)(array, page_index);                                                                        \
        }                                                                                                                                                                 \
        else                                                                                                                                                              \
        {                                                                                                                                                                 \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_068: [ If thread_safe is true, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall acquire the lock in exclusive mode. ]*/       \
            srw_lock_ll_acquire_exclusive(&array->lock);                                                                                                                  \
            result = PAGED_SPARSE_ARRAY_LL_FREE_PAGE_IF_EMPTY_INTERNAL_NAME(C)(array, page_index);                                                                        \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_073: [ If thread_safe is true, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T) shall release the exclusive lock before returning. ]*/ \
            srw_lock_ll_release_exclusive(&array->lock);                                                                                                                  \
        }                                                                                                                                                                 \
    }                                                                                                                                                                     \
    return result;                                                                                                                                                        \
}

#endif /*PAGED_SPARSE_ARRAY_LL_H*/
