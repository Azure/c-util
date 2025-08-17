// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_CONSTBUFFER_THANDLE_H
#define REAL_CONSTBUFFER_THANDLE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_CONSTBUFFER_THANDLE_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        CONSTBUFFER_THANDLE_Create, \
        CONSTBUFFER_THANDLE_CreateFromBuffer, \
        CONSTBUFFER_THANDLE_CreateWithMoveMemory, \
        CONSTBUFFER_THANDLE_CreateWithCustomFree, \
        CONSTBUFFER_THANDLE_CreateFromOffsetAndSize, \
        CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy, \
        CONSTBUFFER_THANDLE_GetContent, \
        CONSTBUFFER_THANDLE_contain_same, \
        CONSTBUFFER_THANDLE_get_serialization_size, \
        CONSTBUFFER_THANDLE_to_buffer, \
        CONSTBUFFER_THANDLE_to_fixed_size_buffer, \
        CONSTBUFFER_THANDLE_from_buffer, \
        CONSTBUFFER_THANDLE_CreateWritableHandle, \
        CONSTBUFFER_THANDLE_GetWritableBuffer, \
        CONSTBUFFER_THANDLE_SealWritableHandle, \
        CONSTBUFFER_THANDLE_GetWritableBufferSize \
)

#include "c_util/constbuffer_thandle.h"

THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) real_CONSTBUFFER_THANDLE_Create(const unsigned char* source, uint32_t size);

THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) real_CONSTBUFFER_THANDLE_CreateFromBuffer(BUFFER_HANDLE buffer);

THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) real_CONSTBUFFER_THANDLE_CreateWithMoveMemory(unsigned char* source, uint32_t size);

THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) real_CONSTBUFFER_THANDLE_CreateWithCustomFree(const unsigned char* source, uint32_t size, CONSTBUFFER_THANDLE_CUSTOM_FREE_FUNC customFreeFunc, void* customFreeFuncContext);

THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) real_CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) handle, uint32_t offset, uint32_t size);

THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) real_CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy(THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) handle, uint32_t offset, uint32_t size);

const CONSTBUFFER_THANDLE* real_CONSTBUFFER_THANDLE_GetContent(THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) constbufferHandle);

bool real_CONSTBUFFER_THANDLE_contain_same(THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) left, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) right);

uint32_t real_CONSTBUFFER_THANDLE_get_serialization_size(THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) source);

unsigned char* real_CONSTBUFFER_THANDLE_to_buffer(THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) source, CONSTBUFFER_THANDLE_to_buffer_alloc alloc, void* alloc_context, uint32_t* serialized_size);

CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT real_CONSTBUFFER_THANDLE_to_fixed_size_buffer(THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) source, unsigned char* destination, uint32_t destination_size, uint32_t* serialized_size);

CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT real_CONSTBUFFER_THANDLE_from_buffer(const unsigned char* source, uint32_t size, uint32_t* consumed, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA)* destination);

THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) real_CONSTBUFFER_THANDLE_CreateWritableHandle(uint32_t size);

unsigned char* real_CONSTBUFFER_THANDLE_GetWritableBuffer(THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) constbufferWritableHandle);

THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) real_CONSTBUFFER_THANDLE_SealWritableHandle(THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) constbufferWritableHandle);

uint32_t real_CONSTBUFFER_THANDLE_GetWritableBufferSize(THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) constbufferWritableHandle);

#endif //REAL_CONSTBUFFER_THANDLE_H
