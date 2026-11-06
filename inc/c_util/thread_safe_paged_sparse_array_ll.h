// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_H
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_H

#ifdef __cplusplus
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#else // __cplusplus
#include <inttypes.h>
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

#include "c_util/paged_sparse_array_ll.h"

#include "umock_c/umock_c_prod.h"

/*
 * THREAD_SAFE_PAGED_SPARSE_ARRAY is a paged sparse array (see paged_sparse_array_ll.h) whose ALLOCATE, RELEASE, GET and
 * FREE_EMPTY_PAGES can be called concurrently from multiple threads.
 * ALLOCATE, RELEASE and GET do not acquire any lock when the page of the element is allocated. While they use a page they
 * increment its use count, and a page is freed only when its use count is 0.
 * Each page has its own lock, which is acquired only to allocate or to free that page.
 * It uses the result codes PAGED_SPARSE_ARRAY_ALLOCATE_RESULT and PAGED_SPARSE_ARRAY_GET_RESULT and the bitmap helpers of paged_sparse_array_ll.h.
 * 2 options are selected at compile time, each one by a name:
 * - empty_pages: AUTO_FREE_EMPTY_PAGES (a page is freed when no element in it is allocated and no call uses it anymore)
 *   or KEEP_EMPTY_PAGES (empty pages are kept until FREE_EMPTY_PAGES is called or the array is disposed).
 * - new_pages: DO_NOT_ZERO_NEW_PAGES (the items of a newly allocated page are left uninitialized)
 *   or ZERO_NEW_PAGES (all the items of a newly allocated page are set to zero).
 * The _DEFINE_WITH_OPTIONS macros take the 2 options. The _DEFINE macros use AUTO_FREE_EMPTY_PAGES and DO_NOT_ZERO_NEW_PAGES.
 * Any other value of an option, including the value of the other option, does not compile.
 * new_pages changes only the allocation of a page and empty_pages changes only the end of the use of a page,
 * the other functions only log the options.
 * All the functions of a type shall be defined with the same options, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_DEFINE_WITH_OPTIONS(C, T, ...)
 * defines all of them at once.
 */

/*result codes for THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT_VALUES \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK, \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS

MU_DEFINE_ENUM(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT_VALUES)

/*values of the empty_pages option, the option is given to the _WITH_OPTIONS macros as AUTO_FREE_EMPTY_PAGES or KEEP_EMPTY_PAGES*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_EMPTY_PAGES_OPTION_AUTO_FREE_EMPTY_PAGES 1
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_EMPTY_PAGES_OPTION_KEEP_EMPTY_PAGES 0

/*values of the new_pages option, the option is given to the _WITH_OPTIONS macros as DO_NOT_ZERO_NEW_PAGES or ZERO_NEW_PAGES*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_NEW_PAGES_OPTION_DO_NOT_ZERO_NEW_PAGES 0
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_NEW_PAGES_OPTION_ZERO_NEW_PAGES 1

/*expand to 1 or 0, an option that is not one of the names above expands to an undeclared identifier and does not compile*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_IS_AUTO_FREE_EMPTY_PAGES(empty_pages) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_EMPTY_PAGES_OPTION_, empty_pages)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_IS_ZERO_NEW_PAGES(new_pages) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_NEW_PAGES_OPTION_, new_pages)

/*string literal that describes a thread safe paged sparse array type and its options, used in logs*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages) \
    "THREAD_SAFE_PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) ") (" MU_TOSTRING(empty_pages) ", " MU_TOSTRING(new_pages) ")"

/*added to the use count of a page while the page is being freed, it is larger than any count of allocated elements and calls that use the page*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING (INT64_C(1) << 62)

/*the array has one page slot for each page, the page slots exist as long as the array exists, even when their page is not allocated*/
typedef struct THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT_TAG
{
    /*count of allocated elements in the page plus count of calls that use the page, plus THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING while the page is being freed*/
    volatile_atomic int64_t use_count;
    /*page_size items of type T followed by the allocation bitmap, NULL when the page is not allocated*/
    void* volatile_atomic page;
    /*acquired in exclusive mode only to allocate or to free the page*/
    SRW_LOCK_LL lock;
} THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT;

