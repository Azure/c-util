// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TWO_D_ARRAY_LL_H
#define TWO_D_ARRAY_LL_H

#ifdef __cplusplus
#include <cinttypes>
#else // __cplusplus
#include <inttypes.h>
#endif // __cplusplus

#include "c_pal/thandle_ll.h"

#include "umock_c/umock_c_prod.h"

/*TWO_D_ARRAY is backed by a THANDLE build on the structure below*/
#define TWO_D_ARRAY_STRUCT_TYPE_NAME_TAG(T) MU_C2(TWO_D_ARRAY_TYPEDEF_NAME(T), _TAG)

#define TWO_D_ARRAY_TYPEDEF_NAME(T) MU_C2(TWO_D_ARRAY_STRUCT_, T)

/*TWO_D_ARRAY_TYPEDEF_NAME(T) introduces the base type that holds an array of array of T*/
#define TWO_D_ARRAY_DEFINE_STRUCT_TYPE(T)                                                                            \
/*forward define the typedef of the TWO_D_ARRAY struct so that it can be used for a function pointer definition*/    \
typedef struct TWO_D_ARRAY_STRUCT_TYPE_NAME_TAG(T) TWO_D_ARRAY_TYPEDEF_NAME(T);                                      \
struct TWO_D_ARRAY_STRUCT_TYPE_NAME_TAG(T)                                                                           \
{                                                                                                                    \
    uint32_t rows;                                                                                                   \
    uint32_t cols;                                                                                                   \
    T* row_arrays[];                                                                                                 \
};                                                                                                                   \

/*TWO_D_ARRAY is-a THANDLE*/
/*given a type "T" TWO_D_ARRAY_LL(T) expands to the name of the type. */
#define TWO_D_ARRAY_LL(T) THANDLE(TWO_D_ARRAY_TYPEDEF_NAME(T))

/*because TWO_D_ARRAY is a THANDLE, all THANDLE's macro APIs are useable with TWO_D_ARRAY.*/
/*the below are just shortcuts of THANDLE's public ones*/
#define TWO_D_ARRAY_LL_INITIALIZE(T) THANDLE_INITIALIZE(TWO_D_ARRAY_TYPEDEF_NAME(T))
#define TWO_D_ARRAY_LL_ASSIGN(T) THANDLE_ASSIGN(TWO_D_ARRAY_TYPEDEF_NAME(T))
#define TWO_D_ARRAY_LL_MOVE(T) THANDLE_MOVE(TWO_D_ARRAY_TYPEDEF_NAME(T))
#define TWO_D_ARRAY_LL_INITIALIZE_MOVE(T) THANDLE_INITIALIZE_MOVE(TWO_D_ARRAY_TYPEDEF_NAME(T))

/*introduces a new name for a function that returns a TWO_D_ARRAY_LL(T)*/
#define TWO_D_ARRAY_LL_CREATE_NAME(C) MU_C2(TWO_D_ARRAY_LL_CREATE_, C)
#define TWO_D_ARRAY_LL_CREATE(C) TWO_D_ARRAY_LL_CREATE_NAME(C)

/*introduces a name for a function that takes a two_d_array and a row index*/
#define TWO_D_ARRAY_LL_FREE_ROW_NAME(C) MU_C2(TWO_D_ARRAY_LL_FREE_ROW_, C)
#define TWO_D_ARRAY_LL_FREE_ROW(C) TWO_D_ARRAY_LL_FREE_ROW_NAME(C)

/*introduces a name for a function that takes a two_d_array and a row index*/
#define TWO_D_ARRAY_LL_ALLOCATE_NEW_ROW_NAME(C) MU_C2(TWO_D_ARRAY_LL_ALLOCATE_NEW_ROW_, C)
#define TWO_D_ARRAY_LL_ALLOCATE_NEW_ROW(C) TWO_D_ARRAY_LL_ALLOCATE_NEW_ROW_NAME(C)

/*introduces a name for a function that takes a two_d_array and a row index*/
#define TWO_D_ARRAY_LL_GET_ROW_NAME(C) MU_C2(TWO_D_ARRAY_LL_GET_ROW_, C)
#define TWO_D_ARRAY_LL_GET_ROW(C) TWO_D_ARRAY_LL_GET_ROW_NAME(C)

