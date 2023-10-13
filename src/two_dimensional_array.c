// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/two_dimensional_array.h"

typedef struct TWO_D_ARRAY_TAG
{
    uint64_t rows;
    uint64_t cols;
    size_t item_size;
    unsigned char* row_arrays[];
}TWO_D_ARRAY;

IMPLEMENT_MOCKABLE_FUNCTION(, TWO_D_ARRAY_HANDLE, two_d_array_create, uint64_t, row_size, uint64_t, col_size, size_t, item_size)
{
    TWO_D_ARRAY_HANDLE result;
    if (row_size == 0)
    {
        LogError("Invalid arguments: uint64_t row_size=%" PRIu64, row_size);
    }
    else
    {
        result = malloc_flex(sizeof(TWO_D_ARRAY_HANDLE), row_size, sizeof(unsigned char*));
        if (result == NULL)
        {
            LogError("failure in malloc_flex(sizeof(TWO_D_ARRAY_HANDLE)=%zu, row_size=%" PRIu64 ", sizeof(unsigned char*)=%zu);", sizeof(TWO_D_ARRAY_HANDLE), row_size, sizeof(unsigned char*));
        }
        else
        {
            result->rows = row_size;
            result->cols = col_size;
            result->item_size = item_size;

            for (uint64_t i = 0; i < row_size; i++)
            {
                result->row_arrays[i] = NULL;
            }
            goto all_ok;
        }
    }
    result = NULL;

all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, two_d_array_free_row, TWO_D_ARRAY_HANDLE, two_d_array_handle, uint64_t, row_index)
{
    int result;

    if (two_d_array_handle == NULL) {
        LogError("Invalid arguments: TWO_D_ARRAY_HANDLE two_d_array_handle=%p", two_d_array_handle);
        result = MU_FAILURE;
    }
    else
    {
        if (row_index >= two_d_array_handle->rows)
        {
            LogError("Invalid arguments: uint64_t row_index=% " PRIu64 " out of bound, total_rows = %" PRIu64, row_index, two_d_array_handle->rows);
            result = MU_FAILURE;
        }
        else
        {
            free(two_d_array_handle->row_arrays[row_index]);
            two_d_array_handle->row_arrays[row_index] = NULL;
            result = 0;
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, two_d_array_allocate_new_row, TWO_D_ARRAY_HANDLE, two_d_array_handle, uint64_t, row_index)
{
    int result;

    if (two_d_array_handle == NULL)
    {
        LogError("Invalid arguments: TWO_D_ARRAY_HANDLE two_d_array_handle=%p", two_d_array_handle);
        result = MU_FAILURE;
    }
    else
    {
        if (row_index >= two_d_array_handle->rows)
        {
            LogError("Invalid arguments: uint64_t row_index=% " PRIu64 " out of bound, total_rows = %" PRIu64, row_index, two_d_array_handle->rows);
            result = MU_FAILURE;
        }
        else
        {
            if (two_d_array_handle->row_arrays[row_index] == NULL)
            {
                unsigned char* new_row = malloc_2(two_d_array_handle->cols, two_d_array_handle->item_size);
                if (new_row == NULL)
                {
                    LogError("malloc(sizeof(col)=%zu) failed", two_d_array_handle->cols * two_d_array_handle->item_size);
                    result = MU_FAILURE;
                }
                else
                {
                    two_d_array_handle->row_arrays[row_index] = new_row;
                    result = 0;
                }
            }
            else
            {
                result = 0;
            }
        }
    }
    return result;
}

//IMPLEMENT_MOCKABLE_FUNCTION(, int, two_d_array_add, TWO_D_ARRAY_HANDLE, two_d_array_handle, uint64_t, row_index, uint64_t, col_index, void*, item)
//{
//    int result;
//    if (two_d_array_handle == NULL)
//    {
//        LogError("Invalid arguments: TWO_D_ARRAY_HANDLE two_d_array_handle=%p", two_d_array_handle);
//        result = MU_FAILURE;
//    }
//    else
//    {
//        if (row_index >= two_d_array_handle->rows)
//        {
//            LogError("Invalid arguments: uint64_t row_index=% " PRIu64 " out of bound, total_rows = %" PRIu64, row_index, two_d_array_handle->rows);
//            result = MU_FAILURE;
//        }
//        else
//        {
//            if (col_index >= two_d_array_handle->cols)
//            {
//                LogError("Invalid arguments: uint64_t col_index=% " PRIu64 " out of bound, total_cols = %" PRIu64, col_index, two_d_array_handle->cols);
//                result = MU_FAILURE;
//            }
//            else
//            {
//                if (two_d_array_handle->row_arrays[row_index] == NULL)
//                {
//                    LogError("Invalid arguments: uint64_t row_index=% " PRIu64 " is not allocated ", row_index);
//                    result = MU_FAILURE;
//                }
//                else
//                {
//                    //two_d_array_handle->row_arrays[row_index][col_index* two_d_array_handle->item_size] = item;
//                    memcpy(&two_d_array_handle->row_arrays[row_index][col_index * two_d_array_handle->item_size], item, two_d_array_handle->item_size);
//                    result = 0;
//                }
//            }
//        }
//    }
//    return result;
//}

IMPLEMENT_MOCKABLE_FUNCTION(, unsigned char*, two_d_array_get, TWO_D_ARRAY_HANDLE, two_d_array_handle, uint64_t, row_index)
{
    unsigned char* result;
    if (two_d_array_handle == NULL)
    {
        LogError("Invalid arguments: TWO_D_ARRAY_HANDLE two_d_array_handle=%p", two_d_array_handle);
    }
    else
    {
        if (row_index >= two_d_array_handle->rows)
        {
            LogError("Invalid arguments: uint64_t row_index=% " PRIu64 " out of bound, total_rows = %" PRIu64, row_index, two_d_array_handle->rows);
        }
        else
        {
            result = two_d_array_handle->row_arrays[row_index];
            goto all_ok;
        }
    }
    result = NULL;
all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, two_d_array_destroy, TWO_D_ARRAY_HANDLE, two_d_array_handle)
{
    if (two_d_array_handle == NULL)
    {
        LogError("Invalid arguments: TWO_D_ARRAY_HANDLE two_d_array_handle=%p", two_d_array_handle);
    }
    else
    {
        for (uint64_t i = 0; i < two_d_array_handle->rows; i++)
        {
            free(two_d_array_handle->row_arrays[i]);
        }
        free(two_d_array_handle);
    }
}