/*the allocation bitmap of a page is stored right after the page_size items of the page*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_BITMAP(page, page_size) ((uint8_t*)&(page)[(page_size)])

/*THREAD_SAFE_PAGED_SPARSE_ARRAY is backed by a THANDLE build on the structure below*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_TYPE_NAME_TAG(T) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T), _TAG)

#define THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_, T)

/*custom item dispose function type*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC_, T)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_ITEM_DISPOSE_FUNC_TYPE(T) typedef void (*THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T))(T* item)

/*THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T) introduces the base type that holds the thread safe paged sparse array*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(T)                                                                         \
/*define the item dispose function type*/                                                                                            \
THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_ITEM_DISPOSE_FUNC_TYPE(T);                                                                     \
/*forward define the typedef of the THREAD_SAFE_PAGED_SPARSE_ARRAY struct so that it can be used for a function pointer definition*/ \
typedef struct THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_TYPE_NAME_TAG(T) THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T);                \
struct THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_TYPE_NAME_TAG(T)                                                                        \
{                                                                                                                                    \
    uint64_t max_size;                                                                                                               \
    uint64_t page_size;                                                                                                              \
    uint64_t page_count;                                                                                                             \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func;                                                        \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT page_slots[];                                                                           \
};                                                                                                                                   \

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

/*introduces a name for the free empty pages function*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_FREE_EMPTY_PAGES_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_FREE_EMPTY_PAGES_, C)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_FREE_EMPTY_PAGES(C) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_FREE_EMPTY_PAGES_NAME(C)

/*introduces names for the internal functions that allocate a page, free a page if it is empty and not used, and begin and end the use of a page*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_, C)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TRY_FREE_PAGE_INTERNAL_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TRY_FREE_PAGE_INTERNAL_, C)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_BEGIN_PAGE_USE_INTERNAL_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_BEGIN_PAGE_USE_INTERNAL_, C)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_, C)

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

/*introduces a function declaration for thread_safe_paged_sparse_array_free_empty_pages*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_FREE_EMPTY_PAGES_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_FREE_EMPTY_PAGES(C), THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T), paged_sparse_array, uint64_t, start_index, uint64_t, end_index);

/*the bitmap bytes are read and changed with interlocked operations because elements of the same page are allocated and released concurrently*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_BITMAP_BYTE(bitmap, index_in_page) ((volatile_atomic int8_t*)&(bitmap)[(index_in_page) / 8])

#define THREAD_SAFE_PAGED_SPARSE_ARRAY_BITMAP_MASK(index_in_page) ((int8_t)(1 << ((index_in_page) % 8)))

/*helper to atomically check if an element is allocated in the bitmap*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_IS_ALLOCATED(bitmap, index_in_page) \
    ((interlocked_or_8(THREAD_SAFE_PAGED_SPARSE_ARRAY_BITMAP_BYTE(bitmap, index_in_page), 0) & THREAD_SAFE_PAGED_SPARSE_ARRAY_BITMAP_MASK(index_in_page)) != 0)

/*helper to atomically set an element as allocated in the bitmap, evaluates to true if the element was already allocated*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_TEST_AND_SET_ALLOCATED(bitmap, index_in_page) \
    ((interlocked_or_8(THREAD_SAFE_PAGED_SPARSE_ARRAY_BITMAP_BYTE(bitmap, index_in_page), THREAD_SAFE_PAGED_SPARSE_ARRAY_BITMAP_MASK(index_in_page)) & THREAD_SAFE_PAGED_SPARSE_ARRAY_BITMAP_MASK(index_in_page)) != 0)

/*helper to atomically clear an element as not allocated in the bitmap, evaluates to true if the element was allocated*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_TEST_AND_CLEAR_ALLOCATED(bitmap, index_in_page) \
    ((interlocked_and_8(THREAD_SAFE_PAGED_SPARSE_ARRAY_BITMAP_BYTE(bitmap, index_in_page), (int8_t)~THREAD_SAFE_PAGED_SPARSE_ARRAY_BITMAP_MASK(index_in_page)) & THREAD_SAFE_PAGED_SPARSE_ARRAY_BITMAP_MASK(index_in_page)) != 0)

/*introduces a function definition for freeing the allocated resources for a THREAD_SAFE_PAGED_SPARSE_ARRAY*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages)                                                                        \
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_NAME(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* paged_sparse_array)                                          \
{                                                                                                                                                                          \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_012: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall return. ]*/                       \
    if (paged_sparse_array == NULL)                                                                                                                                        \
    {                                                                                                                                                                      \
        LogError("invalid arguments " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages) " paged_sparse_array=%p",                                   \
            paged_sparse_array);                                                                                                                                           \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* the reference count reached 0, so no other thread uses the array, the page slots are read without interlocked operations and locks */                           \
        for (uint64_t i = 0; i < paged_sparse_array->page_count; i++)                                                                                                      \
        {                                                                                                                                                                  \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT* page_slot = &paged_sparse_array->page_slots[i];                                                                      \
            T* page = (T*)page_slot->page;                                                                                                                                 \
            if (page == NULL)                                                                                                                                              \
            {                                                                                                                                                              \
                /* do nothing, the page is not allocated */                                                                                                                \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                if (paged_sparse_array->item_dispose_func == NULL)                                                                                                         \
                {                                                                                                                                                          \
                    /* do nothing */                                                                                                                                       \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_013: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call item_dispose_func for each element that is still allocated. ]*/ \
                    uint8_t* bitmap = THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_BITMAP(page, paged_sparse_array->page_size);                                                     \
                    for (uint64_t j = 0; j < paged_sparse_array->page_size; j++)                                                                                           \
                    {                                                                                                                                                      \
                        if (PAGED_SPARSE_ARRAY_IS_ALLOCATED(bitmap, j))                                                                                                    \
                        {                                                                                                                                                  \
                            paged_sparse_array->item_dispose_func(&page[j]);                                                                                               \
                        }                                                                                                                                                  \
                    }                                                                                                                                                      \
                }                                                                                                                                                          \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_014: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall free all pages that are non-NULL. ]*/                \
                free((void*)page);                                                                                                                                         \
            }                                                                                                                                                              \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_015: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T) shall call srw_lock_ll_deinit for the lock of each page slot. ]*/ \
            srw_lock_ll_deinit(&page_slot->lock);                                                                                                                          \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
}

