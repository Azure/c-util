// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PAGED_SPARSE_ARRAY_LL_H
#define PAGED_SPARSE_ARRAY_LL_H

#ifdef __cplusplus
#include <cinttypes>
#include <cstdlib>
#else // __cplusplus
#include <inttypes.h>
#include <stdlib.h>
#endif // __cplusplus

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/thandle_ll.h"

#include "umock_c/umock_c_prod.h"

/*PAGED_SPARSE_ARRAY is backed by a THANDLE build on the structure below*/
#define PAGED_SPARSE_ARRAY_STRUCT_TYPE_NAME_TAG(T) MU_C2(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T), _TAG)

#define PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T) MU_C2(PAGED_SPARSE_ARRAY_STRUCT_, T)

/*PAGE_STRUCT holds a single page with elements and allocation bitmap*/
#define PAGED_SPARSE_ARRAY_PAGE_STRUCT_TYPE_NAME_TAG(T) MU_C2(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T), _TAG)
#define PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T) MU_C2(PAGED_SPARSE_ARRAY_PAGE_STRUCT_, T)

/*PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T) introduces the base type that holds the paged sparse array*/
#define PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(T)                                                                        \
/*define the page structure first - bitmap is embedded after items[] in the same allocation*/                          \
typedef struct PAGED_SPARSE_ARRAY_PAGE_STRUCT_TYPE_NAME_TAG(T)                                                          \
{                                                                                                                       \
    uint32_t allocated_count;                                                                                           \
    T items[];  /*followed by bitmap bytes at offset page_size * sizeof(T)*/                                            \
} PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T);                                                                              \
/*forward define the typedef of the PAGED_SPARSE_ARRAY struct so that it can be used for a function pointer definition*/\
typedef struct PAGED_SPARSE_ARRAY_STRUCT_TYPE_NAME_TAG(T) PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T);                           \
struct PAGED_SPARSE_ARRAY_STRUCT_TYPE_NAME_TAG(T)                                                                       \
{                                                                                                                       \
    uint32_t max_size;                                                                                                  \
    uint32_t page_size;                                                                                                 \
    uint32_t page_count;                                                                                                \
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

/*introduces a name for the dispose function that is called when PAGED_SPARSE_ARRAY ref count goes to 0*/
#define PAGED_SPARSE_ARRAY_LL_DISPOSE_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_DISPOSE_, C)

/*introduces a name for the allocate function*/
#define PAGED_SPARSE_ARRAY_LL_ALLOCATE_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_ALLOCATE_, C)
#define PAGED_SPARSE_ARRAY_LL_ALLOCATE(C) PAGED_SPARSE_ARRAY_LL_ALLOCATE_NAME(C)

/*introduces a name for the release function*/
#define PAGED_SPARSE_ARRAY_LL_RELEASE_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_RELEASE_, C)
#define PAGED_SPARSE_ARRAY_LL_RELEASE(C) PAGED_SPARSE_ARRAY_LL_RELEASE_NAME(C)

/*introduces a name for the allocate_or_get function*/
#define PAGED_SPARSE_ARRAY_LL_ALLOCATE_OR_GET_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_ALLOCATE_OR_GET_, C)
#define PAGED_SPARSE_ARRAY_LL_ALLOCATE_OR_GET(C) PAGED_SPARSE_ARRAY_LL_ALLOCATE_OR_GET_NAME(C)

/*introduces a name for the get function*/
#define PAGED_SPARSE_ARRAY_LL_GET_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_GET_, C)
#define PAGED_SPARSE_ARRAY_LL_GET(C) PAGED_SPARSE_ARRAY_LL_GET_NAME(C)

/*introduces a function declaration for paged_sparse_array_create*/
#define PAGED_SPARSE_ARRAY_LL_CREATE_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, PAGED_SPARSE_ARRAY_LL(T), PAGED_SPARSE_ARRAY_LL_CREATE(C), uint32_t, max_size, uint32_t, page_size);

/*introduces a function declaration for paged_sparse_array_allocate*/
#define PAGED_SPARSE_ARRAY_LL_ALLOCATE_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, T*, PAGED_SPARSE_ARRAY_LL_ALLOCATE(C), PAGED_SPARSE_ARRAY_LL(T), paged_sparse_array, uint32_t, index);

/*introduces a function declaration for paged_sparse_array_release*/
#define PAGED_SPARSE_ARRAY_LL_RELEASE_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, void, PAGED_SPARSE_ARRAY_LL_RELEASE(C), PAGED_SPARSE_ARRAY_LL(T), paged_sparse_array, uint32_t, index);

/*introduces a function declaration for paged_sparse_array_allocate_or_get*/
#define PAGED_SPARSE_ARRAY_LL_ALLOCATE_OR_GET_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, T*, PAGED_SPARSE_ARRAY_LL_ALLOCATE_OR_GET(C), PAGED_SPARSE_ARRAY_LL(T), paged_sparse_array, uint32_t, index);

/*introduces a function declaration for paged_sparse_array_get*/
#define PAGED_SPARSE_ARRAY_LL_GET_DECLARE(C, T) \
    MOCKABLE_FUNCTION(, T*, PAGED_SPARSE_ARRAY_LL_GET(C), PAGED_SPARSE_ARRAY_LL(T), paged_sparse_array, uint32_t, index);

/*helper to compute bitmap size in bytes (1 bit per element, rounded up to nearest byte)*/
#define PAGED_SPARSE_ARRAY_BITMAP_SIZE(page_size) (((page_size) + 7) / 8)

/*helper to get the allocation bitmap from a page - bitmap is stored after items[] in the same allocation*/
#define PAGED_SPARSE_ARRAY_GET_BITMAP(page, page_size, T) ((uint8_t*)((page)->items + (page_size)))

/*helper to check if an element is allocated in the bitmap*/
#define PAGED_SPARSE_ARRAY_IS_ALLOCATED(bitmap, index_in_page) \
    (((bitmap)[(index_in_page) / 8] & (1 << ((index_in_page) % 8))) != 0)

/*helper to set an element as allocated in the bitmap*/
#define PAGED_SPARSE_ARRAY_SET_ALLOCATED(bitmap, index_in_page) \
    ((bitmap)[(index_in_page) / 8] |= (1 << ((index_in_page) % 8)))

/*helper to clear an element as not allocated in the bitmap*/
#define PAGED_SPARSE_ARRAY_CLEAR_ALLOCATED(bitmap, index_in_page) \
    ((bitmap)[(index_in_page) / 8] &= ~(1 << ((index_in_page) % 8)))

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
        for (uint32_t i = 0; i < paged_sparse_array->page_count; i++)                                                          \
        {                                                                                                                      \
            if (paged_sparse_array->pages[i] != NULL)                                                                          \
            {                                                                                                                  \
                /* bitmap is embedded in same allocation as page, so only one free needed */                                  \
                free((void*)paged_sparse_array->pages[i]);                                                                     \
            }                                                                                                                  \
        }                                                                                                                      \
    }                                                                                                                          \
}