/*introduces a name for a function that takes a two_d_array and a row index and allocates the row if needed*/
#define TWO_D_ARRAY_LL_GET_ROW_ALLOCATE_IF_NEEDED_NAME(C) MU_C2(TWO_D_ARRAY_LL_GET_ROW_ALLOCATE_IF_NEEDED_, C)
#define TWO_D_ARRAY_LL_GET_ROW_ALLOCATE_IF_NEEDED(C) TWO_D_ARRAY_LL_GET_ROW_ALLOCATE_IF_NEEDED_NAME(C)

/*introduces a name for the function that free's a TWO_D_ARRAY when it's ref count got to 0*/
#define TWO_D_ARRAY_LL_FREE_NAME(C) MU_C2(TWO_D_ARRAY_LL_FREE_, C)

/*introduces a function declaration for two_d_array_create*/
#define TWO_D_ARRAY_LL_CREATE_DECLARE(C, T) MOCKABLE_FUNCTION(, TWO_D_ARRAY_LL(T), TWO_D_ARRAY_LL_CREATE(C), uint32_t, row_size, uint32_t, col_size);

/*introduces a function declaration for two_d_array_free_row*/
#define TWO_D_ARRAY_LL_FREE_ROW_DECLARE(C, T) MOCKABLE_FUNCTION(, int, TWO_D_ARRAY_LL_FREE_ROW(C), TWO_D_ARRAY_LL(T), two_d_array, uint32_t,  row_index);

/*introduces a function declaration for two_d_array_allocate_new_row*/
#define TWO_D_ARRAY_LL_ALLOCATE_NEW_ROW_DECLARE(C, T) MOCKABLE_FUNCTION(, int, TWO_D_ARRAY_LL_ALLOCATE_NEW_ROW(C), TWO_D_ARRAY_LL(T), two_d_array, uint32_t,  row_index);

/*introduces a function declaration for two_d_array_get_row*/
#define TWO_D_ARRAY_LL_GET_ROW_DECLARE(C, T) MOCKABLE_FUNCTION(, T*, TWO_D_ARRAY_LL_GET_ROW(C), TWO_D_ARRAY_LL(T), two_d_array, uint32_t,  row_index);

/*introduces a function declaration for two_d_array_get_row_allocate_if_needed*/
#define TWO_D_ARRAY_LL_GET_ROW_ALLOCATE_IF_NEEDED_DECLARE(C, T) MOCKABLE_FUNCTION(, T*, TWO_D_ARRAY_LL_GET_ROW_ALLOCATE_IF_NEEDED(C), TWO_D_ARRAY_LL(T), two_d_array, uint32_t,  row_index);

/*introduces a function definition for freeing the allocated resources for a TWO_D_ARRAY*/
#define TWO_D_ARRAY_LL_FREE_DEFINE(C, T)                                                                                        \
void TWO_D_ARRAY_LL_FREE_NAME(C)(TWO_D_ARRAY_TYPEDEF_NAME(T)* two_d_array)                                                      \
{                                                                                                                               \
    /* Codes_SRS_TWO_D_ARRAY_07_007: [ If two_d_array is NULL, TWO_D_ARRAY_FREE(T) shall do nothing. ]*/                        \
    if (two_d_array != NULL)                                                                                                    \
    {                                                                                                                           \
        /* Codes_SRS_TWO_D_ARRAY_07_009: [ TWO_D_ARRAY_FREE(T) shall free the memory associated with TWO_D_ARRAY(T). ]*/        \
        for (uint32_t i = 0; i < two_d_array->rows; i++)                                                                        \
        {                                                                                                                       \
            if (two_d_array->row_arrays[i] != NULL)                                                                             \
            {                                                                                                                   \
                /* Codes_SRS_TWO_D_ARRAY_07_008: [ TWO_D_ARRAY_FREE(T) shall free all rows that are non-NULL. ]*/               \
                free((void*)two_d_array->row_arrays[i]);                                                                        \
            }                                                                                                                   \
        }                                                                                                                       \
    }                                                                                                                           \
}