#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_DEFINE(C, T) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_DEFINE_WITH_OPTIONS(C, T, AUTO_FREE_EMPTY_PAGES, DO_NOT_ZERO_NEW_PAGES)

/*introduces the internal functions that allocate a page, free a page if it is empty and not used, and begin and end the use of a page*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INTERNAL_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages)                                                                       \
MU_SUPPRESS_WARNING(4505) /*warning C4505: 'function': unreferenced function with internal linkage has been removed*/                                                      \
static T* THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C)(uint64_t page_size)                                                                             \
{                                                                                                                                                                          \
    size_t bitmap_size = PAGED_SPARSE_ARRAY_BITMAP_SIZE(page_size);                                                                                                        \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_038: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call malloc_flex to allocate memory for page_size elements followed by the allocation bitmap. ]*/ \
    T* page = malloc_flex(bitmap_size, (size_t)page_size, sizeof(T));                                                                                                      \
    if (page == NULL)                                                                                                                                                      \
    {                                                                                                                                                                      \
        LogError("failure in malloc_flex(bitmap_size=%zu, page_size=%" PRIu64 ", sizeof(" MU_TOSTRING(T) ")=%zu) for a page of " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages), \
            bitmap_size, page_size, sizeof(T));                                                                                                                            \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_040: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall set all the bits of the allocation bitmap to 0. ]*/         \
        (void)memset(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_BITMAP(page, page_size), 0, bitmap_size);                                                                         \
        /* new_pages is a compile-time constant, the compiler keeps only one of the branches */                                                                            \
        if (THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_IS_ZERO_NEW_PAGES(new_pages))                                                                                                \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_041: [ If new_pages is ZERO_NEW_PAGES, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall set all the items of the page to zero. ]*/ \
            (void)memset((void*)page, 0, (size_t)page_size * sizeof(T));                                                                                                   \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* do nothing, items are left uninitialized */                                                                                                                 \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    return page;                                                                                                                                                           \
}                                                                                                                                                                          \
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TRY_FREE_PAGE_INTERNAL_NAME(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array, uint64_t page_index)                   \
{                                                                                                                                                                          \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT* page_slot = &array->page_slots[page_index];                                                                                  \
    T* page_to_free;                                                                                                                                                       \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_021: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall acquire the lock of the page slot in exclusive mode. ]*/   \
    srw_lock_ll_acquire_exclusive(&page_slot->lock);                                                                                                                       \
    T* page = (T*)interlocked_compare_exchange_pointer(&page_slot->page, NULL, NULL);                                                                                      \
    if (page == NULL)                                                                                                                                                      \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_022: [ If the page is not allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release the lock of the page slot and return. ]*/ \
        page_to_free = NULL;                                                                                                                                               \
    }                                                                                                                                                                      \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_023: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall change the use count of the page slot from 0 to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING using interlocked operations. ]*/ \
    else if (interlocked_compare_exchange_64(&page_slot->use_count, THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING, 0) != 0)                                                  \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_024: [ If the use count was not 0, THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release the lock of the page slot and return. ]*/ \
        page_to_free = NULL;                                                                                                                                               \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* the calls that begin to use the page from now on see THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING and do not read the page pointer */                            \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall set the page pointer of the page slot to NULL using interlocked operations. ]*/ \
        (void)interlocked_exchange_pointer(&page_slot->page, NULL);                                                                                                        \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_026: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall subtract THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING from the use count of the page slot using interlocked operations. ]*/ \
        (void)interlocked_add_64(&page_slot->use_count, -THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING);                                                                     \
        page_to_free = page;                                                                                                                                               \
    }                                                                                                                                                                      \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_027: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall release the lock of the page slot. ]*/                     \
    srw_lock_ll_release_exclusive(&page_slot->lock);                                                                                                                       \
    if (page_to_free == NULL)                                                                                                                                              \
    {                                                                                                                                                                      \
        /* do nothing, the page is not allocated, has allocated elements or is used by another call */                                                                     \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_028: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T) shall free the page. ]*/                                     \
        free((void*)page_to_free);                                                                                                                                         \
    }                                                                                                                                                                      \
}                                                                                                                                                                          \
static T* THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_BEGIN_PAGE_USE_INTERNAL_NAME(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array, uint64_t page_index)                    \
{                                                                                                                                                                          \
    T* result;                                                                                                                                                             \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT* page_slot = &array->page_slots[page_index];                                                                                  \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_016: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall increment the use count of the page slot using interlocked operations. ]*/ \
    if (interlocked_increment_64(&page_slot->use_count) >= THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING)                                                                    \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_017: [ If the use count is greater than or equal to THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING, THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall return NULL. ]*/ \
        result = NULL;                                                                                                                                                     \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_018: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T) shall return the page pointer of the page slot, read using interlocked operations. ]*/ \
        result = (T*)interlocked_compare_exchange_pointer(&page_slot->page, NULL, NULL);                                                                                   \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}                                                                                                                                                                          \
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array, uint64_t page_index, int64_t ended_use_count) \
{                                                                                                                                                                          \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT* page_slot = &array->page_slots[page_index];                                                                                  \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_019: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) shall subtract ended_use_count from the use count of the page slot using interlocked operations. ]*/ \
    int64_t use_count = interlocked_add_64(&page_slot->use_count, -ended_use_count);                                                                                       \
    /* empty_pages is a compile-time constant, the compiler keeps only one of the branches */                                                                              \
    if (THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_IS_AUTO_FREE_EMPTY_PAGES(empty_pages))                                                                                           \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_020: [ If empty_pages is AUTO_FREE_EMPTY_PAGES, the use count became 0 and the page is allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T). ]*/ \
        if ((use_count == 0) && (interlocked_compare_exchange_pointer(&page_slot->page, NULL, NULL) != NULL))                                                              \
        {                                                                                                                                                                  \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TRY_FREE_PAGE_INTERNAL_NAME(C)(array, page_index);                                                                           \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* do nothing, the page has allocated elements, is used by another call or is not allocated */                                                                 \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* do nothing, empty pages are kept until FREE_EMPTY_PAGES is called or the array is disposed */                                                                   \
    }                                                                                                                                                                      \
}                                                                                                                                                                          \
MU_UNSUPPRESS_WARNING(4505)