#define PAGED_SPARSE_ARRAY_LL_CREATE_DEFINE(C, T)                                                                                                                         \
PAGED_SPARSE_ARRAY_LL(T) PAGED_SPARSE_ARRAY_LL_CREATE(C)(uint32_t max_size, uint32_t page_size)                                                                           \
{                                                                                                                                                                         \
    PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* result;                                                                                                                           \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_001: [ If max_size is zero, PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/                                           \
    if (max_size == 0)                                                                                                                                                    \
    {                                                                                                                                                                     \
        LogError("Invalid arguments: uint32_t max_size=%" PRIu32 "", max_size);                                                                                           \
    }                                                                                                                                                                     \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_002: [ If page_size is zero, PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/                                          \
    else if (page_size == 0)                                                                                                                                              \
    {                                                                                                                                                                     \
        LogError("Invalid arguments: uint32_t page_size=%" PRIu32 "", page_size);                                                                                         \
    }                                                                                                                                                                     \
    else                                                                                                                                                                  \
    {                                                                                                                                                                     \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_003: [ PAGED_SPARSE_ARRAY_CREATE(T) shall compute the number of pages as (max_size + page_size - 1) / page_size. ]*/           \
        uint32_t page_count = (max_size + page_size - 1) / page_size;                                                                                                     \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_004: [ PAGED_SPARSE_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX to allocate memory for the paged sparse array with the number of pages. ]*/ \
        result = THANDLE_MALLOC_FLEX(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(PAGED_SPARSE_ARRAY_LL_DISPOSE_NAME(C), page_count, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)*)); \
        if (result == NULL)                                                                                                                                               \
        {                                                                                                                                                                 \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_007: [ If there are any errors, PAGED_SPARSE_ARRAY_CREATE(T) shall fail and return NULL. ]*/                               \
            LogError("failure in THANDLE_MALLOC_FLEX page_count=%" PRIu32 ", sizeof(PAGED_SPARSE_ARRAY_PAGE*)=%zu", page_count, sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)*)); \
        }                                                                                                                                                                 \
        else                                                                                                                                                              \
        {                                                                                                                                                                 \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_006: [ PAGED_SPARSE_ARRAY_CREATE(T) shall store max_size and page_size in the structure. ]*/                               \
            result->max_size = max_size;                                                                                                                                  \
            result->page_size = page_size;                                                                                                                                \
            result->page_count = page_count;                                                                                                                              \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_005: [ PAGED_SPARSE_ARRAY_CREATE(T) shall set all page pointers to NULL. ]*/                                               \
            for (uint32_t i = 0; i < page_count; i++)                                                                                                                     \
            {                                                                                                                                                             \
                result->pages[i] = NULL;                                                                                                                                  \
            }                                                                                                                                                             \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_008: [ PAGED_SPARSE_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/                                         \
            goto all_ok;                                                                                                                                                  \
        }                                                                                                                                                                 \
    }                                                                                                                                                                     \
    result = NULL;                                                                                                                                                        \
all_ok:                                                                                                                                                                   \
    return result;                                                                                                                                                        \
}