#define TWO_D_ARRAY_LL_CREATE_DEFINE(C, T)                                                                                                                                                         \
TWO_D_ARRAY_LL(T) TWO_D_ARRAY_LL_CREATE(C)(uint32_t row_size, uint32_t col_size)                                                                                                                   \
{                                                                                                                                                                                                  \
    TWO_D_ARRAY_TYPEDEF_NAME(T)* result;                                                                                                                                                           \
    /* Codes_SRS_TWO_D_ARRAY_07_001: [ If row_size equals to zero, TWO_D_ARRAY_CREATE(T) shall fail and return NULL. ]*/                                                                           \
    if (row_size == 0 ||                                                                                                                                                                           \
    /* Codes_SRS_TWO_D_ARRAY_07_002: [ If col_size equals to zero, TWO_D_ARRAY_CREATE(T) shall fail and return NULL. ]*/                                                                           \
       col_size == 0)                                                                                                                                                                              \
    {                                                                                                                                                                                              \
        LogError("Invalid arguments: uint32_t row_size=%" PRIu32 ", col_size=%" PRIu32 "", row_size, col_size);                                                                                \
    }                                                                                                                                                                                              \
    else                                                                                                                                                                                           \
    {                                                                                                                                                                                              \
        /* Codes_SRS_TWO_D_ARRAY_07_003: [ TWO_D_ARRAY_CREATE(T) shall call THANDLE_MALLOC_FLEX with TWO_D_ARRAY_FREE(T) as dispose function, nmemb set to row_size and size set to sizeof(T*). ]*/\
        result = THANDLE_MALLOC_FLEX(TWO_D_ARRAY_TYPEDEF_NAME(C))(TWO_D_ARRAY_LL_FREE_NAME(C), row_size, sizeof(T*));                                                                              \
        /* Codes_SRS_TWO_D_ARRAY_07_005: [ If there are any errors then TWO_D_ARRAY_CREATE(T) shall fail and return NULL. ]*/                                                                      \
        if (result == NULL)                                                                                                                                                                        \
        {                                                                                                                                                                                          \
            LogError("failure in malloc_flex row_size=%" PRIu32 ", sizeof(T*)=%zu", row_size, sizeof(T*));                                                                                        \
        }                                                                                                                                                                                          \
        else                                                                                                                                                                                       \
        {                                                                                                                                                                                          \
            /* Codes_SRS_TWO_D_ARRAY_07_006: [ TWO_D_ARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/                                                                                \
            result->rows = row_size;                                                                                                                                                               \
            result->cols = col_size;                                                                                                                                                               \
            /* Codes_SRS_TWO_D_ARRAY_07_004: [ TWO_D_ARRAY_CREATE(T) shall set all rows pointers to NULL. ]*/                                                                                      \
            for (uint32_t i = 0; i < row_size; i++)                                                                                                                                                \
            {                                                                                                                                                                                      \
                result->row_arrays[i] = NULL;                                                                                                                                                      \
            }                                                                                                                                                                                      \
            goto all_ok;                                                                                                                                                                           \
        }                                                                                                                                                                                          \
    }                                                                                                                                                                                              \
    result = NULL;                                                                                                                                                                                 \
all_ok:                                                                                                                                                                                            \
    return result;                                                                                                                                                                                 \
}                                                                                                                                                                                                  \