#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INTERNAL_DEFINE(C, T) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INTERNAL_DEFINE_WITH_OPTIONS(C, T, AUTO_FREE_EMPTY_PAGES, DO_NOT_ZERO_NEW_PAGES)

/*introduces a function definition for thread_safe_paged_sparse_array_create*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages)                                                                         \
THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE(C)(uint64_t max_size, uint64_t page_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func) \
{                                                                                                                                                                          \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* result;                                                                                                                \
    if (                                                                                                                                                                   \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_001: [ If max_size is zero, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/                \
        (max_size == 0) ||                                                                                                                                                 \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_002: [ If page_size is zero, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/               \
        (page_size == 0) ||                                                                                                                                                \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_003: [ If max_size + page_size - 1 would overflow, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/ \
        (max_size > UINT64_MAX - (page_size - 1))                                                                                                                          \
        )                                                                                                                                                                  \
    {                                                                                                                                                                      \
        LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages) ", uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", item_dispose_func=%p", \
            max_size, page_size, item_dispose_func);                                                                                                                       \
        result = NULL;                                                                                                                                                     \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_004: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall compute the number of pages as (max_size + page_size - 1) / page_size. ]*/ \
        uint64_t page_count = (max_size + page_size - 1) / page_size;                                                                                                      \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_005: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX to allocate memory for the paged sparse array with one page slot for each page. ]*/ \
        result = THANDLE_MALLOC_FLEX(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_NAME(C), page_count, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT)); \
        if (result == NULL)                                                                                                                                                \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_011: [ If there are any other errors, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/  \
            LogError("failure in THANDLE_MALLOC_FLEX(" THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages) ") page_count=%" PRIu64 ", sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT)=%zu, uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", item_dispose_func=%p", \
                page_count, sizeof(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT), max_size, page_size, item_dispose_func);                                                     \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_006: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall store max_size, page_size, and item_dispose_func in the structure. ]*/ \
            result->max_size = max_size;                                                                                                                                   \
            result->page_size = page_size;                                                                                                                                 \
            result->page_count = page_count;                                                                                                                               \
            result->item_dispose_func = item_dispose_func;                                                                                                                 \
            uint64_t initialized_lock_count;                                                                                                                               \
            for (initialized_lock_count = 0; initialized_lock_count < page_count; initialized_lock_count++)                                                                \
            {                                                                                                                                                              \
                THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT* page_slot = &result->page_slots[initialized_lock_count];                                                         \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_007: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall set the page pointer of each page slot to NULL and its use count to 0. ]*/ \
                (void)interlocked_exchange_pointer(&page_slot->page, NULL);                                                                                                \
                (void)interlocked_exchange_64(&page_slot->use_count, 0);                                                                                                   \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_008: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call srw_lock_ll_init to initialize the lock of each page slot. ]*/ \
                int lock_init_result = srw_lock_ll_init(&page_slot->lock);                                                                                                 \
                if (lock_init_result != 0)                                                                                                                                 \
                {                                                                                                                                                          \
                    LogError("failure in srw_lock_ll_init(&page_slots[%" PRIu64 "].lock)=%d for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages) ", uint64_t max_size=%" PRIu64 ", uint64_t page_size=%" PRIu64 ", page_count=%" PRIu64 ", item_dispose_func=%p", \
                        initialized_lock_count, lock_init_result, max_size, page_size, page_count, item_dispose_func);                                                     \
                    break;                                                                                                                                                 \
                }                                                                                                                                                          \
            }                                                                                                                                                              \
            if (initialized_lock_count < page_count)                                                                                                                       \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_009: [ If srw_lock_ll_init fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall call srw_lock_ll_deinit for the locks that were initialized, free the paged sparse array and return NULL. ]*/ \
                for (uint64_t i = 0; i < initialized_lock_count; i++)                                                                                                      \
                {                                                                                                                                                          \
                    srw_lock_ll_deinit(&result->page_slots[i].lock);                                                                                                       \
                }                                                                                                                                                          \
                THANDLE_FREE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(result);                                                                                      \
                result = NULL;                                                                                                                                             \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/              \
            }                                                                                                                                                              \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}

