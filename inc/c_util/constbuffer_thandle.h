// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CONSTBUFFER_THANDLE_H
#define CONSTBUFFER_THANDLE_H

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"

#include "c_util/buffer_.h"
#include "c_util/constbuffer_format.h"
#include "c_util/constbuffer_version.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*this is what is returned when the content of the buffer needs access*/
typedef struct CONSTBUFFER_THANDLE_TAG
{
    const unsigned char* buffer;
    uint32_t size;
} CONSTBUFFER_THANDLE;

/*forward declaration for the THANDLE type*/
typedef struct CONSTBUFFER_TAG CONSTBUFFER;

/*declare the THANDLE type*/
THANDLE_TYPE_DECLARE(CONSTBUFFER);

/*this is the writable handle*/
typedef struct CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA_TAG CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA;

/*declare the THANDLE type for writable handle*/
THANDLE_TYPE_DECLARE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA);

typedef void(*CONSTBUFFER_THANDLE_CUSTOM_FREE_FUNC)(void* context);

/*what function should CONSTBUFFER_THANDLE_to_buffer use to allocate the returned serialized form. NULL means malloc from gballoc_hl_malloc_redirect.h of this lib.*/
typedef void*(*CONSTBUFFER_THANDLE_to_buffer_alloc)(size_t size, void* context);

#define CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_VALUES \
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_OK, \
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_ERROR, \
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INSUFFICIENT_BUFFER, \
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG

MU_DEFINE_ENUM(CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_VALUES)

#define CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_VALUES \
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_OK, \
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_ERROR, \
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG, \
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA

MU_DEFINE_ENUM(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_VALUES)

MOCKABLE_INTERFACE(constbuffer_thandle,
    /*this creates a new constbuffer_thandle from a memory area*/
    FUNCTION(, THANDLE(CONSTBUFFER), CONSTBUFFER_THANDLE_Create, const unsigned char*, source, uint32_t, size),

    /*this creates a new constbuffer_thandle from an existing BUFFER_HANDLE*/
    FUNCTION(, THANDLE(CONSTBUFFER), CONSTBUFFER_THANDLE_CreateFromBuffer, BUFFER_HANDLE, buffer),

    FUNCTION(, THANDLE(CONSTBUFFER), CONSTBUFFER_THANDLE_CreateWithMoveMemory, unsigned char*, source, uint32_t, size),

    FUNCTION(, THANDLE(CONSTBUFFER), CONSTBUFFER_THANDLE_CreateWithCustomFree, const unsigned char*, source, uint32_t, size, CONSTBUFFER_THANDLE_CUSTOM_FREE_FUNC, customFreeFunc, void*, customFreeFuncContext),

    FUNCTION(, THANDLE(CONSTBUFFER), CONSTBUFFER_THANDLE_CreateFromOffsetAndSize, THANDLE(CONSTBUFFER), handle, uint32_t, offset, uint32_t, size),

    FUNCTION(, THANDLE(CONSTBUFFER), CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy, THANDLE(CONSTBUFFER), handle, uint32_t, offset, uint32_t, size),

    FUNCTION(, const CONSTBUFFER_THANDLE*, CONSTBUFFER_THANDLE_GetContent, THANDLE(CONSTBUFFER), constbufferHandle),

    FUNCTION(, bool, CONSTBUFFER_THANDLE_contain_same, THANDLE(CONSTBUFFER), left, THANDLE(CONSTBUFFER), right),

    FUNCTION(, uint32_t, CONSTBUFFER_THANDLE_get_serialization_size, THANDLE(CONSTBUFFER), source),

    FUNCTION(, unsigned char*, CONSTBUFFER_THANDLE_to_buffer, THANDLE(CONSTBUFFER), source, CONSTBUFFER_THANDLE_to_buffer_alloc, alloc, void*, alloc_context, uint32_t*, serialized_size),

    FUNCTION(, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_to_fixed_size_buffer, THANDLE(CONSTBUFFER), source, unsigned char*, destination, uint32_t, destination_size, uint32_t*, serialized_size),

    FUNCTION(, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_from_buffer, const unsigned char*, source, uint32_t, size, uint32_t*, consumed, THANDLE(CONSTBUFFER)*, destination),

    FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateWritableHandle, uint32_t, size),

    FUNCTION(, unsigned char*, CONSTBUFFER_THANDLE_GetWritableBuffer, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), constbufferWritableHandle),

    FUNCTION(, THANDLE(CONSTBUFFER), CONSTBUFFER_THANDLE_SealWritableHandle, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), constbufferWritableHandle),

    FUNCTION(, uint32_t, CONSTBUFFER_THANDLE_GetWritableBufferSize, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), constbufferWritableHandle)
)

#ifdef __cplusplus
}
#endif

#endif  /* CONSTBUFFER_THANDLE_H */