#define TWO_D_ARRAY_LL_FREE_ROW_DEFINE(C, T)                                                                                                                            \
int TWO_D_ARRAY_LL_FREE_ROW(C)(TWO_D_ARRAY_LL(T) two_d_array, uint32_t row_index)                                                                                       \
{                                                                                                                                                                       \
    int result;                                                                                                                                                         \
    /* Codes_SRS_TWO_D_ARRAY_07_010: [ If two_d_array is NULL, TWO_D_ARRAY_FREE_ROW(T) shall fail return a non-zero value. ]*/                                          \
    if (two_d_array == NULL) {                                                                                                                                          \
        LogError("Invalid arguments: TWO_D_ARRAY (" MU_TOSTRING(T) ") two_d_array=%p", two_d_array);                                                                    \
        result = MU_FAILURE;                                                                                                                                            \
    }                                                                                                                                                                   \
    else                                                                                                                                                                \
    {                                                                                                                                                                   \
        /* Codes_SRS_TWO_D_ARRAY_07_011: [ If row_index is equal or greater than row_size, TWO_D_ARRAY_FREE_ROW(T) shall fail and return a non-zero value. ]*/          \
        if (row_index >= two_d_array->rows)                                                                                                                             \
        {                                                                                                                                                               \
            LogError("Invalid arguments: uint32_t row_index=%" PRIu32 " out of bound, total_rows=%" PRIu32 "", row_index, two_d_array->rows);                      \
            result = MU_FAILURE;                                                                                                                                        \
        }                                                                                                                                                               \
        else                                                                                                                                                            \
        {                                                                                                                                                               \
            TWO_D_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(TWO_D_ARRAY_TYPEDEF_NAME(C))(two_d_array);                                                               \
            if(array->row_arrays[row_index] == NULL)                                                                                                                    \
            {                                                                                                                                                           \
                LogError("Row not allocated yet: uint32_t row_index=%" PRIu32 ", total_rows=%" PRIu32 "", row_index, two_d_array->rows);                          \
                result = MU_FAILURE;                                                                                                                                    \
            }                                                                                                                                                           \
            else                                                                                                                                                        \
            {                                                                                                                                                           \
                /* Codes_SRS_TWO_D_ARRAY_07_012: [ TWO_D_ARRAY_FREE_ROW(T) shall free the memory associated with the row specified by row_index and set it to NULL. ]*/ \
                free((void*)two_d_array->row_arrays[row_index]);                                                                                                        \
                array->row_arrays[row_index] = NULL;                                                                                                                    \
                /* Codes_SRS_TWO_D_ARRAY_07_013: [ TWO_D_ARRAY_FREE_ROW(T) shall return zero on success. ]*/                                                            \
                result = 0;                                                                                                                                             \
            }                                                                                                                                                           \
        }                                                                                                                                                               \
    }                                                                                                                                                                   \
    return result;                                                                                                                                                      \
}                                                                                                                                                                       \


#define TWO_D_ARRAY_LL_ALLOCATE_NEW_ROW_DEFINE(C, T)                                                                                                                                         \
int TWO_D_ARRAY_LL_ALLOCATE_NEW_ROW(C)(TWO_D_ARRAY_LL(T) two_d_array, uint32_t row_index)                                                                                                    \
{                                                                                                                                                                                            \
    int result;                                                                                                                                                                              \
    /* Codes_SRS_TWO_D_ARRAY_07_014: [ If two_d_array is NULL, TWO_D_ARRAY_ALLOCATE_NEW_ROW(T) shall fail and return a non-zero value. ]*/                                                   \
    if (two_d_array == NULL)                                                                                                                                                                 \
    {                                                                                                                                                                                        \
        LogError("Invalid arguments: TWO_D_ARRAY (" MU_TOSTRING(T) ") two_d_array=%p", two_d_array);                                                                                         \
        result = MU_FAILURE;                                                                                                                                                                 \
    }                                                                                                                                                                                        \
    else                                                                                                                                                                                     \
    {                                                                                                                                                                                        \
        /* Codes_SRS_TWO_D_ARRAY_07_015: [ If row_index is equal or greater than row_size, TWO_D_ARRAY_ALLOCATE_NEW_ROW(T) shall fail and return a non-zero value. ]*/                       \
        if (row_index >= two_d_array->rows)                                                                                                                                                  \
        {                                                                                                                                                                                    \
            LogError("Invalid arguments: uint32_t row_index=%" PRIu32 " out of bound, total_rows=%" PRIu32 "", row_index, two_d_array->rows);                                              \
            result = MU_FAILURE;                                                                                                                                                             \
        }                                                                                                                                                                                    \
        else                                                                                                                                                                                 \
        {                                                                                                                                                                                    \
            if (two_d_array->row_arrays[row_index] != NULL)                                                                                                                                  \
            {                                                                                                                                                                                \
                /* Codes_SRS_TWO_D_ARRAY_07_016: [ If the row specified by row_index has already been allocated, TWO_D_ARRAY_ALLOCATE_NEW_ROW(T) shall fail and return a non-zero value. ]*/ \
                result = MU_FAILURE;                                                                                                                                                         \
                LogError("Invalid arguments: uint32_t row_index=%" PRIu32 " has already been allocated", row_index);                                                                        \
            }                                                                                                                                                                                \
            else                                                                                                                                                                             \
            {                                                                                                                                                                                \
                /* Codes_SRS_TWO_D_ARRAY_07_017: [ Otherwise, TWO_D_ARRAY_ALLOCATE_NEW_ROW(T) shall allocate memory for the new row and return zero on success. ]*/                          \
                T* new_row = malloc_2(two_d_array->cols, sizeof(T));                                                                                                                         \
                if (new_row == NULL)                                                                                                                                                         \
                {                                                                                                                                                                            \
                    /* Codes_SRS_TWO_D_ARRAY_07_018: [ If there are any errors then TWO_D_ARRAY_ALLOCATE_NEW_ROW(T) shall fail and return a non-zero value. ]*/                              \
                    LogError("failure in malloc_2(number of cols=%" PRIu32 ", sizeof(T)=%zu)", two_d_array->cols, sizeof(T));                                                                \
                    result = MU_FAILURE;                                                                                                                                                     \
                }                                                                                                                                                                            \
                else                                                                                                                                                                         \
                {                                                                                                                                                                            \
                    TWO_D_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(TWO_D_ARRAY_TYPEDEF_NAME(C))(two_d_array);                                                                            \
                    array->row_arrays[row_index] = new_row;                                                                                                                                  \
                    result = 0;                                                                                                                                                              \
                }                                                                                                                                                                            \
            }                                                                                                                                                                                \
        }                                                                                                                                                                                    \
    }                                                                                                                                                                                        \
    return result;                                                                                                                                                                           \
}