#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE_DEFINE(C, T) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE_DEFINE_WITH_OPTIONS(C, T, AUTO_FREE_EMPTY_PAGES, DO_NOT_ZERO_NEW_PAGES)

/*introduces a function definition for thread_safe_paged_sparse_array_allocate*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages)                                                                       \
/*hint: have THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INTERNAL_DEFINE_WITH_OPTIONS(C, T, ...) before THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_DEFINE_WITH_OPTIONS(C, T, ...)*/  \
PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint64_t index, T** allocated_ptr) \
{                                                                                                                                                                          \
    PAGED_SPARSE_ARRAY_ALLOCATE_RESULT result;                                                                                                                             \
    if (                                                                                                                                                                   \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_029: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/ \
        (paged_sparse_array == NULL) ||                                                                                                                                    \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_030: [ If allocated_ptr is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/ \
        (allocated_ptr == NULL)                                                                                                                                            \
        )                                                                                                                                                                  \
    {                                                                                                                                                                      \
        result = PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS;                                                                                                                 \
        LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages) " paged_sparse_array=%p, uint64_t index=%" PRIu64 ", " MU_TOSTRING(T) "** allocated_ptr=%p, result=%" PRI_MU_ENUM "", \
            paged_sparse_array, index, allocated_ptr, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result));                                                          \
    }                                                                                                                                                                      \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_031: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS. ]*/ \
    else if (index >= paged_sparse_array->max_size)                                                                                                                        \
    {                                                                                                                                                                      \
        result = PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS;                                                                                                                 \
        LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages) " paged_sparse_array=%p, uint64_t index=%" PRIu64 " out of bound, max_size=%" PRIu64 ", page_size=%" PRIu64 ", " MU_TOSTRING(T) "** allocated_ptr=%p, result=%" PRI_MU_ENUM "", \
            paged_sparse_array, index, paged_sparse_array->max_size, paged_sparse_array->page_size, allocated_ptr, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result)); \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array);                         \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_032: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall compute the page index as index / page_size. ]*/            \
        uint64_t page_index = index / array->page_size;                                                                                                                    \
        uint64_t index_in_page = index % array->page_size;                                                                                                                 \
        THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_SLOT* page_slot = &array->page_slots[page_index];                                                                              \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_033: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T). ]*/  \
        T* page = THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_BEGIN_PAGE_USE_INTERNAL_NAME(C)(array, page_index);                                                                    \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_034: [ If the page is not allocated: ]*/                                                                            \
        if (page != NULL)                                                                                                                                                  \
        {                                                                                                                                                                  \
            /* do nothing, the page is allocated and it is not freed while it is used */                                                                                   \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_035: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/ \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 1);                                                                         \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_036: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall acquire the lock of the page slot in exclusive mode. ]*/ \
            srw_lock_ll_acquire_exclusive(&page_slot->lock);                                                                                                               \
            page = (T*)interlocked_compare_exchange_pointer(&page_slot->page, NULL, NULL);                                                                                 \
            if (page != NULL)                                                                                                                                              \
            {                                                                                                                                                              \
                /* do nothing, another thread allocated the page while the lock was not held */                                                                            \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_037: [ If the page is still not allocated: ]*/                                                              \
                page = THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C)(array->page_size);                                                                 \
                if (page == NULL)                                                                                                                                          \
                {                                                                                                                                                          \
                    /* do nothing, the error is logged below */                                                                                                            \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_042: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall store the page pointer in the page slot using interlocked operations. ]*/ \
                    (void)interlocked_exchange_pointer(&page_slot->page, (void*)page);                                                                                     \
                }                                                                                                                                                          \
            }                                                                                                                                                              \
            if (page == NULL)                                                                                                                                              \
            {                                                                                                                                                              \
                /* do nothing, the error is logged below */                                                                                                                \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                /* the page is not being freed, because freeing a page requires its lock */                                                                                \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_043: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall increment the use count of the page slot using interlocked operations. ]*/ \
                (void)interlocked_increment_64(&page_slot->use_count);                                                                                                     \
            }                                                                                                                                                              \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_044: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the lock of the page slot. ]*/                  \
            srw_lock_ll_release_exclusive(&page_slot->lock);                                                                                                               \
        }                                                                                                                                                                  \
        if (page == NULL)                                                                                                                                                  \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [ If malloc_flex fails, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall release the lock of the page slot and return PAGED_SPARSE_ARRAY_ALLOCATE_ERROR. ]*/ \
            result = PAGED_SPARSE_ARRAY_ALLOCATE_ERROR;                                                                                                                    \
            LogError("failure allocating page for " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages) " paged_sparse_array=%p, uint64_t index=%" PRIu64 ", page_index=%" PRIu64 ", page_size=%" PRIu64 ", page_count=%" PRIu64 ", result=%" PRI_MU_ENUM "", \
                paged_sparse_array, index, page_index, array->page_size, array->page_count, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result));                    \
        }                                                                                                                                                                  \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_045: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall mark the element at index as allocated using interlocked operations. ]*/ \
        else if (THREAD_SAFE_PAGED_SPARSE_ARRAY_TEST_AND_SET_ALLOCATED(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_BITMAP(page, array->page_size), index_in_page))                 \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_046: [ If the element at index was already marked as allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 and return PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED. ]*/ \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 1);                                                                         \
            result = PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED;                                                                                                        \
            LogError("Element is already allocated in " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages) " paged_sparse_array=%p, uint64_t index=%" PRIu64 ", page_index=%" PRIu64 ", index_in_page=%" PRIu64 ", result=%" PRI_MU_ENUM "", \
                paged_sparse_array, index, page_index, index_in_page, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, result));                                          \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* the use of the page is not ended, from now on it is the use of the allocated element */                                                                     \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_047: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T) shall store a pointer to the element at index in allocated_ptr and return PAGED_SPARSE_ARRAY_ALLOCATE_OK. ]*/ \
            *allocated_ptr = &page[index_in_page];                                                                                                                         \
            result = PAGED_SPARSE_ARRAY_ALLOCATE_OK;                                                                                                                       \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}

