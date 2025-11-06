// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CONSTBUFFER_H
#define CONSTBUFFER_H

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#endif

#include "c_util/buffer_.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*this is the handle*/
typedef struct CONSTBUFFER_HANDLE_DATA_TAG* CONSTBUFFER_HANDLE;

/*this is the writable handle*/
typedef struct CONSTBUFFER_WRITABLE_HANDLE_DATA_TAG* CONSTBUFFER_WRITABLE_HANDLE;

/*this is what is returned when the content of the buffer needs access*/
typedef struct CONSTBUFFER_TAG
{
    const unsigned char* buffer;
    uint32_t size;
} CONSTBUFFER;

typedef void(*CONSTBUFFER_CUSTOM_FREE_FUNC)(void* context);

/*what function should CONSTBUFFER_HANDLE_to_buffer use to allocate the returned serialized form. NULL means malloc from gballoc_hl_malloc_redirect.h of this lib.*/
typedef void*(*CONSTBUFFER_to_buffer_alloc)(size_t size, void* context);

#define CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_VALUES \
    CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_OK, \
    CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_ERROR, \
    CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INSUFFICIENT_BUFFER, \
    CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG

MU_DEFINE_ENUM(CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_VALUES)

#define CONSTBUFFER_FROM_BUFFER_RESULT_VALUES \
    CONSTBUFFER_FROM_BUFFER_RESULT_OK, \
    CONSTBUFFER_FROM_BUFFER_RESULT_ERROR, \
    CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG, \
    CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA

MU_DEFINE_ENUM(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_VALUES)

/*this creates a new constbuffer from a memory area*/
MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_Create, const unsigned char*, source, uint32_t, size);

/*this creates a new constbuffer from an existing BUFFER_HANDLE*/
MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateFromBuffer, BUFFER_HANDLE, buffer);

MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateWithMoveMemory, unsigned char*, source, uint32_t, size);

MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateWithCustomFree, const unsigned char*, source, uint32_t, size, CONSTBUFFER_CUSTOM_FREE_FUNC, customFreeFunc, void*, customFreeFuncContext);

MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateFromOffsetAndSize, CONSTBUFFER_HANDLE, handle, uint32_t, offset, uint32_t, size);

MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateFromOffsetAndSizeWithCopy, CONSTBUFFER_HANDLE, handle, uint32_t, offset, uint32_t, size);

MOCKABLE_FUNCTION(, void, CONSTBUFFER_IncRef, CONSTBUFFER_HANDLE, constbufferHandle);

MOCKABLE_FUNCTION(, void, CONSTBUFFER_DecRef, CONSTBUFFER_HANDLE, constbufferHandle);

MOCKABLE_FUNCTION(, const CONSTBUFFER*, CONSTBUFFER_GetContent, CONSTBUFFER_HANDLE, constbufferHandle);

MOCKABLE_FUNCTION(, bool, CONSTBUFFER_HANDLE_contain_same, CONSTBUFFER_HANDLE, left, CONSTBUFFER_HANDLE, right);

MOCKABLE_FUNCTION(, uint32_t, CONSTBUFFER_get_serialization_size, CONSTBUFFER_HANDLE, source);

MOCKABLE_FUNCTION(, unsigned char*, CONSTBUFFER_to_buffer, CONSTBUFFER_HANDLE, source, CONSTBUFFER_to_buffer_alloc, alloc, void*, alloc_context, uint32_t*, serialized_size);

MOCKABLE_FUNCTION(, CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_to_fixed_size_buffer, CONSTBUFFER_HANDLE, source, unsigned char*, destination, uint32_t, destination_size, uint32_t*, serialized_size);

MOCKABLE_FUNCTION(, CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_from_buffer, const unsigned char*, source, uint32_t, size, uint32_t*, consumed, CONSTBUFFER_HANDLE*, destination);

MOCKABLE_FUNCTION(, CONSTBUFFER_WRITABLE_HANDLE, CONSTBUFFER_CreateWritableHandle, uint32_t, size);

MOCKABLE_FUNCTION(, unsigned char*, CONSTBUFFER_GetWritableBuffer, CONSTBUFFER_WRITABLE_HANDLE, constbufferWritableHandle);

MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_SealWritableHandle, CONSTBUFFER_WRITABLE_HANDLE, constbufferWritableHandle);

MOCKABLE_FUNCTION(, void, CONSTBUFFER_WritableHandleIncRef, CONSTBUFFER_WRITABLE_HANDLE, constbufferWritableHandle);

MOCKABLE_FUNCTION(, void, CONSTBUFFER_WritableHandleDecRef, CONSTBUFFER_WRITABLE_HANDLE, constbufferWritableHandle);

MOCKABLE_FUNCTION(, uint32_t, CONSTBUFFER_GetWritableBufferSize, CONSTBUFFER_WRITABLE_HANDLE, constbufferWritableHandle);

#ifdef __cplusplus
}
#endif

#endif  /* CONSTBUFFER_H */