#define TWO_D_ARRAY_LL_GET_ROW_DEFINE(C, T)                                                                                                                \
T* TWO_D_ARRAY_LL_GET_ROW(C)(TWO_D_ARRAY_LL(T) two_d_array, uint32_t row_index)                                                                            \
{                                                                                                                                                          \
    T* result;                                                                                                                                             \
    /* Codes_SRS_TWO_D_ARRAY_07_019: [ If two_d_array is NULL, TWO_D_ARRAY_GET_ROW(T) shall fail return NULL. ]*/                                          \
    if (two_d_array == NULL)                                                                                                                               \
    {                                                                                                                                                      \
        LogError("Invalid arguments: TWO_D_ARRAY (" MU_TOSTRING(T) ") two_d_array=%p", two_d_array);                                                       \
    }                                                                                                                                                      \
    else                                                                                                                                                   \
    {                                                                                                                                                      \
        /* Codes_SRS_TWO_D_ARRAY_07_020: [ If row_index is equal or greater than row_size, TWO_D_ARRAY_GET_ROW(T) shall fail return NULL. ]*/              \
        if (row_index >= two_d_array->rows)                                                                                                                \
        {                                                                                                                                                  \
            LogError("Invalid arguments: uint32_t row_index=%" PRIu32 " out of bound, total_rows=%" PRIu32 "", row_index, two_d_array->rows);         \
        }                                                                                                                                                  \
        else                                                                                                                                               \
        {                                                                                                                                                  \
            result = two_d_array->row_arrays[row_index];                                                                                                   \
            if (result != NULL)                                                                                                                            \
            {                                                                                                                                              \
                /* Codes_SRS_TWO_D_ARRAY_07_022: [ Otherwise, TWO_D_ARRAY_GET_ROW(T) shall return the entire row stored in the corresponding row_index. ]*/    \
                goto all_ok;                                                                                                                               \
            }                                                                                                                                              \
            /* Codes_SRS_TWO_D_ARRAY_07_021: [ If the array stored in row_index is NULL, TWO_D_ARRAY_GET_ROW(T) shall fail and return NULL. ]*/             \
            LogError("Row not allocated yet: uint32_t row_index=%" PRIu32 ", total_rows=%" PRIu32 "", row_index, two_d_array->rows);                 \
        }                                                                                                                                                  \
    }                                                                                                                                                      \
    result = NULL;                                                                                                                                         \
all_ok:                                                                                                                                                    \
    return result;                                                                                                                                         \
}                                                                                                                                                          \