#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_DEFINE(C, T) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_DEFINE_WITH_OPTIONS(C, T, AUTO_FREE_EMPTY_PAGES, DO_NOT_ZERO_NEW_PAGES)

/*introduces a function definition for thread_safe_paged_sparse_array_release*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages)                                                                        \
/*hint: have THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INTERNAL_DEFINE_WITH_OPTIONS(C, T, ...) before THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE_DEFINE_WITH_OPTIONS(C, T, ...)*/   \
void THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint64_t index)                                                 \
{                                                                                                                                                                          \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_048: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/                       \
    if (paged_sparse_array == NULL)                                                                                                                                        \
    {                                                                                                                                                                      \
        LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages) " paged_sparse_array=%p, uint64_t index=%" PRIu64 "",      \
            paged_sparse_array, index);                                                                                                                                    \
    }                                                                                                                                                                      \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_049: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/       \
    else if (index >= paged_sparse_array->max_size)                                                                                                                        \
    {                                                                                                                                                                      \
        LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages) " paged_sparse_array=%p, uint64_t index=%" PRIu64 " out of bound, max_size=%" PRIu64 ", page_size=%" PRIu64 "", \
            paged_sparse_array, index, paged_sparse_array->max_size, paged_sparse_array->page_size);                                                                       \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array);                         \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_050: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall compute the page index as index / page_size. ]*/             \
        uint64_t page_index = index / array->page_size;                                                                                                                    \
        uint64_t index_in_page = index % array->page_size;                                                                                                                 \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_051: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T). ]*/   \
        T* page = THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_BEGIN_PAGE_USE_INTERNAL_NAME(C)(array, page_index);                                                                    \
        if (page == NULL)                                                                                                                                                  \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_052: [ If the page is not allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 and return. ]*/ \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 1);                                                                         \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            uint8_t* bitmap = THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_BITMAP(page, array->page_size);                                                                          \
            if (!THREAD_SAFE_PAGED_SPARSE_ARRAY_IS_ALLOCATED(bitmap, index_in_page))                                                                                       \
            {                                                                                                                                                              \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_053: [ If the element at index is not allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 and return. ]*/ \
                THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 1);                                                                     \
            }                                                                                                                                                              \
            else                                                                                                                                                           \
            {                                                                                                                                                              \
                if (array->item_dispose_func == NULL)                                                                                                                      \
                {                                                                                                                                                          \
                    /* do nothing */                                                                                                                                       \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_054: [ If item_dispose_func is not NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call item_dispose_func for the element being released before marking the element as not allocated. ]*/ \
                    array->item_dispose_func(&page[index_in_page]);                                                                                                        \
                }                                                                                                                                                          \
                /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_055: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall mark the element at index as not allocated using interlocked operations. ]*/ \
                if (THREAD_SAFE_PAGED_SPARSE_ARRAY_TEST_AND_CLEAR_ALLOCATED(bitmap, index_in_page))                                                                        \
                {                                                                                                                                                          \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_056: [ If the element was still marked as allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 2. ]*/ \
                    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 2);                                                                 \
                }                                                                                                                                                          \
                else                                                                                                                                                       \
                {                                                                                                                                                          \
                    /* the element was released concurrently by another caller (which callers shall not do), that caller ended the use of the element */                   \
                    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_057: [ Otherwise, THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1. ]*/ \
                    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 1);                                                                 \
                }                                                                                                                                                          \
            }                                                                                                                                                              \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
}

