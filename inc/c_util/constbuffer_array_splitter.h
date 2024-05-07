// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef CONSTBUFFER_ARRAY_SPLITTER_H
#define CONSTBUFFER_ARRAY_SPLITTER_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_util/constbuffer_array_tarray.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, CONSTBUFFER_ARRAY_HANDLE, constbuffer_array_splitter_split, CONSTBUFFER_ARRAY_HANDLE, buffers, uint32_t, max_buffer_size);
MOCKABLE_FUNCTION(, TARRAY(CONSTBUFFER_ARRAY_HANDLE), constbuffer_array_splitter_split_to_array_of_array, CONSTBUFFER_ARRAY_HANDLE, buffers, uint32_t, max_buffer_size, uint32_t*, payload_buffer_count);
#ifdef __cplusplus
}
#endif

#endif // CONSTBUFFER_ARRAY_SPLITTER_H