#define TWO_D_ARRAY_LL_GET_ROW_ALLOCATE_IF_NEEDED_DEFINE(C, T)                                                                                              \
T* TWO_D_ARRAY_LL_GET_ROW_ALLOCATE_IF_NEEDED(C)(TWO_D_ARRAY_LL(T) two_d_array, uint32_t row_index)                                                          \
{                                                                                                                                                          \
    T* result = NULL;                                                                                                                                      \
    /* Codes_SRS_TWO_D_ARRAY_88_001: [ If two_d_array is NULL, TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED(T) shall fail and return NULL. ]*/                   \
    if (two_d_array == NULL)                                                                                                                               \
    {                                                                                                                                                      \
        LogError("Invalid arguments: TWO_D_ARRAY (" MU_TOSTRING(T) ") two_d_array=%p", two_d_array);                                                     \
    }                                                                                                                                                      \
    else                                                                                                                                                   \
    {                                                                                                                                                      \
        /* Codes_SRS_TWO_D_ARRAY_88_002: [ If row_index is equal or greater than row_size, TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED(T) shall fail and return NULL. ]*/ \
        if (row_index >= two_d_array->rows)                                                                                                                \
        {                                                                                                                                                  \
            LogError("Invalid arguments: uint32_t row_index=%" PRIu32 " out of bound, total_rows=%" PRIu32 "", row_index, two_d_array->rows);         \
        }                                                                                                                                                  \
        else                                                                                                                                               \
        {                                                                                                                                                  \
            result = two_d_array->row_arrays[row_index];                                                                                                   \
            /* Codes_SRS_TWO_D_ARRAY_88_003: [ If the array stored in row_index is non-NULL, TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED(T) shall return the existing row. ]*/ \
            if (result == NULL)                                                                                                                            \
            {                                                                                                                                              \
                /* Codes_SRS_TWO_D_ARRAY_88_004: [ If the array stored in row_index is NULL, TWO_D_ARRAY_GET_ROW_ALLOCATE_IF_NEEDED(T) shall allocate a new row and return it. ]*/ \
                result = malloc_2(two_d_array->cols, sizeof(T));                                                                                           \
                if (result == NULL)                                                                                                                        \
                {                                                                                                                                          \
                    LogError("failure in malloc_2(number of cols=%" PRIu32 ", sizeof(T)=%zu)", two_d_array->cols, sizeof(T));                          \
                }                                                                                                                                          \
                else                                                                                                                                       \
                {                                                                                                                                          \
                    TWO_D_ARRAY_TYPEDEF_NAME(T)* array = THANDLE_GET_T(TWO_D_ARRAY_TYPEDEF_NAME(C))(two_d_array);                                          \
                    array->row_arrays[row_index] = result;                                                                                                 \
                }                                                                                                                                          \
            }                                                                                                                                              \
        }                                                                                                                                                  \
    }                                                                                                                                                      \
    return result;                                                                                                                                         \
}                                                                                                                                                          \

/*"macros to use in 'reals_' headers"*/
#define TWO_D_ARRAY_LL_TYPE_DECLARE(C, T)                                                                           \
    /*hint: have TWO_D_ARRAY_DEFINE_STRUCT_TYPE(T) before TWO_D_ARRAY_LL_TYPE_DECLARE*/                             \
    THANDLE_LL_TYPE_DECLARE(TWO_D_ARRAY_TYPEDEF_NAME(C), TWO_D_ARRAY_TYPEDEF_NAME(T))                               \
    TWO_D_ARRAY_LL_CREATE_DECLARE(C, T)                                                                             \
    TWO_D_ARRAY_LL_FREE_ROW_DECLARE(C, T)                                                                           \
    TWO_D_ARRAY_LL_ALLOCATE_NEW_ROW_DECLARE(C, T)                                                                   \
    TWO_D_ARRAY_LL_GET_ROW_DECLARE(C, T)                                                                            \
    TWO_D_ARRAY_LL_GET_ROW_ALLOCATE_IF_NEEDED_DECLARE(C, T)                                                         \

#define TWO_D_ARRAY_LL_TYPE_DEFINE(C, T)                                                                            \
    /*hint: have THANDLE_TYPE_DEFINE(TWO_D_ARRAY_TYPEDEF_NAME(T)) before TWO_D_ARRAY_LL_TYPE_DEFINE*/               \
    TWO_D_ARRAY_LL_FREE_DEFINE(C, T)                                                                                \
    TWO_D_ARRAY_LL_CREATE_DEFINE(C, T)                                                                              \
    TWO_D_ARRAY_LL_FREE_ROW_DEFINE(C, T)                                                                            \
    TWO_D_ARRAY_LL_ALLOCATE_NEW_ROW_DEFINE(C, T)                                                                    \
    TWO_D_ARRAY_LL_GET_ROW_DEFINE(C, T)                                                                             \
    TWO_D_ARRAY_LL_GET_ROW_ALLOCATE_IF_NEEDED_DEFINE(C, T)                                                          \

#endif  /*TWO_D_ARRAY_LL_H*/