#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE_DEFINE(C, T) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE_DEFINE_WITH_OPTIONS(C, T, AUTO_FREE_EMPTY_PAGES, DO_NOT_ZERO_NEW_PAGES)

/*introduces a function definition for thread_safe_paged_sparse_array_get*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_GET_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages)                                                                            \
/*hint: have THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INTERNAL_DEFINE_WITH_OPTIONS(C, T, ...) before THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_GET_DEFINE_WITH_OPTIONS(C, T, ...)*/       \
PAGED_SPARSE_ARRAY_GET_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_GET(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint64_t index, T** item)                  \
{                                                                                                                                                                          \
    PAGED_SPARSE_ARRAY_GET_RESULT result;                                                                                                                                  \
    if (                                                                                                                                                                   \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_058: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/ \
        (paged_sparse_array == NULL) ||                                                                                                                                    \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_059: [ If item is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/ \
        (item == NULL)                                                                                                                                                     \
        )                                                                                                                                                                  \
    {                                                                                                                                                                      \
        result = PAGED_SPARSE_ARRAY_GET_INVALID_ARGS;                                                                                                                      \
        LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages) " paged_sparse_array=%p, uint64_t index=%" PRIu64 ", " MU_TOSTRING(T) "** item=%p, result=%" PRI_MU_ENUM "", \
            paged_sparse_array, index, item, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, result));                                                                        \
    }                                                                                                                                                                      \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_060: [ If index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall fail and return PAGED_SPARSE_ARRAY_GET_INVALID_ARGS. ]*/ \
    else if (index >= paged_sparse_array->max_size)                                                                                                                        \
    {                                                                                                                                                                      \
        result = PAGED_SPARSE_ARRAY_GET_INVALID_ARGS;                                                                                                                      \
        LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages) " paged_sparse_array=%p, uint64_t index=%" PRIu64 " out of bound, max_size=%" PRIu64 ", page_size=%" PRIu64 ", " MU_TOSTRING(T) "** item=%p, result=%" PRI_MU_ENUM "", \
            paged_sparse_array, index, paged_sparse_array->max_size, paged_sparse_array->page_size, item, MU_ENUM_VALUE(PAGED_SPARSE_ARRAY_GET_RESULT, result));           \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array);                         \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_061: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall compute the page index as index / page_size. ]*/                 \
        uint64_t page_index = index / array->page_size;                                                                                                                    \
        uint64_t index_in_page = index % array->page_size;                                                                                                                 \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T). ]*/       \
        T* page = THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_BEGIN_PAGE_USE_INTERNAL_NAME(C)(array, page_index);                                                                    \
        if (page == NULL)                                                                                                                                                  \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_063: [ If the page is not allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 and return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/ \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 1);                                                                         \
            result = PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED;                                                                                                                 \
        }                                                                                                                                                                  \
        else if (!THREAD_SAFE_PAGED_SPARSE_ARRAY_IS_ALLOCATED(THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_BITMAP(page, array->page_size), index_in_page))                          \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_064: [ If the element at index is not allocated, THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 and return PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED. ]*/ \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 1);                                                                         \
            result = PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED;                                                                                                                 \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_065: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall store in item a pointer to the element at index. ]*/         \
            *item = &page[index_in_page];                                                                                                                                  \
            /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_066: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T) with ended_use_count 1 and return PAGED_SPARSE_ARRAY_GET_OK. ]*/ \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_END_PAGE_USE_INTERNAL_NAME(C)(array, page_index, 1);                                                                         \
            result = PAGED_SPARSE_ARRAY_GET_OK;                                                                                                                            \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}