/*internal helper to allocate a page*/
#define PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C) MU_C2(PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_, C)

#define PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_DEFINE(C, T)                                                                                                         \
static PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)* PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C)(uint32_t page_size)                                                  \
{                                                                                                                                                                         \
    PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)* page;                                                                                                                        \
    /* Allocate page struct + items array + bitmap in a single allocation */                                                                                             \
    size_t bitmap_size = PAGED_SPARSE_ARRAY_BITMAP_SIZE(page_size);                                                                                                       \
    size_t items_and_bitmap_size = (size_t)page_size * sizeof(T) + bitmap_size;                                                                                           \
    page = malloc_flex(sizeof(PAGED_SPARSE_ARRAY_PAGE_TYPEDEF_NAME(T)), items_and_bitmap_size, 1);                                                                        \
    if (page == NULL)                                                                                                                                                     \
    {                                                                                                                                                                     \
        LogError("failure in malloc_flex for page, page_size=%" PRIu32 ", sizeof(" MU_TOSTRING(T) ")=%zu, bitmap_size=%zu", page_size, sizeof(T), bitmap_size);          \
    }                                                                                                                                                                     \
    else                                                                                                                                                                  \
    {                                                                                                                                                                     \
        /* Initialize bitmap to all zeros (nothing allocated) - bitmap is at end of items array */                                                                       \
        uint8_t* bitmap = PAGED_SPARSE_ARRAY_GET_BITMAP(page, page_size, T);                                                                                              \
        (void)memset(bitmap, 0, bitmap_size);                                                                                                                             \
        page->allocated_count = 0;                                                                                                                                        \
    }                                                                                                                                                                     \
    return page;                                                                                                                                                          \
}

