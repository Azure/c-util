// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef TWO_DIMENSIONAL_ARRAY_H
#define TWO_DIMENSIONAL_ARRAY_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#include <stdbool.h>
#endif

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TWO_D_ARRAY_TAG* TWO_D_ARRAY_HANDLE;

MOCKABLE_FUNCTION(, TWO_D_ARRAY_HANDLE, two_d_array_create, uint64_t, row_size, uint64_t, col_size, size_t, item_size);
MOCKABLE_FUNCTION(, int, two_d_array_free_row, TWO_D_ARRAY_HANDLE, two_d_array_handle, uint64_t, row_index);
MOCKABLE_FUNCTION(, int, two_d_array_allocate_new_row, TWO_D_ARRAY_HANDLE, two_d_array_handle, uint64_t, row_index);
MOCKABLE_FUNCTION(, int, two_d_array_add, TWO_D_ARRAY_HANDLE, handle, uint64_t, row_index, uint64_t, col_index, void*, item);
MOCKABLE_FUNCTION(, unsigned char*, two_d_array_get, TWO_D_ARRAY_HANDLE, two_d_array_handle, uint64_t, row_index);
MOCKABLE_FUNCTION(, void, two_d_array_destroy, TWO_D_ARRAY_HANDLE, handle);

#ifdef __cplusplus
}
#endif

#endif /*TWO_DIMENSIONAL_ARRAY_H*/