#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_GET_DEFINE(C, T) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_GET_DEFINE_WITH_OPTIONS(C, T, AUTO_FREE_EMPTY_PAGES, DO_NOT_ZERO_NEW_PAGES)

/*introduces a function definition for thread_safe_paged_sparse_array_free_empty_pages*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_FREE_EMPTY_PAGES_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages)                                                               \
/*hint: have THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INTERNAL_DEFINE_WITH_OPTIONS(C, T, ...) before THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_FREE_EMPTY_PAGES_DEFINE_WITH_OPTIONS(C, T, ...)*/ \
THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_FREE_EMPTY_PAGES(C)(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint64_t start_index, uint64_t end_index) \
{                                                                                                                                                                          \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT result;                                                                                                         \
    /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_067: [ If paged_sparse_array is NULL, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T) shall fail and return THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS. ]*/ \
    if (paged_sparse_array == NULL)                                                                                                                                        \
    {                                                                                                                                                                      \
        result = THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS;                                                                                             \
        LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages) " paged_sparse_array=%p, uint64_t start_index=%" PRIu64 ", uint64_t end_index=%" PRIu64 ", result=%" PRI_MU_ENUM "", \
            paged_sparse_array, start_index, end_index, MU_ENUM_VALUE(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, result));                                    \
    }                                                                                                                                                                      \
    else if (                                                                                                                                                              \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_068: [ If start_index is greater than end_index, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T) shall fail and return THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS. ]*/ \
        (start_index > end_index) ||                                                                                                                                       \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_069: [ If end_index is greater than or equal to max_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T) shall fail and return THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS. ]*/ \
        (end_index >= paged_sparse_array->max_size)                                                                                                                        \
        )                                                                                                                                                                  \
    {                                                                                                                                                                      \
        result = THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS;                                                                                             \
        LogError("Invalid arguments: " THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_STRING(T, empty_pages, new_pages) " paged_sparse_array=%p, uint64_t start_index=%" PRIu64 ", uint64_t end_index=%" PRIu64 ", max_size=%" PRIu64 ", page_size=%" PRIu64 ", result=%" PRI_MU_ENUM "", \
            paged_sparse_array, start_index, end_index, paged_sparse_array->max_size, paged_sparse_array->page_size, MU_ENUM_VALUE(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, result)); \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array);                         \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_070: [ For each page from start_index / page_size to end_index / page_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T) shall call THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T). ]*/ \
        for (uint64_t page_index = start_index / array->page_size; page_index <= end_index / array->page_size; page_index++)                                               \
        {                                                                                                                                                                  \
            THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TRY_FREE_PAGE_INTERNAL_NAME(C)(array, page_index);                                                                           \
        }                                                                                                                                                                  \
        /* Codes_SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_071: [ THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T) shall succeed and return THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK. ]*/ \
        result = THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK;                                                                                                       \
    }                                                                                                                                                                      \
    return result;                                                                                                                                                         \
}

#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_FREE_EMPTY_PAGES_DEFINE(C, T) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_FREE_EMPTY_PAGES_DEFINE_WITH_OPTIONS(C, T, AUTO_FREE_EMPTY_PAGES, DO_NOT_ZERO_NEW_PAGES)

/*introduces all the function definitions of a THREAD_SAFE_PAGED_SPARSE_ARRAY, all with the same options*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages)         \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_DISPOSE_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages)          \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_INTERNAL_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages)         \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_CREATE_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages)           \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ALLOCATE_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages)         \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_RELEASE_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages)          \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_GET_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages)              \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_FREE_EMPTY_PAGES_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages) \

#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_DEFINE(C, T) THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_DEFINE_WITH_OPTIONS(C, T, AUTO_FREE_EMPTY_PAGES, DO_NOT_ZERO_NEW_PAGES)

#endif /*THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_H*/