#define PAGED_SPARSE_ARRAY_LL_ALLOCATE_DEFINE(C, T)                                                                                                                        \
T* PAGED_SPARSE_ARRAY_LL_ALLOCATE(C)(PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint32_t index)                                                                          \
{                                                                                                                                                                          \
    T* result;                                                                                                                                                             \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_011: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return NULL. ]*/                                \
    if (paged_sparse_array == NULL)                                                                                                                                        \
    {                                                                                                                                                                      \
        LogError("Invalid arguments: PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) ") paged_sparse_array=%p", paged_sparse_array);                                                   \
    }                                                                                                                                                                      \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_012: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return NULL. ]*/                \
    else if (index >= paged_sparse_array->max_size)                                                                                                                        \
    {                                                                                                                                                                      \
        LogError("Invalid arguments: uint32_t index=%" PRIu32 " out of bound, max_size=%" PRIu32 "", index, paged_sparse_array->max_size);                                 \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_013: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall compute the page index as index / page_size. ]*/                                    \
        uint32_t page_index = index / paged_sparse_array->page_size;                                                                                                       \
        uint32_t index_in_page = index % paged_sparse_array->page_size;                                                                                                    \
        PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array);                                                 \
                                                                                                                                                                           \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_014: [ If the page is not allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall allocate memory for the page containing page_size elements and an allocation bitmap, and initialize all elements as not allocated. ]*/ \
        if (                                                                                                                                                               \
            (array->pages[page_index] == NULL) &&                                                                                                                          \
            ((array->pages[page_index] = PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C)(paged_sparse_array->page_size)) == NULL)                                     \
            )                                                                                                                                                              \
        {                                                                                                                                                                  \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_018: [ If there are any errors, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return NULL. ]*/                              \
            LogError("failure allocating page at page_index=%" PRIu32 "", page_index);                                                                                     \
        }                                                                                                                                                                  \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_015: [ If the element at index is already allocated, PAGED_SPARSE_ARRAY_ALLOCATE(T) shall fail and return NULL. ]*/             \
        else if (PAGED_SPARSE_ARRAY_IS_ALLOCATED(PAGED_SPARSE_ARRAY_GET_BITMAP(array->pages[page_index], paged_sparse_array->page_size, T), index_in_page))                \
        {                                                                                                                                                                  \
            LogError("Element at index=%" PRIu32 " is already allocated", index);                                                                                          \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_016: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall mark the element at index as allocated. ]*/                                     \
            PAGED_SPARSE_ARRAY_SET_ALLOCATED(PAGED_SPARSE_ARRAY_GET_BITMAP(array->pages[page_index], paged_sparse_array->page_size, T), index_in_page);                                                                  \
            array->pages[page_index]->allocated_count++;                                                                                                                   \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_017: [ PAGED_SPARSE_ARRAY_ALLOCATE(T) shall return a pointer to the element at index. ]*/                                   \
            result = &array->pages[page_index]->items[index_in_page];                                                                                                      \
            goto all_ok;                                                                                                                                                   \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    result = NULL;                                                                                                                                                         \
all_ok:                                                                                                                                                                    \
    return result;                                                                                                                                                         \
}

#define PAGED_SPARSE_ARRAY_LL_RELEASE_DEFINE(C, T)                                                                                                                          \
void PAGED_SPARSE_ARRAY_LL_RELEASE(C)(PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint32_t index)                                                                          \
{                                                                                                                                                                           \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_019: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/                                                \
    if (paged_sparse_array == NULL)                                                                                                                                         \
    {                                                                                                                                                                       \
        LogError("Invalid arguments: PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) ") paged_sparse_array=%p", paged_sparse_array);                                                    \
    }                                                                                                                                                                       \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_020: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/                                \
    else if (index >= paged_sparse_array->max_size)                                                                                                                         \
    {                                                                                                                                                                       \
        LogError("Invalid arguments: uint32_t index=%" PRIu32 " out of bound, max_size=%" PRIu32 "", index, paged_sparse_array->max_size);                                  \
    }                                                                                                                                                                       \
    else                                                                                                                                                                    \
    {                                                                                                                                                                       \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_021: [ PAGED_SPARSE_ARRAY_RELEASE(T) shall compute the page index as index / page_size. ]*/                                      \
        uint32_t page_index = index / paged_sparse_array->page_size;                                                                                                        \
        uint32_t index_in_page = index % paged_sparse_array->page_size;                                                                                                     \
        PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array);                                                  \
                                                                                                                                                                            \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_022: [ If the page is not allocated, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/                                             \
        if (array->pages[page_index] == NULL)                                                                                                                               \
        {                                                                                                                                                                   \
            LogError("Page at page_index=%" PRIu32 " is not allocated", page_index);                                                                                        \
        }                                                                                                                                                                   \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_023: [ If the element at index is not allocated, PAGED_SPARSE_ARRAY_RELEASE(T) shall return. ]*/                                 \
        else if (!PAGED_SPARSE_ARRAY_IS_ALLOCATED(PAGED_SPARSE_ARRAY_GET_BITMAP(array->pages[page_index], paged_sparse_array->page_size, T), index_in_page))                \
        {                                                                                                                                                                   \
            LogError("Element at index=%" PRIu32 " is not allocated", index);                                                                                               \
        }                                                                                                                                                                   \
        else                                                                                                                                                                \
        {                                                                                                                                                                   \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_024: [ PAGED_SPARSE_ARRAY_RELEASE(T) shall mark the element at index as not allocated. ]*/                                   \
            PAGED_SPARSE_ARRAY_CLEAR_ALLOCATED(PAGED_SPARSE_ARRAY_GET_BITMAP(array->pages[page_index], paged_sparse_array->page_size, T), index_in_page);                   \
            array->pages[page_index]->allocated_count--;                                                                                                                    \
                                                                                                                                                                            \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_025: [ If all elements in the page are now not allocated, PAGED_SPARSE_ARRAY_RELEASE(T) shall free the page and set the page pointer to NULL. ]*/ \
            if (array->pages[page_index]->allocated_count == 0)                                                                                                             \
            {                                                                                                                                                               \
                /* bitmap is embedded in same allocation as page, so only one free needed */                                                                               \
                free((void*)array->pages[page_index]);                                                                                                                      \
                array->pages[page_index] = NULL;                                                                                                                            \
            }                                                                                                                                                               \
        }                                                                                                                                                                   \
    }                                                                                                                                                                       \
}

#define PAGED_SPARSE_ARRAY_LL_ALLOCATE_OR_GET_DEFINE(C, T)                                                                                                                 \
T* PAGED_SPARSE_ARRAY_LL_ALLOCATE_OR_GET(C)(PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint32_t index)                                                                   \
{                                                                                                                                                                          \
    T* result;                                                                                                                                                             \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_027: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T) shall fail and return NULL. ]*/                         \
    if (paged_sparse_array == NULL)                                                                                                                                        \
    {                                                                                                                                                                      \
        LogError("Invalid arguments: PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) ") paged_sparse_array=%p", paged_sparse_array);                                                   \
    }                                                                                                                                                                      \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_028: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T) shall fail and return NULL. ]*/         \
    else if (index >= paged_sparse_array->max_size)                                                                                                                        \
    {                                                                                                                                                                      \
        LogError("Invalid arguments: uint32_t index=%" PRIu32 " out of bound, max_size=%" PRIu32 "", index, paged_sparse_array->max_size);                                 \
    }                                                                                                                                                                      \
    else                                                                                                                                                                   \
    {                                                                                                                                                                      \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_029: [ PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T) shall compute the page index as index / page_size. ]*/                             \
        uint32_t page_index = index / paged_sparse_array->page_size;                                                                                                       \
        uint32_t index_in_page = index % paged_sparse_array->page_size;                                                                                                    \
        PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(C))(paged_sparse_array);                                                 \
                                                                                                                                                                           \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_030: [ If the page is not allocated, PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T) shall allocate memory for the page containing page_size elements and an allocation bitmap, and initialize all elements as not allocated. ]*/ \
        if (                                                                                                                                                               \
            (array->pages[page_index] == NULL) &&                                                                                                                          \
            ((array->pages[page_index] = PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_NAME(C)(paged_sparse_array->page_size)) == NULL)                                     \
            )                                                                                                                                                              \
        {                                                                                                                                                                  \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_033: [ If there are any errors, PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T) shall fail and return NULL. ]*/                       \
            LogError("failure allocating page at page_index=%" PRIu32 "", page_index);                                                                                     \
        }                                                                                                                                                                  \
        else                                                                                                                                                               \
        {                                                                                                                                                                  \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_031: [ If the element at index is not allocated, PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T) shall mark it as allocated. ]*/      \
            if (!PAGED_SPARSE_ARRAY_IS_ALLOCATED(PAGED_SPARSE_ARRAY_GET_BITMAP(array->pages[page_index], paged_sparse_array->page_size, T), index_in_page))                \
            {                                                                                                                                                              \
                PAGED_SPARSE_ARRAY_SET_ALLOCATED(PAGED_SPARSE_ARRAY_GET_BITMAP(array->pages[page_index], paged_sparse_array->page_size, T), index_in_page);                \
                array->pages[page_index]->allocated_count++;                                                                                                               \
            }                                                                                                                                                              \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_032: [ PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T) shall return a pointer to the element at index. ]*/                            \
            result = &array->pages[page_index]->items[index_in_page];                                                                                                      \
            goto all_ok;                                                                                                                                                   \
        }                                                                                                                                                                  \
    }                                                                                                                                                                      \
    result = NULL;                                                                                                                                                         \
all_ok:                                                                                                                                                                    \
    return result;                                                                                                                                                         \
}

#define PAGED_SPARSE_ARRAY_LL_GET_DEFINE(C, T)                                                                                                                               \
T* PAGED_SPARSE_ARRAY_LL_GET(C)(PAGED_SPARSE_ARRAY_LL(T) paged_sparse_array, uint32_t index)                                                                                 \
{                                                                                                                                                                            \
    T* result;                                                                                                                                                               \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_034: [ If paged_sparse_array is NULL, PAGED_SPARSE_ARRAY_GET(T) shall fail and return NULL. ]*/                                       \
    if (paged_sparse_array == NULL)                                                                                                                                          \
    {                                                                                                                                                                        \
        LogError("Invalid arguments: PAGED_SPARSE_ARRAY(" MU_TOSTRING(T) ") paged_sparse_array=%p", paged_sparse_array);                                                     \
    }                                                                                                                                                                        \
    /* Codes_SRS_PAGED_SPARSE_ARRAY_88_035: [ If index is greater than or equal to max_size, PAGED_SPARSE_ARRAY_GET(T) shall fail and return NULL. ]*/                       \
    else if (index >= paged_sparse_array->max_size)                                                                                                                          \
    {                                                                                                                                                                        \
        LogError("Invalid arguments: uint32_t index=%" PRIu32 " out of bound, max_size=%" PRIu32 "", index, paged_sparse_array->max_size);                                   \
    }                                                                                                                                                                        \
    else                                                                                                                                                                     \
    {                                                                                                                                                                        \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_036: [ PAGED_SPARSE_ARRAY_GET(T) shall compute the page index as index / page_size. ]*/                                           \
        uint32_t page_index = index / paged_sparse_array->page_size;                                                                                                         \
        uint32_t index_in_page = index % paged_sparse_array->page_size;                                                                                                      \
                                                                                                                                                                             \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_037: [ If the page is not allocated, PAGED_SPARSE_ARRAY_GET(T) shall fail and return NULL. ]*/                                    \
        if (paged_sparse_array->pages[page_index] == NULL)                                                                                                                   \
        {                                                                                                                                                                    \
            LogError("Page at page_index=%" PRIu32 " is not allocated", page_index);                                                                                         \
        }                                                                                                                                                                    \
        /* Codes_SRS_PAGED_SPARSE_ARRAY_88_038: [ If the element at index is not allocated, PAGED_SPARSE_ARRAY_GET(T) shall fail and return NULL. ]*/                        \
        else if (!PAGED_SPARSE_ARRAY_IS_ALLOCATED(PAGED_SPARSE_ARRAY_GET_BITMAP(paged_sparse_array->pages[page_index], paged_sparse_array->page_size, T), index_in_page))    \
        {                                                                                                                                                                    \
            LogError("Element at index=%" PRIu32 " is not allocated", index);                                                                                                \
        }                                                                                                                                                                    \
        else                                                                                                                                                                 \
        {                                                                                                                                                                    \
            /* Codes_SRS_PAGED_SPARSE_ARRAY_88_039: [ PAGED_SPARSE_ARRAY_GET(T) shall return a pointer to the element at index. ]*/                                          \
            result = &paged_sparse_array->pages[page_index]->items[index_in_page];                                                                                           \
            goto all_ok;                                                                                                                                                     \
        }                                                                                                                                                                    \
    }                                                                                                                                                                        \
    result = NULL;                                                                                                                                                           \
all_ok:                                                                                                                                                                      \
    return result;                                                                                                                                                           \
}

#endif /*PAGED_SPARSE_ARRAY_LL_H*/
